#include <array>
#include <atomic>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <memory>
#include <mutex>
#include <poll.h>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pipeline/pipeline.h"
#include "utils/utils.h"


std::uint16_t parsePort(std::string_view value)
{
    unsigned int parsedPort = 0;
    const auto [ptr, ec] =
        std::from_chars(value.data(), value.data() + value.size(), parsedPort);

    if (ec != std::errc{} ||
        ptr != value.data() + value.size() ||
        parsedPort < 1 ||
        parsedPort > 65535)
    {
        throw std::invalid_argument("invalid port, expected integer in range 1..65535");
    }

    return static_cast<std::uint16_t>(parsedPort);
}

void closeAndThrow(int fd, const std::string& message)
{
    const int error = errno;
    ::close(fd);
    throw std::system_error(error, std::generic_category(), message);
}

void closeConnectionFd(int fd) noexcept
{
    if (fd < 0) {
        return;
    }

    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
}

int createServerSocket(std::uint16_t port)
{
    constexpr int ListenBacklog = 128;

    const int listenFd =
        ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

    if (listenFd < 0) {
        throw std::system_error(errno, std::generic_category(), "failed to create listen socket");
    }

    int reuseAddr = 1;

    if (::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) != 0) {
        closeAndThrow(listenFd, "failed to set SO_REUSEADDR");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if (::bind(listenFd, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0) {
        closeAndThrow(listenFd, "failed to bind on port " + std::to_string(port));
    }

    if (::listen(listenFd, ListenBacklog) != 0) {
        closeAndThrow(listenFd, "failed to listen");
    }

    return listenFd;
}

void addToEpoll(int epollFd, int fd, std::uint32_t events)
{
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;

    if (::epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) != 0) {
        throw std::system_error(errno, std::generic_category(), "failed to add fd to epoll");
    }
}

struct ServerState;

class Connection
{
public:
    explicit Connection(int connectionFd)
        : fd(connectionFd),
          pipeline({.buffer_size = 128}),
          writer(pipeline.writer())
    {
    }

    void startWorker()
    {
        workerThread = std::thread([this]()
        {
            run();
        });
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
        inputCompletedByEpoll.store(true, std::memory_order_release);
        fd.store(-1, std::memory_order_release);
        writer.complete();
    }

    void write(const char* data, std::size_t size)
    {
        writer.write(reinterpret_cast<const std::byte*>(data), size);
    }

    bool copyFromSocket()
    {
        static std::array<char, 4096> buffer{};

        while (true)
        {
            const ssize_t received = ::recv(fd.load(std::memory_order_acquire), buffer.data(), buffer.size(), 0);

            if (received == 0) {
                return false;
            }

            if (received > 0)
            {
                try {
                    write(buffer.data(), static_cast<std::size_t>(received));
                }
                catch (const std::exception& ex) {
                    println_locked("error writing to pipeline: {}", ex.what());
                    return false;
                }

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

private:

    void sendAllSync(std::string_view message)
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
                if (!waitUntilWritable(currentFd)) {
                    throw std::runtime_error("socket is no longer writable");
                }

                continue;
            }

            throw std::system_error(errno, std::generic_category(), "failed to send response");
        }
    }

    void run() noexcept
    {
        try
        {
            println_locked("client connected");

            sendAllSync("[Server: You are connected. This is a sample echo server. Welcome using EPOLL!]\n");

            constexpr char Delimiter = '\n';

            auto& reader = pipeline.reader();
            xtd::position newLinePos{};
            while (true)
            {
                const xtd::read_result result = reader.read();
                xtd::segmented_byte_view ros = result.buffer();
                println_locked("segmented_byte_view (size: {}, Segments: {})", ros.size(), ros.segment_count());
                while (ros.position_of(Delimiter, newLinePos))
                {
                    std::string message = ros.slice(newLinePos).to_string();
                    println_locked("Sending back: [Server: {}]", message);
                    sendAllSync("[Server: " + message + "]\n");
                    ros = ros.slice(newLinePos + 1, ros.end());
                }

                reader.advance(ros.start(), ros.end());

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
    static bool waitUntilWritable(int fd)
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

    std::atomic<int> fd;
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer;
    std::mutex sendMutex;
    std::atomic_bool inputCompletedByEpoll{false};
    std::thread workerThread;
};

struct ServerState
{
    int epollFd = -1;
    std::unordered_map<int, std::shared_ptr<Connection>> connections;
};

void finishConnection(ServerState& state, int fd)
{
    auto connectionIt = state.connections.find(fd);
    if (connectionIt == state.connections.end()) return;

    std::shared_ptr<Connection> connection = connectionIt->second;

    ::epoll_ctl(state.epollFd, EPOLL_CTL_DEL, fd, nullptr);

    closeConnectionFd(fd);

    connection->close();

    state.connections.erase(connectionIt);
}

bool shouldExitOnEnter()
{
    static std::array<char, 256> inputBuffer{};

    while (true)
    {
        const ssize_t received = ::read(STDIN_FILENO, inputBuffer.data(), inputBuffer.size());

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

        throw std::system_error(errno, std::generic_category(), "failed to read from stdin");
    }
}

void startConnection(ServerState& state, int connectionFd)
{
    auto connection = std::make_shared<Connection>(connectionFd);

    state.connections.emplace(connectionFd, connection);

    try
    {
        addToEpoll(state.epollFd, connectionFd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP);
    }
    catch (...)
    {
        closeConnectionFd(connectionFd);
        state.connections.erase(connectionFd);
        throw;
    }

    try
    {
        connection->startWorker();
    }
    catch (...)
    {
        finishConnection(state, connectionFd);
        throw;
    }
}

void acceptPendingConnections(ServerState& state, int listenFd)
{
    while (true)
    {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        const int connectionFd = ::accept4(
            listenFd, 
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

            throw std::system_error(errno, std::generic_category(), "failed to accept connection");
        }

        startConnection(state, connectionFd);
    }
}

void cleanupServer(ServerState& state)
{
    for (auto& [fd, connection] : state.connections)
    {
        closeConnectionFd(fd);
        connection->close();
    }

    state.connections.clear();

    if (state.epollFd >= 0) {
        ::close(state.epollFd);
        state.epollFd = -1;
    }
}

int main(int argc, char** argv)
{
    int listenFd = -1;
    ServerState state;

    try
    {
        const std::uint16_t port = argc > 1 ? parsePort(argv[1]) : 9090;

        listenFd = createServerSocket(port);

        state.epollFd = ::epoll_create1(EPOLL_CLOEXEC);

        if (state.epollFd < 0) {
            closeAndThrow(listenFd, "failed to create epoll instance");
        }

        addToEpoll(state.epollFd, listenFd, EPOLLIN);
        addToEpoll(state.epollFd, STDIN_FILENO, EPOLLIN);

        println_locked("Server is listening on port: {}", port);
        println_locked("Press Enter to stop the server.");

        std::array<epoll_event, 128> events{};
        bool shutdownRequested = false;

        while (!shutdownRequested)
        {
            const int count = ::epoll_wait(state.epollFd, events.data(), static_cast<int>(events.size()), -1);

            if (count < 0)
            {
                if (errno == EINTR) continue;
                throw std::system_error(errno, std::generic_category(), "epoll_wait failed");
            }

            //printMessageConsole("epoll_wait returned " + std::to_string(count) + " events");

            for (int i = 0; i < count; ++i)
            {
                const int fd = events[i].data.fd;
                const std::uint32_t eventFlags = events[i].events;

                if (fd == listenFd)
                {
                    acceptPendingConnections(state, listenFd);
                    continue;
                }

                if (fd == STDIN_FILENO)
                {
                    if (shouldExitOnEnter()) {
                        println_locked("Shutdown requested by user input.");
                        shutdownRequested = true;
                    }
                    continue;
                }

                auto connectionIt = state.connections.find(fd);
                if (connectionIt == state.connections.end()) continue;

                bool finish = false;
                try
                {
                    if ((eventFlags & EPOLLIN) != 0)
                    {
                        finish = !connectionIt->second->copyFromSocket();
                    }

                    if (!finish && (eventFlags & EPOLLERR) != 0)
                    {
                        finish = true;
                    }

                    if (!finish && (eventFlags & (EPOLLHUP | EPOLLRDHUP)) != 0)
                    {
                        finish = true;
                    }
                }
                catch (const std::exception& ex)
                {
                    println_locked("epoll client error: {}", ex.what());
                    finish = true;
                }

                if (finish) {
                    finishConnection(state, fd);
                }
            }
        }

        cleanupServer(state);

        if (listenFd >= 0) {
            ::close(listenFd);
        }

        return 0;
    }
    catch (const std::exception& ex)
    {
        cleanupServer(state);

        if (listenFd >= 0) {
            ::close(listenFd);
        }

        println_locked("error: {}", ex.what());
        return 1;
    }
}