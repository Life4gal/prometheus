// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <string>
#include <source_location>
#include <stacktrace>

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

	[[nodiscard]] auto is_debugger_present() noexcept -> bool;

	auto breakpoint_if_debugging(const char* message) noexcept -> void;

	auto breakpoint_or_terminate(const char* message) noexcept -> void;
}
