#pragma once

#include <comdef.h>

#include <source_location>
#include <print>
#include <cstdio>

template<bool Abort = true>
auto check_hr_error(
	const HRESULT hr,
	const std::source_location& location = std::source_location::current()
) -> std::conditional_t<Abort, void, bool>
{
	if (SUCCEEDED(hr))
	{
		if constexpr (Abort)
		{
			return;
		}
		else
		{
			return true;
		}
	}

	const _com_error err(hr);
	std::println(stderr, "Error: {} --- at {}:{}", err.ErrorMessage(), location.file_name(), location.line());

	if constexpr (Abort)
	{
		#if defined(DEBUG) or defined(_DEBUG)
		__debugbreak();
		#endif
		std::abort();
	}
	else
	{
		return false;
	}
}
