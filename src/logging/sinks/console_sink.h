#ifndef CONSOLE_SINK_H
#define CONSOLE_SINK_H

#include <array>
#include <format>
#include <source_location>
#include <string_view>
#include <unistd.h>
#include <print>

#include "log_sink.h"

struct console_sink_opts {
    log_level min_log_level = log_level::information;
    bool use_local_time = true;
    bool show_timezone = true;
    bool use_colors = true;
};

class console_sink final : public log_sink {
private:
    console_sink_opts m_opts;

    static constexpr std::array<std::string_view, 6> colored_level_strings {
        "\033[90mTRACE\033[0m",
        "\033[36mDEBUG\033[0m",
        "\033[32mINFO.\033[0m",
        "\033[33mWARN.\033[0m",
        "\033[31mERROR\033[0m",
        "\033[35mFATAL\033[0m"
    };

    static constexpr std::array<std::string_view, 6> plain_level_strings {
        "TRACE",
        "DEBUG",
        "INFO.",
        "WARN.",
        "ERROR",
        "FATAL"
    };

    static_assert(colored_level_strings.size() == 6);
    static_assert(plain_level_strings.size() == 6);

public:
    explicit console_sink(const console_sink_opts& opts)
        : m_opts{opts}
    {
    }

    console_sink(const console_sink&) = delete;
    console_sink& operator=(const console_sink&) = delete;
    console_sink(console_sink&&) = default;
    console_sink& operator=(console_sink&&) = default;

    void write(const log_message& message) override {
        if (message.level() < m_opts.min_log_level) {
            return;
        }

        const auto [fmtd_dt_tm, us, tz] =
            message.get_timestamp(m_opts.use_local_time);

        std::println(
            "[{}.{:06}{}] {} <{}:{}> {}",
            fmtd_dt_tm,
            us,
            m_opts.show_timezone ? tz : "",
            m_opts.use_colors
                ? colored_level_strings[static_cast<std::size_t>(message.level())]
                : plain_level_strings[static_cast<std::size_t>(message.level())],
            message.location().file_name(),
            message.location().line(),
            message.get_formatted_message()
        );
    }

    ~console_sink() override = default;
};

#endif