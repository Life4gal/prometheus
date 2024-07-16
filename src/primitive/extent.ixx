// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:extent;

import std;
import gal.prometheus.meta;

import :multidimensional;
import :point;

#else
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

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::primitive)
{
	template<typename T, std::size_t N>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent;

	template<typename>
	struct is_basic_extent : std::false_type {};

	template<typename T, std::size_t N>
	struct is_basic_extent<basic_extent<T, N>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_extent_v = is_basic_extent<T>::value;

	template<typename T>
	concept basic_extent_t = is_basic_extent_v<T>;

	template<typename, typename>
	struct is_extent_compatible : std::false_type {};

	template<typename ExtentValueType, member_gettable_but_not_same_t<basic_extent<ExtentValueType, 2>> OtherType>
		requires(meta::member_size<OtherType>() == 2)
	struct is_extent_compatible<OtherType, basic_extent<ExtentValueType, 2>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, OtherType>, ExtentValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, OtherType>, ExtentValueType>
			> {};

	template<typename ExtentValueType, member_gettable_but_not_same_t<basic_extent<ExtentValueType, 3>> OtherType>
		requires(meta::member_size<OtherType>() == 3)
	struct is_extent_compatible<OtherType, basic_extent<ExtentValueType, 3>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, OtherType>, ExtentValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, OtherType>, ExtentValueType> and
				std::convertible_to<meta::member_type_of_index_t<2, OtherType>, ExtentValueType>
			> {};

	template<typename T, typename U>
	constexpr auto is_extent_compatible_v = is_extent_compatible<T, U>::value;

	template<typename T, typename U>
	concept extent_compatible_t = is_extent_compatible_v<T, U>;

	template<typename L, typename R, std::size_t N>
	[[nodiscard]] constexpr auto operator==(const basic_extent<L, N>& lhs, const basic_extent<R, N>& rhs) noexcept -> bool
	{
		return lhs.width == rhs.width and lhs.height == rhs.height;
	}

	template<typename T, std::size_t N, extent_compatible_t<basic_extent<T, N>> R>
	[[nodiscard]] constexpr auto operator==(const basic_extent<T, N>& lhs, const R& rhs) noexcept -> bool
	{
		return lhs.width == meta::member_of_index<0>(rhs) and lhs.height == meta::member_of_index<1>(rhs);
	}

	template<typename T, std::size_t N, extent_compatible_t<basic_extent<T, N>> L>
	[[nodiscard]] constexpr auto operator==(const L& lhs, const basic_extent<T, N>& rhs) noexcept -> bool
	{
		return rhs.width == meta::member_of_index<0>(lhs) and rhs.height == meta::member_of_index<1>(lhs);
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent<T, 2> final : multidimensional<T, basic_extent<T, 2>>
	{
		using value_type = T;

		constexpr static auto is_always_equal = true;

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
	};

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent<T, 3> final : multidimensional<T, basic_extent<T, 3>>
	{
		using value_type = T;

		constexpr static auto is_always_equal = true;

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

		[[nodiscard]] constexpr explicit(false) operator basic_extent<value_type, 2>() const noexcept { return {width, height}; }
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
{
	template<std::size_t Index, typename T, std::size_t N>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_extent<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<typename T, std::size_t N>
	struct tuple_size<gal::prometheus::primitive::basic_extent<T, N>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, N> {};

	template<typename T, std::size_t N>
	struct formatter<gal::prometheus::primitive::basic_extent<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_extent<T, N>& extent, FormatContext& context) const noexcept -> auto
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
