// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <string>
#include <exception>
#include <source_location>
#include <stacktrace>

#if __has_include(<debugging>)
#include <debugging>
#define HAS_STD_DEBUGGING
#endif

#include <prometheus/macro.hpp>

#include <platform/exception.hpp>

namespace gal::prometheus::platform
{
	[[nodiscard]] auto os_error_reason() -> std::string;

	class OsError final : public Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<OsError>(os_error_reason(), location, std::move(stacktrace));
		}
	};

	[[nodiscard]] auto is_debugger_present() noexcept -> bool
		#if defined(HAS_STD_DEBUGGING)
		{
			return std::is_debugger_present();
		}
		#else
	;
	#endif

	auto breakpoint_message(std::string_view message) noexcept -> void;

	#if defined(HAS_STD_DEBUGGING)
	inline auto breakpoint_if_debugging(const std::string_view message) noexcept -> void
	{
		breakpoint_message(message);
		std::breakpoint_if_debugging();
	}
	#else
	// use GAL_PROMETHEUS_ERROR_BREAKPOINT_IF
	#endif

	#if defined(HAS_STD_DEBUGGING)
	// inline auto breakpoint_or_terminate(const std::string_view message) noexcept -> void
	// {
	// 	breakpoint_message(message);
	// 	if (is_debugger_present())
	// 	{
	// 		GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
	// 	}
	// 	else
	// 	{
	// 		std::terminate();
	// 	}
	// }
	#else
	// use GAL_PROMETHEUS_ERROR_BREAKPOINT_OR_TERMINATE_IF
	#endif
}
