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
import :multidimensional;
import :point;
import :extent;

#else

#include <type_traits>
#include <format>

#include <prometheus/macro.hpp>
#include <primitive/multidimensional.hpp>
#include <primitive/point.hpp>
#include <primitive/extent.hpp>
#endif

namespace gal::prometheus::primitive
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	template<typename T, std::size_t N>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_rect;

	template<typename>
	struct is_basic_rect : std::false_type {};

	template<typename T, std::size_t N>
	struct is_basic_rect<basic_rect<T, N>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_rect_v = is_basic_rect<T>::value;

	template<typename T>
	concept basic_rect_t = is_basic_rect_v<T>;

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_rect<T, 2> final : multidimensional<T, basic_rect<T, 2>>
	{
		using value_type = T;

		using point_type = basic_point<value_type, 2>;
		using extent_type = basic_extent<value_type, 2>;

		constexpr static auto is_always_equal = true;

		point_type point;
		extent_type extent;

		constexpr explicit(false) basic_rect(const value_type value = value_type{0}) noexcept
			: point{value},
			  extent{value} {}

		constexpr basic_rect(const value_type left, const value_type top, const value_type right, const value_type bottom) noexcept
			: point{left, top},
			  extent{right - left, bottom - top} {}

		constexpr basic_rect(const point_type& left_top, const point_type& right_bottom) noexcept
			: basic_rect{left_top.x, left_top.y, right_bottom.x, right_bottom.y} {}

		constexpr basic_rect(const point_type& left_top, const extent_type& extent) noexcept
			: point{left_top},
			  extent{extent} {}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> value_type
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> value_type&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr auto left_top() const noexcept -> point_type { return point; }

		[[nodiscard]] constexpr auto left_bottom() const noexcept -> point_type { return {point.x, point.y + extent.height}; }

		[[nodiscard]] constexpr auto right_top() const noexcept -> point_type { return {point.x + extent.width, point.y}; }

		[[nodiscard]] constexpr auto right_bottom() const noexcept -> point_type { return {point.x + extent.width, point.y + extent.height}; }

		[[nodiscard]] constexpr auto center() const noexcept -> point_type { return {point.x + width() / 2, point.y + height() / 2}; }

		[[nodiscard]] constexpr auto empty() const noexcept -> bool { return extent.width == 0 or extent.height == 0; }

		[[nodiscard]] constexpr auto valid() const noexcept -> bool { return extent.width >= 0 and extent.height >= 0; }

		[[nodiscard]] constexpr auto width() const noexcept -> value_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			return extent.width;
		}

		[[nodiscard]] constexpr auto height() const noexcept -> value_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			return extent.height;
		}

		[[nodiscard]] constexpr auto size() const noexcept -> extent_type { return {width(), height()}; }

		[[nodiscard]] constexpr auto includes(const point_type& p) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());

			return p.between(left_top(), right_bottom());
		}

		[[nodiscard]] constexpr auto includes(const basic_rect& rect) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());
			GAL_PROMETHEUS_DEBUG_ASSUME(size().exact_greater_than(rect.size()));

			return
					rect.point.x >= point.x and
					rect.point.x + rect.width() < point.x + width() and
					rect.point.y >= point.y and
					rect.point.y + rect.height() < point.y + height();
		}

		[[nodiscard]] constexpr auto intersects(const basic_rect& rect) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());

			return not(
				rect.point.x >= point.x + width() or
				rect.point.x + rect.width() <= point.x or
				rect.point.y >= point.y + height() or
				rect.point.y + rect.height() <= point.y
			);
		}

		[[nodiscard]] constexpr auto combine_max(const basic_rect& rect) const noexcept -> basic_rect
		{
			return
			{
					std::ranges::min(point.x, rect.point.x),
					std::ranges::min(point.y, rect.point.y),
					std::ranges::max(point.x + width(), rect.point.x + rect.width()),
					std::ranges::max(point.y + height(), rect.point.y + rect.height())
			};
		}

		[[nodiscard]] constexpr auto combine_min(const basic_rect& rect) const noexcept -> basic_rect
		{
			return
			{
					std::ranges::max(point.x, rect.point.x),
					std::ranges::max(point.y, rect.point.y),
					std::ranges::min(point.x + width(), rect.point.x + rect.width()),
					std::ranges::min(point.y + height(), rect.point.y + rect.height())
			};
		}
	};

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_rect<T, 3> final : multidimensional<T, basic_rect<T, 3>>
	{
		using value_type = T;

		using point_type = basic_point<value_type, 3>;
		using extent_type = basic_extent<value_type, 3>;

		constexpr static auto is_always_equal = true;

		point_type point;
		extent_type extent;

		constexpr explicit(false) basic_rect(const value_type value = value_type{0}) noexcept
			: point{value},
			  extent{value} {}

		// fixme: ICE here
		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		#pragma push_macro("near")
		#pragma push_macro("far")
		#undef near
		#undef far
		#endif

		constexpr basic_rect(
				const value_type left,
				const value_type top,
				const value_type near,
				const value_type right,
				const value_type bottom,
				const value_type far
				) noexcept
			: point{left, top, near},
			  extent{right - left, bottom - top, far - near} {}

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		#pragma pop_macro("far")
		#pragma pop_macro("near")
		#endif

		constexpr basic_rect(const point_type& left_top_near, const point_type& right_bottom_far) noexcept
			: basic_rect{left_top_near.x, left_top_near.y, left_top_near.z, right_bottom_far.x, right_bottom_far.y, right_bottom_far.z} {}

		constexpr basic_rect(const point_type& left_top, const extent_type& extent) noexcept
			: point{left_top},
			  extent{extent} {}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> value_type
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> value_type&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr auto left_top_near() const noexcept -> point_type //
		{
			return point;
		}

		[[nodiscard]] constexpr auto left_bottom_near() const noexcept -> point_type //
		{
			return {point.x, point.y + extent.height, point.z};
		}

		[[nodiscard]] constexpr auto left_top_far() const noexcept -> point_type //
		{
			return {point.x, point.y, point.z + extent.depth};
		}

		[[nodiscard]] constexpr auto left_bottom_far() const noexcept -> point_type //
		{
			return {point.x, point.y + extent.height, point.z + extent.depth};
		}

		[[nodiscard]] constexpr auto right_top_near() const noexcept -> point_type //
		{
			return {point.x + extent.width, point.y, point.z};
		}

		[[nodiscard]] constexpr auto right_bottom_near() const noexcept -> point_type //
		{
			return {point.x + extent.width, point.y + extent.height, point.z};
		}

		[[nodiscard]] constexpr auto right_top_far() const noexcept -> point_type //
		{
			return {point.x + extent.width, point.y, point.z + extent.depth};
		}

		[[nodiscard]] constexpr auto right_bottom_far() const noexcept -> point_type //
		{
			return {point.x + extent.width, point.y + extent.height, point.z + extent.depth};
		}

		[[nodiscard]] constexpr auto center() const noexcept -> point_type
		{
			return {point.x + width() / 2, point.y + height() / 2, point.z + depth()};
		}

		[[nodiscard]] constexpr auto empty() const noexcept -> bool { return extent.width == 0 or extent.height == 0 or extent.depth == 0; }

		[[nodiscard]] constexpr auto valid() const noexcept -> bool { return extent.width >= 0 and extent.height >= 0 and extent.depth >= 0; }

		[[nodiscard]] constexpr auto width() const noexcept -> value_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			return extent.width;
		}

		[[nodiscard]] constexpr auto height() const noexcept -> value_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			return extent.height;
		}

		[[nodiscard]] constexpr auto depth() const noexcept -> value_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			return extent.depth;
		}

		[[nodiscard]] constexpr auto size() const noexcept -> extent_type { return {width(), height()}; }

		[[nodiscard]] constexpr auto includes(const point_type& p) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());

			return p.between(left_top_near(), right_bottom_near()) and p.z < point.z + depth();
		}

		[[nodiscard]] constexpr auto includes(const basic_rect& rect) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());
			GAL_PROMETHEUS_DEBUG_ASSUME(size().exact_greater_than(rect.size()));

			return
					rect.point.x >= point.x and
					rect.point.x + rect.width() < point.x + width() and
					rect.point.y >= point.y and
					rect.point.y + rect.height() < point.y + height() and
					rect.point.z >= point.z and
					rect.point.z + rect.depth() < point.z + depth();
		}

		[[nodiscard]] constexpr auto intersects(const basic_rect& rect) const noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());

			return not(
				rect.point.x >= point.x + width() or
				rect.point.x + rect.width() <= point.x or
				rect.point.y >= point.y + height() or
				rect.point.y + rect.height() <= point.y or
				rect.point.z >= point.z + depth() or
				rect.point.z + rect.depth() <= point.z
			);
		}

		[[nodiscard]] constexpr auto combine_max(const basic_rect& rect) const noexcept -> basic_rect
		{
			return
			{
					std::ranges::min(point.x, rect.point.x),
					std::ranges::min(point.y, rect.point.y),
					std::ranges::min(point.z, rect.point.z),
					std::ranges::max(point.x + width(), rect.point.x + rect.width()),
					std::ranges::max(point.y + height(), rect.point.y + rect.height()),
					std::ranges::max(point.z + depth(), rect.point.z + rect.depth())
			};
		}

		[[nodiscard]] constexpr auto combine_min(const basic_rect& rect) const noexcept -> basic_rect
		{
			return
			{
					std::ranges::max(point.x, rect.point.x),
					std::ranges::max(point.y, rect.point.y),
					std::ranges::max(point.z, rect.point.z),
					std::ranges::min(point.x + width(), rect.point.x + rect.width()),
					std::ranges::min(point.y + height(), rect.point.y + rect.height()),
					std::ranges::min(point.z + depth(), rect.point.z + rect.depth())
			};
		}
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_BEGIN
	template<std::size_t Index, typename T, std::size_t N>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_rect<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<typename T, std::size_t N>
	struct tuple_size<gal::prometheus::primitive::basic_rect<T, N>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, N> {};

	template<typename T, std::size_t N>
	struct formatter<gal::prometheus::primitive::basic_rect<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_rect<T, N>& rect, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
					context.out(),
					"[{}-{}]",
					rect.point,
					rect.extent
					);
		}
	};

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_END
