// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/windows.hpp>

export module gal.prometheus.error:debug.impl;

import std;

namespace gal::prometheus::error
{
	auto try_wakeup_debugger() noexcept -> bool
	{
		if (IsDebuggerPresent())
		{
			// When running under the debugger, __debugbreak() after returning.
			return true;
		}

		__try
		{
			__try
			{
				// Attempt to break, causing an exception.
				DebugBreak();

				// The UnhandledExceptionFilter() will be called to attempt to attach a debugger.
				//  * If the jit-debugger is not configured the user gets a error dialogue-box that
				//    with "Abort", "Retry (Debug)", "Ignore". The "Retry" option will only work
				//    when the application is already being debugged.
				//  * When the jit-debugger is configured the user gets a dialogue window which allows
				//    a selection of debuggers and a "OK (Debug)", "Cancel (aborts application)".
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

		// The jit-debugger was configured, but the use pressed Cancel.
		return false;
	}
}// namespace gal::prometheus::error
