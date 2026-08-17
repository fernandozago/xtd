#ifndef OTEL_SINK_H
#define OTEL_SINK_H

//#define OTEL_SINK_DEBUG

#include <cerrno>
#include <fcntl.h>
#include <format>
#include <memory>
#include <netdb.h>
#ifdef OTEL_SINK_DEBUG
    #include <print>
#endif
#include <poll.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "channel/channel.h"
#include "otel_serializer.h"
#include "log_sink.h"

namespace xtd 
{

    class otel_sink final : public log_sink {
    private:
        static constexpr std::string_view instrumentation_library_name = "xtd.logging";

        static constexpr std::string_view request_payload =
            "{{\"resourceLogs\":["
                "{{\"resource\":{{\"attributes\":["
                    "{{\"key\":\"service.name\",\"value\":{{\"stringValue\":\"{}\"}}}}"
                "]}},"
                "\"scopeLogs\":["
                    "{{\"scope\":{{\"name\":\"{}\"}},"
                    "\"logRecords\":[{}]}}"
                "]"
            "}}]}}";

        static constexpr std::string_view header_payload =
            "POST {} HTTP/1.1\r\n"
            "Host: {}:{}\r\n"
            "Authorization: {}\r\n"
            "stream-name: {}\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n"
            "Content-Length: {}\r\n"
            "\r\n"
            "{}";

        otel_sink_opts m_opts;
        std::string m_host;
        std::string m_path;
        std::string m_body;

        int m_port = 80;

        xtd::channel<std::shared_ptr<log_message>> m_channel;
        xtd::channel_writer<std::shared_ptr<log_message>> m_writer;
        std::jthread m_worker;

        static void process_messages(xtd::channel<std::shared_ptr<log_message>>& channel, otel_sink* const sink)
        {
            xtd::channel_reader<std::shared_ptr<log_message>> reader{channel};

            while (const auto& message = reader.read()) {
                sink->m_body.clear();
                sink->process_message(*message);

                while (auto next_message = reader.try_read()) {
                    sink->process_message(*next_message);
                }

                sink->write_to_output(sink->m_body);
            }
        }

        void process_message(const std::shared_ptr<log_message>& message)
        {
            if (!m_body.empty()) {
                m_body += ',';
            }

            m_body += otel_serializer::serialize_record(*message);
        }

    public:
        explicit otel_sink(const otel_sink_opts& opts)
            : log_sink(log_sink_opts{
                .min_log_level = opts.min_log_level,
                .fd = -1,
                .use_local_time = opts.use_local_time,
                .use_structured_log = true,
                .use_colors = false,
                .flush_on_write = true,
            }, true)
            , m_opts(opts)
            , m_writer{m_channel}
        {
            if (m_opts.endpoint.empty()) {
                throw std::invalid_argument{"endpoint cannot be empty"};
            }

            parse_endpoint(m_opts.endpoint);

            m_worker = std::jthread{
                process_messages,
                std::ref(m_channel),
                this
            };
        }

        ~otel_sink() override
        {
            m_writer.complete();

            if (m_worker.joinable()) {
                m_worker.join();
            }
        }

    protected:
        void write(const std::shared_ptr<log_message>& message) override
        {
            if (message->level() < m_opts.min_log_level) return;
            (void)m_writer.try_push(message);
        }

        void write_to_output(std::string_view data) const override
        {
            const std::string body = std::format(
                request_payload,
                m_opts.service_name,
                instrumentation_library_name,
                data);

            const std::string request = std::format(
                header_payload,
                m_path,
                m_host,
                m_port,
                m_opts.auth_token,
                m_opts.stream_name,
                body.size(),
                body);

    #ifdef OTEL_SINK_DEBUG
            std::println("OTEL request:\n{}", request);
    #endif

            const int fd = connect_to_host();

            if (fd < 0) {
    #ifdef OTEL_SINK_DEBUG
                std::println(
                    "OTEL: failed to connect to {}:{}",
                    m_host,
                    m_port);
    #endif
                return;
            }

            if (!send_all(fd, request)) {
    #ifdef OTEL_SINK_DEBUG
                std::println("OTEL: failed to send request");
    #endif
                ::close(fd);
                return;
            }

            discard_response(fd);
            ::close(fd);
        }

    private:
        void parse_endpoint(std::string endpoint)
        {
            constexpr std::string_view http_prefix = "http://";
            constexpr std::string_view https_prefix = "https://";

            if (endpoint.starts_with(https_prefix)) {
                throw std::invalid_argument{
                    "https is not supported by this sink"
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

            if (authority.empty()) {
                throw std::invalid_argument{"invalid endpoint host"};
            }

            if (authority.front() == '[') {
                const std::size_t closing_bracket = authority.find(']');

                if (closing_bracket == std::string::npos) {
                    throw std::invalid_argument{"invalid endpoint host"};
                }

                m_host = authority.substr(1, closing_bracket - 1);

                if (closing_bracket + 1 < authority.size()) {
                    if (authority[closing_bracket + 1] != ':') {
                        throw std::invalid_argument{"invalid endpoint"};
                    }

                    m_port = parse_port(
                        authority.substr(closing_bracket + 2));
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
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            addrinfo* addresses = nullptr;
            const std::string port = std::to_string(m_port);
            const int gai_result = ::getaddrinfo(m_host.c_str(), port.c_str(), &hints, &addresses);

            if (gai_result != 0) {
    #ifdef OTEL_SINK_DEBUG
                std::println(
                    "OTEL: getaddrinfo failed: {}",
                    ::gai_strerror(gai_result));
    #endif
                return -1;
            }

            int fd = -1;

            for (addrinfo* address = addresses;
                address != nullptr;
                address = address->ai_next) {

                fd = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);

                if (fd < 0) {
                    continue;
                }

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
                    poll_fd.fd = fd;
                    poll_fd.events = POLLOUT;

                    int poll_result;

                    do {
                        poll_result = ::poll(&poll_fd, 1, 1000);
                    } while (poll_result < 0 && errno == EINTR);

                    if (poll_result > 0) {
                        int socket_error = 0;
                        socklen_t socket_error_size = sizeof(socket_error);

                        if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_size) == 0 
                            && socket_error == 0) {
                            connected = true;
                        }
                    }
                }

                if (!connected) {
                    ::close(fd);
                    fd = -1;
                    continue;
                }

                if (::fcntl(fd, F_SETFL, original_flags) < 0) {
                    ::close(fd);
                    fd = -1;
                    continue;
                }

                timeval timeout{};
                timeout.tv_sec = 3;
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

                if (sent > 0) {
                    data.remove_prefix(static_cast<std::size_t>(sent));
                    continue;
                }

                if (sent < 0 && errno == EINTR) {
                    continue;
                }

                return false;
            }

            return true;
        }

        static void discard_response(int fd)
        {
            char buffer[4096];

        #ifdef OTEL_SINK_DEBUG
            std::string response;
        #endif

            while (true) {
                const ssize_t received = ::recv(fd, buffer, sizeof(buffer), 0);

                if (received > 0) {
        #ifdef OTEL_SINK_DEBUG
                    response.append(buffer, static_cast<std::size_t>(received));
        #endif
                    continue;
                }

                if (received < 0 && errno == EINTR) {
                    continue;
                }

                break;
            }

        #ifdef OTEL_SINK_DEBUG
            std::println("OTEL response:\n{}", response);
        #endif
        }
    };

}

#endif