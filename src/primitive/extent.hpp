// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <algorithm>
#include <format>

#include <prometheus/macro.hpp>

#include <meta/dimension.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::primitive
{
	template<std::size_t, typename>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent;

	template<typename T>
		requires(std::is_arithmetic_v<T>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent<2, T> final : meta::dimension<basic_extent<2, T>>
	{
		using value_type = T;

		value_type width;
		value_type height;

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

		[[nodiscard]] constexpr explicit operator basic_extent<3, value_type>() const noexcept
		{
			return {.width = width, .height = height, .depth = value_type{0}};
		}
	};

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent<3, T> final : meta::dimension<basic_extent<3, T>>
	{
		using value_type = T;

		value_type width;
		value_type height;
		value_type depth;

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

		[[nodiscard]] constexpr explicit operator basic_extent<2, value_type>() const noexcept
		{
			return {.width = width, .height = height};
		}
	};

	template<typename T>
	using basic_extent_2d = basic_extent<2, T>;

	template<typename T>
	using basic_extent_3d = basic_extent<3, T>;
}

namespace std
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
				return std::format_to(context.out(), "[{},{}]", extent.width, extent.height);
			}
			else if constexpr (N == 3)
			{
				return std::format_to(context.out(), "[{},{},{}]", extent.width, extent.height, extent.depth);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
	};
} // namespace std
