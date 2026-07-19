#ifndef SRC_UTILS_H
#define SRC_UTILS_H

#include <print>
#include <mutex>
#include <format>
#include <cctype>
#include <algorithm>


inline bool is_equal_ignore_case(const std::string_view& a, const std::string_view& b) {
	if (a.size() != b.size()) {
		return false;
	}
	return std::equal(a.begin(), a.end(), b.begin(), b.end(),
		[](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
}

inline constinit std::mutex __utils_get_println_mutex__;

template<typename... Args>
void println_locked(
	std::format_string<Args...> format,
	Args&&... args)
{
	std::lock_guard lock(__utils_get_println_mutex__);

	std::println(
		format,
		std::forward<Args>(args)...);
}


#endif // SRC_UTILS_H