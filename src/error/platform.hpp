// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

	#include <prometheus/macro.hpp>

export module gal.prometheus.error:platform;

import std;
import :exception;

#else
#include <system_error>

#include <prometheus/macro.hpp>
#include <error/exception.hpp>
#endif

namespace gal::prometheus::error
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	/**
	 * @brief Get the OS error message from the last error received on this thread.
	 * @return A formatted message.
	 */
	[[nodiscard]] inline auto get_error_message() noexcept -> std::string
	{
		return std::system_category().message(
				#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
				GetLastError()
				#else
				errno
				#endif
				);
	}

	class OsError final : public Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] constexpr static auto panic(
				const std::source_location& location = std::source_location::current(),
				std::stacktrace stacktrace = std::stacktrace::current()
				) noexcept(false) -> OsError //
		{
			return error::panic<OsError>(get_error_message(), location, std::move(stacktrace));
		}
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
