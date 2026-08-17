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
        std::string m_body;

        xtd::channel<std::shared_ptr<log_message>> m_channel;
        xtd::channel_writer<std::shared_ptr<log_message>> m_writer;
        std::jthread m_worker;

        static size_t discard_response(char*, size_t size, size_t count, void*)
        {
            return size * count;
        }

        static void process_messages(xtd::channel<std::shared_ptr<log_message>>& channel, curl_otel_sink* const sink)
        {
            xtd::channel_reader<std::shared_ptr<log_message>> reader{channel};

            while (auto message = reader.read()) {
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
            , m_worker{std::jthread{process_messages, std::ref(m_channel), this}}
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
            const std::string body = std::format(request_payload, m_opts.service_name, instrumentation_library_name, data);

            #ifdef OTEL_SINK_DEBUG
                std::println("OTEL request:\n{}", body);
            #endif

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

}

#endif
