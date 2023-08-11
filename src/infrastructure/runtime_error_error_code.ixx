// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.infrastructure:runtime_error.error_code;

import std;

export namespace gal::prometheus::infrastructure
{
	/**
	 * @brief Get the OS error code from the last error received on this thread.
	 * @return The error code.
	 */
	[[nodiscard]] auto get_last_error_code() noexcept -> std::uint32_t;

	/**
	 * @brief Get the error message from an error code.
	 * @param error_code The error code returned by an os call.
	 * @return A formatted message.
	 */
	[[nodiscard]] auto get_error_message(std::uint32_t error_code) -> std::string;

	/**
	 * @brief Get the OS error message from the last error received on this thread.
	 * @return A formatted message.
	 */
	[[nodiscard]] auto get_last_error_message() -> std::string;
}// namespace gal::prometheus::infrastructure
