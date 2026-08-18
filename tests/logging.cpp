#include "logging/log_message.h"
#include "logging/logging.h"
#include "logging/sinks/log_sink.h"
#include "logging/sinks/sinks_opts.h"
#include "third_party/catch2/catch_amalgamated.hpp"

#include <cstdio>
#include <source_location>
#include <string>
#include <unistd.h>

//#define DEBUG_TEST

namespace xtd_logging_tests {
    struct LoggingTests {
        static std::string_view get_plain_level_strings(xtd::log_level level) {
            return xtd::log_sink::plain_level_strings[static_cast<size_t>(level)];
        }

        static std::string_view get_colored_level_strings(xtd::log_level level) {
            return xtd::log_sink::colored_level_strings[static_cast<size_t>(level)];
        }
    };

    TEST_CASE_METHOD(LoggingTests, "file sink options reject empty file path")
    {
        xtd::file_sink_opts opts;
        opts.file_path = "";

        REQUIRE_THROWS_AS(opts.validate(), std::invalid_argument);
    }

    TEST_CASE_METHOD(LoggingTests, "otel sink options reject invalid combinations")
    {
        const auto valid_opts = []() {
            xtd::otel_sink_opts opts;
            opts.endpoint = "http://localhost:4318/v1/logs";
            opts.auth_token = "Bearer token";
            opts.service_namespace = "chat-app";
            opts.service_name = "chat-app";
            opts.service_version = "1.0.0";
            opts.service_instance_id = "instance-1";
            opts.environment_name = "development";
            opts.batch_size = 10;
            return opts;
        };

        const auto invalid_cases = std::vector<std::function<void(xtd::otel_sink_opts&)>>{
            [](xtd::otel_sink_opts& opts) { opts.endpoint = ""; },
            [](xtd::otel_sink_opts& opts) { opts.endpoint = "   "; },
            [](xtd::otel_sink_opts& opts) { opts.auth_token = "   "; },
            [](xtd::otel_sink_opts& opts) { opts.service_namespace = ""; },
            [](xtd::otel_sink_opts& opts) { opts.service_namespace = "   "; },
            [](xtd::otel_sink_opts& opts) { opts.service_name = ""; },
            [](xtd::otel_sink_opts& opts) { opts.service_name = "   "; },
            [](xtd::otel_sink_opts& opts) { opts.service_version = ""; },
            [](xtd::otel_sink_opts& opts) { opts.service_version = "   "; },
            [](xtd::otel_sink_opts& opts) { opts.service_instance_id = ""; },
            [](xtd::otel_sink_opts& opts) { opts.service_instance_id = "   "; },
            [](xtd::otel_sink_opts& opts) { opts.environment_name = ""; },
            [](xtd::otel_sink_opts& opts) { opts.environment_name = "   "; },
            [](xtd::otel_sink_opts& opts) { opts.batch_size = 0; },
        };

        for (const auto& mutate : invalid_cases) {
            auto opts = valid_opts();
            mutate(opts);
            REQUIRE_THROWS_AS(opts.validate(), std::invalid_argument);
        }
    }

    TEST_CASE_METHOD(LoggingTests, "console sink writes formatted message")
    {
        const auto level = GENERATE(
            xtd::log_level::trace,
            xtd::log_level::debug,
            xtd::log_level::information,
            xtd::log_level::warning,
            xtd::log_level::error,
            xtd::log_level::critical
        );

        const std::string value = GENERATE("a", "b");
        const bool use_colors = GENERATE(true, false);
        const bool use_local_time = GENERATE(true, false);
        const bool flush_on_write = GENERATE(true, false);
        const std::optional<std::system_error> exception = GENERATE(
            std::optional<std::system_error>{std::nullopt},
            std::optional<std::system_error>{{EPERM, std::generic_category(), "some generic error"}}
        );

        int fds[2];
        REQUIRE(::pipe(fds) == 0);

        const int read_fd  = fds[0];
        const int write_fd = fds[1];

        {
            xtd::logger _logger;

            _logger.add_sink<xtd::log_sink>(xtd::log_sink_opts {
                .min_log_level = xtd::log_level::trace,
                .fd = write_fd,
                .use_local_time = use_local_time,
                .use_colors = use_colors,
                .flush_on_write = flush_on_write
            });

            #ifdef DEBUG_TEST
            _logger.add_sink<xtd::log_sink>(xtd::log_sink_opts {
                .min_log_level = xtd::log_level::trace,
                .use_local_time = use_local_time,
                .use_colors = use_colors,
                .flush_on_write = flush_on_write
            });
            #endif

            _logger.log(exception, std::source_location::current(), level, "teste {}", value);
        }
        
        if (!flush_on_write) {
            //force flush on exit when log_sink not flushing on write
            ::fsync(write_fd);
        }

        // sinks does not own the fd`s
        ::close(write_fd);

        char buffer[4096] {};
        const auto size = ::read(read_fd, buffer, sizeof(buffer) - 1);

        REQUIRE(size >= 0);

        const std::string output(
            buffer,
            static_cast<std::size_t>(size)
        );

        ::close(read_fd);

        CAPTURE(level);
        CAPTURE(use_colors);
        CAPTURE(use_local_time);
        CAPTURE(flush_on_write);
        CAPTURE(exception);
        CHECK(output.find(std::format("teste {}\n", value)) != std::string::npos);
        CHECK(output.find("<tests/logging.cpp:") != std::string::npos);
        if (exception) {
            CHECK(output.find("-> [Exception]") != std::string::npos);
            CHECK(output.find("system_error") != std::string::npos);
            CHECK(output.find("some generic error: Operation not permitted") != std::string::npos);
        }

        CHECK(output.find(LoggingTests::get_plain_level_strings(level)) != std::string::npos);
        if (use_colors) {
            CHECK(output.find(LoggingTests::get_colored_level_strings(level)) != std::string::npos);
        }
    }
}