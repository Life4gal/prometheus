// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:rect;

import std;
import :multi_dimension;
import :point;
import :extent;

#else

#include <type_traits>
#include <format>

#include <prometheus/macro.hpp>
#include <primitive/multi_dimension.hpp>
#include <primitive/point.hpp>
#include <primitive/extent.hpp>
#endif

namespace gal::prometheus::primitive
{
	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_rect final : multi_dimension<T, basic_rect<T>>
	{
		using value_type = T;
		using point_type = basic_point<value_type>;
		using extent_type = basic_extent<value_type>;

		constexpr static auto is_always_equal = true;

		value_type left;
		value_type top;
		value_type right;
		value_type bottom;

		constexpr explicit(false) basic_rect(const value_type value = value_type{0}) noexcept
			: left{value},
			  top{value},
			  right{value},
			  bottom{value} {}

		constexpr basic_rect(const value_type value_0, const value_type value_1, const value_type value_2, const value_type value_3) noexcept
			: left{value_0},
			  top{value_1},
			  right{value_2},
			  bottom{value_3} {}

		constexpr basic_rect(const point_type& left_top, const point_type& right_bottom) noexcept
			: left{left_top.x},
			  top{left_top.y},
			  right{right_bottom.x},
			  bottom{right_bottom.y} {}

		constexpr basic_rect(const point_type& left_top, const extent_type& size) noexcept
			: left{left_top.x},
			  top{left_top.y},
			  right{left + size.width},
			  bottom{top + size.height} {}

		template<std::size_t Index>
			requires(Index < 4)
		[[nodiscard]] constexpr auto get() const noexcept -> value_type
		{
			if constexpr (Index == 0) { return left; }
			else if constexpr (Index == 1) { return top; }
			else if constexpr (Index == 2) { return right; }
			else if constexpr (Index == 3) { return bottom; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 4)
		[[nodiscard]] constexpr auto get() noexcept -> value_type&
		{
			if constexpr (Index == 0) { return left; }
			else if constexpr (Index == 1) { return top; }
			else if constexpr (Index == 2) { return right; }
			else if constexpr (Index == 3) { return bottom; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr auto left_top() const noexcept -> point_type { return {left, top}; }

		[[nodiscard]] constexpr auto left_bottom() const noexcept -> point_type { return {left, bottom}; }

		[[nodiscard]] constexpr auto right_top() const noexcept -> point_type { return {right, top}; }

		[[nodiscard]] constexpr auto right_bottom() const noexcept -> point_type { return {right, bottom}; }

		[[nodiscard]] constexpr auto empty() const noexcept -> bool { return left == right or top == bottom; }

		[[nodiscard]] constexpr auto valid() const noexcept -> bool { return left <= right and top <= bottom; }

		[[nodiscard]] constexpr auto size() const noexcept -> extent_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());

			return {right - left, bottom - top};
		}

		[[nodiscard]] constexpr auto includes(const point_type& point) const noexcept -> bool { return point.between(left_top(), right_bottom()); }

		[[nodiscard]] constexpr auto includes(const basic_rect& rect) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());

			return (rect.left >= left) and (rect.left < right) and
			       (rect.top >= top) and (rect.top < bottom) and
			       (rect.right >= left) and (rect.right < right) and
			       (rect.bottom >= top) and (rect.bottom < bottom);
		}

		[[nodiscard]] constexpr auto intersects(const basic_rect& rect) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());

			return not(rect.left >= right or rect.right <= left or rect.top >= bottom or rect.bottom <= top);
		}

		[[nodiscard]] constexpr auto combine_max(const basic_rect& rect) const noexcept -> basic_rect
		{
			return {
					std::ranges::max(left, rect.left),
					std::ranges::max(top, rect.top),
					std::ranges::max(right, rect.right),
					std::ranges::max(bottom, rect.bottom)
			};
		}

		[[nodiscard]] constexpr auto combine_min(const basic_rect& rect) const noexcept -> basic_rect
		{
			return {
					std::ranges::min(left, rect.left),
					std::ranges::min(top, rect.top),
					std::ranges::min(right, rect.right),
					std::ranges::min(bottom, rect.bottom)
			};
		}
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_BEGIN
	template<std::size_t Index, typename T>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_rect<T>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<typename T>
	struct tuple_size<gal::prometheus::primitive::basic_rect<T>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 4> {};

	template<typename T>
	struct formatter<gal::prometheus::primitive::basic_rect<T>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_rect<T>& rect, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
					context.out(),
					"{}-{}",
					rect.left_top(),
					rect.right_bottom()
					);
		}
	};

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_END
