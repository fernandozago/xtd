#ifndef CURL_OTEL_SINK_H
#define CURL_OTEL_SINK_H

//#define OTEL_SINK_DEBUG

#include <format>
#include <memory>
#ifdef OTEL_SINK_DEBUG
    #include <print>
#endif
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#include <curl/curl.h>

#include "channel/channel.h"
#include "log_sink.h"
#include "otel_serializer.h"

namespace xtd 
{
    class curl_otel_sink final : public log_sink {
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
                "{{\"scope\":{{"
                    "\"name\":\"xtd.logging\","
                    "\"version\":\"1.0.0\""
                "}},"
                "\"logRecords\":[";

        static constexpr std::string_view request_footer = "]}]}]}";

        CURL* m_curl = nullptr;
        curl_slist* m_headers = nullptr;

        otel_sink_opts m_opts;
        std::string m_body;

        xtd::channel<std::shared_ptr<log_message>> m_channel;
        xtd::channel_writer<std::shared_ptr<log_message>> m_writer;
        std::jthread m_worker;

        static size_t discard_response(char*, size_t size, size_t count, void*)
        {
            return size * count;
        }

    #ifdef OTEL_SINK_DEBUG
        static size_t write_response(char* data, size_t size, size_t count, void* user_data)
        {
            const size_t data_size = size * count;
            auto& response = *static_cast<std::string*>(user_data);
            response.append(data, data_size);
            return data_size;
        }
    #endif

        static void process_messages(xtd::channel<std::shared_ptr<log_message>>& channel, curl_otel_sink& sink)
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
                sink.write_to_output(sink.m_body);
                sink.m_body.clear();
            }
        }

    public:
        explicit curl_otel_sink(const otel_sink_opts& opts)
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

            m_curl = curl_easy_init();

            if (!m_curl) throw std::runtime_error{"curl_easy_init failed"};

            const std::string auth = std::format("Authorization: {}", m_opts.auth_token);
            const std::string stream = std::format("stream-name: {}", m_opts.service_namespace);
            m_headers = curl_slist_append(m_headers, "Content-Type: application/json");
            m_headers = curl_slist_append(m_headers, auth.c_str());
            m_headers = curl_slist_append(m_headers, stream.c_str());

            curl_easy_setopt(m_curl, CURLOPT_URL, m_opts.endpoint.c_str());
            curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);
            curl_easy_setopt(m_curl, CURLOPT_POST, 1L);

            // For http:// where the server supports h2c directly.
            curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);

            curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &discard_response);

            // Do not let logging block indefinitely.
            curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 1000L);
            curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, 3000L);

            m_worker = std::jthread{process_messages, std::ref(m_channel), std::ref(*this)};
        }

        ~curl_otel_sink() override
        {
            m_writer.complete();

            if (m_worker.joinable()) {
                m_worker.join();
            }

            if (m_curl)
                curl_easy_cleanup(m_curl);

            if (m_headers)
                curl_slist_free_all(m_headers);
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
            std::string response;
            curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &write_response);
            curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    #endif

            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, data.data());
            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(data.size()));

            const CURLcode result = curl_easy_perform(m_curl);

    #ifdef OTEL_SINK_DEBUG
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
    };

}

#endif