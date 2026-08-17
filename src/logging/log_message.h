#ifndef LOG_MESSAGE_H
#define LOG_MESSAGE_H

#include <chrono>
#include <source_location>
#include <string>
#include <vector>
#include <tuple>
#include <ctime>
#include <optional>
#include <mutex>

enum class log_level {
    trace,
    debug,
    information,
    warning,
    error,
    critical
};

struct exception_info {
    std::string type;
    std::string message;

    exception_info(const std::exception& ex)
        : type(typeid(ex).name())
        , message(ex.what())
    {
    }
};

struct log_message {

    virtual ~log_message() = default;

    virtual const std::string& get_formatted_message() const = 0;
    virtual const std::vector<std::string>& get_formatted_args() const = 0;

    const std::string_view format() const { return m_format; }
    const std::source_location& location() const { return m_location; }
    const log_level& level() const { return m_level; }
    const std::chrono::system_clock::time_point& timestamp() const { return m_timestamp; }
    const exception_info& exception() const { return *m_exception; }
    bool has_exception() const { return m_exception.has_value(); }

    std::tuple<std::string, int, std::string> get_timestamp(bool use_local_time) const {
        const auto time = std::chrono::system_clock::to_time_t(m_timestamp);

        std::tm tm{};
        if ((use_local_time ? localtime_r(&time, &tm) : gmtime_r(&time, &tm)) == nullptr) {
            tm = {};
        }

        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);

        const auto microseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(m_timestamp.time_since_epoch()).count() % 1'000'000;

        std::string tz = "Z";

        if (use_local_time) {
            char timezone_buffer[8];
            std::strftime(timezone_buffer, sizeof(timezone_buffer), "%z", &tm);

            tz = timezone_buffer;
            if (tz.size() == 5) {
                tz.insert(3, ":");
            }
        }

        return {buffer, static_cast<int>(microseconds), tz};
    }

    
protected:
    log_message(std::optional<exception_info>&& ex_info, std::source_location&& location, log_level level, std::string_view format)
        : m_timestamp{std::chrono::system_clock::now()}
        , m_exception{std::move(ex_info)}
        , m_format{format}
        , m_location{std::move(location)}
        , m_level{level}
    {
    }

    std::chrono::system_clock::time_point m_timestamp;
    std::optional<exception_info> m_exception;
    std::string m_format;
    std::source_location m_location;
    log_level m_level;
    
private:
    template <typename... Args>
    friend struct log_message_impl;
};

template <typename... Args>
struct log_message_impl : public log_message {

private:
    template <typename T>
    static auto own(T&& value) {
        using raw_t = std::remove_reference_t<T>;
        using type =  std::remove_cvref_t<T>;

        if constexpr (std::is_same_v<type, std::string_view>) {
            return std::string{value};
        } else if constexpr (std::is_array_v<raw_t> && std::is_same_v<std::remove_cv_t<std::remove_extent_t<raw_t>>, char>) {
            return std::string{value};
        } else if constexpr (std::is_pointer_v<type> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<type>>, char>) {
            return std::string{value ? value : ""};
        } else {
            return type{std::forward<T>(value)};
        }
    }

    template <typename T>
    using owned_t = decltype(own(std::declval<T>()));

    std::tuple<owned_t<Args>...> m_args;

private:
    mutable std::once_flag m_format_once;
    mutable std::once_flag m_format_args_once;
    mutable std::string m_formatted_message;
    mutable std::vector<std::string> m_formatted_args;

    const std::string& get_formatted_message() const override
    {
        std::call_once(m_format_once, [this] {
            m_formatted_message.reserve(128);

            try {
                std::apply([&](const auto&... values) {
                    std::vformat_to(std::back_inserter(m_formatted_message), m_format, std::make_format_args(values...));
                }, m_args);
            } catch (const std::exception& ex) {
                m_formatted_message = std::format("Error formatting message: {} -> format=`{}`", ex.what(), m_format);
            }
        });

        return m_formatted_message;
    }

    const std::vector<std::string>& get_formatted_args() const override {
        std::call_once(m_format_args_once, [this] {
            m_formatted_args.reserve(sizeof...(Args));

            std::apply([this](const auto&... args) {
                (m_formatted_args.push_back(std::format("{}", args)), ...);
            }, m_args);
        });

        return m_formatted_args;
    }

public:
    log_message_impl(std::optional<exception_info>&& ex_info, std::source_location&& location, log_level level, std::format_string<Args...> format, Args&&... args)
        : log_message{std::forward<std::optional<exception_info>>(ex_info), std::forward<std::source_location>(location), level, std::string{format.get()}}
        , m_args{own(std::forward<Args>(args))...}
    {
    }
};

#endif // LOG_MESSAGE_H