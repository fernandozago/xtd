#pragma once

#include <atomic>
#include <cstddef>
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
	void println(std::format_string<Args...> format, Args&&... args)
	{
		append_line_to_buffer(std::format(format, std::forward<Args>(args)...) + "\n");
	}

	void println(std::string_view message)
	{
		append_line_to_buffer(std::string(message) + "\n");
	}

	[[nodiscard]]
	const std::string& file_path() const noexcept
	{
		return m_file_path;
	}

private:
	xtd::pipeline m_pipeline;
	xtd::pipe_writer m_writer;
	std::string m_file_path;
	std::jthread m_worker;
	std::atomic_bool m_closed{false};

	buffered_logging()
		: m_pipeline()
		, m_writer(m_pipeline)
		, m_file_path("/tmp/xtd-chat.log")
		, m_worker([this](std::stop_token stop_token) {
			process_logs(stop_token);
		})
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

	void append_line_to_buffer(std::string message)
	{
		if (m_closed.load()) {
			return;
		}
		static_cast<void>(m_writer.write(message));
	}

	void process_logs(std::stop_token stop_token)
	{
        // Open the log file in binary mode and truncate it if it already exists
        constexpr std::string_view path = "/tmp/xtd-chat.log";
		::unlink(path.data());
        std::ofstream file(std::string(path), std::ios::binary | std::ios::trunc);
        
		xtd::pipe_reader reader(m_pipeline);
		try {
			while (const xtd::read_result result = reader.read(stop_token))
			{
				const xtd::segmented_byte_view buffer = result.buffer();
                if (!buffer.empty()) {
                    write_to_sinks(file, buffer);
                }
				reader.advance(buffer.end(), buffer.end());

				if (result.completed() || stop_token.stop_requested()) {
					break;
				}
			}
		}
		catch (...) {
		}

		reader.complete();
		file.flush();
	}

	void write_to_sinks(std::ofstream& file, const xtd::segmented_byte_view& buffer)
	{
		for (const std::span<const std::byte> segment : buffer.segments())
		{
			static_cast<void>(::write(STDOUT_FILENO, reinterpret_cast<const char*>(segment.data()), segment.size()));

			if (file) {
				file.write(reinterpret_cast<const char*>(segment.data()), static_cast<std::streamsize>(segment.size()));
			}
		}

		if (file) {
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