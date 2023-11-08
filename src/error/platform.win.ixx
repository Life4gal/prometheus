// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/windows.hpp>

export module gal.prometheus.error:platform.impl;

import std;

// import gal.prometheus.string;

import :exception;

// fixme: string:cast;
namespace
{
	template<typename String = std::string>
		requires requires(String& s)
		{
			s.resize(typename String::size_type{1});
			s.data();
		}
	[[nodiscard]] auto wstring_to_string(const std::wstring_view string) -> String
	{
		const auto in_length  = static_cast<int>(string.size());//utility::narrow_cast<int>(string.size());
		const auto out_length = WideCharToMultiByte(CP_UTF8, 0, string.data(), in_length, nullptr, 0, nullptr, nullptr);

		if (out_length == 0) { return {}; }

		String result{};
		// result.resize(utility::narrow_cast<typename String::size_type>(out_length));
		result.resize(static_cast<typename String::size_type>(out_length));
		WideCharToMultiByte(CP_UTF8, 0, string.data(), in_length, result.data(), out_length, nullptr, nullptr);
		return result;
	}
}

namespace gal::prometheus::error
{
	[[nodiscard]] auto get_last_error_code() noexcept -> std::uint32_t { return static_cast<std::uint32_t>(GetLastError()); }

	[[nodiscard]] auto get_error_message(const std::uint32_t error_code) -> std::string
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

		if (length == 0)
		[[unlikely]]
		{
			throw OSError{std::format("Could not format OS error message for error_code [{}]", real_error_code)};
		}

		auto message = wstring_to_string({message_buffer, length});
		LocalFree(message_buffer);

		// Windows messages will have a '\n' or '\r\n', delete it.
		if (const auto end_r = message.find_last_of('\r'); end_r != std::string::npos) { message.resize(end_r); }
		else { message.resize(message.find_last_of('\n')); }

		return message;
	}

	auto get_last_error_message() -> std::string { return get_error_message(get_last_error_code()); }
}// namespace gal::prometheus::error
