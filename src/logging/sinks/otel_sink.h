#ifndef OTEL_SINK_H
#define OTEL_SINK_H

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "log_sink.h"

struct otel_sink_opts {
    std::string endpoint;
    std::string auth_token;
    log_level min_log_level = log_level::information;
    bool use_local_time = true;
    bool flush_on_write = true;
};

class otel_sink : public log_sink {
private:
    int m_read_fd = -1;
    int m_write_fd = -1;
    std::jthread m_worker;
    std::string m_endpoint;
    std::string m_host;
    std::string m_path;
    std::string m_authorization_header;
    int m_port = 4318;
    std::atomic_bool m_stopped{false};

public:
    explicit otel_sink(const otel_sink_opts& opts)
        : otel_sink(opts, create_socket_pair())
    {
    }

private:
    otel_sink(const otel_sink_opts& opts, std::pair<int, int> fds)
        : log_sink(log_sink_opts{
              .fd = fds.first,
              .min_log_level = opts.min_log_level,
              .use_local_time = opts.use_local_time,
              .use_structured_log = true,
              .use_colors = false,
              .flush_on_write = opts.flush_on_write,
          })
        , m_read_fd(fds.second)
        , m_write_fd(m_fd())
        , m_worker([this](std::stop_token st) { drain_loop(st); })
        , m_endpoint(opts.endpoint)
    {
        if (m_endpoint.empty()) {
            throw std::invalid_argument{"endpoint cannot be empty"};
        }

        if (opts.auth_token.empty()) {
            throw std::invalid_argument{"auth_token cannot be empty"};
        }
        
        parse_endpoint();
        if (!opts.auth_token.empty()) {
            m_authorization_header = std::format("Authorization: {}\r\n", opts.auth_token);
        }
    }

public:

    ~otel_sink() {
        m_stopped.store(true, std::memory_order_release);
        if (m_write_fd >= 0) {
            ::shutdown(m_write_fd, SHUT_RDWR);
            ::close(m_write_fd);
        }
        if (m_read_fd >= 0) {
            ::close(m_read_fd);
        }
    }

private:
    static std::pair<int, int> create_socket_pair() {
        int fds[2] = {-1, -1};
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
            return {-1, -1};
        }
        return {fds[0], fds[1]};
    }

    void parse_endpoint() {
        std::string endpoint = m_endpoint;
        const std::string prefix = "http://";
        if (endpoint.rfind(prefix, 0) == 0) {
            endpoint.erase(0, prefix.size());
        }

        const std::size_t slash = endpoint.find('/');
        const std::string authority = slash == std::string::npos ? endpoint : endpoint.substr(0, slash);
        const std::string path = slash == std::string::npos ? "/v1/logs" : endpoint.substr(slash);

        const std::size_t colon = authority.rfind(':');
        if (colon == std::string::npos) {
            m_host = authority;
            m_port = 4318;
        } else {
            m_host = authority.substr(0, colon);
            m_port = std::stoi(authority.substr(colon + 1));
        }

        m_path = path.empty() ? "/v1/logs" : path;
    }

    void drain_loop(std::stop_token stop_token) {
        char buffer[4096];
        std::string pending;

        while (!stop_token.stop_requested()) {
            const ssize_t read_count = ::read(m_read_fd, buffer, sizeof(buffer) - 1);
            if (read_count <= 0) {
                break;
            }

            pending.append(buffer, static_cast<std::size_t>(read_count));

            std::size_t pos = 0;
            while ((pos = pending.find('\n')) != std::string::npos) {
                std::string payload = pending.substr(0, pos);
                pending.erase(0, pos + 1);
                if (!payload.empty()) {
                    send_payload(payload);
                }
            }
        }
    }

    void send_payload(const std::string& payload) const {
        const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            return;
        }

        struct sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(m_port));

        struct hostent* host = ::gethostbyname(m_host.c_str());
        if (host == nullptr) {
            ::close(fd);
            return;
        }

        std::memcpy(&address.sin_addr, host->h_addr_list[0], static_cast<std::size_t>(host->h_length));

        if (::connect(fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
            ::close(fd);
            return;
        }

        const std::string request = std::format(
            "POST {} HTTP/1.1\r\n"
            "Host: {}\r\n"
            "{}"
            "Content-Type: application/json\r\n"
            "User-Agent: chat-app-v1\r\n"
            "Content-Length: {}\r\n"
            "Connection: close\r\n\r\n"
            "{}",
            m_path,
            m_host,
            m_authorization_header,
            payload.size(),
            payload);

        ::send(fd, request.data(), request.size(), 0);
        ::close(fd);
    }
};

#endif
