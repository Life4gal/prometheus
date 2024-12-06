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

#if not defined(HAS_STD_DEBUGGING)
#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#elif defined(GAL_PROMETHEUS_PLATFORM_LINUX)
#include <fstream>
#include <string/charconv.hpp>
#elif defined(GAL_PROMETHEUS_PLATFORM_DARWIN)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>
#else
#error "fixme"
#endif
#endif

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

	auto is_debugger_present() noexcept -> bool
	{
		#if defined(HAS_STD_DEBUGGING)
		return std::is_debugger_present();
		#else

		#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)

		if (IsDebuggerPresent())
		{
			return true;
		}

		BOOL present = FALSE;
		if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &present))
		{
			return present == TRUE;
		}

		return false;

		#elif defined(GAL_PROMETHEUS_PLATFORM_LINUX)

		// TracerPid:       0
		std::ifstream status_file("/proc/self/status");
		std::string line;
		while (std::getline(status_file, line))
		{
			if (line.starts_with("TracerPid"))
			{
				const std::string_view sub{line.begin() + sizeof("TracerPid:") - 1, line.end()};

				if (const auto pid = string::from_string<int>(sub);
					pid.has_value() && *pid != 0)
				{
					return true;
				}
				return false;
			}
		}

		#elif defined(GAL_PROMETHEUS_PLATFORM_DARWIN)

		int mib[4];
	    struct kinfo_proc info;
	    size_t size = sizeof(info);

	    mib[0] = CTL_KERN;
	    mib[1] = KERN_PROC;
	    mib[2] = KERN_PROC_PID;
	    mib[3] = getpid();

	    if (sysctl(mib, 4, &info, &size, NULL, 0) == -1) {
	        return false;
	    }

	    return (info.kp_proc.p_flag & P_TRACED) != 0;

		#else
		#error "fixme"
		#endif

		#endif
	}

	auto breakpoint_if_debugging(const char* message) noexcept -> void
	{
		std::println(stderr, "BREAKPOINT: {}", message);

		#if defined(HAS_STD_DEBUGGING)
		return std::breakpoint_if_debugging();
		#else

		if (is_debugger_present())
		{
			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
		}

		#endif
	}

	auto breakpoint_or_terminate(const char* message) noexcept -> void
	{
		std::println(stderr, "BREAKPOINT: {}", message);

		if (is_debugger_present())
		{
			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
		}
		else
		{
			std::terminate();
		}
	}
}
