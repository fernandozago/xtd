#ifndef OTEL_NATIVE_TRANSPORT_H
#define OTEL_NATIVE_TRANSPORT_H

#include <cerrno>
#include <fcntl.h>
#include <format>
#ifdef OTEL_DEBUG_TRANSPORT
    #include <print>
#endif
#include <netdb.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

#include "logging/sinks/otel_transports/transport_base.h"

namespace xtd {

struct otel_native_transport : otel_transport_base {

    explicit otel_native_transport(const otel_sink_opts& opts)
    {
        if (opts.endpoint.empty()) {
            throw std::invalid_argument{"endpoint cannot be empty"};
        }

        parse_endpoint(opts.endpoint);
        build_request_prefix(opts);
    }

    void send(std::string_view json_body) const override
    {
        const std::string content_length = std::format("Content-Length: {}\r\n\r\n", json_body.size());

#ifdef OTEL_DEBUG_TRANSPORT
        std::string request;
        request.reserve(m_request_prefix.size() + content_length.size() + json_body.size());
        request += m_request_prefix;
        request += content_length;
        request += json_body;
        std::println("-> Request: {}", request);
#endif

        const int fd = connect_to_host();

        if (fd < 0) {
#ifdef OTEL_DEBUG_TRANSPORT
            std::println("<- Response [Error: failed to connect to {}:{}]", m_host, m_port);
#endif
            return;
        }

        if (!send_all(fd, m_request_prefix)
            || !send_all(fd, content_length)
            || !send_all(fd, json_body)) {
#ifdef OTEL_DEBUG_TRANSPORT
            std::println("<- Response [Error: send failed]");
#endif
            ::close(fd);
            return;
        }

#ifdef OTEL_DEBUG_TRANSPORT
        const std::string response = read_response(fd);
        std::println("<- Response [StatusCode={}] -> {}", parse_status_code(response), response);
#else
        discard_response(fd);
#endif
        ::close(fd);
    }

private:
    void parse_endpoint(std::string endpoint)
    {
        constexpr std::string_view http_prefix = "http://";
        constexpr std::string_view https_prefix = "https://";

        if (endpoint.starts_with(https_prefix)) {
            throw std::invalid_argument{"https is not supported by this transport"};
        }

        if (endpoint.starts_with(http_prefix)) {
            endpoint.erase(0, http_prefix.size());
        }

        const std::size_t slash = endpoint.find('/');

        const std::string authority =
            slash == std::string::npos
                ? endpoint
                : endpoint.substr(0, slash);

        m_path =
            slash == std::string::npos
                ? "/"
                : endpoint.substr(slash);

        if (authority.empty()) {
            throw std::invalid_argument{"invalid endpoint host"};
        }

        if (authority.front() == '[') {
            m_ipv6 = true;

            const std::size_t closing_bracket = authority.find(']');

            if (closing_bracket == std::string::npos) {
                throw std::invalid_argument{"invalid endpoint host"};
            }

            m_host = authority.substr(1, closing_bracket - 1);

            if (closing_bracket + 1 < authority.size()) {
                if (authority[closing_bracket + 1] != ':') {
                    throw std::invalid_argument{"invalid endpoint"};
                }

                m_port = parse_port(authority.substr(closing_bracket + 2));
            }
        }
        else {
            const std::size_t colon = authority.rfind(':');

            if (colon == std::string::npos) {
                m_host = authority;
            }
            else {
                m_host = authority.substr(0, colon);
                m_port = parse_port(authority.substr(colon + 1));
            }
        }

        if (m_host.empty()) {
            throw std::invalid_argument{"invalid endpoint host"};
        }
    }

    [[nodiscard]]
    static int parse_port(const std::string& value)
    {
        if (value.empty()) {
            throw std::invalid_argument{"invalid endpoint port"};
        }

        std::size_t parsed = 0;
        const int port = std::stoi(value, &parsed);

        if (parsed != value.size() || port <= 0 || port > 65535) {
            throw std::invalid_argument{"invalid endpoint port"};
        }

        return port;
    }

    [[nodiscard]]
    int connect_to_host() const
    {
        addrinfo hints{};
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* addresses = nullptr;
        const std::string port = std::to_string(m_port);
        const int gai_result = ::getaddrinfo(m_host.c_str(), port.c_str(), &hints, &addresses);

        if (gai_result != 0) {
#ifdef OTEL_DEBUG_TRANSPORT
            std::println("OTEL: getaddrinfo failed: {}", ::gai_strerror(gai_result));
#endif
            return -1;
        }

        int fd = -1;

        for (addrinfo* address = addresses; address != nullptr; address = address->ai_next) {
            fd = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);

            if (fd < 0) continue;

            const int original_flags = ::fcntl(fd, F_GETFL, 0);

            if (original_flags < 0 || ::fcntl(fd, F_SETFL, original_flags | O_NONBLOCK) < 0) {
                ::close(fd);
                fd = -1;
                continue;
            }

            const int connect_result = ::connect(fd, address->ai_addr, address->ai_addrlen);
            bool connected = false;

            if (connect_result == 0) {
                connected = true;
            }
            else if (errno == EINPROGRESS) {
                pollfd poll_fd{};
                poll_fd.fd     = fd;
                poll_fd.events = POLLOUT;

                int poll_result;
                do { poll_result = ::poll(&poll_fd, 1, 1000); } while (poll_result < 0 && errno == EINTR);

                if (poll_result > 0) {
                    int socket_error = 0;
                    socklen_t socket_error_size = sizeof(socket_error);

                    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_size) == 0
                            && socket_error == 0) {
                        connected = true;
                    }
                }
            }

            if (!connected) { ::close(fd); fd = -1; continue; }

            if (::fcntl(fd, F_SETFL, original_flags) < 0) { ::close(fd); fd = -1; continue; }

            timeval timeout{};
            timeout.tv_sec  = 3;
            timeout.tv_usec = 0;
            (void)::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
            (void)::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            break;
        }

        ::freeaddrinfo(addresses);
        return fd;
    }

    [[nodiscard]]
    static bool send_all(int fd, std::string_view data)
    {
        while (!data.empty()) {
            const ssize_t sent = ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);

            if (sent > 0) { data.remove_prefix(static_cast<std::size_t>(sent)); continue; }
            if (sent < 0 && errno == EINTR) continue;
            return false;
        }
        return true;
    }

    static void discard_response(int fd)
    {
        char buffer[4096];
        while (true) {
            const ssize_t received = ::recv(fd, buffer, sizeof(buffer), 0);
            if (received > 0) continue;
            if (received < 0 && errno == EINTR) continue;
            break;
        }
    }

#ifdef OTEL_DEBUG_TRANSPORT
    static int parse_status_code(std::string_view response)
    {
        const auto sp1 = response.find(' ');
        if (sp1 == std::string_view::npos) return 0;
        const auto sp2 = response.find_first_of(" \r\n", sp1 + 1);
        int code = 0;
        for (char c : response.substr(sp1 + 1, sp2 == std::string_view::npos ? sp2 : sp2 - sp1 - 1)) {
            if (c < '0' || c > '9') return 0;
            code = code * 10 + (c - '0');
        }
        return code;
    }

    static std::string read_response(int fd)
    {
        std::string response;
        char buffer[4096];
        while (true) {
            const ssize_t received = ::recv(fd, buffer, sizeof(buffer), 0);
            if (received > 0) { response.append(buffer, static_cast<std::size_t>(received)); continue; }
            if (received < 0 && errno == EINTR) continue;
            break;
        }
        return response;
    }
#endif
};

} // namespace xtd

#endif
