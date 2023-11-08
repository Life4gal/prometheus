// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.error:debug;

import std;

namespace gal::prometheus::error
{
	//
	inline std::atomic<const char*> terminate_reason{nullptr};

	/**
	 * @brief This will check if a debugger exists and potentially launch the Just-In-Time debugger if one is configured.
	 * @return Return true if the debugger is attached.
	 *
	 * @note It does not do the actual breaking.
	 */
	auto try_wakeup_debugger() noexcept -> bool;

	export
	{
		auto debug_break(const char* message) noexcept -> void
		{
			if (not try_wakeup_debugger())
			{
				std::cerr << std::format("Unexpected behavior occurred but did not run under the debugger, terminate the program. \nReason. {}\n", message);
				terminate_reason.store(message, std::memory_order_relaxed);
				std::terminate();
			}
		}
	}
}// namespace gal::prometheus::error
