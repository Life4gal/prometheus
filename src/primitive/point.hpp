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
import :multidimensional;

#else

#include <type_traits>
#include <format>

#include <prometheus/macro.hpp>
#include <primitive/multidimensional.hpp>
#endif

namespace gal::prometheus::primitive
{
	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point final : multidimensional<T, basic_point<T>>
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

		template<std::convertible_to<value_type> U = value_type>
		[[nodiscard]] constexpr auto distance(const basic_point<U>& other) const noexcept -> value_type
		{
			// fixme
			#if __cpp_lib_hypot >= 202601L
			return std::hypot(x - static_cast<value_type>(other.x), y - static_cast<value_type>(other.y));
			#else
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				constexpr auto sqrt = [](const value_type value) noexcept
				{
					GAL_PROMETHEUS_DEBUG_AXIOM(value >= 0);

					if (value == 0) // NOLINT(clang-diagnostic-float-equal)
					{
						return value;
					}

					value_type prev = 0;
					value_type current = value / 2;

					while (current != prev) // NOLINT(clang-diagnostic-float-equal)
					{
						prev = current;
						current = (current + value / current) / 2;
					}

					return current;
				};

				return sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
			}

			return std::hypot(x - static_cast<value_type>(other.x), y - static_cast<value_type>(other.y));
			#endif
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] constexpr auto clamp(const basic_point<Low>& low, const basic_point<High>& high) noexcept -> basic_point&
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(low.x < high.x);
			GAL_PROMETHEUS_DEBUG_ASSUME(low.y < high.y);

			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				x = std::ranges::min(std::ranges::max(x, static_cast<value_type>(low.x)), static_cast<value_type>(high.x));
				y = std::ranges::min(std::ranges::max(y, static_cast<value_type>(low.y)), static_cast<value_type>(high.y));
			}
			else
			{
				x = std::ranges::clamp(x, low.x, high.x);
				y = std::ranges::clamp(y, low.y, high.y);
			}

			return *this;
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] friend constexpr auto clamp(
				const basic_point& point,
				const basic_point<Low>& low,
				const basic_point<High>& high
				) noexcept -> basic_point
		{
			auto result{point};

			result.clamp(low, high);
			return result;
		}

		template<std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
		[[nodiscard]] constexpr auto between_horizontal(const basic_point<T1>& left_top, const basic_point<T2>& right_bottom) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(static_cast<value_type>(left_top.x) < static_cast<value_type>(right_bottom.x));

			return x >= static_cast<value_type>(left_top.x) and x < static_cast<value_type>(right_bottom.x);
		}

		template<std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
		[[nodiscard]] constexpr auto between_vertical(const basic_point<T1>& left_top, const basic_point<T2>& right_bottom) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(static_cast<value_type>(left_top.y) < static_cast<value_type>(right_bottom.y));

			return y >= static_cast<value_type>(left_top.y) and y < static_cast<value_type>(right_bottom.y);
		}

		template<std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
		[[nodiscard]] constexpr auto between(const basic_point<T1>& left_top, const basic_point<T2>& right_bottom) const noexcept -> bool
		{
			return between_horizontal(left_top, right_bottom) and between_vertical(left_top, right_bottom);
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
