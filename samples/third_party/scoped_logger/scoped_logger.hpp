#pragma once
// source: https://github.com/philsquared/cpp_coroutines_helpers/blob/main/scoped_logger.h

#include <iostream>
#include <print>
#include <string>
#include <utility>
#include <source_location> // IWYU pragma: export

static inline int S_INDENT;
// Logs to cout, with current indent
#define LOG(...) std::cout << std::string(S_INDENT, '\t') << __VA_ARGS__ << std::endl

// Logs in and out of a scope
struct ScopedLogger
{
public:
	std::string m_name;
	ScopedLogger(std::string name)
		: m_name(std::move(name))
	{
		LOG("\\" << m_name);
		S_INDENT++;
	}

	~ScopedLogger()
	{
		S_INDENT--;
		LOG("/" << m_name);
	}
};

inline std::string get_indent_string() {
	return std::string(S_INDENT, '\t');
}

inline void logger_println(std::string_view message) {
	std::println("{}{}", get_indent_string(), message);
}

// Creates a ScopedLogger for the current function
#define LOGF() ScopedLogger logger##__LINE__(std::source_location::current().function_name())
