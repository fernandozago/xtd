#include <array>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <memory>
#include <mutex>
#include <poll.h>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <functional>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pipeline/pipeline.h"
#include "utils/utils.h"


using SendFn = std::function<void(std::string_view)>;
using BroadcastFn = std::function<void(std::string)>;

class connection_handler
{
public:
    connection_handler(SendFn sendFn, BroadcastFn broadcastFn)
        : pipeline()
        , writer(pipeline.writer())
        , send(std::move(sendFn))
        , broadcast(std::move(broadcastFn))
        , workerThread(&connection_handler::process_incoming_data, this)
    {
    }

    ~connection_handler()
    {
        close();

        if (workerThread.joinable()) {
            workerThread.join();
        }

        println_locked("Connection destroyed");
    }

    connection_handler(const connection_handler&) = delete;
    connection_handler& operator=(const connection_handler&) = delete;

    void receive_data(const std::byte* data, std::size_t size)
    {
        writer.write(data, size);
    }

    void close() noexcept
    {
        writer.complete();
    }

private:
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer;

    SendFn send;
    BroadcastFn broadcast;
    std::thread workerThread;

    void process_incoming_data() noexcept
    {
        try
        {
            println_locked("client connected");

            send("[Server: You are connected. This is a sample echo server. Welcome using EPOLL!]\n");

            auto& reader = pipeline.reader();

            while (const xtd::read_result result = reader.read())
            {
                xtd::segmented_byte_view data = result.buffer();

                while (xtd::position newline = data.position_of('\n'))
                {
                    std::string message = data.slice(newline).to_string();
                    println_locked("Broadcasting: [Server: {}]", message);
                    broadcast("[Server: " + message + "]\n");
                    data = data.slice(newline + 1, data.end());
                }

                reader.advance(data.begin(), data.end());

                if (result.completed()) {
                    break;
                }
            }

            reader.complete();
        }
        catch (const std::exception& ex)
        {
            println_locked("client error: {}", ex.what());
        }
        catch (...)
        {
            println_locked("client error: unknown exception");
        }

        println_locked("client disconnected");
    }
};

class server
{
struct connection_data
{
    std::atomic<int> fd{-1};
    std::mutex m_send_mutex;
    std::shared_ptr<connection_handler> m_connection;
};

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

        while (true)
        {
            const int count = ::epoll_wait(m_epollFd, events.data(), static_cast<int>(events.size()), -1);

            if (count < 0)
            {
                if (errno == EINTR) {
                    continue;
                }

                throw_system_error("epoll_wait failed");
            }

            for (int i = 0; i < count; ++i)
            {
                const epoll_event& event = events[static_cast<std::size_t>(i)];

                const int fd = event.data.fd;

                if (fd == m_listenFd)
                {
                    accept_connections();
                }
                else if (fd == STDIN_FILENO)
                {
                    if (enter_pressed()) {
                        return;
                    }
                }
                else
                {
                    process_client_event(fd, event.events);
                }
            }
        }
    }

private:
    static constexpr int ListenBacklog = 16;
    static constexpr int EpollMaxEvents = 128;

    std::uint16_t m_port;
    int m_listenFd = -1;
    int m_epollFd = -1;

    std::mutex m_clients_mutex;
    std::unordered_map<int, std::shared_ptr<connection_data>> m_clients;

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
        epoll_event event{};
        event.events = events;
        event.data.fd = fd;

        if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) < 0)
        {
            throw_system_error("failed to add descriptor to epoll");
        }
    }

    void accept_connections()
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
                    println_locked(
                        "failed to start connection: {}",
                        ex.what());
                }

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
            client->m_connection = std::make_shared<connection_handler>(
                [this, weak_client = std::weak_ptr<connection_data>(client)](std::string_view message) {
                    if (auto client = weak_client.lock()) {
                        send_data(*client, message);
                    }
                },
                [this](std::string message) {
                    broadcast(message);
                });

            add_to_epoll(fd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP);

            std::scoped_lock lock(m_clients_mutex);

            if (!m_clients.emplace(fd, client).second) {
                throw std::runtime_error("connection is already registered");
            }
        }
        catch (...)
        {
            ::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr);
            close_socket(*client);
            if (client->m_connection) {
                client->m_connection->close();
            }
            throw;
        }
    }

    void process_client_event(int fd, std::uint32_t events)
    {
        const auto client = find_client(fd);

        if (!client) {
            return;
        }

        try
        {
            bool open = true;

            if (events & EPOLLIN) {
                open = receive_data(*client);
            }

            if (!open ||
                (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)))
            {
                close_client(fd);
            }
        }
        catch (const std::exception& ex)
        {
            println_locked("client error: {}", ex.what());
            close_client(fd);
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

    bool receive_data(connection_data& client)
    {
        std::array<char, 4096> buffer{};

        while (true)
        {
            const int fd = client.fd.load();

            if (fd < 0) {
                return false;
            }

            const ssize_t received = ::recv(fd, buffer.data(), buffer.size(), 0);

            if (received > 0)
            {
                client.m_connection->receive_data(static_cast<std::byte*>(buffer.data()), static_cast<std::size_t>(received));
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

    void send_data(connection_data& client, std::string_view message)
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

    void broadcast(const std::string& message)
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

                throw std::runtime_error(
                    "socket is no longer writable");
            }

            if (result < 0 && errno == EINTR) {
                continue;
            }

            if (result < 0) {
                throw_system_error("poll failed while sending");
            }
        }
    }

    static bool enter_pressed()
    {
        std::array<char, 256> buffer{};

        const ssize_t size =
            ::read(STDIN_FILENO, buffer.data(), buffer.size());

        if (size > 0)
        {
            return std::find(
                buffer.begin(),
                buffer.begin() + size,
                '\n') != buffer.begin() + size;
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

        for (auto& [fd, client] : clients)
        {
            if (m_epollFd >= 0)
            {
                ::epoll_ctl(
                    m_epollFd,
                    EPOLL_CTL_DEL,
                    fd,
                    nullptr);
            }

            close_socket(*client);

            if (client->m_connection) {
                client->m_connection->close();
            }
        }

        if (m_listenFd >= 0)
        {
            ::close(m_listenFd);
            m_listenFd = -1;
        }

        if (m_epollFd >= 0)
        {
            ::close(m_epollFd);
            m_epollFd = -1;
        }
    }
};

int main(int argc, char** argv)
{
    try
    {
        const int port = std::stoi(argc > 1 ? argv[1] : "9090");

        if (port < 1 || port > 65535) {
            throw std::invalid_argument("invalid port");
        }

        server server(static_cast<std::uint16_t>(port));
        server.run();

        return 0;
    }
    catch (const std::exception& ex)
    {
        println_locked("error: {}", ex.what());
        return 1;
    }
}