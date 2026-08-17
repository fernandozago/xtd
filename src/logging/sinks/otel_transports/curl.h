#ifndef OTEL_CURL_TRANSPORT_H
#define OTEL_CURL_TRANSPORT_H

#include <format>
#ifdef OTEL_DEBUG_TRANSPORT
    #include <print>
#endif
#include <stdexcept>
#include <string>
#include <string_view>

#include <curl/curl.h>

#include "logging/sinks/otel_transports/transport_base.h"

namespace xtd {

struct otel_curl_transport : otel_transport_base {
    CURL*       m_curl    = nullptr;
    curl_slist* m_headers = nullptr;

    explicit otel_curl_transport(const otel_sink_opts& opts)
    {
        if (opts.endpoint.empty()) {
            throw std::invalid_argument{"endpoint cannot be empty"};
        }

        m_curl = curl_easy_init();
        if (!m_curl) throw std::runtime_error{"curl_easy_init failed"};

        auto append_header = [this](const char* header) {
            curl_slist* result = curl_slist_append(m_headers, header);
            if (!result) {
                throw std::runtime_error{"curl_slist_append failed"};
            }
            m_headers = result;
        };

        try {
            const std::string auth   = std::format("Authorization: {}", opts.auth_token);
            const std::string stream = std::format("stream-name: {}",   opts.service_namespace);

            append_header("Content-Type: application/json");
            append_header(auth.c_str());
            append_header(stream.c_str());

            curl_easy_setopt(m_curl, CURLOPT_URL,           opts.endpoint.c_str());
            curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER,    m_headers);
            curl_easy_setopt(m_curl, CURLOPT_POST,          1L);
            curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION,  CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);
            curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &discard_response);
            curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 1000L);
            curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS,        3000L);
        }
        catch (...) {
            if (m_headers) {
                curl_slist_free_all(m_headers);
                m_headers = nullptr;
            }
            curl_easy_cleanup(m_curl);
            m_curl = nullptr;
            throw;
        }
    }

    ~otel_curl_transport()
    {
        if (m_curl)    curl_easy_cleanup(m_curl);
        if (m_headers) curl_slist_free_all(m_headers);
    }

    otel_curl_transport(const otel_curl_transport&)            = delete;
    otel_curl_transport& operator=(const otel_curl_transport&) = delete;

    void send(std::string_view json_body) const override
    {
#ifdef OTEL_DEBUG_TRANSPORT
        std::println("-> Request: {}", json_body);
        std::string response;
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &write_response);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA,     &response);
#endif

        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS,        json_body.data());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(json_body.size()));

        const CURLcode result = curl_easy_perform(m_curl);

#ifdef OTEL_DEBUG_TRANSPORT
        if (result != CURLE_OK) {
            std::println("<- Response [Error: {}]", curl_easy_strerror(result));
            return;
        }

        long status_code = 0;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &status_code);
        std::println("<- Response [StatusCode={}] -> {}", status_code, response);
#else
        (void)result;
#endif
    }

private:
    static size_t discard_response(char*, size_t size, size_t count, void*)
    {
        return size * count;
    }

#ifdef OTEL_DEBUG_TRANSPORT
    static size_t write_response(char* data, size_t size, size_t count, void* user_data)
    {
        const size_t data_size = size * count;
        auto& response = *static_cast<std::string*>(user_data);
        response.append(data, data_size);
        return data_size;
    }
#endif
};

} // namespace xtd

#endif
