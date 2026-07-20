#pragma once

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <poll.h>
#include <unordered_map>
#include "../utils/utils.h"
#include "connection_handler.h"


static constexpr int ListenBacklog = 16;
static constexpr int EpollMaxEvents = 128;

class server
{
public:
    explicit server(std::uint16_t port)
        : m_port(port)
    {
        try
        {
            m_listenFd = create_listen_socket();

            m_epollFd = ::epoll_create1(EPOLL_CLOEXEC);
            if (m_epollFd < 0) {
                throw_system_error("failed to create epoll");
            }

            add_to_epoll(m_listenFd, EPOLLIN);
            add_to_epoll(STDIN_FILENO, EPOLLIN);
        }
        catch (...)
        {
            cleanup();
            throw;
        }
    }

    ~server()
    {
        cleanup();
    }

    server(const server&) = delete;
    server& operator=(const server&) = delete;

    void run()
    {
        println_locked("Server is listening on port: {}", m_port);
        println_locked("Press Enter to stop the server.");

        std::array<epoll_event, EpollMaxEvents> events{};
        const int events_size = static_cast<int>(events.size());

        while (true)
        {
            const int count = ::epoll_wait(m_epollFd, events.data(), events_size, -1);

            if (count < 0)
            {
                if (errno == EINTR) continue;
                throw_system_error("epoll_wait failed");
            }
            
            const std::size_t max_count = static_cast<std::size_t>(count);
            for (std::size_t i = 0; i < max_count; ++i)
            {
                const epoll_event& event = events[i];
                const int fd = event.data.fd;

                // check if the fd is the listening socket
                if (fd == m_listenFd) {
                    handle_incoming_connection();
                    continue;
                }

                // check if the fd is stdin
                if (fd == STDIN_FILENO)
                {
                    if (enter_pressed()) {
                        broadcast("[📣: I'm shutting down... Bye! 👋]\n");
                        return;
                    }

                    continue;
                }

                // otherwise, it must be a client socket
                process_client_event(fd, event.events);
            }
        }
    }

private:
    void reply(const int originFd, const std::string_view message) {
        if (auto client = find_client(originFd)) {
            send_data(*client, message);
        }
    }

    void broadcast(const std::string_view message)
    {
        std::vector<std::shared_ptr<connection_data>> clients;
        {
            std::scoped_lock lock(m_clients_mutex);
            clients.reserve(m_clients.size());
            for (const auto& [fd, client] : m_clients) {
                clients.push_back(client);
            }
        }

        for (const auto& client : clients)
        {
            try {
                send_data(*client, message);
            }
            catch (const std::exception& ex) {
                println_locked("broadcast error: {}", ex.what());
            }
        }
    }

    friend connection_handler<server>;
    using connection_handler_t = connection_handler<server>;
    struct connection_data
    {
        std::atomic<int> fd{-1};
        mutable std::mutex m_send_mutex;
        std::shared_ptr<connection_handler_t> m_connection;
    };

    std::uint16_t m_port;
    int m_listenFd = -1;
    int m_epollFd = -1;

    std::mutex m_clients_mutex;
    std::unordered_map<int, std::shared_ptr<connection_data>> m_clients;

    std::array<char, 4096> receive_buffer{};
    std::array<char, 4096> enter_buffer{};

    [[noreturn]]
    static void throw_system_error(const std::string& message, int error = errno)
    {
        throw std::system_error(error, std::generic_category(), message);
    }

    int create_listen_socket() const
    {
        const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

        if (fd < 0) {
            throw_system_error("failed to create listen socket");
        }

        try
        {
            int reuseAddress = 1;

            if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress)) < 0)
            {
                throw_system_error("failed to set SO_REUSEADDR");
            }

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_ANY);
            address.sin_port = htons(m_port);

            if (::bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0)
            {
                throw_system_error( "failed to bind on port " + std::to_string(m_port));
            }

            if (::listen(fd, ListenBacklog) < 0) {
                throw_system_error("failed to listen");
            }

            return fd;
        }
        catch (...)
        {
            ::close(fd);
            throw;
        }
    }

    void add_to_epoll(int fd, std::uint32_t events) const
    {
        epoll_event event {
            .events = events,
            .data = {
                .fd = fd
            }
        };

        if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) < 0)
        {
            throw_system_error("failed to add descriptor to epoll");
        }
    }

    void handle_incoming_connection()
    {
        while (true)
        {
            const int fd = ::accept4(m_listenFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);

            if (fd >= 0)
            {
                try {
                    start_connection_handler(fd);
                }
                catch (const std::exception& ex) {
                    println_locked("failed to start connection: {}", ex.what());
                }

                println_locked("accepted client fd: {}", fd);
                continue;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }

            throw_system_error("failed to accept connection");
        }
    }

    void start_connection_handler(int fd)
    {
        auto client = std::make_shared<connection_data>();
        client->fd = fd;

        try
        {
            {
                std::scoped_lock lock(m_clients_mutex) ;
                if (!m_clients.emplace(fd, client).second) {
                    throw std::runtime_error("connection is already registered");
                }
            }
            
            client->m_connection = std::make_shared<connection_handler_t>(fd, *this);
            add_to_epoll(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP);
        }
        catch (...)
        {
            if (client->m_connection) {
                client->m_connection->close();
            }
            ::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr);
            close_socket(*client);
            throw;
        }
    }

    void process_client_event(int fd, std::uint32_t events)
    {
        if (auto client = find_client(fd)) {
            try {
                bool leave_open = true;

                if (events & EPOLLIN) {
                    leave_open = receive_data(*client);
                }

                if (!leave_open || (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))) {
                    close_client(fd);
                }
            }
            catch (const std::exception& ex)
            {
                println_locked("client error: {}", ex.what());
                close_client(fd);
            }
        }
    }

    std::shared_ptr<connection_data> find_client(int fd)
    {
        std::scoped_lock lock(m_clients_mutex);

        const auto it = m_clients.find(fd);

        if (it == m_clients.end()) {
            return {};
        }

        return it->second;
    }

    bool receive_data(const connection_data& client)
    {
        while (true)
        {
            const int fd = client.fd.load();

            if (fd < 0) {
                return false;
            }

            const ssize_t received = ::recv(fd, receive_buffer.data(), receive_buffer.size(), 0);

            if (received > 0)
            {
                client.m_connection->receive_data(reinterpret_cast<std::byte*>(receive_buffer.data()), static_cast<std::size_t>(received));
                continue;
            }

            if (received == 0) {
                return false;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return true;
            }

            throw_system_error("failed to receive from client");
        }
    }

    void send_data(const connection_data& client, std::string_view message)
    {
        std::scoped_lock lock(client.m_send_mutex);

        const int fd = client.fd.load();

        if (fd < 0) {
            throw std::runtime_error("socket is closed");
        }

        while (!message.empty())
        {
            const ssize_t sent = ::send(fd, message.data(), message.size(), MSG_NOSIGNAL);

            if (sent > 0)
            {
                message.remove_prefix(static_cast<std::size_t>(sent));
                continue;
            }

            if (sent == 0) {
                throw std::runtime_error("socket closed while sending");
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                wait_writable(fd);
                continue;
            }

            throw_system_error("failed to send data");
        }
    }

    void close_client(int fd)
    {
        std::shared_ptr<connection_data> client;

        {
            std::scoped_lock lock(m_clients_mutex);
            
            const auto it = m_clients.find(fd);
            if (it == m_clients.end()) {
                return;
            }

            client = std::move(it->second);
            m_clients.erase(it);
        }

        ::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr);
        close_socket(*client);
        client->m_connection->close();
    }

    static void close_socket(connection_data& client) noexcept
    {
        const int fd = client.fd.exchange(-1);

        if (fd < 0) {
            return;
        }

        // Wake send() or poll() before waiting for the send lock.
        ::shutdown(fd, SHUT_RDWR);

        std::scoped_lock lock(client.m_send_mutex);
        ::close(fd);
    }

    static void wait_writable(int fd)
    {
        pollfd event{};
        event.fd = fd;
        event.events = POLLOUT;

        while (true)
        {
            const int result = ::poll(&event, 1, -1);

            if (result > 0)
            {
                if (event.revents & POLLOUT) {
                    return;
                }

                throw std::runtime_error("socket is no longer writable");
            }

            if (result < 0 && errno == EINTR) {
                continue;
            }

            if (result < 0) {
                throw_system_error("poll failed while sending");
            }
        }
    }

    bool enter_pressed()
    {
        const ssize_t size = ::read(STDIN_FILENO, enter_buffer.data(), enter_buffer.size());

        if (size > 0)
        {
            return std::find(enter_buffer.begin(), enter_buffer.begin() + size, '\n') != enter_buffer.begin() + size;
        }

        if (size < 0 && errno != EINTR) {
            throw_system_error("failed to read stdin");
        }

        return false;
    }

    void cleanup() noexcept
    {
        std::unordered_map<int, std::shared_ptr<connection_data>> clients;

        {
            std::scoped_lock lock(m_clients_mutex);
            clients.swap(m_clients);
        }

        for (auto& client : clients)
        {
            if (client.second->m_connection) {
                client.second->m_connection->close(); 
            }
            
            if (m_epollFd >= 0) {
                ::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, client.first, nullptr);
            }
            
            close_socket(*client.second);
        }

        if (m_listenFd >= 0) {
            ::close(m_listenFd);
            m_listenFd = -1;
        }

        if (m_epollFd >= 0) {
            ::close(m_epollFd);
            m_epollFd = -1;
        }
    }
};