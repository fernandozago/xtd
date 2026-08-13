#pragma once

#include <arpa/inet.h>
#include <poll.h>
#include <string>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <execution>

#include "connection_handler.h"
#include "logging/logging.h"

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
            if (m_epollFd < 0) {
                LOG_ERROR("failed to create epoll (errno: {})", errno);
                throw std::system_error(errno, std::generic_category(), "failed to create epoll");
            }
            if (!add_to_epoll(m_listenFd, EPOLLIN)) {
                LOG_ERROR("failed to add listen socket to epoll (errno: {})", errno);
                throw std::system_error(errno, std::generic_category(), "failed to add listen socket to epoll");
            }
            if (!add_to_epoll(STDIN_FILENO, EPOLLIN)) {
                LOG_ERROR("failed to add stdin to epoll (errno: {})", errno);
                throw std::system_error(errno, std::generic_category(), "failed to add stdin to epoll");
            }
        } catch (...) {
            cleanup();
            throw;
        }
    }

    ~server() { cleanup(); }

    server(const server&) = delete;
    server& operator=(const server&) = delete;

    void run() {
        LOG_INFO("Server is listening on port: {}", m_port);
        LOG_INFO("Press Enter to stop the server.");

        std::array<epoll_event, EPOLL_MAX_EVENTS> events{};
        while(true) {
            const int count = retry([&] {
                return ::epoll_wait(m_epollFd, events.data(), events.size(), -1);
            });
            if (count < 0) {
                LOG_ERROR("epoll_wait failed (errno: {})", errno);
                throw std::system_error(errno, std::generic_category(), "epoll_wait failed");
            }

            LOG_INFO("epoll_wait returned {} events", count);

            for (int i = 0; i < count; ++i) {
                const auto& event = events[i];

                LOG_INFO("processing event for fd {}: events = {} ({})", event.data.fd, event.events, epoll_events_to_string(event.events));

                if (event.data.fd == m_listenFd) {
                    handle_incoming_connections();
                } else if (event.data.fd == STDIN_FILENO) {
                    if (enter_pressed()) {
                        broadcast("[📣: I'm shutting down... Bye! 👋]\n");
                        return;
                    }
                } else {
                    process_client_event(event);
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

    static std::string epoll_events_to_string(std::uint32_t events)
    {
        struct event_flag {
            std::uint32_t mask;
            const char* name;
        };

        constexpr std::array<event_flag, 8> flags{{
            {EPOLLIN, "EPOLLIN"},
            {EPOLLOUT, "EPOLLOUT"},
            {EPOLLERR, "EPOLLERR"},
            {EPOLLHUP, "EPOLLHUP"},
            {EPOLLRDHUP, "EPOLLRDHUP"},
            {EPOLLPRI, "EPOLLPRI"},
            {EPOLLET, "EPOLLET"},
            {EPOLLONESHOT, "EPOLLONESHOT"},
        }};

        std::string result;
        std::uint32_t remaining = events;

        for (const event_flag& flag : flags)
        {
            if ((events & flag.mask) == 0) {
                continue;
            }

            if (!result.empty()) {
                result += '|';
            }

            result += flag.name;
            remaining &= ~flag.mask;
        }

        if (remaining != 0 || result.empty()) {
            if (!result.empty()) {
                result += '|';
            }
            result += std::format("0x{:X}", remaining);
        }

        return result;
    }

    int create_listen_socket() const {
        const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            LOG_ERROR("failed to create listen socket (errno: {})", errno);
            throw std::system_error(errno, std::generic_category(), "failed to create listen socket");
        }

        try {
            const int reuse = 1;
            if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
                LOG_ERROR("failed to set SO_REUSEADDR (errno: {})", errno);
                throw std::system_error(errno, std::generic_category(), "failed to set SO_REUSEADDR");
            }

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_ANY);
            address.sin_port = htons(m_port);

            if (::bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
                LOG_ERROR("failed to bind on port {} (errno: {})", m_port, errno);
                throw std::system_error(errno, std::generic_category(), "failed to bind on port " + std::to_string(m_port));
            }
            if (::listen(fd, EPOLL_MAX_LISTEN_BACKLOG) < 0) {
                LOG_ERROR("failed to listen (errno: {})", errno);
                throw std::system_error(errno, std::generic_category(), "failed to listen");
            }
            return fd;
        } catch (...) {
            ::close(fd);
            throw;
        }
    }

    bool add_to_epoll(int fd, std::uint32_t flags) const {
        epoll_event event{};
        event.events = flags;
        event.data.fd = fd;
        if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) < 0) {
            LOG_ERROR("failed to add descriptor {} to epoll (errno: {})", fd, errno);
            return false;
        }
        LOG_INFO("added fd {} to epoll with flags: {} ({})", fd, flags, epoll_events_to_string(flags));
        return true;
    }

    void handle_incoming_connections() {
        while(true) {
            const int fd = retry([&] {
                return ::accept4(m_listenFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            });
            LOG_INFO("accept4 returned fd: {}", fd);

            if (fd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                LOG_ERROR("failed to accept connection (errno: {})", errno);
                throw std::system_error(errno, std::generic_category(), "failed to accept connection");
            }

            try {
                if (!start_connection_handler(fd)) {
                    continue;
                }
                LOG_INFO("accepted client fd: {}", fd);
            } catch (const std::exception& ex) {
                LOG_ERROR("failed to start connection: {}", ex.what());
            }
        }
    }

    bool start_connection_handler(int fd)
    {
        auto client = std::make_shared<connection_data>();
        client->fd = fd;

        try
        {
            std::scoped_lock lock(m_clients_mutex);
            if (!m_clients.emplace(fd, client).second) {
                LOG_WARN("connection already registered for fd {}; ignoring duplicate accept", fd);
                close_connection(*client);
                return false;
            }
        }
        catch (...)
        {
            close_connection(*client);
            throw;
        }

        try
        {
            client->m_connection = std::make_unique<connection_handler<server>>(fd, *this);
            if (!add_to_epoll(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                LOG_ERROR("failed to register client fd {} with epoll; disconnecting client", fd);
                disconnect_client(fd);
                return false;
            }
            return true;
        }
        catch (...) {
            disconnect_client(fd);
            return false;
        }
    }

    void process_client_event(const epoll_event& event)
    {
        auto client = find_client(event.data.fd);
        if (!client) return;

        try {
            const bool alive = !(event.events & EPOLLIN) || receive_data(*client);
            if (alive && !(event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))) return;
        }
        catch (const std::exception& ex) {
            LOG_ERROR("client fd {} error: {}", event.data.fd, ex.what());
        }
        
        disconnect_client(event.data.fd);
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
                LOG_INFO("recv returned {} bytes for client fd {}", size, fd);
            } else if (size == 0) {
                return false;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                LOG_INFO("client fd {} would block on receive, waiting for writable", fd);
                return true;
            } else {
                LOG_ERROR("failed to receive from client for fd {} (errno: {}); disconnecting client", fd, errno);
                return false;
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
                return ::send(fd, message.data(), message.size(), MSG_NOSIGNAL);
            });

            LOG_INFO("sent {} bytes for client fd {}", sent, fd);

            if (sent > 0) {
                message.remove_prefix(static_cast<std::size_t>(sent));
            }
            else if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
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
        pollfd event{.fd = fd, .events = POLLOUT, .revents = 0};

        const int result = retry([&] {
            return ::poll(&event, 1, EPOLL_MAX_SEND_TIMEOUT_MS);
        });

        return result > 0 && (event.revents & POLLOUT) && !(event.revents & (POLLERR | POLLHUP | POLLNVAL));
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
            LOG_INFO("client fd {} (AKA: `{}`) disconnected", fd, client.m_connection->name());
            client.m_connection->close();
        }
    }

    bool enter_pressed() {
        const ssize_t size = retry([&] {
            return ::read(STDIN_FILENO, m_epoll_buffer.data(), m_epoll_buffer.size());
        });
        if (size < 0)  {
            LOG_ERROR("failed to read stdin (errno: {})", errno);
            throw std::system_error(errno, std::generic_category(), "failed to read stdin");
        }
        return size > 0 && std::find(m_epoll_buffer.begin(), m_epoll_buffer.begin() + size, '\n') != m_epoll_buffer.begin() + size;
    }

    void cleanup() noexcept {
        LOG_INFO("server is shutting down, cleaning up resources...");
        decltype(m_clients) clients;
        {
            std::scoped_lock lock(m_clients_mutex);
            clients.swap(m_clients);
        }

        for (auto& [fd, client] : clients) {
            LOG_INFO("closing client fd {} (AKA: `{}`)", fd, client->m_connection ? client->m_connection->name() : "unknown");
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