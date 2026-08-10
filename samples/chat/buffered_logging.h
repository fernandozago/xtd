#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <source_location>

#include <unistd.h>

#include "channel/channel.h"

struct log_message
{
private:
	template <typename T>
	static auto own(T&& value)
	{
		using raw_t = std::remove_reference_t<T>;
		using type = std::remove_cvref_t<T>;

		if constexpr (std::is_same_v<type, std::string_view>) {
			return std::string{value};
		}
		else if constexpr (std::is_array_v<raw_t> && std::is_same_v<std::remove_cv_t<std::remove_extent_t<raw_t>>, char>) {
			return std::string{value};
		}
		else if constexpr (std::is_pointer_v<type> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<type>>, char>) {
			return std::string{value ? value : ""};
		}
		else {
			return type{std::forward<T>(value)};
		}
	}

	using formatter_t = std::move_only_function<void(std::string&, std::string_view) const>;

	template <typename... Args>
	static formatter_t make_formatter(Args&&... args)
	{
		return [args = std::tuple{own(std::forward<Args>(args))...}](std::string& result, const std::string_view format)
		{
			std::apply([&](const auto&... values) {
				std::vformat_to(std::back_inserter(result), format, std::make_format_args(values...));
			}, args);
		};
	}

	static std::tuple<std::string, int> get_formated_timestamp(const std::chrono::system_clock::time_point& timestamp)
	{
		const std::time_t time = std::chrono::system_clock::to_time_t(timestamp);

		std::tm local_time{};
		static_cast<void>(::localtime_r(&time, &local_time));

		char buffer[20]{};
		static_cast<void>(std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time));

		const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()) % 1000000;

		return {std::string(buffer), static_cast<int>(microseconds.count())};
	}

	formatter_t m_formatter;
	std::string m_format;
	std::source_location m_location;
	std::chrono::system_clock::time_point m_timestamp = std::chrono::system_clock::now();

public:
	template <typename... Args>
	log_message(const std::source_location& location, const std::format_string<Args...> format, Args&&... args)
		: m_formatter(make_formatter(std::forward<Args>(args)...))
		, m_format(format.get())
		, m_location(location)
	{
	}

	log_message(log_message&&) noexcept = default;
	log_message& operator=(log_message&&) noexcept = default;

	log_message(const log_message&) = delete;
	log_message& operator=(const log_message&) = delete;

	constexpr std::string to_string() const
	{
		static constexpr std::string_view default_format = "[{}.{:06}] <{}:{}>: ";
		static constexpr std::string_view default_error = "{} -> format=`{}`";

		const auto [timestamp_text, microseconds] = get_formated_timestamp(m_timestamp);
		std::string result;
		result.reserve(128);
		std::format_to(std::back_inserter(result),default_format, timestamp_text, microseconds, m_location.file_name(), m_location.line());

		try {
			m_formatter(result, m_format);
		}
		catch (const std::exception& ex) {
			std::format_to(std::back_inserter(result), default_error, ex.what(), m_format);
		}

		result.push_back('\n');
		return result;
	}
};

class buffered_channel_logging final
{
private:
	xtd::channel<log_message> m_channel;
	xtd::channel_writer<log_message> m_writer;
	std::jthread m_worker;

	std::atomic_bool m_closed{false};

	buffered_channel_logging(bool flush_on_write = false)
		: m_channel{}
		, m_writer{m_channel}
		, m_worker{buffered_channel_logging::process_logs, std::ref(m_channel), flush_on_write}
	{
	}

	~buffered_channel_logging()
	{
		m_writer.complete();
	}

	static std::tuple<std::string, int> get_current_time()
	{
		const auto now_tp = std::chrono::system_clock::now();
		const std::time_t now = std::chrono::system_clock::to_time_t(now_tp);

		std::tm local_time{};
		static_cast<void>(::localtime_r(&now, &local_time));

		char timestamp[20]{};
		static_cast<void>(std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time));
		const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now_tp.time_since_epoch()) % 1000000;

		return {timestamp, static_cast<int>(microseconds.count())};
	}

	static void process_logs(xtd::channel<log_message>& channel, bool flush_on_write)
	{
		std::ofstream log_file{
			std::filesystem::read_symlink("/proc/self/exe").string() + ".log",
			std::ios::binary | std::ios::trunc
		};

		xtd::channel_reader<log_message> reader{channel};

		while (auto message = reader.read()) {
			write_to_sinks(log_file, *message);
			if (flush_on_write) log_file.flush();
		}

		// Flush any remaining logs before exiting
		// Only if flush_on_write is false
		// because if it's true, we already flushed after each write
		if (!flush_on_write) log_file.flush();
	}

	static void write_to_sinks(std::ofstream& file, const log_message& message)
	{
		// Get formatted log message
		std::string message_text = message.to_string();

		if (file) {
			file.write(message_text.data(), static_cast<std::streamsize>(message_text.size()));
		}

		static_cast<void>(::write(STDOUT_FILENO, message_text.data(), message_text.size()));

		// Uncomment next line to simulate a small delay to mimic real-world logging scenarios and avoid overwhelming the sinks
		// std::this_thread::sleep_for(std::chrono::milliseconds{500});
	}

public:
	static buffered_channel_logging& instance()
	{
		static buffered_channel_logging logger{true};
		return logger;
	}

	buffered_channel_logging(const buffered_channel_logging&) = delete;
	buffered_channel_logging& operator=(const buffered_channel_logging&) = delete;

	bool enqueue_log(log_message&& message)
	{
		return m_writer.try_emplace(std::move(message));
	}
};

#define LOGLN(format, ...) \
	buffered_channel_logging::instance().enqueue_log({ \
		std::source_location::current(), \
		format __VA_OPT__(,) __VA_ARGS__ \
	})
