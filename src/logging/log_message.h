#ifndef LOG_MESSAGE_H
#define LOG_MESSAGE_H

#include <chrono>
#include <source_location>
#include <string>
#include <vector>
#include <tuple>
#include <ctime>

enum class log_level {
    trace,
    debug,
    information,
    warning,
    error,
    critical
};

struct log_message {
    virtual ~log_message() = default;

    virtual std::string get_formatted_message() const = 0;
    virtual std::vector<std::string> get_formatted_args() const = 0;

    std::string_view format() const { return m_format; }
    std::source_location location() const { return m_location; }
    log_level level() const { return m_level; }
    std::chrono::system_clock::time_point timestamp() const { return m_timestamp; }

    std::tuple<std::string, int, std::string> get_timestamp(bool use_local_time) const {
        std::time_t time_t_timestamp = std::chrono::system_clock::to_time_t(m_timestamp);
        std::tm tm_timestamp{};

        if (use_local_time) {
            localtime_r(&time_t_timestamp, &tm_timestamp);
        } else {
            gmtime_r(&time_t_timestamp, &tm_timestamp);
        }

        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_timestamp);

        const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(m_timestamp.time_since_epoch()) % 1000000;

        char timezone_buffer[16];
        if (use_local_time) {
            std::strftime(timezone_buffer, sizeof(timezone_buffer), "%z", &tm_timestamp);
        } else {
            std::snprintf(timezone_buffer, sizeof(timezone_buffer), "+0000");
        }

        return {std::string{buffer}, static_cast<int>(microseconds.count()), std::string{timezone_buffer}};
    }

    
protected:
    log_message(std::source_location location, log_level level, std::string_view format)
        : m_timestamp{std::chrono::system_clock::now()}
        , m_format{format}
        , m_location{location}
        , m_level{level}
    {
    }

    std::chrono::system_clock::time_point m_timestamp;
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

public:
    log_message_impl(std::source_location location, log_level level, std::format_string<Args...> format, Args&&... args)
        : log_message{location, level, std::string{format.get()}}
        , m_args{own(std::forward<Args>(args))...}
    {
    }

private:
    std::string get_formatted_message() const override {
        std::string result;
        result.reserve(128);
        try {
            std::apply([&](const auto&... values) {
                std::vformat_to(std::back_inserter(result), m_format, std::make_format_args(values...));
            }, m_args);
        } catch (const std::exception& ex) {
            result = std::format("Error formatting message: {} -> format=`{}`", ex.what(), m_format);
        }
        return result;
    }

    std::vector<std::string> get_formatted_args() const override {
        std::vector<std::string> format_specs;
        size_t pos = 0;

        while (pos < m_format.size()) {
            const size_t start = m_format.find('{', pos);
            if (start == std::string::npos) break;

            const size_t end = m_format.find('}', start);
            if (end == std::string::npos) break;

            const std::string placeholder = m_format.substr(start, end - start + 1);
            format_specs.push_back(placeholder);
            pos = end + 1;
        }

        std::vector<std::string> result;
        get_formatted_args_impl(result, format_specs, m_args, std::index_sequence_for<Args...>{});
        return result;
    }

    template<std::size_t... Is>
    void get_formatted_args_impl(
        std::vector<std::string>& result, 
        const std::vector<std::string>& format_specs, 
        const std::tuple<owned_t<Args>...>& args, 
        std::index_sequence<Is...>) const 
    {
        size_t arg_index = 0;
        (
            (
                result.push_back(
                    arg_index < format_specs.size() 
                        ? try_format_arg_with_spec(std::get<Is>(args), format_specs[arg_index]) 
                        : std::format("{}", std::get<Is>(args))
                ),
                arg_index++
            ),
            ...
        );
    }

    template<typename T>
    std::string try_format_arg_with_spec(const T& arg, const std::string& spec) const {
        try {
            return std::vformat(spec, std::make_format_args(arg));
        }
        catch (const std::exception&) {
            return std::format("{}", arg);
        }
    }
};

#endif // LOG_MESSAGE_H