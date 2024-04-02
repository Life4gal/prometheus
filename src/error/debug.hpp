// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.error:debug;

import std;

#else
#include <atomic>
#include <print>

#include <prometheus/macro.hpp>
#endif

namespace gal::prometheus::error
{
	inline std::atomic<const char*> terminate_reason{nullptr};

	/**
	 * @brief This will check if a debugger exists and potentially launch the Just-In-Time debugger if one is configured.
	 * @return Return true if the debugger is attached.
	 *
	 * @note It does not do the actual breaking.
	 */
	auto try_wakeup_debugger() noexcept -> bool;

	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	GAL_PROMETHEUS_MODULE_INLINE auto debug_break(const char* message) noexcept -> void
	{
		if (not try_wakeup_debugger())
		{
			std::println(std::cerr, "Unexpected behavior occurred but did not run under the debugger, terminate the program. \nReason: {}", message);
			terminate_reason.store(message, std::memory_order_relaxed);
			std::terminate();
		}
	}

	GAL_PROMETHEUS_MODULE_EXPORT_END
}
