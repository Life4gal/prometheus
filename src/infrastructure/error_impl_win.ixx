// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/windows.hpp>

export module gal.prometheus.infrastructure:error.impl;

import std;
import :cast;
import :error.platform;
import :error.exception;

namespace gal::prometheus::infrastructure
{
	template<typename WString = std::wstring>
		requires requires(WString& s)
		{
			s.resize(typename WString::size_type{1});
			s.data();
		}
	[[nodiscard]] auto string_to_wstring(const std::string_view string) -> WString
	{
		const auto in_length  = narrow_cast<int>(string.size());
		const auto out_length = MultiByteToWideChar(CP_UTF8, 0, string.data(), in_length, nullptr, 0);

		if (out_length == 0) { throw OSError{"string_to_wstring failed!"}; }

		WString result{};
		result.resize(narrow_cast<typename WString::size_type>(out_length));
		MultiByteToWideChar(CP_UTF8, 0, string.data(), in_length, result.data(), out_length);
		return result;
	}

	template<typename String = std::string>
		requires requires(String& s)
		{
			s.resize(typename String::size_type{1});
			s.data();
		}
	[[nodiscard]] auto wstring_to_string(const std::wstring_view string) -> String
	{
		const auto in_length  = narrow_cast<int>(string.size());
		const auto out_length = WideCharToMultiByte(CP_UTF8, 0, string.data(), in_length, nullptr, 0, nullptr, nullptr);

		if (out_length == 0) { throw OSError{"wstring_to_string failed!"}; }

		String result{};
		result.resize(narrow_cast<typename String::size_type>(out_length));
		WideCharToMultiByte(CP_UTF8, 0, string.data(), in_length, result.data(), out_length, nullptr, nullptr);
		return result;
	}

	auto get_last_error_code() noexcept -> std::uint32_t { return static_cast<std::uint32_t>(GetLastError()); }

	auto get_error_message(const std::uint32_t error_code) -> std::string
	{
		const auto real_error_code = static_cast<DWORD>(error_code);

		LPWSTR     message_buffer;
		const auto length = FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				real_error_code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPWSTR>(&message_buffer),
				0,
				nullptr);

		if (length == 0) { throw OSError{std::format("Could not format OS error message for error_code [{}]", real_error_code)}; }

		auto message = wstring_to_string({message_buffer, length});
		LocalFree(message_buffer);

		// Windows messages will have a '\n' or '\r\n', delete it.
		if (const auto end_r = message.find_last_of('\r'); end_r != std::string::npos) { message.resize(end_r); }
		else { message.resize(message.find_last_of('\n')); }

		return message;
	}

	auto get_last_error_message() -> std::string { return get_error_message(get_last_error_code()); }

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
}// namespace gal::prometheus::infrastructure
