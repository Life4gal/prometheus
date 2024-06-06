// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.error:platform;

import std;
import :exception;

export namespace gal::prometheus::error
{
	/**
	 * @brief Get the OS error message from the last error received on this thread.
	 * @return A formatted message.
	 */
	[[nodiscard]] auto get_error_message() -> std::string;

	class OsError final : public Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
				const std::source_location& location = std::source_location::current(),
				std::stacktrace stacktrace = std::stacktrace::current()
				) noexcept(false) -> void //
		{
			error::panic<OsError>(get_error_message(), location, std::move(stacktrace));
		}
	};
} // namespace gal::prometheus::error
