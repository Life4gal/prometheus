// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else
#endif

#include <prometheus/macro.hpp>

export module gal.prometheus:error.debug.impl;

import std;

import :error.debug;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else
#endif

#if __has_include(<print>)
#include <print>
#endif
#include <iostream>
#include <atomic>
#include <format>

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
		// Check if we're running under a debugger by checking the process status.
		static bool debugger_present = false;
		static bool checked = false;

		if (not checked) {
			debugger_present = (isatty(STDERR_FILENO) != 0);
			checked = true;
		}

		if (debugger_present) {
			return true;
		}

		// Raise a SIGTRAP signal to invoke the debugger.
		raise(SIGTRAP);

		// After raising SIGTRAP, if a debugger is attached, the execution will stop.
		// If no debugger is attached, the signal might be ignored, or the program may terminate.
		// For simplicity, we assume that if we reach here without terminating, no debugger was attached.
		return false;
		#endif
	}

	thread_local std::atomic<const char*> terminate_reason{nullptr};
}

GAL_PROMETHEUS_COMPILER_MODULE_IMPL_NAMESPACE(gal::prometheus::error)
{
	auto debug_break(const char* message) noexcept -> void
	{
		if (not call_debugger())
		{
			#if __has_include(<print>)
			std::println(
				std::cerr,
				"Unexpected behavior occurred but did not run under the debugger, terminate the program. \nReason. {}\n",
				message
			);
			#else
			std::cerr << std::format("Unexpected behavior occurred but did not run under the debugger, terminate the program. \nReason. {}\n\n", message);
			#endif
			terminate_reason.store(message, std::memory_order_relaxed);
			std::terminate();
		}
	}
}
