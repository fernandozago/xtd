#ifndef LOG_SINK_H
#define LOG_SINK_H

#include <array>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <format>
#include <source_location>
#include <string_view>
#include <unistd.h>

#include "../log_message.h"
#include "sinks_opts.h"

namespace xtd_logging_tests {
    struct LoggingTests;
}

namespace xtd 
{

    class log_sink {
    private:
        log_sink_opts m_opts;
        log_level m_min_log_level;
        bool m_use_local_time;
        bool m_use_colors;
        bool m_flush_on_write;
        bool m_own_channel;
    
        static constexpr std::array<std::string_view, 6> plain_level_strings {
            "TRACE", "DEBUG", "INFO.",
            "WARN.", "ERROR", "FATAL"
        };

        static constexpr std::array<std::string_view, 6> colored_level_strings {
            "\033[90mTRACE\033[0m", "\033[36mDEBUG\033[0m", "\033[32mINFO.\033[0m",
            "\033[33mWARN.\033[0m", "\033[31mERROR\033[0m", "\033[35mFATAL\033[0m"
        };

    public:
        friend struct xtd_logging_tests::LoggingTests;
        
        explicit log_sink(const log_sink_opts& opts, bool own_channel = false)
            : m_opts(opts)
            , m_min_log_level(opts.min_log_level)
            , m_use_local_time(opts.use_local_time)
            , m_use_colors(opts.use_colors)
            , m_flush_on_write(opts.flush_on_write)
            , m_own_channel(own_channel)
            , m_fd(opts.fd)
        {
            m_opts.validate();
        }

        virtual ~log_sink() = default;

        bool own_channel() const noexcept {
            return m_own_channel;
        }

        virtual void write(const std::shared_ptr<log_message>& message) {
            if (message->level() < m_min_log_level) {
                return;
            }

            write_message_log(message);
        }

    protected:
        int m_fd = -1;

        void write_message_log(const std::shared_ptr<log_message>& message) const {
            const auto [fmtd_dt_tm, us, tz] = message->get_timestamp(m_use_local_time);
            const std::string_view& level_str = m_use_colors
                ? colored_level_strings[static_cast<std::size_t>(message->level())]
                : plain_level_strings[static_cast<std::size_t>(message->level())];

            const std::source_location& location = message->location();

            if (message->has_exception()) {
                static constexpr std::string_view log_format = "[{}.{:06}{}] {} <{}:{}> {}\n    -> [Exception] {}: {}\n";
                const exception_info& ex_info = message->exception();
                write_to_output(std::format(log_format,
                    fmtd_dt_tm, us, tz,
                    level_str,
                    location.file_name(), location.line(),
                    message->get_formatted_message(),
                    ex_info.type,
                    ex_info.message
                ));
            } else {
                static constexpr std::string_view log_format = "[{}.{:06}{}] {} <{}:{}> {}\n";
                write_to_output(std::format(log_format,
                    fmtd_dt_tm, us, tz,
                    level_str,
                    location.file_name(), location.line(),
                    message->get_formatted_message()
                ));
            }
        }

        virtual void write_to_output(std::string_view data) const {
            std::size_t offset = 0;
            while (offset < data.size()) {
                const ssize_t written = ::write(m_fd, data.data() + offset, data.size() - offset);
                if (written < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    return;
                }
                offset += static_cast<std::size_t>(written);
            }

            if (m_flush_on_write) {
                ::fsync(m_fd);
            }
        }
    };
}

#endif // LOG_SINK_H