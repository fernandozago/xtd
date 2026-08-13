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

    static constexpr int to_otel_severity(log_level level) {
        switch (level) {
            case log_level::trace: return 1;
            case log_level::debug: return 5;
            case log_level::information: return 9;
            case log_level::warning: return 13;
            case log_level::error: return 17;
            case log_level::critical: return 21;
            default: return 0;
        }
    }

    static std::string json_value(std::string_view value) {
        if (value.empty()) {
            return "\"\"";
        }

        if (value == "true" || value == "false" || value == "null") {
            return std::string{value};
        }

        bool has_sign = false;
        bool has_digit = false;
        bool has_decimal = false;
        bool has_exponent = false;
        bool has_only_numeric = true;

        for (char c : value) {
            if (c == '+' || c == '-') {
                if (has_sign || has_digit || has_decimal || has_exponent) {
                    has_only_numeric = false;
                    break;
                }
                has_sign = true;
                continue;
            }

            if (c >= '0' && c <= '9') {
                has_digit = true;
                continue;
            }

            if (c == '.') {
                if (has_decimal || has_exponent) {
                    has_only_numeric = false;
                    break;
                }
                has_decimal = true;
                continue;
            }

            if (c == 'e' || c == 'E') {
                if (!has_digit || has_exponent) {
                    has_only_numeric = false;
                    break;
                }
                has_exponent = true;
                continue;
            }

            has_only_numeric = false;
            break;
        }

        if (has_only_numeric && has_digit) {
            return std::string{value};
        }

        return std::format("\"{}\"", escape_json(value));
    }

    static std::string get_attribute_entries(const log_message& message) {
        std::string result;
        const auto formatted_args = message.get_formatted_args();

        result += std::format("\"message.template\": \"{}\"", escape_json(message.format()));

        for (std::size_t index = 0; index < formatted_args.size(); ++index) {
            result += std::format(", \"arg{}\": {}", index, json_value(formatted_args[index]));
        }

        result += std::format(", \"code.file.path\": \"{}\", \"code.line.number\": {}, \"code.function.name\": \"{}\"",
            escape_json(message.location().file_name()),
            message.location().line(),
            escape_json(message.location().function_name()));

        return result;
    }

    void write_structured_log(const log_message& message) const {
        const auto time_nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
            message.timestamp().time_since_epoch()).count();

        std::string json = std::format(
            "{{\"timeUnixNano\":\"{}\",\"severityNumber\":{},\"severityText\":\"{}\",\"body\":{{\"stringValue\":\"{}\"}},\"attributes\":[",
            time_nanos,
            to_otel_severity(message.level()),
            severity_texts[static_cast<std::size_t>(message.level())],
            escape_json(message.get_formatted_message()));

        json += std::format(
            "{{\"key\":\"message.template\",\"value\":{{\"stringValue\":\"{}\"}}}}",
            escape_json(message.format()));

        const auto formatted_args = message.get_formatted_args();
        for (std::size_t index = 0; index < formatted_args.size(); ++index) {
            json += std::format(
                ",{{\"key\":\"arg{}\",\"value\":{{\"stringValue\":\"{}\"}}}}",
                index,
                escape_json(formatted_args[index]));
        }

        json += std::format(
            ",{{\"key\":\"code.file.path\",\"value\":{{\"stringValue\":\"{}\"}}}}",
            escape_json(message.location().file_name()));
        json += std::format(
            ",{{\"key\":\"code.line.number\",\"value\":{{\"intValue\":{}}}}}",
            message.location().line());
        json += std::format(
            ",{{\"key\":\"code.function.name\",\"value\":{{\"stringValue\":\"{}\"}}}}",
            escape_json(message.location().function_name()));

        json += "]}\n";

        write_all(json);
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