// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.infrastructure:runtime_error.terminate_message;

import std;

export namespace gal::prometheus::infrastructure
{
	inline std::atomic<const char*> g_terminate_reason{nullptr};

	/**
	 * @brief This will check if a debugger exists and potentially launch the Just-In-Time debugger if one is configured.
	 * @return Return true if the debugger is attached.
	 *
	 * @note It does not do the actual breaking.
	 */
	auto try_wakeup_debugger() noexcept -> bool;

	auto try_debug_or_terminate(const char* message) noexcept -> void
	{
		if (not try_wakeup_debugger())
		{
			std::cerr << std::format("Unexpected behavior occurred but did not run under the debugger, terminate the program. \nReason. {}\n", message);
			g_terminate_reason.store(message, std::memory_order_relaxed);
			std::terminate();
		}
	}
}// namespace gal::prometheus::infrastructure
