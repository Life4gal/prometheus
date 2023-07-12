#include <prometheus/debug/exception.hpp>

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <prometheus/platform/windows.hpp>
#else

#endif

namespace gal::prometheus::debug
{
	auto get_last_error_message() noexcept -> std::string
	{
		#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
		const auto last_error = GetLastError();

		LPWSTR message_buffer;
		FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				last_error,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPWSTR>(&message_buffer),
				0,
				nullptr);
		auto reason = platform::wstring_to_string<std::string>(message_buffer);
		LocalFree(message_buffer);

		// Windows messages always have a '\n' or '\r\n', delete it.
		if (const auto end_r = reason.find_last_of('\r'); end_r != std::string::npos) { reason.resize(end_r); }
		else { reason.resize(reason.find_last_of('\n')); }

		return reason;
		#else
		GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED("fixme");

		return {};
		#endif
	}

}
