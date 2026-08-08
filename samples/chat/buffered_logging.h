#pragma once

#include <chrono>
#include <atomic>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>

#include <unistd.h>

#include "pipeline/pipeline.h"

class buffered_logging final
{
public:
	static buffered_logging& instance()
	{
		static buffered_logging logger;
		return logger;
	}

	buffered_logging(const buffered_logging&) = delete;
	buffered_logging& operator=(const buffered_logging&) = delete;

	template <typename... Args>
	void writeln(std::format_string<Args...> format, Args&&... args)
	{
		if (m_closed.load(std::memory_order_relaxed)) {
			return;
		}

		std::string final;
		final.reserve(128);

		write_current_time(final);
		std::format_to(std::back_inserter(final), format, std::forward<Args>(args)...);
		final.push_back('\n');

		assert(m_writer.write(final) == final.size());
	}

private:
	xtd::pipeline m_pipeline;
	xtd::pipe_writer m_writer;
	std::jthread m_worker;
	std::atomic_bool m_closed{false};

	static std::tuple<std::string, int> get_current_time() {
		const auto now_tp = std::chrono::system_clock::now();
		const std::time_t now = std::chrono::system_clock::to_time_t(now_tp);

		std::tm local_time{};
		static_cast<void>(::localtime_r(&now, &local_time));

		char timestamp[20]{};
		static_cast<void>(std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time));
		const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now_tp.time_since_epoch()) % 1000;
		return {timestamp, static_cast<int>(milliseconds.count())};
	}

	static void write_current_time(std::string& message) {
		const auto [timestamp, milliseconds] = get_current_time();
		std::format_to(std::back_inserter(message), "[{}.{:03}] ", timestamp, milliseconds);
	}

	buffered_logging()
		: m_pipeline{}
		, m_writer{m_pipeline}
		, m_worker{buffered_logging::process_logs, xtd::pipe_reader{m_pipeline}}
	{}

	~buffered_logging()
	{
		if (!m_closed.exchange(true)) {
			m_writer.complete();
		}
	}

	static void process_logs(std::stop_token stop_token, xtd::pipe_reader reader)
	{
		std::ofstream log_file{(std::filesystem::read_symlink("/proc/self/exe").string() + ".log"), std::ios::binary | std::ios::trunc};
		local_logln(log_file, "log processing thread started");
		try {
			while (const xtd::read_result result = reader.read(stop_token))
			{
				xtd::segmented_byte_view buffer = result.buffer();
				for (const std::span<const std::byte> segment : buffer.segments()) {
					if (!segment.empty()) {
						write_to_sinks(log_file, std::string_view{reinterpret_cast<const char*>(segment.data()), segment.size()});
					}
				}
				
				reader.advance(buffer.end());
				if (result.completed() || stop_token.stop_requested()) {
					break;
				}
			}
		}
		catch (const std::exception& ex) {
			local_logln(log_file, std::format("error occurred while processing logs: {}", ex.what()));
		}
		
		reader.complete();

		local_logln(log_file, "log processing thread exiting");
		log_file.flush();
	}

	static void local_logln(std::ofstream& file, std::string_view message)
	{
		const auto& [timestamp, milliseconds] = get_current_time();
		write_to_sinks(file, std::format("[{}.{:03}] <buffered_logging>: {}\n", timestamp, milliseconds, message));
	}

	static void write_to_sinks(std::ofstream& file, const std::string_view message, bool simulate_delay = false)
	{
		if (file) {
			file.write(message.data(), static_cast<std::streamsize>(message.size()));
			file.flush();
		}
		
		static_cast<void>(::write(STDOUT_FILENO, message.data(), message.size()));
		
		// Simulate a small delay to mimic real-world logging scenarios and avoid overwhelming the sinks
		if (simulate_delay) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
};

template <typename... Args>
inline void logln(std::format_string<Args...> format, Args&&... args)
{
	buffered_logging::instance().writeln(format, std::forward<Args>(args)...);
}

inline void logln(std::string_view message)
{
	buffered_logging::instance().writeln("{}", message);
}