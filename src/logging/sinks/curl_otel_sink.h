#ifndef CURL_OTEL_SINK_H
#define CURL_OTEL_SINK_H
// #define OTEL_SINK_DEBUG

#include <format>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#ifdef CURL_OTEL_SINK_DEBUG
    #include <print>
#endif

#include <curl/curl.h>
#include "log_sink.h"
#include "otel_sink.h"

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

    CURL* m_curl = nullptr;
    curl_slist* m_headers = nullptr;

    otel_sink_opts m_opts;

    static size_t discard_response(
        char*,
        size_t size,
        size_t count,
        void*)
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
        if (m_opts.endpoint.empty())
            throw std::invalid_argument{"endpoint cannot be empty"};

        m_curl = curl_easy_init();

        if (!m_curl)
            throw std::runtime_error{"curl_easy_init failed"};

        const std::string auth =
            std::format("Authorization: {}", m_opts.auth_token);

        const std::string stream =
            std::format("stream-name: {}", m_opts.stream_name);

        m_headers = curl_slist_append(
            m_headers,
            "Content-Type: application/json");

        m_headers = curl_slist_append(
            m_headers,
            auth.c_str());

        m_headers = curl_slist_append(
            m_headers,
            stream.c_str());

        curl_easy_setopt(
            m_curl,
            CURLOPT_URL,
            m_opts.endpoint.c_str());

        curl_easy_setopt(
            m_curl,
            CURLOPT_HTTPHEADER,
            m_headers);

        curl_easy_setopt(
            m_curl,
            CURLOPT_POST,
            1L);

        // For http:// where the server supports h2c directly.
        curl_easy_setopt(
            m_curl,
            CURLOPT_HTTP_VERSION,
            CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);

        // We don't care about the response payload.
        curl_easy_setopt(
            m_curl,
            CURLOPT_WRITEFUNCTION,
            &discard_response);

        // Do not let logging block indefinitely.
        curl_easy_setopt(
            m_curl,
            CURLOPT_CONNECTTIMEOUT_MS,
            1000L);

        curl_easy_setopt(
            m_curl,
            CURLOPT_TIMEOUT_MS,
            3000L);
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

        const std::string body = std::format(
            request_payload,
            m_opts.service_name,
            instrumentation_library_name,
            data);

        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, body.data());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size()));
        const CURLcode result = curl_easy_perform(m_curl);
        if (result != CURLE_OK) {
            #ifdef CURL_OTEL_SINK_DEBUG
            std::println("OTEL: curl error: {}", curl_easy_strerror(result));
            #endif
        }

    }
};

#endif // CURL_OTEL_SINK_H