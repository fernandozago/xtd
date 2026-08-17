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
        static std::string_view get_plain_level_strings(log_level level) {
            return log_sink::plain_level_strings[static_cast<size_t>(level)];
        }

        static std::string_view get_colored_level_strings(log_level level) {
            return log_sink::colored_level_strings[static_cast<size_t>(level)];
        }
    };

    TEST_CASE_METHOD(LoggingTests, "console sink writes formatted message")
    {
        const auto level = GENERATE(
            log_level::trace,
            log_level::debug,
            log_level::information,
            log_level::warning,
            log_level::error,
            log_level::critical
        );

        const std::string value = GENERATE("a", "b");
        const bool use_colors = GENERATE(true, false);
        const bool use_local_time = GENERATE(true, false);
        const bool flush_on_write = GENERATE(true, false);
        const std::optional<std::runtime_error> exception = GENERATE(
            std::optional<std::runtime_error>{std::nullopt},
            std::optional<std::runtime_error>{std::runtime_error{"exception_test_what"}}
        );

        int fds[2];
        REQUIRE(::pipe(fds) == 0);

        const int read_fd  = fds[0];
        const int write_fd = fds[1];

        {
            logger _logger;

            _logger.add_sink<log_sink>(log_sink_opts {
                .min_log_level = log_level::trace,
                .fd = write_fd,
                .use_local_time = use_local_time,
                .use_colors = use_colors,
                .flush_on_write = flush_on_write
            });

            #ifdef DEBUG_TEST
            _logger.add_sink<log_sink>(log_sink_opts {
                .min_log_level = log_level::trace,
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
        CHECK(output.find(std::format("teste {}\n", value)) != std::string::npos);
        CHECK(output.find("<tests/logging.cpp:") != std::string::npos);
        if (exception) {
            CHECK(output.find("-> [Exception]") != std::string::npos);
            CHECK(output.find("runtime_error") != std::string::npos);
            CHECK(output.find("exception_test_what") != std::string::npos);
        }

        CHECK(output.find(LoggingTests::get_plain_level_strings(level)) != std::string::npos);
        if (use_colors) {
            CHECK(output.find(LoggingTests::get_colored_level_strings(level)) != std::string::npos);
        }
    }
}