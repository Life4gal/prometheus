// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.meta:to_string;

import std;
export import :string;
export import :member_name;

#else
#include <format>

#include <prometheus/macro.hpp>
#include <meta/member_name.hpp>
#endif

namespace gal::prometheus::meta
{
	template<
		std::ranges::input_range StringType = std::basic_string<char>,
		std::ranges::view StringViewType = std::basic_string_view<char>,
		bool ContainsTypeName = true,
		typename T>
		requires(
			std::is_same_v<typename StringType::value_type, typename StringViewType::value_type> and
			std::is_same_v<typename StringType::value_type, char>)
	[[nodiscard]] constexpr auto to_string(const T& t) noexcept -> decltype(auto)
	{
		using type = T;

		// construct from T
		if constexpr (std::is_constructible_v<StringType, type>) { return StringType{t}; }
		// construct from T
		else if constexpr (std::is_constructible_v<StringViewType, type>) { return StringViewType{t}; }
		// member function
		else if constexpr (requires { t.to_string(); }) { return t.to_string(); }
		// nullptr
		else if constexpr (std::is_null_pointer_v<type>) { return StringViewType{"nullptr"}; }
		// pointer
		else if constexpr (std::is_pointer_v<type>)
		{
			if (t == nullptr)
			{
				if constexpr (ContainsTypeName)
				{
					StringType s;
					std::format_to(std::back_inserter(s), "{}(0x00000000)", meta::name_of<type>());
					return s;
				}
				else { return StringType{"nullptr"}; }
			}

			if constexpr (ContainsTypeName)
			{
				StringType s;
				std::format_to(
						std::back_inserter(s),
						"{}(0x{:x} => {})",
						meta::name_of<type>(),
						reinterpret_cast<std::uintptr_t>(t),
						// sub-element does not contains type name.
						meta::to_string<StringType, StringViewType, false>(*t)
						);
				return s;
			}
			else
			{
				StringType s;
				std::format_to(
						std::back_inserter(s),
						"0x{:x} => {}",
						reinterpret_cast<std::uintptr_t>(t),
						// sub-element does not contains type name.
						meta::to_string<StringType, StringViewType, false>(*t));
				return s;
			}
		}
		// container
		else if constexpr (std::ranges::range<type>)
		{
			StringType result;

			if constexpr (ContainsTypeName) { result.append_range(meta::name_of<type>()); }

			result.push_back('[');

			std::ranges::for_each(
					t,
					[&result]<typename S>(S&& string) noexcept -> void
					{
						result.append_range(std::forward<S>(string));
						result.push_back(',');
					},
					[]<typename E>(const E& element) noexcept -> decltype(auto)
					{
						// sub-element does not contains type name.
						return meta::to_string<StringType, StringViewType, false>(element);
					});

			result.back() = ']';

			return result;
		}
		// aggregate 
		else if constexpr (std::is_aggregate_v<type>)
		{
			StringType result;

			if constexpr (ContainsTypeName) { result.append_range(meta::name_of<type>()); }

			result.push_back('{');

			if constexpr (meta::member_size<type>() == 0) { result.push_back(','); }
			else
			{
				meta::member_for_each(
						[&result]<std::size_t Index, typename E>(const E& element) noexcept -> void
						{
							std::format_to(std::back_inserter(result), ".{} = ", meta::name_of_member<type, Index>());
							// sub-element does not contains type name.
							result.append_range(meta::to_string<StringType, StringViewType, false>(element));
							result.push_back(',');
						},
						t);
			}

			result.back() = '}';

			return result;
		}
		// formatter
		else if constexpr (std::formattable<type, char>)
		{
			if constexpr (ContainsTypeName)
			{
				StringType s;
				std::format_to(std::back_inserter(s), "{}({})", meta::name_of<type>(), t);
				return s;
			}
			else
			{
				StringType s;
				std::format_to(std::back_inserter(s), "{}", t);
				return s;
			}
		}
		// any
		else
		{
			StringType s;
			std::format_to(std::back_inserter(s), "{}(?)", meta::name_of<type>());
			return s;
		}
	}
}
