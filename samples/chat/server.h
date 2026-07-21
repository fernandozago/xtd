#pragma once

#include <arpa/inet.h>
#include <poll.h>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <execution>

#include "../utils/utils.h"
#include "connection_handler.h"

const constinit int EPOLL_MAX_LISTEN_BACKLOG = 16;
const constinit int EPOLL_MAX_EVENTS = 256;
const constinit int EPOLL_MAX_SEND_TIMEOUT_MS = 100;
const constinit std::size_t EPOLL_MAX_BUFFER_SIZE = 4096;

class server {
private:
    std::uint16_t m_port;
    int m_listenFd{-1};
    int m_epollFd{-1};
    std::mutex m_clients_mutex;
    std::array<char, EPOLL_MAX_BUFFER_SIZE> m_epoll_buffer{};

public:
    explicit server(std::uint16_t port) : m_port(port) {
        try {
            m_listenFd = create_listen_socket();
            m_epollFd = ::epoll_create1(EPOLL_CLOEXEC);
            if (m_epollFd < 0) throw_system_error("failed to create epoll");
            add_to_epoll(m_listenFd, EPOLLIN);
            add_to_epoll(STDIN_FILENO, EPOLLIN);
        } catch (...) {
            cleanup();
            throw;
        }
    }

    ~server() { cleanup(); }

    server(const server&) = delete;
    server& operator=(const server&) = delete;

    void run() {
        println_locked("Server is listening on port: {}", m_port);
        println_locked("Press Enter to stop the server.");

        std::array<epoll_event, EPOLL_MAX_EVENTS> events{};
        while(true) {
            const int count = retry([&] {
                return ::epoll_wait(m_epollFd, events.data(), events.size(), -1);
            });
            if (count < 0) throw_system_error("epoll_wait failed");

            for (int i = 0; i < count; ++i) {
                const auto& event = events[i];
                const int fd = event.data.fd;

                if (fd == m_listenFd) {
                    handle_incoming_connections();
                } else if (fd == STDIN_FILENO) {
                    if (enter_pressed()) {
                        broadcast("[📣: I'm shutting down... Bye! 👋]\n");
                        return;
                    }
                } else {
                    process_client_event(fd, event.events);
                }
            }
        }
    }

private:
    void reply(const int client_id, std::string_view message) noexcept
    {
        if (auto client = find_client(client_id); client && !send_data(*client, message)) {
            disconnect_client(client_id);
        }
    }

    void broadcast(std::string_view message) noexcept
    {
        std::vector<client_ptr> clients;
        {
            std::scoped_lock lock(m_clients_mutex);
            clients.reserve(m_clients.size());
            for (const auto& [fd, client] : m_clients) {
                clients.emplace_back(client);
            }
        }

        std::for_each(std::execution::par, clients.begin(), clients.end(),
            [&](const client_ptr& client)
            {
                if (!send_data(*client, message))
                    disconnect_client(client->fd.load());
            }
        );

    }

    void user_quit(int client_id) noexcept
    {
        if (auto client = find_client(client_id)) {
            std::scoped_lock lock(client->m_connection_mutex);
            if (const int fd = client->fd.load(); fd >= 0)
                ::shutdown(fd, SHUT_RDWR);
        }
    }

    friend connection_handler<server>;
    struct connection_data 
    {
        std::atomic<int> fd{-1};
        mutable std::mutex m_connection_mutex;
        std::unique_ptr<connection_handler<server>> m_connection;
    };
    using client_ptr = std::shared_ptr<connection_data>;

    std::unordered_map<int, client_ptr> m_clients;

    template<class F>
    static auto retry(F&& operation) -> decltype(operation()) {
        auto result = operation();
        while (result < 0 && errno == EINTR)  {
            result = operation();
        }
        return result;
    }

    [[noreturn]] static void throw_system_error(const std::string& message, int error = errno) {
        throw std::system_error(error, std::generic_category(), message);
    }

    static bool would_block() { 
        return errno == EAGAIN || errno == EWOULDBLOCK; 
    }

    int create_listen_socket() const {
        const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (fd < 0) throw_system_error("failed to create listen socket");

        try {
            const int reuse = 1;
            if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
                throw_system_error("failed to set SO_REUSEADDR");

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_ANY);
            address.sin_port = htons(m_port);

            if (::bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
                throw_system_error("failed to bind on port " + std::to_string(m_port));
            }
            if (::listen(fd, EPOLL_MAX_LISTEN_BACKLOG) < 0) {
                throw_system_error("failed to listen");
            }
            return fd;
        } catch (...) {
            ::close(fd);
            throw;
        }
    }

    void add_to_epoll(int fd, std::uint32_t flags) const {
        epoll_event event{};
        event.events = flags;
        event.data.fd = fd;
        if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) < 0) {
            throw_system_error("failed to add descriptor to epoll");
        }
    }

    void handle_incoming_connections() {
        while(true) {
            const int fd = retry([&] {
                return ::accept4(m_listenFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            });

            if (fd < 0) {
                if (would_block()) return;
                throw_system_error("failed to accept connection");
            }

            try {
                start_connection_handler(fd);
                println_locked("accepted client fd: {}", fd);
            } catch (const std::exception& ex) {
                println_locked("failed to start connection: {}", ex.what());
            }
        }
    }

    void start_connection_handler(int fd)
    {
        auto client = std::make_shared<connection_data>();
        client->fd = fd;

        try
        {
            std::scoped_lock lock(m_clients_mutex);
            if (!m_clients.emplace(fd, client).second)
                throw std::runtime_error("connection is already registered");
        }
        catch (...)
        {
            close_connection(*client);
            throw;
        }

        try
        {
            client->m_connection = std::make_unique<connection_handler<server>>(fd, *this);
            add_to_epoll(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP );
        }
        catch (...)
        {
            disconnect_client(fd);
            throw;
        }
    }

    void process_client_event(int fd, std::uint32_t events)
    {
        auto client = find_client(fd);
        if (!client) return;

        try {
            const bool alive = !(events & EPOLLIN) || receive_data(*client);
            if (alive && !(events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))) return;
        }
        catch (const std::exception& ex) {
            println_locked("client fd {} error: {}", fd, ex.what());
        }
        disconnect_client(fd);
    }

    client_ptr find_client(int fd) {
        std::scoped_lock lock(m_clients_mutex);
        const auto it = m_clients.find(fd);
        return it == m_clients.end() ? nullptr : it->second;
    }

    bool receive_data(const connection_data& client) {
        while(true) {
            const int fd = client.fd.load();
            if (fd < 0) return false;

            const ssize_t size = retry([&] {
                return ::recv(fd, m_epoll_buffer.data(), m_epoll_buffer.size(), 0);
            });

            if (size > 0) {
                client.m_connection->receive_data(reinterpret_cast<std::byte*>(m_epoll_buffer.data()), static_cast<std::size_t>(size));
            } else if (size == 0) {
                return false;
            } else if (would_block()) {
                return true;
            } else {
                throw_system_error("failed to receive from client");
            }
        }
    }

    [[nodiscard]]
    bool send_data(const connection_data& client, std::string_view message) noexcept
    {
        std::scoped_lock lock(client.m_connection_mutex);

        const int fd = client.fd.load();
        if (fd < 0) return false;

        while (!message.empty())
        {
            const ssize_t sent = retry([&] {
                return ::send(
                    fd,
                    message.data(),
                    message.size(),
                    MSG_NOSIGNAL
                );
            });

            if (sent > 0) {
                message.remove_prefix(static_cast<std::size_t>(sent));
            }
            else if (sent < 0 && would_block()) {
                if (!wait_writable(fd)) return false;
            }
            else {
                return false;
            }
        }

        return true;
    }

    static bool wait_writable(int fd) noexcept
    {
        pollfd event{
            .fd = fd,
            .events = POLLOUT,
            .revents = 0
        };

        const int result = retry([&] {
            return ::poll(&event, 1, EPOLL_MAX_SEND_TIMEOUT_MS);
        });

        return result > 0 &&
            (event.revents & POLLOUT) &&
            !(event.revents & (POLLERR | POLLHUP | POLLNVAL));
    }

    void disconnect_client(int fd)
    {
        client_ptr client;
        {
            std::scoped_lock lock(m_clients_mutex);
            auto node = m_clients.extract(fd);
            if (node.empty()) return;
            client = std::move(node.mapped());
        }
        ::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr);
        close_connection(*client);
    }

    static void close_connection(connection_data& client)
    {
        int fd = client.fd.exchange(-1);
        if (fd < 0) return;

        ::shutdown(fd, SHUT_RDWR);

        {
            std::scoped_lock lock(client.m_connection_mutex);
            ::close(fd);
        }

        if (client.m_connection) {
            println_locked("client fd {} (AKA: `{}`) disconnected", fd, client.m_connection->name());
            client.m_connection->close();
        }
    }

    bool enter_pressed() {
        const ssize_t size = retry([&] {
            return ::read(STDIN_FILENO, m_epoll_buffer.data(), m_epoll_buffer.size());
        });
        if (size < 0) throw_system_error("failed to read stdin");
        return size > 0 && std::find(m_epoll_buffer.begin(), m_epoll_buffer.begin() + size, '\n') != m_epoll_buffer.begin() + size;
    }

    void cleanup() noexcept {
        decltype(m_clients) clients;
        {
            std::scoped_lock lock(m_clients_mutex);
            clients.swap(m_clients);
        }

        for (auto& [fd, client] : clients) {
            if (m_epollFd >= 0)  {
                ::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr);
            }
            close_connection(*client);
        }

        if (m_listenFd >= 0) {
            ::close(std::exchange(m_listenFd, -1));
        }
        if (m_epollFd >= 0) {
            ::close(std::exchange(m_epollFd, -1));
        }
    }
};