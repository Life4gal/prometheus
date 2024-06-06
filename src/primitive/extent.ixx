// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:extent;

import std;
import :multidimensional;

export namespace gal::prometheus::primitive
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

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> value_type
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

export namespace std
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
