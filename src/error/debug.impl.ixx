// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>
#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else

#endif

export module gal.prometheus.error:debug.impl;

import std;
import :debug;

#else
#pragma once

#include <print>
#include <iostream>

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else

#endif

#include <prometheus/macro.hpp>

#endif

namespace
{
	auto call_debugger() noexcept -> bool
	{
		#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
		if (IsDebuggerPresent())
		{
			// When running under the debugger, __debugbreak() after returning.
			return true;
		}

		__try // NOLINT(clang-diagnostic-language-extension-token)
		{
			__try // NOLINT(clang-diagnostic-language-extension-token)
			{
				// Attempt to break, causing an exception.
				DebugBreak();

				// The UnhandledExceptionFilter() will be called to attempt to attach a debugger.
				//  * If the jit-debugger is not configured the user gets an error dialogue-box that
				//    with "Abort", "Retry (Debug)", "Ignore". The "Retry" option will only work
				//    when the application is already being debugged.
				//  * When the jit-debugger is configured the user gets a dialogue window which allows
				//    a selection of debuggers and an "OK (Debug)", "Cancel (aborts application)".
			}
			__except (UnhandledExceptionFilter(GetExceptionInformation()))
			{
				// The jit-debugger is not configured and the user pressed any of the buttons.
				return false;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			// User pressed "OK", debugger has been attached, __debugbreak() after return.
			return true;
		}

		// The jit-debugger was configured, but the user pressed Cancel.
		return false;
		#else
		#error "fixme"
		#endif
	}

	thread_local std::atomic<const char*> terminate_reason{nullptr};
}

namespace gal::prometheus::error
{
	auto debug_break(const char* message) noexcept -> void
	{
		if (not call_debugger())
		{
			std::println(
					std::cerr,
					"Unexpected behavior occurred but did not run under the debugger, terminate the program. \nReason. {}\n",
					message
					);
			terminate_reason.store(message, std::memory_order_relaxed);
			std::terminate();
		}
	}
}
