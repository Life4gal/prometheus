// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:primitive.point;

import std;

import :functional;
import :meta;
#if GAL_PROMETHEUS_COMPILER_DEBUG
import :platform;
#endif

import :primitive.multidimensional;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>
#include <algorithm>
#include <format>

#include <prometheus/macro.hpp>

#include <functional/functional.ixx>
#include <meta/meta.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <primitive/multidimensional.ixx>

#endif

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: primitive
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(primitive)
#endif
{
	enum class DirectionCategory
	{
		X,
		Y,
		Z,

		ALL,
	};

	template<std::size_t N, typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point;

	template<typename>
	struct is_basic_point : std::false_type {};

	template<std::size_t N, typename T>
	struct is_basic_point<basic_point<N, T>> : std::true_type {};

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

	template<typename PointValueType, member_gettable_but_not_same_t<basic_point<2, PointValueType>> U>
		requires(meta::member_size<U>() == 2)
	struct is_point_compatible<U, basic_point<2, PointValueType>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, U>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, U>, PointValueType>
			> {};

	template<typename PointValueType, member_gettable_but_not_same_t<basic_point<3, PointValueType>> U>
		requires(meta::member_size<PointValueType>() == 3)
	struct is_point_compatible<U, basic_point<3, PointValueType>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, U>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, U>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<2, U>, PointValueType>
			> {};

	template<typename OtherType, typename PointType>
	constexpr auto is_point_compatible_v = is_point_compatible<OtherType, PointType>::value;

	template<typename OtherType, typename PointType>
	concept point_compatible_t = is_point_compatible_v<OtherType, PointType>;

	template<std::size_t N, typename L, typename R>
	[[nodiscard]] constexpr auto operator==(const basic_point<N, L>& lhs, const basic_point<N, R>& rhs) noexcept -> bool
	{
		if constexpr (N == 2)
		{
			return lhs.x == rhs.x and lhs.y == rhs.y;
		}
		else if constexpr (N == 3)
		{
			return lhs.x == rhs.x and lhs.y == rhs.y and lhs.z == rhs.z;
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<std::size_t N, typename T, point_compatible_t<basic_point<N, T>> R>
	[[nodiscard]] constexpr auto operator==(const basic_point<N, T>& lhs, const R& rhs) noexcept -> bool
	{
		if constexpr (N == 2)
		{
			return lhs.x == meta::member_of_index<0>(rhs) and lhs.y == meta::member_of_index<1>(rhs);
		}
		else if constexpr (N == 3)
		{
			return
					lhs.x == meta::member_of_index<0>(rhs) and
					lhs.y == meta::member_of_index<1>(rhs) and
					lhs.z == meta::member_of_index<2>(rhs);
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<std::size_t N, typename T, point_compatible_t<basic_point<N, T>> L>
	[[nodiscard]] constexpr auto operator==(const L& lhs, const basic_point<N, T>& rhs) noexcept -> bool
	{
		return rhs == lhs;
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point<2, T> final : multidimensional<basic_point<2, T>>
	{
		using value_type = T;

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
			x = static_cast<value_type>(_x);
			y = static_cast<value_type>(_y);

			return *this;
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> const value_type&
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

		[[nodiscard]] constexpr explicit operator basic_point<3, value_type>() const noexcept { return {x, y, 0}; }

		template<std::convertible_to<value_type> U = value_type>
		[[nodiscard]] constexpr auto distance(const basic_point<2, U>& other) const noexcept -> value_type //
		{
			return functional::hypot(x - static_cast<value_type>(other.x), y - static_cast<value_type>(other.y));
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] constexpr auto clamp(const basic_point<2, Low>& low, const basic_point<2, High>& high) noexcept -> basic_point&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.x < high.x);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.y < high.y);

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
			const basic_point<2, Low>& low,
			const basic_point<2, High>& high
		) noexcept -> basic_point
		{
			auto result{point};

			result.clamp(low, high);
			return result;
		}

		template<DirectionCategory Category, std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
		[[nodiscard]] constexpr auto between(const basic_point<2, T1>& left_top, const basic_point<2, T2>& right_bottom) const noexcept -> bool
		{
			if constexpr (Category == DirectionCategory::ALL) //
			{
				return
						between<DirectionCategory::X>(left_top, right_bottom) and
						between<DirectionCategory::Y>(left_top, right_bottom);
			}
			else if constexpr (Category == DirectionCategory::X)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(left_top.x) < static_cast<value_type>(right_bottom.x));

				return x >= static_cast<value_type>(left_top.x) and x < static_cast<value_type>(right_bottom.x);
			}
			else if constexpr (Category == DirectionCategory::Y)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(left_top.y) < static_cast<value_type>(right_bottom.y));

				return y >= static_cast<value_type>(left_top.y) and y < static_cast<value_type>(right_bottom.y);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point<3, T> final : multidimensional<basic_point<3, T>>
	{
		using value_type = T;

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
			const auto [_x, _y, _z] = value;
			x = static_cast<value_type>(_x);
			y = static_cast<value_type>(_y);
			z = static_cast<value_type>(_z);

			return *this;
		}

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

		[[nodiscard]] constexpr explicit(false) operator basic_point<2, value_type>() const noexcept { return {x, y}; }

		template<std::convertible_to<value_type> U = value_type>
		[[nodiscard]] constexpr auto distance(const basic_point<2, U>& other) const noexcept -> value_type //
		{
			return functional::hypot(
				x - static_cast<value_type>(other.x),
				y - static_cast<value_type>(other.y),
				z - static_cast<value_type>(other.z)
			);
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] constexpr auto clamp(const basic_point<3, Low>& low, const basic_point<3, High>& high) noexcept -> basic_point&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.x < high.x);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.y < high.y);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.z < high.z);

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
			const basic_point<3, Low>& low,
			const basic_point<3, High>& high
		) noexcept -> basic_point
		{
			auto result{point};

			result.clamp(low, high);
			return result;
		}

		template<DirectionCategory Category, std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
		[[nodiscard]] constexpr auto between(const basic_point<3, T1>& left_top, const basic_point<3, T2>& right_bottom) const noexcept -> bool
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
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(left_top.x) < static_cast<value_type>(right_bottom.x));

				return x >= static_cast<value_type>(left_top.x) and x < static_cast<value_type>(right_bottom.x);
			}
			else if constexpr (Category == DirectionCategory::Y)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(left_top.y) < static_cast<value_type>(right_bottom.y));

				return y >= static_cast<value_type>(left_top.y) and y < static_cast<value_type>(right_bottom.y);
			}
			else if constexpr (Category == DirectionCategory::Z)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(left_top.z) < static_cast<value_type>(right_bottom.z));

				return z >= static_cast<value_type>(left_top.z) and z < static_cast<value_type>(right_bottom.z);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};

	template<typename T>
	using basic_point_2d = basic_point<2, T>;

	template<typename T>
	using basic_point_3d = basic_point<3, T>;
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_STD
{
	template<std::size_t Index, std::size_t N, typename T>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_point<N, T>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<std::size_t N, typename T>
	struct tuple_size<gal::prometheus::primitive::basic_point<N, T>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, N> {};

	template<std::size_t N, typename T>
	struct formatter<gal::prometheus::primitive::basic_point<N, T>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_point<N, T>& point, FormatContext& context) const noexcept -> auto
		{
			if constexpr (N == 2) { return std::format_to(context.out(), "({},{})", point.x, point.y); }
			else if constexpr (N == 3) { return std::format_to(context.out(), "({},{},{})", point.x, point.y, point.z); }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};
} // namespace std
