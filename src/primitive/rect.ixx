// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:rect;

import std;
import gal.prometheus.error;
import gal.prometheus.meta;

import :multidimensional;
import :point;
import :extent;

#else
#pragma once

#include <type_traits>
#include <tuple>
#include <utility>
#include <format>

#include <prometheus/macro.hpp>
#include <error/error.ixx>
#include <meta/meta.ixx>
#include <primitive/multidimensional.ixx>
#include <primitive/point.ixx>
#include <primitive/extent.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::primitive)
{
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

	template<typename, typename>
	struct is_rect_compatible : std::false_type {};

	// point + extent
	template<typename ValueType, member_gettable_but_not_same_t<basic_rect<ValueType, 2>> OtherType>
		requires (meta::member_size<OtherType>() == 2)
	struct is_rect_compatible<OtherType, basic_rect<ValueType, 2>> :
			std::bool_constant<
				point_compatible_t<meta::member_type_of_index_t<0, OtherType>, basic_point<ValueType, 2>> and
				extent_compatible_t<meta::member_type_of_index_t<1, OtherType>, basic_extent<ValueType, 2>>
			> {};

	// point + extent
	template<typename ValueType, member_gettable_but_not_same_t<basic_rect<ValueType, 3>> OtherType>
		requires(meta::member_size<OtherType>() == 2)
	struct is_rect_compatible<OtherType, basic_rect<ValueType, 3>> :
			std::bool_constant<
				point_compatible_t<meta::member_type_of_index_t<0, OtherType>, basic_point<ValueType, 3>> and
				extent_compatible_t<meta::member_type_of_index_t<1, OtherType>, basic_extent<ValueType, 3>>
			> {};

	// left + top + right + bottom
	template<typename ValueType, member_gettable_but_not_same_t<basic_rect<ValueType, 2>> OtherType>
		requires(meta::member_size<OtherType>() == 4)
	struct is_rect_compatible<OtherType, basic_rect<ValueType, 2>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, OtherType>, ValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, OtherType>, ValueType> and
				std::convertible_to<meta::member_type_of_index_t<2, OtherType>, ValueType> and
				std::convertible_to<meta::member_type_of_index_t<3, OtherType>, ValueType>
			> {};

	template<typename T, typename U>
	constexpr auto is_rect_compatible_v = is_rect_compatible<T, U>::value;

	template<typename T, typename U>
	concept rect_compatible_t = is_rect_compatible_v<T, U>;

	template<typename L, typename R, std::size_t N>
	[[nodiscard]] constexpr auto operator==(const basic_rect<L, N>& lhs, const basic_rect<R, N>& rhs) noexcept -> bool
	{
		return lhs.point == rhs.point and lhs.extent == rhs.extent;
	}

	template<typename T, std::size_t N, rect_compatible_t<basic_rect<T, N>> R>
	[[nodiscard]] constexpr auto operator==(const basic_rect<T, N>& lhs, const R& rhs) noexcept -> bool
	{
		return lhs.point == meta::member_of_index<0>(rhs) and lhs.extent == meta::member_of_index<1>(rhs);
	}

	template<typename T, std::size_t N, rect_compatible_t<basic_rect<T, N>> L>
	[[nodiscard]] constexpr auto operator==(const L& lhs, const basic_rect<T, N>& rhs) noexcept -> bool
	{
		return rhs.point == meta::member_of_index<0>(lhs) and rhs.extent == meta::member_of_index<1>(lhs);
	}

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

		template<rect_compatible_t<basic_rect> U>
		constexpr explicit basic_rect(const U& value) noexcept
			: basic_rect{}
		{
			*this = value;
		}

		constexpr basic_rect(const basic_rect&) noexcept = default;
		constexpr basic_rect(basic_rect&&) noexcept = default;
		constexpr auto operator=(const basic_rect&) noexcept -> basic_rect& = default;
		constexpr auto operator=(basic_rect&&) noexcept -> basic_rect& = default;
		constexpr ~basic_rect() noexcept = default;

		template<rect_compatible_t<basic_rect> U>
		constexpr auto operator=(const U& value) noexcept -> basic_rect&
		{
			if constexpr (meta::member_size<U>() == 2)
			{
				const auto [_point, _extent] = value;
				point = static_cast<point_type>(_point);
				extent = static_cast<extent_type>(_extent);
			}
			else if constexpr (meta::member_size<U>() == 4)
			{
				const auto [_left, _top, _right, _bottom] = value;
				*this = basic_rect{
						static_cast<value_type>(_left),
						static_cast<value_type>(_top),
						static_cast<value_type>(_right),
						static_cast<value_type>(_bottom)
				};
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }

			return *this;
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> std::conditional_t<Index == 0, const point_type&, const extent_type&>
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> std::conditional_t<Index == 0, point_type&, extent_type&>
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
			GAL_PROMETHEUS_DEBUG_ASSUME(valid());
			return extent.width;
		}

		[[nodiscard]] constexpr auto height() const noexcept -> value_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(valid());
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

		constexpr basic_rect(const point_type& left_top_near, const point_type& right_bottom_far) noexcept
			: basic_rect{left_top_near.x, left_top_near.y, left_top_near.z, right_bottom_far.x, right_bottom_far.y, right_bottom_far.z} {}

		constexpr basic_rect(const point_type& left_top_near, const extent_type& extent) noexcept
			: point{left_top_near},
			  extent{extent} {}

		constexpr basic_rect(const basic_rect&) noexcept = default;
		constexpr basic_rect(basic_rect&&) noexcept = default;
		constexpr auto operator=(const basic_rect&) noexcept -> basic_rect& = default;
		constexpr auto operator=(basic_rect&&) noexcept -> basic_rect& = default;
		constexpr ~basic_rect() noexcept = default;

		template<rect_compatible_t<basic_rect> U>
		constexpr auto operator=(const U& value) noexcept -> basic_rect&
		{
			if constexpr (meta::member_size<U>() == 2)
			{
				const auto [_point, _extent] = value;
				point = static_cast<point_type>(_point);
				extent = static_cast<extent_type>(_extent);
			}
			else if constexpr (meta::member_size<U>() == 6)
			{
				const auto [_left, _top, _near, _right, _bottom, _far] = value;
				*this = basic_rect{
						static_cast<value_type>(_left),
						static_cast<value_type>(_top),
						static_cast<value_type>(_near),
						static_cast<value_type>(_right),
						static_cast<value_type>(_bottom),
						static_cast<value_type>(_far)
				};
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }

			return *this;
		}

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		#pragma pop_macro("far")
		#pragma pop_macro("near")
		#endif

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
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
{
	template<std::size_t Index, typename T, std::size_t N>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_rect<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		using type = std::conditional_t<
			N == 0,
			typename gal::prometheus::primitive::basic_rect<T, N>::point_type,
			typename gal::prometheus::primitive::basic_rect<T, N>::extent_type
		>;
	};

	template<typename T, std::size_t N>
	struct tuple_size<gal::prometheus::primitive::basic_rect<T, N>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 2> {};

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
}
