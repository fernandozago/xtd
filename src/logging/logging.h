#ifndef LOGGING_H
#define LOGGING_H

#include <optional>
#include <thread>
#include <utility>
#include <vector>
#include <source_location>
#include <memory.h>

#include "log_message.h"
#include "sinks/log_sink.h"
#include "channel/channel.h"

namespace xtd {

    class logger {
    public:
        logger()
            : m_channel{}
            , m_writer{m_channel}
            , m_sinks{}
            , m_worker{std::jthread{process_logs, std::ref(m_channel), std::ref(m_sinks)}}
        {
        }

        ~logger() {
            m_writer.complete();
            if (m_worker.joinable()) {
                m_worker.join();
            }
        }

        logger(const logger&) = delete;
        logger& operator=(const logger&) = delete;

        static logger& instance() {
            static logger instance;
            return instance;
        }

        template<typename T, typename... Args>
        void add_sink(Args&&... args) {
            std::unique_ptr<T> sink = std::make_unique<T>(std::forward<Args>(args)...);
            if (sink->own_channel()) {
                m_own_channel_sinks.push_back(std::move(sink));
            } else {
                m_sinks.push_back(std::move(sink));
            }
        }

        template<typename... Args>
        void log(std::optional<exception_info>&& ex, std::source_location&& location, log_level level, std::format_string<Args...> format, Args&&... args) {
            // Create a log message and push it to the channel for processing by the sinks.
            std::shared_ptr<log_message_impl<Args...>> message = std::make_shared<log_message_impl<Args...>>(
                std::forward<std::optional<exception_info>>(ex), 
                std::forward<std::source_location>(location), 
                level, 
                format, 
                std::forward<Args>(args)...
            );
            
            for (const auto& sink : m_own_channel_sinks) {
                // Push a copy of the message to each sink that owns its own channel.
                sink->write(message);
            }

            if (!m_sinks.empty()) {
                // Move the message to the main channel for processing by the sinks that share the channel.
                (void)m_writer.try_push(std::move(message));
            }
        }
    private:
        xtd::channel<std::shared_ptr<log_message>> m_channel;
        xtd::channel_writer<std::shared_ptr<log_message>> m_writer;
        std::vector<std::unique_ptr<log_sink>> m_sinks;
        std::vector<std::unique_ptr<log_sink>> m_own_channel_sinks;
        std::jthread m_worker;

        static void process_logs(xtd::channel<std::shared_ptr<log_message>>& channel, std::vector<std::unique_ptr<log_sink>>& sinks) {
            xtd::channel_reader<std::shared_ptr<log_message>> reader{channel};

            for (const auto& message : reader.read_all()) {
                for (const auto& sink : sinks) {
                    sink->write(message);
                }
            }
        }
    };

    #define LOG(level, format, ...) \
        logger::instance().log( \
            std::nullopt, std::source_location::current(), level, format, ##__VA_ARGS__ \
        )

    #define LOG_EX(ex, level, format, ...) \
        logger::instance().log( \
            std::make_optional(xtd::exception_info{ex}), std::source_location::current(), level, format, ##__VA_ARGS__ \
        )

    #define LOG_TRACE(...) \
        LOG(xtd::log_level::trace, __VA_ARGS__)

    #define LOG_DEBUG(...) \
        LOG(xtd::log_level::debug, __VA_ARGS__)

    #define LOG_INFO(...) \
        LOG(xtd::log_level::information, __VA_ARGS__)

    #define LOG_WARN(...) \
        LOG(xtd::log_level::warning, __VA_ARGS__)

    #define LOG_WARN_EX(ex, ...) \
        LOG_EX(ex, xtd::log_level::warning, __VA_ARGS__)

    #define LOG_ERROR(...) \
        LOG(xtd::log_level::error, __VA_ARGS__)

    #define LOG_ERROR_EX(ex, ...) \
        LOG_EX(ex, xtd::log_level::error, __VA_ARGS__)

    #define LOG_FATAL(...) \
        LOG(xtd::log_level::critical, __VA_ARGS__)

    #define LOG_FATAL_EX(ex, ...) \
        LOG_EX(ex, xtd::log_level::critical, __VA_ARGS__)

}

#endif
