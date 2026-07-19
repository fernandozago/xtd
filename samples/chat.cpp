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


using BroadcastFn = std::function<void(std::string)>;

class Connection
{
    public:
    explicit Connection(int connectionFd, BroadcastFn broadcast)
    : fd(connectionFd)
    , pipeline()
    , writer(pipeline.writer())
    , sendMutex()
    , broadcast(std::move(broadcast))
    , workerThread(std::thread(&Connection::process_incoming_data, this))
    {
    }

    ~Connection()
    {
        if (workerThread.joinable()) {
            workerThread.join();
        }

        println_locked("Connection destroyed");
    }

    void close() noexcept
    {
        fd.store(-1, std::memory_order_release);
        writer.complete();
    }

    bool receive_socket_data()
    {
        while (true)
        {
            const ssize_t received = ::recv(fd.load(std::memory_order_acquire), buffer.data(), buffer.size(), 0);

            if (received == 0) {
                return false;
            }
            
            if (received > 0)
            {
                writer.write(reinterpret_cast<const std::byte*>(buffer.data()), static_cast<std::size_t>(received));
                continue;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return true;
            }

            throw std::system_error(errno, std::generic_category(), "failed to receive from client");
        }
    }

    void send_data(std::string_view message)
    {
        std::scoped_lock lock(sendMutex);
    
        const char* data = message.data();
        std::size_t remaining = message.size();
    
        while (remaining > 0)
        {
            const int currentFd = fd.load();
    
            if (currentFd < 0) {
                throw std::runtime_error("socket is closed");
            }
    
            const ssize_t sent = ::send(currentFd, data, remaining, MSG_NOSIGNAL);
    
            if (sent > 0)
            {
                data += sent;
                remaining -= sent;
                continue;
            }
    
            if (sent == 0) {
                throw std::runtime_error("socket closed while sending response");
            }
    
            if (errno == EINTR) {
                continue;
            }
    
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (!wait_writable(currentFd)) {
                    throw std::runtime_error("socket is no longer writable");
                }
    
                continue;
            }
    
            throw std::system_error(errno, std::generic_category(), "failed to send response");
        }
    }
private:
    std::atomic<int> fd;
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer;
    std::mutex sendMutex;
    BroadcastFn broadcast;
    std::thread workerThread;
    std::array<char, 4096> buffer{};

    void process_incoming_data() noexcept
    {
        try
        {
            println_locked("client connected");

            send_data("[Server: You are connected. This is a sample echo server. Welcome using EPOLL!]\n");

            constexpr char Delimiter = '\n';

            auto& reader = pipeline.reader();
            while (const xtd::read_result result = reader.read())
            {
                xtd::segmented_byte_view ros = result.buffer();
                while (xtd::position newLinePos = ros.position_of(Delimiter))
                {
                    std::string message = ros.slice(newLinePos).to_string();
                    println_locked("Broadcasting: [Server: {}]", message);
                    broadcast("[Server: " + message + "]\n");
                    ros = ros.slice(newLinePos + 1, ros.end());
                }

                reader.advance(ros.begin(), ros.end());

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

    static bool wait_writable(int fd)
    {
        while (true)
        {
            pollfd pfd{};
            pfd.fd = fd;
            pfd.events = POLLOUT;

            const int result = ::poll(&pfd, 1, -1);

            if (result > 0)
            {
                if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                    return false;
                }

                if ((pfd.revents & POLLOUT) != 0) {
                    return true;
                }

                continue;
            }

            if (result < 0 && errno == EINTR) {
                continue;
            }

            if (result < 0) {
                throw std::system_error(errno, std::generic_category(), "poll failed while sending");
            }
        }
    }
};

class Server
{
public:
    explicit Server(std::uint16_t port)
        : m_port(port)
        , m_listen_fd(create_listen_socket())
        , m_epoll_fd(create_epoll_instance())
    {
        add_to_epoll(m_listen_fd, EPOLLIN);
        add_to_epoll(STDIN_FILENO, EPOLLIN);

        println_locked("Server is listening on port: {}", m_port);
        println_locked("Press Enter to stop the server.");

        std::array<epoll_event, MaxEvents> events{};
        bool shutdownRequested = false;

        while (!shutdownRequested)
        {
            const int count = ::epoll_wait(m_epoll_fd, events.data(), static_cast<int>(events.size()), -1);

            if (count < 0)
            {
                if (errno == EINTR) {
                    continue;
                }

                throw_sys_error("epoll_wait failed");
            }

            for (int i = 0; i < count; ++i)
            {
                const int fd = events[static_cast<std::size_t>(i)].data.fd;
                const std::uint32_t eventFlags = events[static_cast<std::size_t>(i)].events;

                if (fd == m_listen_fd)
                {
                    accept_pending_connections();
                    continue;
                }

                if (fd == STDIN_FILENO)
                {
                    if (exit_on_enter())
                    {
                        println_locked("Shutdown requested by user input.");
                        shutdownRequested = true;
                    }

                    continue;
                }

                process_connection_event(fd, eventFlags);
            }
        }
    }

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    ~Server()
    {
        std::unordered_map<int, std::shared_ptr<Connection>> connections;

        {
            std::scoped_lock lock(m_mutex);
            connections.swap(m_connections);
        }

        for (auto& [fd, connection] : connections)
        {
            if (m_epoll_fd >= 0) {
                ::epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
            }

            close_connection_fd(fd);

            try {
                connection->close();
            }
            catch (...) {
                // Destructors and cleanup must not throw.
            }
        }

        if (m_listen_fd >= 0)
        {
            ::close(m_listen_fd);
            m_listen_fd = -1;
        }

        if (m_epoll_fd >= 0)
        {
            ::close(m_epoll_fd);
            m_epoll_fd = -1;
        }
    }

private:
    static constexpr int ListenBacklog = 128;
    static constexpr std::size_t MaxEvents = 128;

    const std::uint16_t m_port;
    int m_listen_fd = -1;
    int m_epoll_fd = -1;

    std::mutex m_mutex;
    std::unordered_map<int, std::shared_ptr<Connection>> m_connections;

    [[noreturn]] 
    static void throw_sys_error(const std::string& message, int errorCode = errno)
    {
        throw std::system_error(errorCode, std::generic_category(), message);
    }

    [[noreturn]] 
    static void close_and_throw(int fd, const std::string& message)
    {
        const int error = errno;

        if (fd >= 0) {
            ::close(fd);
        }
        
        throw_sys_error(message, error);
    }

    static void close_connection_fd(int fd) noexcept
    {
        if (fd < 0) {
            return;
        }

        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
    }

    int create_listen_socket()
    {
        int listen_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

        if (listen_fd < 0) {
            throw_sys_error("failed to create listen socket");
        }

        int reuseAddr = 1;

        if (::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) != 0)
        {
            const int fd = std::exchange(listen_fd, -1);
            close_and_throw(fd, "failed to set SO_REUSEADDR");
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(m_port);

        if (::bind(listen_fd, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0)
        {
            const int fd = std::exchange(listen_fd, -1);
            close_and_throw(fd, "failed to bind on port " + std::to_string(m_port));
        }

        if (::listen(listen_fd, ListenBacklog) != 0)
        {
            const int fd = std::exchange(listen_fd, -1);
            close_and_throw(fd, "failed to listen");
        }
        return listen_fd;
    }

    int create_epoll_instance()
    {
        int epoll_fd = ::epoll_create1(EPOLL_CLOEXEC);

        if (epoll_fd < 0) {
            throw_sys_error("failed to create epoll instance");
        }
        return epoll_fd;
    }

    void add_to_epoll(int fd, std::uint32_t events) const
    {
        epoll_event event{};
        event.events = events;
        event.data.fd = fd;

        if (::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) != 0) {
            throw_sys_error("failed to add fd to epoll");
        }
    }

    void broadcast(const std::string& message)
    {
        std::vector<std::shared_ptr<Connection>> recipients;

        {
            std::scoped_lock lock(m_mutex);

            recipients.reserve(m_connections.size());

            for (const auto& [fd, connection] : m_connections) {
                recipients.push_back(connection);
            }
        }

        // Do not hold connectionsMutex_ while sending.
        for (const auto& connection : recipients)
        {
            try {
                connection->send_data(message);
            }
            catch (const std::exception& ex) {
                println_locked("broadcast error: {}", ex.what());
            }
        }
    }

    void start_connection(int connectionFd)
    {
        std::shared_ptr<Connection> connection;

        try
        {
            connection = std::make_shared<Connection>(connectionFd, [this](std::string message) {
                broadcast(message);
            });

            add_to_epoll(connectionFd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP);

            std::scoped_lock lock(m_mutex);
            const auto [it, inserted] = m_connections.emplace(connectionFd, std::move(connection));

            if (!inserted) {
                throw std::runtime_error(
                    "connection file descriptor is already registered");
            }
        }
        catch (...)
        {
            ::epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, connectionFd, nullptr);
            close_connection_fd(connectionFd);

            if (connection) {
                connection->close();
            }

            throw;
        }
    }

    void accept_pending_connections()
    {
        while (true)
        {
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);

            const int connectionFd = ::accept4(
                m_listen_fd,
                reinterpret_cast<sockaddr*>(&clientAddr),
                &clientAddrLen,
                SOCK_NONBLOCK | SOCK_CLOEXEC);

            if (connectionFd < 0)
            {
                if (errno == EINTR) {
                    continue;
                }

                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return;
                }

                throw_sys_error("failed to accept connection");
            }

            try {
                start_connection(connectionFd);
            }
            catch (const std::exception& ex) {
                println_locked("failed to start connection: {}", ex.what());
            }
        }
    }

    void process_connection_event(int fd, std::uint32_t eventFlags)
    {
        std::shared_ptr<Connection> connection;

        {
            std::scoped_lock lock(m_mutex);

            const auto connectionIt = m_connections.find(fd);

            if (connectionIt == m_connections.end()) {
                return;
            }

            connection = connectionIt->second;
        }

        bool finish = false;

        try
        {
            if ((eventFlags & EPOLLIN) != 0) {
                finish = !connection->receive_socket_data();
            }

            if (!finish && (eventFlags & EPOLLERR) != 0) {
                finish = true;
            }

            if (!finish && (eventFlags & (EPOLLHUP | EPOLLRDHUP)) != 0) {
                finish = true;
            }
        }
        catch (const std::exception& ex)
        {
            println_locked("epoll client error: {}", ex.what());
            finish = true;
        }

        if (finish) {
            close_connection(fd);
        }
    }

    void close_connection(int fd)
    {
        std::shared_ptr<Connection> connection;

        {
            std::scoped_lock lock(m_mutex);

            const auto connectionIt = m_connections.find(fd);

            if (connectionIt == m_connections.end()) {
                return;
            }

            connection = std::move(connectionIt->second);
            m_connections.erase(connectionIt);
        }

        // Remove the connection from shared state before closing it.
        ::epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);

        close_connection_fd(fd);
        connection->close();
    }

    static bool exit_on_enter()
    {
        std::array<char, 256> inputBuffer{};

        while (true)
        {
            const ssize_t received = ::read(
                STDIN_FILENO,
                inputBuffer.data(),
                inputBuffer.size());

            if (received > 0)
            {
                for (ssize_t i = 0; i < received; ++i)
                {
                    if (inputBuffer[static_cast<std::size_t>(i)] == '\n') {
                        return true;
                    }
                }

                continue;
            }

            if (received == 0) {
                return false;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            }

            throw_sys_error("failed to read from stdin");
        }
    }
};

int main(int argc, char** argv)
{
    const int port = std::stoi(argc > 1 ? argv[1] : "9090");
    if (port < 1 || port > 65535) {
        throw std::invalid_argument("invalid port");
    }
    
    try
    {
        Server server(port); // this will block until user press 'Enter' or an error occurs
        return 0;
    }
    catch (const std::exception& ex)
    {
        println_locked("error: {}", ex.what());
        return 1;
    }
}