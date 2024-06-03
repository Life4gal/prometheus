// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:extent;

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
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_extent final : multidimensional<T, basic_extent<T>>
	{
		using value_type = T;

		constexpr static auto is_always_equal = true;

		value_type width;
		value_type height;

		constexpr explicit(false) basic_extent(const value_type value = value_type{0}) noexcept
			: width{value},
			  height{value} {}

		constexpr basic_extent(const value_type value_0, const value_type value_1) noexcept
			: width{value_0},
			  height{value_1} {}

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
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_BEGIN
	template<std::size_t Index, typename T>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_extent<T>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<typename T>
	struct tuple_size<gal::prometheus::primitive::basic_extent<T>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 2> {};

	template<typename T>
	struct formatter<gal::prometheus::primitive::basic_extent<T>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_extent<T>& extent, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
					context.out(),
					"({},{})",
					extent.width,
					extent.height
					);
		}
	};

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_END
