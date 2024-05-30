// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:point;

import std;
import :multi_dimension;

#else

#include <type_traits>
#include <format>

#include <prometheus/macro.hpp>
#include <primitive/multi_dimension.hpp>
#endif

namespace gal::prometheus::primitive
{
	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point final : multi_dimension<T, basic_point<T>>
	{
		using value_type = T;

		constexpr static auto is_always_equal = true;

		value_type x;
		value_type y;

		constexpr explicit(false) basic_point(const value_type value = value_type{0}) noexcept
			: x{value},
			  y{value} {}

		constexpr basic_point(const value_type value_0, const value_type value_1) noexcept
			: x{value_0},
			  y{value_1} {}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> value_type
		{
			if constexpr (Index == 0) { return x; }
			else if constexpr (Index == 1) { return y; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> value_type&
		{
			if constexpr (Index == 0) { return x; }
			else if constexpr (Index == 1) { return y; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto distance(const basic_point<U>& other) const noexcept -> value_type
		{
			return std::sqrt(
					std::pow(x - static_cast<value_type>(other.x), 2),
					std::pow(y - static_cast<value_type>(other.y), 2)
					);
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto clamp_upper(const basic_point<U>& other) noexcept -> basic_point&
		{
			x = (x >= static_cast<value_type>(other.x)) ? static_cast<value_type>(other.x) : x;
			y = (y >= static_cast<value_type>(other.y)) ? static_cast<value_type>(other.y) : y;

			return *this;
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto clamp_lower(const basic_point<U>& other) noexcept -> basic_point&
		{
			x = (x < static_cast<value_type>(other.x)) ? static_cast<value_type>(other.x) : x;
			y = (y < static_cast<value_type>(other.y)) ? static_cast<value_type>(other.y) : y;

			return *this;
		}

		template<std::convertible_to<value_type> T1, std::convertible_to<value_type> T2>
		[[nodiscard]] constexpr auto between_horizontal(const basic_point<T1>& p1, const basic_point<T2>& p2) const noexcept -> bool
		{
			return x >= static_cast<value_type>(p1.x) and x < static_cast<value_type>(p2.x);
		}

		template<std::convertible_to<value_type> T1, std::convertible_to<value_type> T2>
		[[nodiscard]] constexpr auto between_vertical(const basic_point<T1>& p1, const basic_point<T2>& p2) const noexcept -> bool
		{
			return y >= static_cast<value_type>(p1.y) and y < static_cast<value_type>(p2.y);
		}

		template<std::convertible_to<value_type> T1, std::convertible_to<value_type> T2>
		[[nodiscard]] constexpr auto between(const basic_point<T1>& p1, const basic_point<T2>& p2) const noexcept -> bool
		{
			return between_horizontal(p1, p2) and between_vertical(p1, p2);
		}
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_BEGIN
	template<std::size_t Index, typename T>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_point<T>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<typename T>
	struct tuple_size<gal::prometheus::primitive::basic_point<T>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 2> {};

	template<typename T>
	struct formatter<gal::prometheus::primitive::basic_point<T>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_point<T>& point, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
					context.out(),
					"({},{})",
					point.x,
					point.y
					);
		}
	};

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_END
