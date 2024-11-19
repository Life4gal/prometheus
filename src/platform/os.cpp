#include <string>
#include <print>
#include <atomic>
#include <format>
#include <system_error>

#if __has_include(<debugging>)
#include <debugging>
#define HAS_STD_DEBUGGING
#endif

#include <prometheus/macro.hpp>

#include <platform/os.hpp>

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else
#include <cerrno>
#endif

namespace
{
	thread_local std::atomic<const char*> breakpoint_reason;

	auto call_debugger() noexcept -> bool
	{
		#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
		if (IsDebuggerPresent())
		{
			// When running under the debugger, __debugbreak() after returning.
			return true;
		}

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
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
		EXCEPTION_RECORD ExceptionRecord;
		ZeroMemory(&ExceptionRecord, sizeof(ExceptionRecord));
		ExceptionRecord.ExceptionCode = EXCEPTION_BREAKPOINT;

		CONTEXT Context;
		ZeroMemory(&Context, sizeof(Context));
		Context.ContextFlags = CONTEXT_FULL;
		RtlCaptureContext(&Context);

		EXCEPTION_POINTERS ExceptionPointers;
		ExceptionPointers.ExceptionRecord = &ExceptionRecord;
		ExceptionPointers.ContextRecord = &Context;

		if (UnhandledExceptionFilter(&ExceptionPointers) == EXCEPTION_CONTINUE_EXECUTION)
		{
			return true;
		}

		return false;
		#endif

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
}

namespace gal::prometheus::platform
{
	auto os_error_reason() -> std::string
	{
		return std::system_category().message(
			#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
			static_cast<int>(GetLastError())
			#else
			errno
			#endif
		);
	}

	auto breakpoint(const char* reason) noexcept -> void
	{
		breakpoint_reason.store(reason, std::memory_order::relaxed);

		auto fallback = [reason]
		{
			std::println(
				stderr,
				"Unexpected behavior occurred but did not run under the debugger, terminate the program. \nReason. {}\n",
				reason
			);
			std::terminate();
		};

		#if defined(HAS_STD_DEBUGGING)
		#if __cpp_lib_debugging >= 202601L
		std::breakpoint_if_debugging();
		#elif __cpp_lib_debugging >= 202403L
		if (std::is_debugger_present())
		{
			std::breakpoint();
			return;
		}
		else
		{
			fallback();
		}
		#endif
		#endif

		if (call_debugger())
		{
			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
		}
		else
		{
			fallback();
		}
	}
}
