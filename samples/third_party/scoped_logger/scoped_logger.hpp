#pragma once
// source: https://github.com/philsquared/cpp_coroutines_helpers/blob/main/scoped_logger.h

#include <iostream>
#include <string>
#include <utility>

static int indent;
// Logs to cout, with current indent
#define LOG(...) std::cout << std::string(indent, '\t') << __VA_ARGS__ << std::endl

// Logs in and out of a scope
struct ScopedLogger
{
public:
	std::string m_name;
	ScopedLogger(std::string name)
		: m_name(std::move(name))
	{
		LOG("\\" << m_name);
		indent++;
	}

	~ScopedLogger()
	{
		indent--;
		LOG("/" << m_name);
	}
};

// Creates a ScopedLogger for the current function
#define LOGF() ScopedLogger logger##__LINE__(std::source_location::current().function_name())
