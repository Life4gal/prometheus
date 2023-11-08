// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/windows.hpp>

export module gal.prometheus.string:cast.impl;

import std;
import gal.prometheus.utility;

namespace gal::prometheus::string
{
	template<typename WString = std::wstring>
		requires requires(WString& s)
		{
			s.resize(typename WString::size_type{1});
			s.data();
		}
	[[nodiscard]] auto string_to_wstring(const std::string_view string) -> WString
	{
		const auto in_length  = utility::narrow_cast<int>(string.size());
		const auto out_length = MultiByteToWideChar(CP_UTF8, 0, string.data(), in_length, nullptr, 0);

		if (out_length == 0) { return {}; }

		WString result{};
		result.resize(utility::narrow_cast<typename WString::size_type>(out_length));
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
		const auto in_length  = utility::narrow_cast<int>(string.size());
		const auto out_length = WideCharToMultiByte(CP_UTF8, 0, string.data(), in_length, nullptr, 0, nullptr, nullptr);

		if (out_length == 0) { return {}; }

		String result{};
		result.resize(utility::narrow_cast<typename String::size_type>(out_length));
		WideCharToMultiByte(CP_UTF8, 0, string.data(), in_length, result.data(), out_length, nullptr, nullptr);
		return result;
	}
}
