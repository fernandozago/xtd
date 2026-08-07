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

#include "pipeline/pipe_reader.h"
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
	void println(std::format_string<Args...> format, Args&&... args)
	{
		write_line(std::format(format, std::forward<Args>(args)...) + "\n");
	}

	void println(std::string_view message)
	{
		write_line(std::string(message) + "\n");
	}

private:
	xtd::pipeline m_pipeline;
	xtd::pipe_writer m_writer;
	std::jthread m_worker;
	std::atomic_bool m_closed{false};

	static std::jthread create_worker_thread(xtd::pipeline& pipeline)
	{
		return std::jthread([&pipeline](std::stop_token stop_token) {
			const std::string file_path = (std::filesystem::read_symlink("/proc/self/exe").parent_path() / "xtd-chat.log").string();
			process_logs(xtd::pipe_reader(pipeline), file_path, stop_token);
		});
	}

	buffered_logging()
		: m_pipeline()
		, m_writer(m_pipeline)
		, m_worker(create_worker_thread(m_pipeline))
	{}

	~buffered_logging()
	{
		if (!m_closed.exchange(true)) {
			m_writer.complete();
		}

		if (m_worker.joinable()) {
			m_worker.join();
		}
	}

	void write_line(std::string message)
	{
		if (!m_closed.load()) {
			assert(m_writer.write(message) == message.size());
		}
	}

	static void process_logs(xtd::pipe_reader reader, const std::string& file_path, std::stop_token stop_token)
	{
		::unlink(file_path.c_str());
		std::ofstream log_file{file_path, std::ios::binary | std::ios::trunc};
		try {
			while (const xtd::read_result result = reader.read(stop_token))
			{
				xtd::segmented_byte_view buffer = result.buffer();

				while (const auto position = buffer.position_of('\n')) {
					const xtd::segmented_byte_view line = buffer.slice(position + 1);
					write_to_sinks(log_file, line);
					buffer.slice_in_place(position + 1, buffer.end());
				}
				
				reader.advance(buffer.begin(), buffer.end());
				if (result.completed() || stop_token.stop_requested()) {
					break;
				}
			}
		}
		catch (...) {
		}

		reader.complete();
		log_file.flush();
	}

	static void write_to_sinks(std::ofstream& file, const xtd::segmented_byte_view& buffer)
	{
		const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm local_time{};
		static_cast<void>(::localtime_r(&now, &local_time));

		char timestamp[20]{};
		static_cast<void>(std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time));

		const std::string formatted = std::format("[{}] {}", timestamp, buffer.to_string());
		
		static_cast<void>(::write(STDOUT_FILENO, formatted.data(), formatted.size()));

		if (file) {
			file.write(formatted.data(), static_cast<std::streamsize>(formatted.size()));
			file.flush();
		}
	}
};

template <typename... Args>
inline void logln(std::format_string<Args...> format, Args&&... args)
{
	buffered_logging::instance().println(format, std::forward<Args>(args)...);
}

inline void logln(std::string_view message)
{
	buffered_logging::instance().println(message);
}