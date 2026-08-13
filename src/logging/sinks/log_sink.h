#ifndef LOG_SINK_H
#define LOG_SINK_H

#include <array>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <format>
#include <string>
#include <string_view>
#include <unistd.h>

#include "../log_message.h"

struct log_sink_opts {
    int fd = STDOUT_FILENO;
    log_level min_log_level = log_level::information;
    bool use_local_time = true;
    bool show_timezone = true;
    bool use_structured_log = false;
    bool use_colors = true;
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
    bool m_show_timezone;
    bool m_use_structured_log;
    bool m_use_colors;

    static constexpr std::array<std::string_view, 6> plain_level_strings {
        "TRACE", "DEBUG", "INFO.",
        "WARN.", "ERROR", "FATAL"
    };

    static constexpr std::array<std::string_view, 6> structured_level_strings {
        "trace", "debug", "info",
        "warning", "error", "critical"
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
        , m_show_timezone(opts.show_timezone)
        , m_use_structured_log(opts.use_structured_log)
        , m_use_colors(opts.use_colors)
    {
    }

    ~log_sink() = default;

    void write(const log_message& message) {
        if (message.level() < m_min_log_level) {
            return;
        }

        if (m_use_structured_log) {
            write_json(message);
        } else {
            write_plain(message);
        }
    }

protected:
    void write_plain(const log_message& message) const {
        const auto [fmtd_dt_tm, us, tz] = message.get_timestamp(m_use_local_time);
        const std::string_view& level_str = m_use_colors
            ? colored_level_strings[static_cast<std::size_t>(message.level())]
            : plain_level_strings[static_cast<std::size_t>(message.level())];

        write_all(std::format(
            "[{}.{:06}{}] {} <{}:{}> {}\n",
            fmtd_dt_tm,
            us,
            m_show_timezone ? tz : "",
            level_str,
            message.location().file_name(),
            message.location().line(),
            message.get_formatted_message()
        ));
    }

    void write_json(const log_message& message) const {
        const auto [fmtd_dt_tm, us, tz] = message.get_timestamp(m_use_local_time);
        write_all(std::format(
            "{{\"timestamp\": \"{}.{:06}{}\", "
            "\"level\": \"{}\", "
            "\"location\": \"{}:{}\", "
            "\"format\": \"{}\", "
            "\"message\": \"{}\", "
            "\"args\": {{{}}}}}\n",
            fmtd_dt_tm,
            us,
            m_show_timezone ? tz : "",
            structured_level_strings[static_cast<std::size_t>(message.level())],
            message.location().file_name(),
            message.location().line(),
            escape_json(message.format()),
            escape_json(message.get_formatted_message()),
            get_formatted_args(message)
        ));
    }

    static std::string escape_json(std::string_view value) {
        std::string escaped;
        escaped.reserve(value.size());

        for (char c : value) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        escaped += "\\u";
                        char buffer[5];
                        std::snprintf(buffer, sizeof(buffer), "%04x", static_cast<unsigned char>(c));
                        escaped += buffer;
                    } else {
                        escaped += c;
                    }
            }
        }

        return escaped;
    }

    static std::string get_formatted_args(const log_message& message) {
        std::string args_json;
        int arg_index = 0;
        for (const auto& arg : message.get_formatted_args()) {
            if (!args_json.empty()) {
                args_json += ", ";
            }
            args_json += std::format("\"arg{}\": \"{}\"", arg_index, escape_json(arg));
            ++arg_index;
        }
        return args_json;
    }

    void write_all(std::string_view data) const {
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
    }
};

#endif // LOG_SINK_H