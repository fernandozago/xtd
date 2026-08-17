#ifndef OTEL_SINK_H
#define OTEL_SINK_H

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>

#include "channel/channel.h"
#include "otel_serializer.h"
#include "log_sink.h"

//#define OTEL_NATIVE_REQUEST
#ifdef OTEL_NATIVE_REQUEST
#include "otel_transports/native.h"
using transport_impl = xtd::otel_native_transport;
#else
#include "otel_transports/curl.h"
using transport_impl = xtd::otel_curl_transport;
#endif


namespace xtd
{
    class otel_sink final : public log_sink {
    private:
        static const std::string& get_hostname()
        {
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
        std::string m_body;
        
        xtd::channel<std::shared_ptr<log_message>> m_channel;
        xtd::channel_writer<std::shared_ptr<log_message>> m_writer;
        std::unique_ptr<otel_transport_base> m_transport;
        std::jthread m_worker;

        static void process_messages(xtd::channel<std::shared_ptr<log_message>>& channel, otel_sink& sink)
        {
            xtd::channel_reader<std::shared_ptr<log_message>> reader{channel};
            const std::size_t batch_size = sink.m_opts.batch_size;

            while (auto message = reader.read()) {
                // Append Request Data
                std::format_to(std::back_inserter(sink.m_body), request_header,
                    sink.m_opts.service_namespace, sink.m_opts.service_name, sink.m_opts.service_version,
                    sink.m_opts.service_instance_id, get_hostname(), sink.m_opts.environment_name);

                sink.m_body += otel_serializer::serialize_record(**message);

                size_t msg_count = 1;

                // Append Records
                while (msg_count < batch_size) {
                    auto next_message = reader.try_read();
                    if (!next_message) break;

                    sink.m_body += ',';
                    sink.m_body += otel_serializer::serialize_record(**next_message);

                    ++msg_count;
                }

                sink.m_body += request_footer;
                sink.m_transport->send(sink.m_body);
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
            , m_transport{std::make_unique<transport_impl>(opts)}
            , m_worker{process_messages, std::ref(m_channel), std::ref(*this)}
        {}

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
    };

}

#endif