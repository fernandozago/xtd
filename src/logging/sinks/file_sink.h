#ifndef FILE_SINK_H
#define FILE_SINK_H

#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include "log_sink.h"

struct file_sink_opts {
    std::string file_path;
    std::ios::openmode open_mode = std::ios::binary | std::ios::trunc;
    log_level min_log_level = log_level::information;
    bool show_timezone = true;
    bool use_local_time = true;
    bool use_structured_log = true;
    bool flush_on_write = true;
};

class file_sink final : public log_sink {
private:
    file_sink_opts m_opts;
    std::ofstream m_file;

    static constexpr std::array<std::string_view, 6> plain_level_strings {
        "TRACE", "DEBUG", "INFO.",
        "WARN.", "ERROR", "FATAL"
    };

    static constexpr std::array<std::string_view, 6> structured_level_strings {
        "trace", "debug", "info",
        "warning", "error", "critical"
    };

public: 
    explicit file_sink(const file_sink_opts& opts)
        : m_opts{opts}
    {
        if (m_opts.file_path.empty()) {
            throw std::invalid_argument{"file path cannot be empty"};
        }

        //Create parent directories if they don't exist
        std::filesystem::path file_path{m_opts.file_path};
        std::filesystem::path parent_path = file_path.parent_path();
        if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
            std::filesystem::create_directories(parent_path);
        }

        m_file.open(m_opts.file_path, m_opts.open_mode);
        if (!m_file.is_open()) {
            throw std::runtime_error{"failed to open log file: " + m_opts.file_path};
        }
    }

    file_sink(const file_sink&) = delete;
    file_sink& operator=(const file_sink&) = delete;
    file_sink(file_sink&&) = default;
    file_sink& operator=(file_sink&&) = default;

    void write(const log_message& message) override {
        if (message.level() < m_opts.min_log_level) {
            return;
        }

        if (m_opts.use_structured_log) {
            write_json(message);
        } else {
            write_plain(message);
        }

        if (m_opts.flush_on_write) {
            m_file.flush();
        }
    }

    ~file_sink() override {
        if (m_file.is_open()) {
            if (!m_opts.flush_on_write) {
                m_file.flush();
            }
            m_file.close();
        }
    }

private:
    void write_plain(const log_message& message) {
        static constexpr std::string_view plain_format = "[{}.{:06}] {} <{}:{}> {}\n";
        const auto [fmtd_dt_tm, us, tz] = message.get_timestamp(m_opts.use_local_time);
        const std::string_view& level_str = plain_level_strings[static_cast<std::size_t>(message.level())];

        m_file << std::format(
            plain_format, fmtd_dt_tm, us,
            level_str,
            message.location().file_name(),
            message.location().line(),
            message.get_formatted_message()
        );

        if (m_opts.flush_on_write) {
            m_file.flush();
        }
    }

    void write_json(const log_message& message) {
        static constexpr std::string_view json_format = 
            "{{\"timestamp\": \"{}.{:06}{}\", "
            "\"level\": \"{}\", "
            "\"location\": \"{}:{}\", "
            "\"format\": \"{}\", "
            "\"message\": \"{}\", "
            "\"args\": {{{}}}}}\n";

        const auto [fmtd_dt_tm, us, tz] = message.get_timestamp(m_opts.use_local_time);
        m_file << std::format(json_format,
            fmtd_dt_tm, us, m_opts.show_timezone ? tz : "",
            structured_level_strings[static_cast<std::size_t>(message.level())],
            message.location().file_name(), message.location().line(),
            escape_json(message.format()),
            escape_json(message.get_formatted_message()),
            get_formatted_args(message)
        );
        
    }

    static std::string escape_json(std::string_view value) {
        std::string escaped;
        escaped.reserve(value.size());

        for (char c : value) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b";  break;
                case '\f': escaped += "\\f";  break;
                case '\n': escaped += "\\n";  break;
                case '\r': escaped += "\\r";  break;
                case '\t': escaped += "\\t";  break;
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
    
};

#endif