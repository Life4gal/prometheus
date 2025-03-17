#pragma once

#include <source_location>
#include <print>
#include <chrono>

inline auto print_time(const std::source_location& location = std::source_location::current()) noexcept -> void
{
	std::println(stdout, "[{:%F %T}] {}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()), location.function_name());
}
