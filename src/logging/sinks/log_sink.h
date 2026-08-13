#ifndef LOG_SINK_H
#define LOG_SINK_H

#include <array>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <format>
#include <string_view>
#include <unistd.h>

#include "../log_message.h"
#include "otel_serializer.h"

struct log_sink_opts {
    int fd = STDOUT_FILENO;
    log_level min_log_level = log_level::information;
    bool use_local_time = true;
    bool use_structured_log = false;
    bool use_colors = true;
    bool flush_on_write = true;
};

class log_sink {
private:
    log_sink_opts m_opts;

public:
    int m_fd() const noexcept {
        return m_opts.fd;
    }

private:
    log_level m_min_log_level;
    bool m_use_local_time;
    bool m_use_structured_log;
    bool m_use_colors;
    bool m_flush_on_write;

    static constexpr std::array<std::string_view, 6> plain_level_strings {
        "TRACE", "DEBUG", "INFO.",
        "WARN.", "ERROR", "FATAL"
    };

    static constexpr std::array<std::string_view, 6> severity_texts {
        "TRACE", "DEBUG", "INFO",
        "WARN", "ERROR", "FATAL"
    };

    static constexpr std::array<std::string_view, 6> colored_level_strings {
        "\033[90mTRACE\033[0m", "\033[36mDEBUG\033[0m", "\033[32mINFO.\033[0m",
        "\033[33mWARN.\033[0m", "\033[31mERROR\033[0m", "\033[35mFATAL\033[0m"
    };

public:
    explicit log_sink(const log_sink_opts& opts)
        : m_opts(opts)
        , m_min_log_level(opts.min_log_level)
        , m_use_local_time(opts.use_local_time)
        , m_use_structured_log(opts.use_structured_log)
        , m_use_colors(opts.use_colors)
        , m_flush_on_write(opts.flush_on_write)
    {
    }

    virtual ~log_sink() = default;

    void write(const log_message& message) {
        if (message.level() < m_min_log_level) {
            return;
        }

        if (m_use_structured_log) {
            write_structured_log(message);
        } else {
            write_message_log(message);
        }
    }

protected:
    bool should_write(log_level level) const noexcept {
        return level >= m_min_log_level;
    }

    void write_message_log(const log_message& message) const {
        const auto [fmtd_dt_tm, us, tz] = message.get_timestamp(m_use_local_time);
        const std::string_view& level_str = m_use_colors
            ? colored_level_strings[static_cast<std::size_t>(message.level())]
            : plain_level_strings[static_cast<std::size_t>(message.level())];

        static constexpr std::string_view log_format = "[{}.{:06}{}] {} <{}:{}> {}\n";
        write_all(std::format(log_format,
            fmtd_dt_tm, us, tz,
            level_str,
            message.location().file_name(), message.location().line(),
            message.get_formatted_message()
        ));
    }

    void write_structured_log(const log_message& message) const {
        write_all(otel_serializer::serialize(message));
    }

    virtual void write_all(std::string_view data) const {
        std::size_t offset = 0;
        while (offset < data.size()) {
            const ssize_t written = ::write(m_opts.fd, data.data() + offset, data.size() - offset);
            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return;
            }
            offset += static_cast<std::size_t>(written);
        }

        if (m_flush_on_write) {
            ::fsync(m_opts.fd);
        }
    }
};

#endif // LOG_SINK_H