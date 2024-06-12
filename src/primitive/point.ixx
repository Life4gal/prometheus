// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:point;

import std;
import gal.prometheus.functional;
import gal.prometheus.error;
import gal.prometheus.meta;

import :multidimensional;

#else
#pragma once

#include <type_traits>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <functional/functional.ixx>
#include <error/error.ixx>
#include <meta/meta.ixx>
#include <primitive/multidimensional.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::primitive)
{
	enum class DirectionCategory
	{
		X,
		Y,
		Z,

		ALL,
	};

	template<typename T, std::size_t N>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point;

	template<typename>
	struct is_basic_point : std::false_type {};

	template<typename T, std::size_t N>
	struct is_basic_point<basic_point<T, N>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_point_v = is_basic_point<T>::value;

	template<typename T>
	concept basic_point_t = is_basic_point_v<T>;

	template<typename, typename>
	struct is_point_compatible : std::false_type {};

	// force use copy-ctor
	template<typename OtherType, typename SelfType>
	concept member_gettable_but_not_same_t = (not std::is_same_v<OtherType, SelfType>) and meta::member_gettable_t<OtherType>;

	template<typename T>
	struct is_point_compatible<T, T> : std::false_type {};

	template<typename PointValueType, member_gettable_but_not_same_t<basic_point<PointValueType, 2>> U>
		requires(meta::member_size<U>() == 2)
	struct is_point_compatible<U, basic_point<PointValueType, 2>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, U>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, U>, PointValueType>
			> {};

	template<typename PointValueType, member_gettable_but_not_same_t<basic_point<PointValueType, 3>> U>
		requires(meta::member_size<PointValueType>() == 3)
	struct is_point_compatible<U, basic_point<PointValueType, 3>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, U>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, U>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<2, U>, PointValueType>
			> {};

	template<typename OtherType, typename PointType>
	constexpr auto is_point_compatible_v = is_point_compatible<OtherType, PointType>::value;

	template<typename OtherType, typename PointType>
	concept point_compatible_t = is_point_compatible_v<OtherType, PointType>;

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point<T, 2> final : multidimensional<T, basic_point<T, 2>>
	{
		using value_type = T;

		constexpr static auto is_always_equal = true;

		value_type x;
		value_type y;

		constexpr explicit(false) basic_point(const value_type value = value_type{0}) noexcept
			: x{value},
			  y{value} {}

		constexpr basic_point(const value_type x, const value_type y) noexcept
			: x{x},
			  y{y} {}

		template<point_compatible_t<basic_point> U>
		constexpr explicit basic_point(const U& value) noexcept
			: basic_point{}
		{
			*this = value;
		}

		constexpr basic_point(const basic_point&) noexcept = default;
		constexpr basic_point(basic_point&&) noexcept = default;
		constexpr auto operator=(const basic_point&) noexcept -> basic_point& = default;
		constexpr auto operator=(basic_point&&) noexcept -> basic_point& = default;
		constexpr ~basic_point() noexcept = default;

		template<point_compatible_t<basic_point> U>
		constexpr auto operator=(const U& value) noexcept -> basic_point&
		{
			const auto [_x, _y] = value;
			x = _x;
			y = _y;

			return *this;
		}

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
		[[nodiscard]] constexpr auto distance(const basic_point<U, 2>& other) const noexcept -> value_type //
		{
			return functional::hypot(x - static_cast<value_type>(other.x), y - static_cast<value_type>(other.y));
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] constexpr auto clamp(const basic_point<Low, 2>& low, const basic_point<High, 2>& high) noexcept -> basic_point&
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
			const basic_point<Low, 2>& low,
			const basic_point<High, 2>& high
		) noexcept -> basic_point
		{
			auto result{point};

			result.clamp(low, high);
			return result;
		}

		template<DirectionCategory Category, std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
		[[nodiscard]] constexpr auto between(const basic_point<T1, 2>& left_top, const basic_point<T2, 2>& right_bottom) const noexcept -> bool
		{
			if constexpr (Category == DirectionCategory::ALL) //
			{
				return
						between<DirectionCategory::X>(left_top, right_bottom) and
						between<DirectionCategory::Y>(left_top, right_bottom);
			}
			else if constexpr (Category == DirectionCategory::X)
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(static_cast<value_type>(left_top.x) < static_cast<value_type>(right_bottom.x));

				return x >= static_cast<value_type>(left_top.x) and x < static_cast<value_type>(right_bottom.x);
			}
			else if constexpr (Category == DirectionCategory::Y)
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(static_cast<value_type>(left_top.y) < static_cast<value_type>(right_bottom.y));

				return y >= static_cast<value_type>(left_top.y) and y < static_cast<value_type>(right_bottom.y);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point<T, 3> final : multidimensional<T, basic_point<T, 3>>
	{
		using value_type = T;

		constexpr static auto is_always_equal = true;

		value_type x;
		value_type y;
		value_type z;

		constexpr explicit(false) basic_point(const value_type value = value_type{0}) noexcept
			: x{value},
			  y{value},
			  z{value} {}

		constexpr basic_point(const value_type x, const value_type y, const value_type z) noexcept
			: x{x},
			  y{y},
			  z{z} {}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() const noexcept -> value_type
		{
			if constexpr (Index == 0) { return x; }
			else if constexpr (Index == 1) { return y; }
			else if constexpr (Index == 2) { return z; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() noexcept -> value_type&
		{
			if constexpr (Index == 0) { return x; }
			else if constexpr (Index == 1) { return y; }
			else if constexpr (Index == 2) { return z; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr explicit(false) operator basic_point<value_type, 2>() const noexcept { return {x, y}; }

		template<std::convertible_to<value_type> U = value_type>
		[[nodiscard]] constexpr auto distance(const basic_point<U, 3>& other) const noexcept -> value_type //
		{
			return functional::hypot(
				x - static_cast<value_type>(other.x),
				y - static_cast<value_type>(other.y),
				z - static_cast<value_type>(other.z)
			);
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] constexpr auto clamp(const basic_point<Low, 3>& low, const basic_point<High, 3>& high) noexcept -> basic_point&
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(low.x < high.x);
			GAL_PROMETHEUS_DEBUG_ASSUME(low.y < high.y);
			GAL_PROMETHEUS_DEBUG_ASSUME(low.z < high.z);

			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				x = std::ranges::min(std::ranges::max(x, static_cast<value_type>(low.x)), static_cast<value_type>(high.x));
				y = std::ranges::min(std::ranges::max(y, static_cast<value_type>(low.y)), static_cast<value_type>(high.y));
				z = std::ranges::min(std::ranges::max(z, static_cast<value_type>(low.z)), static_cast<value_type>(high.z));
			}
			else
			{
				x = std::ranges::clamp(x, low.x, high.x);
				y = std::ranges::clamp(y, low.y, high.y);
				z = std::ranges::clamp(z, low.z, high.z);
			}

			return *this;
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] friend constexpr auto clamp(
			const basic_point& point,
			const basic_point<Low, 3>& low,
			const basic_point<High, 3>& high
		) noexcept -> basic_point
		{
			auto result{point};

			result.clamp(low, high);
			return result;
		}

		template<DirectionCategory Category, std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
		[[nodiscard]] constexpr auto between(const basic_point<T1, 3>& left_top, const basic_point<T2, 3>& right_bottom) const noexcept -> bool
		{
			if constexpr (Category == DirectionCategory::ALL) //
			{
				return
						between<DirectionCategory::X>(left_top, right_bottom) and
						between<DirectionCategory::Y>(left_top, right_bottom) and
						between<DirectionCategory::Z>(left_top, right_bottom);
			}
			else if constexpr (Category == DirectionCategory::X)
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(static_cast<value_type>(left_top.x) < static_cast<value_type>(right_bottom.x));

				return x >= static_cast<value_type>(left_top.x) and x < static_cast<value_type>(right_bottom.x);
			}
			else if constexpr (Category == DirectionCategory::Y)
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(static_cast<value_type>(left_top.y) < static_cast<value_type>(right_bottom.y));

				return y >= static_cast<value_type>(left_top.y) and y < static_cast<value_type>(right_bottom.y);
			}
			else if constexpr (Category == DirectionCategory::Z)
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(static_cast<value_type>(left_top.z) < static_cast<value_type>(right_bottom.z));

				return z >= static_cast<value_type>(left_top.z) and z < static_cast<value_type>(right_bottom.z);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
{
	template<std::size_t Index, typename T, std::size_t N>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_point<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<typename T, std::size_t N>
	struct tuple_size<gal::prometheus::primitive::basic_point<T, N>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, N> {};

	template<typename T, std::size_t N>
	struct formatter<gal::prometheus::primitive::basic_point<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_point<T, N>& point, FormatContext& context) const noexcept -> auto
		{
			if constexpr (N == 2) { return std::format_to(context.out(), "({},{})", point.x, point.y); }
			else if constexpr (N == 3) { return std::format_to(context.out(), "({},{},{})", point.x, point.y, point.z); }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};
} // namespace std
