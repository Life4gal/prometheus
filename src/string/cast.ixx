// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.string:cast;

import std;

export namespace gal::prometheus::string
{
	template<typename WString = std::wstring>
		requires requires(WString& s)
		{
			s.resize(typename WString::size_type{1});
			s.data();
		}
	[[nodiscard]] auto string_to_wstring(std::string_view string) -> WString;

	template<typename String = std::string>
		requires requires(String& s)
		{
			s.resize(typename String::size_type{1});
			s.data();
		}
	[[nodiscard]] auto wstring_to_string(std::wstring_view string) -> String;
}
