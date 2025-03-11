// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// fixme
#if __has_include(<print>)
#include <print>
#else
#include <format>
#include <cstdio>
#endif

#include <platform/exception.hpp>

namespace gal::prometheus::platform
{
	auto IException::print() const noexcept -> void
	{
		const auto& message = what();
		const auto& location = where();
		const auto& stacktrace = when();

		#if __has_include(<print>)
		std::println(
			stderr,
			"Error occurs while invoke function:\n{}\nat {}:{}\nReason:\n{}\nStack trace:\n{}",
			location.function_name(),
			location.file_name(),
			location.line(),
			message,
			stacktrace
		);
		#else
		const auto output = std::format(
			"Error occurs while invoke function:\n{}\nat {}:{}\nReason:\n{}\nStack trace:\n{}",
			location.function_name(),
			location.file_name(),
			location.line(),
			message,
			stacktrace
		);
		std::fputs(output.c_str(), stderr);
		#endif
	}
}
