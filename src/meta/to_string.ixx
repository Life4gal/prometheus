// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:meta.to_string;

import std;

import :meta.member;
import :meta.enumeration;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <format>
#include <type_traits>
#include <cstdint>

#include <prometheus/macro.hpp>
#include <meta/member.ixx>
#include <meta/enumeration.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::meta)
{
	template<
		std::ranges::output_range<char> StringType = std::basic_string<char>,
		bool ContainsTypeName = true,
		typename T
	>
		requires std::ranges::contiguous_range<StringType>
	constexpr auto to_string(const T& t, StringType& out) noexcept -> void
	{
		using type = T;

		// formatter
		if constexpr (std::formattable<type, char>)
		{
			if constexpr (ContainsTypeName) { std::format_to(std::back_inserter(out), "{}({})", meta::name_of<type>(), t); }
			else { std::format_to(std::back_inserter(out), "{}", t); }
		}
		// appendable
		// note: prefer formattable than appendable
		else if constexpr (requires { out.append(t); }) { out.append(t); }
		else if constexpr (requires { out.emplace_back(t); }) { out.emplace_back(t); }
		else if constexpr (requires { out.push_back(t); }) { out.push_back(t); }
		else if constexpr (requires { out += t; }) { out += t; }
		// construct from T
		else if constexpr (std::is_constructible_v<StringType, type>)
		{
			if constexpr (requires { out.append(StringType{t}); }) { out.append(StringType{t}); }
			else if constexpr (requires { out.emplace_back(StringType{t}); }) { out.emplace_back(StringType{t}); }
			else if constexpr (requires { out.push_back(StringType{t}); }) { out.push_back(StringType{t}); }
			else if constexpr (requires { out += StringType{t}; }) { out += StringType{t}; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("not appendable."); }
		}
		// member function
		else if constexpr (requires { t.to_string(out); })
		{
			if constexpr (ContainsTypeName) { std::format_to(std::back_inserter(out), "{}(", meta::name_of<type>()); }
			t.to_string(out);
			if constexpr (ContainsTypeName) { out.push_back(')'); }
		}
		else if constexpr (requires { t.to_string(); })
		{
			if constexpr (ContainsTypeName) { std::format_to(std::back_inserter(out), "{}(", meta::name_of<type>()); }
			meta::to_string<StringType, false>(t.to_string(), out);
			if constexpr (ContainsTypeName) { out.push_back(')'); }
		}
		// pointer
		// note: std::nullptr_t satisfies std::formattable<type, char>
		// else if constexpr (std::is_null_pointer_v<type> or std::is_pointer_v<type>)
		else if constexpr (std::is_pointer_v<type>)
		{
			// if constexpr (std::is_null_pointer_v<type>) { meta::to_string<StringType, false>("nullptr", out); }
			// else
			// {
			if (t == nullptr)
			{
				if constexpr (ContainsTypeName) { std::format_to(std::back_inserter(out), "{}(0x00000000)", meta::name_of<type>()); }
				else { meta::to_string<StringType, false>(nullptr, out); }
				return;
			}

			if constexpr (ContainsTypeName) { std::format_to(std::back_inserter(out), "{}(", meta::name_of<type>()); }
			// address
			std::format_to(std::back_inserter(out), "0x{:x} => ", reinterpret_cast<std::uintptr_t>(t));
			// sub-element does not contains type name.
			meta::to_string<StringType, false>(*t, out);
			if constexpr (ContainsTypeName) { out.push_back(')'); }
			// }
		}
		// container
		else if constexpr (std::ranges::range<type>)
		{
			if constexpr (ContainsTypeName) { std::format_to(std::back_inserter(out), "{}", meta::name_of<type>()); }
			out.push_back('[');

			std::ranges::for_each(
				t,
				[&out]<typename E>(const E& element) noexcept -> decltype(auto)
				{
					// sub-element does not contains type name.
					meta::to_string<StringType, false>(element, out);
					out.push_back(',');
				});

			out.back() = ']';
		}
		// meta::member_gettable
		else if constexpr (meta::member_gettable_t<type>)
		{
			if constexpr (ContainsTypeName) { std::format_to(std::back_inserter(out), "{}", meta::name_of<type>()); }
			out.push_back('{');

			if constexpr (meta::member_size<type>() == 0) { out.push_back(','); }
			else
			{
				meta::member_walk(
					[&out]<std::size_t Index, typename E>(const E& element) noexcept -> void
					{
						std::format_to(std::back_inserter(out), ".{} = ", meta::name_of_member<type, Index>());
						// sub-element does not contains type name.
						meta::to_string<StringType, false>(element, out);
						out.push_back(',');
					},
					t);
			}

			out.back() = '}';
		}
		// enum
		else if constexpr (std::is_enum_v<type>) //
		{
			std::format_to(std::back_inserter(out), "{}", meta::name_of(t));
		}
		// any
		else //
		{
			std::format_to(std::back_inserter(out), "{}(?)", meta::name_of<type>());
		}
	}

	template<
		std::ranges::output_range<char> StringType = std::basic_string<char>,
		bool ContainsTypeName = true,
		typename T
	>
		requires std::ranges::contiguous_range<StringType>
	[[nodiscard]] constexpr auto to_string(const T& t) noexcept -> StringType
	{
		StringType out;
		meta::to_string<StringType, ContainsTypeName, T>(t, out);
		return out;
	}
}
