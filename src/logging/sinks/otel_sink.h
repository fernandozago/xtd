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
        static std::string get_hostname() {
            static const std::string hostname = []() {
                char buffer[256];
                if (::gethostname(buffer, sizeof(buffer)) == 0) {
                    return std::string{buffer};
                }
                return std::string{"unknown"};
            }();

            return hostname;
        }

        static constexpr std::string_view request_header =
            "{{\"resourceLogs\":["
                "{{\"resource\":{{\"attributes\":["
                    "{{\"key\":\"service.namespace\",\"value\":{{\"stringValue\":\"{}\"}}}}"
                    ",{{\"key\":\"service.name\",\"value\":{{\"stringValue\":\"{}\"}}}}"
                    ",{{\"key\":\"service.version\",\"value\":{{\"stringValue\":\"{}\"}}}}"
                    ",{{\"key\":\"service.instance.id\",\"value\":{{\"stringValue\":\"{}\"}}}}"
                    ",{{\"key\":\"host.name\",\"value\":{{\"stringValue\":\"{}\"}}}}"
                    ",{{\"key\":\"deployment.environment.name\",\"value\":{{\"stringValue\":\"{}\"}}}}"
                    ",{{\"key\":\"telemetry.sdk.name\",\"value\":{{\"stringValue\":\"xtd.logging\"}}}}"
                    ",{{\"key\":\"telemetry.sdk.language\",\"value\":{{\"stringValue\":\"cpp\"}}}}"
                "]}},"
                "\"scopeLogs\":["
                    "{{\"scope\":{{\"name\":\"xtd.logging\",\"version\":\"1.0.0\"}},"
                    "\"logRecords\":[";

        static constexpr std::string_view request_footer = "]}]}]}";

        otel_sink_opts m_opts;
        std::string m_host;
        std::string m_path;
        std::string m_request_prefix;
        mutable std::string m_body;

        int m_port = 80;

        xtd::channel<std::shared_ptr<log_message>> m_channel;
        xtd::channel_writer<std::shared_ptr<log_message>> m_writer;
        std::jthread m_worker;

        static void process_messages(xtd::channel<std::shared_ptr<log_message>>& channel, otel_sink& sink)
        {
            xtd::channel_reader<std::shared_ptr<log_message>> reader{channel};

            while (auto message = reader.read()) {
                // Append Request Data
                std::format_to(std::back_inserter(sink.m_body), request_header,
                    sink.m_opts.service_namespace, sink.m_opts.service_name, sink.m_opts.service_version,
                    sink.m_opts.service_instance_id, get_hostname(), sink.m_opts.environment_name);

                sink.m_body += otel_serializer::serialize_record(**message);

                size_t msg_count = 1;

                // Append Records
                while (msg_count < 100) {
                    auto next_message = reader.try_read();
                    if (!next_message) break;

                    sink.m_body += ',';
                    sink.m_body += otel_serializer::serialize_record(**next_message);

                    ++msg_count;
                }

                // Append Footer
                sink.m_body += request_footer;

                const size_t content_length = sink.m_body.size();

                // Prepend HTTP Headers
                sink.m_body.insert(0, "\r\n\r\n");
                sink.m_body.insert(0, std::to_string(content_length));
                sink.m_body.insert(0, "Content-Length: ");
                sink.m_body.insert(0, sink.m_request_prefix);

                sink.write_to_output(sink.m_body);
                sink.m_body.clear();
            }
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

            m_request_prefix = std::format(
                "POST {} HTTP/1.1\r\n"
                "Host: {}:{}\r\n"
                "Authorization: {}\r\n"
                "stream-name: {}\r\n"
                "Content-Type: application/json\r\n"
                "Connection: close\r\n",
                m_path, m_host, m_port,
                m_opts.auth_token, m_opts.service_namespace);

            m_worker = std::jthread{process_messages, std::ref(m_channel), std::ref(*this)};
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
    #ifdef OTEL_SINK_DEBUG
            std::println("-> Request: {}", data);
    #endif

            const int fd = connect_to_host();

            if (fd < 0) {
    #ifdef OTEL_SINK_DEBUG
                std::println("<- Response [Error: failed to connect to {}:{}]", m_host, m_port);
    #endif
                return;
            }

            if (!send_all(fd, data)) {
    #ifdef OTEL_SINK_DEBUG
                std::println("<- Response [Error: send failed]");
    #endif
                ::close(fd);
                return;
            }

    #ifdef OTEL_SINK_DEBUG
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
            while (true) {
                const ssize_t received = ::recv(fd, buffer, sizeof(buffer), 0);
                if (received > 0) continue;
                if (received < 0 && errno == EINTR) continue;
                break;
            }
        }

    #ifdef OTEL_SINK_DEBUG
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

}

#endif