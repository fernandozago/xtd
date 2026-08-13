#ifndef LOGGING_H
#define LOGGING_H

#include <mutex>
#include <thread>
#include <vector>
#include <source_location>
#include <memory.h>

#include "log_message.h"
#include "sinks/log_sink.h"
#include "channel/channel.h"

class logger {
public:
    logger()
        : m_channel{}
        , m_writer{m_channel}
        , m_sinks{}
        , m_sinks_mutex{}
        , m_worker{std::jthread{process_logs, std::ref(m_channel), std::ref(m_sinks), std::ref(m_sinks_mutex)}}
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
        std::lock_guard lock{m_sinks_mutex};
        m_sinks.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    template<typename... Args>
    void log(std::source_location location, log_level level, std::format_string<Args...> format, Args&&... args) {
        auto message = std::make_unique<log_message_impl<Args...>>(location, level, format, std::forward<Args>(args)...);
        (void)m_writer.try_push(std::move(message));
    }
private:
    xtd::channel<std::unique_ptr<log_message>> m_channel;
    xtd::channel_writer<std::unique_ptr<log_message>> m_writer;
    std::vector<std::unique_ptr<log_sink>> m_sinks;
    std::mutex m_sinks_mutex;
    std::jthread m_worker;

    static void process_logs(xtd::channel<std::unique_ptr<log_message>>& channel, std::vector<std::unique_ptr<log_sink>>& sinks, std::mutex& sinks_mutex) {
        xtd::channel_reader<std::unique_ptr<log_message>> reader{channel};

        while (auto message = reader.read()) {
            std::lock_guard lock{sinks_mutex};
            for (const auto& sink : sinks) {
                sink->write(**message);
            }
        }
    }
};

#define LOG(level, format, ...) \
    logger::instance().log( \
        std::source_location::current(), level, format, ##__VA_ARGS__ \
    )

#define LOG_TRACE(...) \
    LOG(log_level::trace, __VA_ARGS__)

#define LOG_DEBUG(...) \
    LOG(log_level::debug, __VA_ARGS__)

#define LOG_INFO(...) \
    LOG(log_level::information, __VA_ARGS__)

#define LOG_WARN(...) \
    LOG(log_level::warning, __VA_ARGS__)

#define LOG_ERROR(...) \
    LOG(log_level::error, __VA_ARGS__)

#define LOG_FATAL(...) \
    LOG(log_level::critical, __VA_ARGS__)

#endif