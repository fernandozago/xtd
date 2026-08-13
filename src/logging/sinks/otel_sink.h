#ifndef OTEL_SINK_H
#define OTEL_SINK_H

// #define OTEL_SINK_DEBUG

#include <cerrno>
#include <format>
#include <netdb.h>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

#include <format>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

#include <curl/curl.h>
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

    int m_port = 80;

public:
    explicit otel_sink(const otel_sink_opts& opts)
        : log_sink(log_sink_opts{
              .fd = -1, /* UNUSED */
              .min_log_level = opts.min_log_level,
              .use_local_time = opts.use_local_time,
              .use_structured_log = true,
              .use_colors = false,
              .flush_on_write = true,
          })
        , m_opts(opts)
    {
        if (m_opts.endpoint.empty()) {
            throw std::invalid_argument{"endpoint cannot be empty"};
        }

        if (m_opts.auth_token.empty()) {
            throw std::invalid_argument{"auth_token cannot be empty"};
        }

        if (m_opts.stream_name.empty()) {
            throw std::invalid_argument{"stream_name cannot be empty"};
        }

        if (m_opts.service_name.empty()) {
            throw std::invalid_argument{"service_name cannot be empty"};
        }

        parse_endpoint(m_opts.endpoint);
    }

protected:
    void write_all(std::string_view data) const override
    {
        if (!data.empty() && data.back() == '\n') {
            data.remove_suffix(1);
        }

        const std::string body = std::format(request_payload, m_opts.service_name, instrumentation_library_name, data);
        const std::string request = std::format(header_payload, m_path, m_host, m_port, m_opts.auth_token, m_opts.stream_name, body.size(), body);

        #ifdef OTEL_SINK_DEBUG
            std::println("OTEL request:\n{}", request);
        #endif

        const int fd = connect_to_host();

        if (fd < 0) {
            std::println("OTEL: failed to connect to {}:{}", m_host, m_port);
            return;
        }

        if (!send_all(fd, request)) {
            std::println("OTEL: failed to send request");
            ::close(fd);
            return;
        }

        char buffer[4096];
        #ifdef OTEL_SINK_DEBUG
            std::string response;
            while (true) {
                const ssize_t received = ::recv(fd, buffer, sizeof(buffer), 0);

                if (received >= 0) {
                    if (received == 0) break;
                    response.append(buffer, static_cast<std::size_t>(received));
                    continue;
                }

                if (errno != EINTR) {
                    break;
                }
            }

            std::println("OTEL response:\n{}", response);
        #else
            while (true) {
                const ssize_t received = ::recv(fd, buffer, sizeof(buffer), 0);

                if (received >= 0) {
                    if (received == 0) break;
                    continue;
                }

                if (errno != EINTR) {
                    break;
                }
            }
        #endif

        ::close(fd);
    }

private:
    void parse_endpoint(std::string endpoint)
    {
        constexpr std::string_view http_prefix = "http://";
        constexpr std::string_view https_prefix = "https://";

        if (endpoint.starts_with(https_prefix)) {
            throw std::invalid_argument{"https is not supported by this sink yet"};
        }

        if (endpoint.starts_with(http_prefix)) {
            endpoint.erase(0, http_prefix.size());
        }

        const std::size_t slash = endpoint.find('/');
        const std::string authority = endpoint.substr(0, slash);

        m_path =
            slash == std::string::npos
                ? "/"
                : endpoint.substr(slash);

        const std::size_t colon = authority.rfind(':');

        if (colon == std::string::npos) {
            m_host = authority;
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
        hints.ai_flags = 0;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* addresses = nullptr;
        const std::string port = std::to_string(m_port);

        if (::getaddrinfo(m_host.c_str(), port.c_str(), &hints, &addresses) != 0) {
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

            if (::connect(fd, address->ai_addr, address->ai_addrlen) == 0) {
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
};


class curl_otel_sink final : public log_sink {
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

    CURL* m_curl = nullptr;
    curl_slist* m_headers = nullptr;

    otel_sink_opts m_opts;

    static size_t discard_response(char*, size_t size, size_t count, void*)
    {
        return size * count;
    }

public:
    explicit curl_otel_sink(const otel_sink_opts& opts)
        : log_sink(log_sink_opts{
              .fd = -1,
              .min_log_level = opts.min_log_level,
              .use_local_time = opts.use_local_time,
              .use_structured_log = true,
              .use_colors = false,
              .flush_on_write = true,
          }),
          m_opts(opts)
    {
        if (m_opts.endpoint.empty()) {
            throw std::invalid_argument{"endpoint cannot be empty"};
        }

        m_curl = curl_easy_init();

        if (!m_curl) throw std::runtime_error{"curl_easy_init failed"};

        const std::string auth = std::format("Authorization: {}", m_opts.auth_token);
        const std::string stream = std::format("stream-name: {}", m_opts.stream_name);
        m_headers = curl_slist_append(m_headers, "Content-Type: application/json");
        m_headers = curl_slist_append(m_headers, auth.c_str());
        m_headers = curl_slist_append(m_headers, stream.c_str());

        curl_easy_setopt(m_curl, CURLOPT_URL, m_opts.endpoint.c_str());
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);
        curl_easy_setopt(m_curl, CURLOPT_POST, 1L);

        // For http:// where the server supports h2c directly.
        curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);

        // We don't care about the response payload.
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &discard_response);

        // Do not let logging block indefinitely.
        curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 1000L);
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, 3000L);
    }

    ~curl_otel_sink() override
    {
        if (m_curl)
            curl_easy_cleanup(m_curl);

        if (m_headers)
            curl_slist_free_all(m_headers);
    }

protected:
    void write_all(std::string_view data) const override
    {
        if (!data.empty() && data.back() == '\n')
            data.remove_suffix(1);

        const std::string body = std::format(request_payload, m_opts.service_name, instrumentation_library_name, data);

        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, body.data());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size()));
        const CURLcode result = curl_easy_perform(m_curl);
        if (result != CURLE_OK) {
            #ifdef OTEL_SINK_DEBUG
                std::println("OTEL: curl error: {}", curl_easy_strerror(result));
            #endif
        }

    }
};

#endif