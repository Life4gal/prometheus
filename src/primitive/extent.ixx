// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:primitive.extent;

import std;

import :meta;

import :primitive.multidimensional;
import :primitive.point;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>
#include <tuple>
#include <utility>
#include <format>

#include <prometheus/macro.hpp>
#include <meta/meta.ixx>
#include <primitive/multidimensional.ixx>
#include <primitive/point.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(primitive)
{
	template<std::size_t N, typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent;

	template<typename>
	struct is_basic_extent : std::false_type {};

	template<std::size_t N, typename T>
	struct is_basic_extent<basic_extent<N, T>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_extent_v = is_basic_extent<T>::value;

	template<typename T>
	concept basic_extent_t = is_basic_extent_v<T>;

	template<typename, typename>
	struct is_extent_compatible : std::false_type {};

	template<typename ExtentValueType, member_gettable_but_not_same_t<basic_extent<2, ExtentValueType>> OtherType>
		requires(meta::member_size<OtherType>() == 2)
	struct is_extent_compatible<OtherType, basic_extent<2, ExtentValueType>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, OtherType>, ExtentValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, OtherType>, ExtentValueType>
			> {};

	template<typename ExtentValueType, member_gettable_but_not_same_t<basic_extent<3, ExtentValueType>> OtherType>
		requires(meta::member_size<OtherType>() == 3)
	struct is_extent_compatible<OtherType, basic_extent<3, ExtentValueType>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, OtherType>, ExtentValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, OtherType>, ExtentValueType> and
				std::convertible_to<meta::member_type_of_index_t<2, OtherType>, ExtentValueType>
			> {};

	template<typename T, typename U>
	constexpr auto is_extent_compatible_v = is_extent_compatible<T, U>::value;

	template<typename T, typename U>
	concept extent_compatible_t = is_extent_compatible_v<T, U>;

	template<std::size_t N, typename L, typename R>
	[[nodiscard]] constexpr auto operator==(const basic_extent<N, L>& lhs, const basic_extent<N, R>& rhs) noexcept -> bool
	{
		if constexpr (N == 2)
		{
			return lhs.width == rhs.width and lhs.height == rhs.height;
		}
		else if constexpr (N == 3)
		{
			return lhs.width == rhs.width and lhs.height == rhs.height and lhs.depth == rhs.depth;
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<std::size_t N, typename T, extent_compatible_t<basic_extent<N, T>> R>
	[[nodiscard]] constexpr auto operator==(const basic_extent<N, T>& lhs, const R& rhs) noexcept -> bool
	{
		if constexpr (N == 2)
		{
			return lhs.width == meta::member_of_index<0>(rhs) and lhs.height == meta::member_of_index<1>(rhs);
		}
		else if constexpr (N == 3)
		{
			return
					lhs.width == meta::member_of_index<0>(rhs) and
					lhs.height == meta::member_of_index<1>(rhs) and
					lhs.depth == meta::member_of_index<2>(rhs);
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<std::size_t N, typename T, extent_compatible_t<basic_extent<N, T>> L>
	[[nodiscard]] constexpr auto operator==(const L& lhs, const basic_extent<N, T>& rhs) noexcept -> bool
	{
		return rhs == lhs;
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent<2, T> final : multidimensional<basic_extent<2, T>>
	{
		using value_type = T;

		value_type width;
		value_type height;

		constexpr explicit(false) basic_extent(const value_type value = value_type{0}) noexcept
			: width{value},
			  height{value} {}

		constexpr basic_extent(const value_type width, const value_type height) noexcept
			: width{width},
			  height{height} {}

		template<extent_compatible_t<basic_extent> U>
		constexpr explicit basic_extent(const U& value) noexcept
			: basic_extent{}
		{
			*this = value;
		}

		constexpr basic_extent(const basic_extent&) noexcept = default;
		constexpr basic_extent(basic_extent&&) noexcept = default;
		constexpr auto operator=(const basic_extent&) noexcept -> basic_extent& = default;
		constexpr auto operator=(basic_extent&&) noexcept -> basic_extent& = default;
		constexpr ~basic_extent() noexcept = default;

		template<extent_compatible_t<basic_extent> U>
		constexpr auto operator=(const U& value) noexcept -> basic_extent&
		{
			const auto [_width, _height] = value;
			width = static_cast<value_type>(_width);
			height = static_cast<value_type>(_height);

			return *this;
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> const value_type&
		{
			if constexpr (Index == 0) { return width; }
			else if constexpr (Index == 1) { return height; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> value_type&
		{
			if constexpr (Index == 0) { return width; }
			else if constexpr (Index == 1) { return height; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr explicit operator basic_extent<3, value_type>() const noexcept { return {width, height, 0}; }
	};

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent<3, T> final : multidimensional<basic_extent<3, T>>
	{
		using value_type = T;

		value_type width;
		value_type height;
		value_type depth;

		constexpr explicit(false) basic_extent(const value_type value = value_type{0}) noexcept
			: width{value},
			  height{value},
			  depth{value} {}

		constexpr basic_extent(const value_type width, const value_type height, const value_type depth) noexcept
			: width{width},
			  height{height},
			  depth{depth} {}

		template<extent_compatible_t<basic_extent> U>
		constexpr explicit basic_extent(const U& value) noexcept
			: basic_extent{}
		{
			*this = value;
		}

		constexpr basic_extent(const basic_extent&) noexcept = default;
		constexpr basic_extent(basic_extent&&) noexcept = default;
		constexpr auto operator=(const basic_extent&) noexcept -> basic_extent& = default;
		constexpr auto operator=(basic_extent&&) noexcept -> basic_extent& = default;
		constexpr ~basic_extent() noexcept = default;

		template<extent_compatible_t<basic_extent> U>
		constexpr auto operator=(const U& value) noexcept -> basic_extent&
		{
			const auto [_width, _height, _depth] = value;
			width = static_cast<value_type>(_width);
			height = static_cast<value_type>(_height);
			depth = static_cast<value_type>(_depth);

			return *this;
		}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() const noexcept -> value_type
		{
			if constexpr (Index == 0) { return width; }
			else if constexpr (Index == 1) { return height; }
			else if constexpr (Index == 2) { return depth; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() noexcept -> value_type&
		{
			if constexpr (Index == 0) { return width; }
			else if constexpr (Index == 1) { return height; }
			else if constexpr (Index == 2) { return depth; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr explicit(false) operator basic_extent<2, value_type>() const noexcept { return {width, height}; }
	};

	template<typename T>
	using basic_extent_2d = basic_extent<2, T>;

	template<typename T>
	using basic_extent_3d = basic_extent<3, T>;
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_STD
{
	template<std::size_t Index, std::size_t N, typename T>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_extent<N, T>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<std::size_t N, typename T>
	struct tuple_size<gal::prometheus::primitive::basic_extent<N, T>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, N> {};

	template<std::size_t N, typename T>
	struct formatter<gal::prometheus::primitive::basic_extent<N, T>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_extent<N, T>& extent, FormatContext& context) const noexcept -> auto
		{
			if constexpr (N == 2)
			{
				return std::format_to(
					context.out(),
					"({},{})",
					extent.width,
					extent.height
				);
			}
			else if constexpr (N == 3)
			{
				return std::format_to(
					context.out(),
					"({},{},{})",
					extent.width,
					extent.height,
					extent.depth
				);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};
}
