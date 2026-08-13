#ifndef OTEL_SINK_H
#define OTEL_SINK_H

#include <cerrno>
#include <cstring>
#include <format>
#include <netdb.h>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

#include "log_sink.h"

struct otel_sink_opts {
    std::string endpoint;
    std::string auth_token;
    std::string stream_name;
    std::string service_name;

    log_level min_log_level = log_level::information;

    bool use_local_time = false;
};

class otel_sink final : public log_sink {
private:
    static constexpr std::string_view instrumentation_library_name = "xtd.logging";

    std::string m_host;
    std::string m_path;
    std::string m_auth_token;
    std::string m_stream_name;
    std::string m_service_name;
    std::string m_request_template;
    std::string m_payload_prefix;
    std::string m_payload_suffix;

    int m_port = 5080;

public:
    explicit otel_sink(const otel_sink_opts& opts)
        : log_sink(log_sink_opts{
              .fd = STDOUT_FILENO,
              .min_log_level = opts.min_log_level,
              .use_local_time = opts.use_local_time,
              .use_structured_log = true,
              .use_colors = false,
              .flush_on_write = true,
          })
        , m_auth_token(opts.auth_token)
        , m_stream_name(opts.stream_name)
        , m_service_name(opts.service_name)
    {
        if (opts.endpoint.empty()) {
            throw std::invalid_argument{"endpoint cannot be empty"};
        }

        if (m_auth_token.empty()) {
            throw std::invalid_argument{"auth_token cannot be empty"};
        }

        if (m_stream_name.empty()) {
            throw std::invalid_argument{"stream_name cannot be empty"};
        }

        if (m_service_name.empty()) {
            throw std::invalid_argument{"service_name cannot be empty"};
        }

        parse_endpoint(opts.endpoint);

        m_request_template = std::format(
            "POST {} HTTP/1.1\r\n"
            "Host: {}:{}\r\n"
            "Authorization: {}\r\n"
            "stream-name: {}\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n"
            "Content-Length: {{}}\r\n"
            "\r\n"
            "{{}}",
            m_path,
            m_host,
            m_port,
            m_auth_token,
            m_stream_name);

        m_payload_prefix = std::format(
            "{{\"resourceLogs\":[{{\"resource\":{{\"attributes\":[{{\"key\":\"service.name\",\"value\":{{\"stringValue\":\"{}\"}}}}]}},"
            "\"scopeLogs\":[{{\"scope\":{{\"name\":\"{}\"}},\"logRecords\":[",
            m_service_name,
            instrumentation_library_name);

        m_payload_suffix = R"(]}]}]})";
    }

protected:
    void write_all(std::string_view data) const override
    {
        if (!data.empty() && data.back() == '\n') {
            data.remove_suffix(1);
        }

        std::string payload;
        payload.reserve(m_payload_prefix.size() + data.size() + m_payload_suffix.size());
        payload += m_payload_prefix;
        payload += data;
        payload += m_payload_suffix;

        const int fd = connect_to_host();
        if (fd < 0) {
            std::println("OTEL: failed to connect to {}:{}", m_host, m_port);
            return;
        }

        const std::string request = build_request(payload);

        // std::println("OTEL payload: {}", payload);

        if (!send_all(fd, request)) {
            std::println("OTEL: failed to send request");
            ::close(fd);
            return;
        }

        std::string response;
        char buffer[4096];

        while (true) {
            const ssize_t received = ::recv(fd, buffer, sizeof(buffer), 0);

            if (received > 0) {
                response.append(
                    buffer,
                    static_cast<std::size_t>(received));

                continue;
            }

            if (received < 0 && errno == EINTR) {
                continue;
            }

            break;
        }

        ::close(fd);

        // if (!response.empty()) {
        //     std::println("OTEL response:\n{}", response);
        // }
    }

private:
    void parse_endpoint(std::string endpoint)
    {
        constexpr std::string_view http_prefix = "http://";
        constexpr std::string_view https_prefix = "https://";

        if (endpoint.starts_with(https_prefix)) {
            throw std::invalid_argument{
                "https is not supported by this sink yet"
            };
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

        const std::size_t colon = authority.rfind(':');

        if (colon == std::string::npos) {
            m_host = authority;
            m_port = 80;
        }
        else {
            m_host = authority.substr(0, colon);
            m_port = std::stoi(authority.substr(colon + 1));
        }

        if (m_host.empty()) {
            throw std::invalid_argument{"invalid endpoint host"};
        }
    }

    [[nodiscard]]
    int connect_to_host() const
    {
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* addresses = nullptr;

        const std::string port = std::to_string(m_port);

        if (::getaddrinfo(
                m_host.c_str(),
                port.c_str(),
                &hints,
                &addresses) != 0) {
            return -1;
        }

        int fd = -1;

        for (addrinfo* address = addresses;
             address != nullptr;
             address = address->ai_next) {

            fd = ::socket(
                address->ai_family,
                address->ai_socktype,
                address->ai_protocol);

            if (fd < 0) {
                continue;
            }

            if (::connect(
                    fd,
                    address->ai_addr,
                    address->ai_addrlen) == 0) {
                break;
            }

            ::close(fd);
            fd = -1;
        }

        ::freeaddrinfo(addresses);

        return fd;
    }

    [[nodiscard]]
    static bool send_all(int fd, std::string_view data)
    {
        while (!data.empty()) {
            const ssize_t sent = ::send(
                fd,
                data.data(),
                data.size(),
                MSG_NOSIGNAL);

            if (sent > 0) {
                data.remove_prefix(
                    static_cast<std::size_t>(sent));

                continue;
            }

            if (sent < 0 && errno == EINTR) {
                continue;
            }

            return false;
        }

        return true;
    }

    [[nodiscard]]
    std::string build_request(std::string_view payload) const
    {
        static constexpr std::string_view placeholder = "{}";

        const std::size_t first = m_request_template.find(placeholder);
        if (first == std::string::npos) {
            return m_request_template;
        }

        const std::size_t second = m_request_template.find(placeholder, first + placeholder.size());
        if (second == std::string::npos) {
            return m_request_template;
        }

        const std::string content_length = std::to_string(payload.size());

        std::string request;
        request.reserve(
            m_request_template.size() - (placeholder.size() * 2) + content_length.size() + payload.size());

        request.append(m_request_template, 0, first);
        request += content_length;
        request.append(
            m_request_template,
            first + placeholder.size(),
            second - (first + placeholder.size()));
        request += payload;
        request.append(m_request_template, second + placeholder.size(), std::string::npos);

        return request;
    }
};

#endif