// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:primitive.rect;

import std;

import :meta;
#if GAL_PROMETHEUS_COMPILER_DEBUG
import :error;
#endif

import :primitive.multidimensional;
import :primitive.point;
import :primitive.extent;

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
#include <primitive/extent.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(primitive)
{
	template<std::size_t N, typename PointValueType, typename ExtentValueType = PointValueType>
		requires (std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<ExtentValueType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_rect;

	template<typename>
	struct is_basic_rect : std::false_type {};

	template<std::size_t N, typename PointValueType, typename ExtentValueType>
	struct is_basic_rect<basic_rect<N, PointValueType, ExtentValueType>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_rect_v = is_basic_rect<T>::value;

	template<typename T>
	concept basic_rect_t = is_basic_rect_v<T>;

	template<typename, typename>
	struct is_rect_compatible : std::false_type {};

	// point + extent
	template<typename PointValueType, typename ExtentValueType, member_gettable_but_not_same_t<basic_rect<2, PointValueType, ExtentValueType>> OtherType>
		requires (meta::member_size<OtherType>() == 2)
	struct is_rect_compatible<OtherType, basic_rect<2, PointValueType, ExtentValueType>> :
			std::bool_constant<
				point_compatible_t<meta::member_type_of_index_t<0, OtherType>, basic_point<2, PointValueType>> and
				extent_compatible_t<meta::member_type_of_index_t<1, OtherType>, basic_extent<2, ExtentValueType>>
			> {};

	// point + extent
	template<typename PointValueType, typename ExtentValueType, member_gettable_but_not_same_t<basic_rect<3, PointValueType, ExtentValueType>> OtherType>
		requires(meta::member_size<OtherType>() == 2)
	struct is_rect_compatible<OtherType, basic_rect<3, PointValueType, ExtentValueType>> :
			std::bool_constant<
				point_compatible_t<meta::member_type_of_index_t<0, OtherType>, basic_point<3, PointValueType>> and
				extent_compatible_t<meta::member_type_of_index_t<1, OtherType>, basic_extent<3, ExtentValueType>>
			> {};

	// left + top + right + bottom
	template<typename PointValueType, typename ExtentValueType, member_gettable_but_not_same_t<basic_rect<2, PointValueType, ExtentValueType>> OtherType>
		requires(meta::member_size<OtherType>() == 4)
	struct is_rect_compatible<OtherType, basic_rect<2, PointValueType, ExtentValueType>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, OtherType>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, OtherType>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<2, OtherType>, ExtentValueType> and
				std::convertible_to<meta::member_type_of_index_t<3, OtherType>, ExtentValueType>
			> {};

	// left + top + near + right + bottom + far
	template<typename PointValueType, typename ExtentValueType, member_gettable_but_not_same_t<basic_rect<3, PointValueType, ExtentValueType>> OtherType>
		requires(meta::member_size<OtherType>() == 6)
	struct is_rect_compatible<OtherType, basic_rect<3, PointValueType, ExtentValueType>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, OtherType>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, OtherType>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<2, OtherType>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<3, OtherType>, ExtentValueType> and
				std::convertible_to<meta::member_type_of_index_t<4, OtherType>, ExtentValueType> and
				std::convertible_to<meta::member_type_of_index_t<5, OtherType>, ExtentValueType>
			> {};

	template<typename T, typename U>
	constexpr auto is_rect_compatible_v = is_rect_compatible<T, U>::value;

	template<typename T, typename U>
	concept rect_compatible_t = is_rect_compatible_v<T, U>;

	template<std::size_t N, typename PointValueTypeL, typename ExtentValueTypeL, typename PointValueTypeR, typename ExtentValueTypeR>
	[[nodiscard]] constexpr auto operator==(
		const basic_rect<N, PointValueTypeL, ExtentValueTypeL>& lhs,
		const basic_rect<N, PointValueTypeR, ExtentValueTypeR>& rhs
	) noexcept -> bool
	{
		return lhs.point == rhs.point and lhs.extent == rhs.extent;
	}

	template<std::size_t N, typename PointValueType, typename ExtentValueType, rect_compatible_t<basic_rect<N, PointValueType, ExtentValueType>> R>
	[[nodiscard]] constexpr auto operator==(const basic_rect<N, PointValueType, ExtentValueType>& lhs, const R& rhs) noexcept -> bool
	{
		if constexpr (N == 2)
		{
			if constexpr (meta::member_size<R>() == 2)
			{
				// point.ixx => operator==
				// extent.ixx => operator==
				return lhs.point == meta::member_of_index<0>(rhs) and lhs.extent == meta::member_of_index<1>(rhs);
			}
			else if constexpr (meta::member_size<R>() == 4)
			{
				const auto x = meta::member_of_index<0>(rhs);
				const auto y = meta::member_of_index<1>(rhs);
				const auto width = meta::member_of_index<2>(rhs);
				const auto height = meta::member_of_index<3>(rhs);

				return x == lhs.point.x and y == lhs.point.y and width == lhs.extent.width and height = lhs.extent.height;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
		else if constexpr (N == 3)
		{
			if constexpr (meta::member_size<R>() == 2)
			{
				// point.ixx => operator==
				// extent.ixx => operator==
				return lhs.point == meta::member_of_index<0>(rhs) and lhs.extent == meta::member_of_index<1>(rhs);
			}
			else if constexpr (meta::member_size<R>() == 6)
			{
				const auto x = meta::member_of_index<0>(rhs);
				const auto y = meta::member_of_index<1>(rhs);
				const auto z = meta::member_of_index<2>(rhs);
				const auto width = meta::member_of_index<4>(rhs);
				const auto height = meta::member_of_index<5>(rhs);
				const auto depth = meta::member_of_index<6>(rhs);

				return
						x == lhs.point.x and y == lhs.point.y and z == lhs.point.z and
						width == lhs.extent.width and height = lhs.extent.height and depth == lhs.extent.depth;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<std::size_t N, typename PointValueType, typename ExtentValueType, rect_compatible_t<basic_rect<N, PointValueType, ExtentValueType>> L>
	[[nodiscard]] constexpr auto operator==(const L& lhs, const basic_rect<N, PointValueType, ExtentValueType>& rhs) noexcept -> bool
	{
		return rhs == lhs;
	}

	template<typename PointValueType, typename ExtentValueType>
		requires(std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<ExtentValueType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_rect<2, PointValueType, ExtentValueType> final
			: multidimensional<basic_rect<2, PointValueType, ExtentValueType>>
	{
		using point_value_type = PointValueType;
		using extent_value_type = ExtentValueType;

		using point_type = basic_point<2, point_value_type>;
		using extent_type = basic_extent<2, extent_value_type>;

		constexpr static std::size_t element_size{2};
		template<std::size_t Index>
			requires(Index < element_size)
		using element_type = std::conditional_t<Index == 0, point_type, extent_type>;

		point_type point;
		extent_type extent;

		constexpr explicit(false) basic_rect(const point_value_type point_value = point_value_type{0}, const extent_value_type extent_value = extent_value_type{0}) noexcept
			: point{point_value},
			  extent{extent_value} {}

		constexpr basic_rect(const point_value_type left, const point_value_type top, const point_value_type right, const point_value_type bottom) noexcept
			: point{left, top},
			  extent{static_cast<extent_value_type>(right - left), static_cast<extent_value_type>(bottom - top)} {}

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
						static_cast<point_value_type>(_left),
						static_cast<point_value_type>(_top),
						static_cast<point_value_type>(_right),
						static_cast<point_value_type>(_bottom)
				};
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }

			return *this;
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> std::add_lvalue_reference_t<std::add_const_t<element_type<Index>>>
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> std::add_lvalue_reference_t<element_type<Index>>
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

		[[nodiscard]] constexpr auto width() const noexcept -> extent_value_type { return extent.width; }

		[[nodiscard]] constexpr auto height() const noexcept -> extent_value_type { return extent.height; }

		[[nodiscard]] constexpr auto size() const noexcept -> extent_type { return {width(), height()}; }

		[[nodiscard]] constexpr auto includes(const point_type& p) const noexcept -> bool
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not empty() and valid());

			return p.template between<DirectionCategory::ALL>(left_top(), right_bottom());
		}

		[[nodiscard]] constexpr auto includes(const basic_rect& rect) const noexcept -> bool
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not rect.empty() and rect.valid());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size().exact_greater_than(rect.size()));

			return
					rect.point.x >= point.x and
					rect.point.x + rect.width() < point.x + width() and
					rect.point.y >= point.y and
					rect.point.y + rect.height() < point.y + height();
		}

		[[nodiscard]] constexpr auto intersects(const basic_rect& rect) const noexcept -> bool
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not rect.empty() and rect.valid());

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

	template<typename PointValueType, typename ExtentValueType>
		requires(std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<ExtentValueType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_rect<3, PointValueType, ExtentValueType> final
			: multidimensional<basic_rect<3, PointValueType, ExtentValueType>>
	{
		using point_value_type = PointValueType;
		using extent_value_type = ExtentValueType;

		using point_type = basic_point<3, point_value_type>;
		using extent_type = basic_extent<3, extent_value_type>;

		constexpr static std::size_t element_size{2};
		template<std::size_t Index>
			requires(Index < element_size)
		using element_type = std::conditional_t<Index == 0, point_type, extent_type>;

		point_type point;
		extent_type extent;

		constexpr explicit(false) basic_rect(const point_value_type point_value = point_value_type{0}, const extent_value_type extent_value = extent_value_type{0}) noexcept
			: point{point_value},
			  extent{extent_value} {}

		// fixme: ICE here
		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		#pragma push_macro("near")
		#pragma push_macro("far")
		#undef near
		#undef far
		#endif

		constexpr basic_rect(
			const point_value_type left,
			const point_value_type top,
			const point_value_type near,
			const point_value_type right,
			const point_value_type bottom,
			const point_value_type far
		) noexcept
			: point{left, top, near},
			  extent{static_cast<extent_value_type>(right - left), static_cast<extent_value_type>(bottom - top), static_cast<extent_value_type>(far - near)} {}

		constexpr basic_rect(const point_type& left_top_near, const point_type& right_bottom_far) noexcept
			: basic_rect{left_top_near.x, left_top_near.y, left_top_near.z, right_bottom_far.x, right_bottom_far.y, right_bottom_far.z} {}

		constexpr basic_rect(const point_type& left_top_near, const extent_type& extent) noexcept
			: point{left_top_near},
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
			else if constexpr (meta::member_size<U>() == 6)
			{
				const auto [_left, _top, _near, _right, _bottom, _far] = value;
				*this = basic_rect{
						static_cast<point_value_type>(_left),
						static_cast<point_value_type>(_top),
						static_cast<point_value_type>(_near),
						static_cast<point_value_type>(_right),
						static_cast<point_value_type>(_bottom),
						static_cast<point_value_type>(_far)
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
		[[nodiscard]] constexpr auto get() const noexcept -> std::add_lvalue_reference_t<std::add_const_t<element_type<Index>>>
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> std::add_lvalue_reference_t<element_type<Index>>
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

		[[nodiscard]] constexpr auto width() const noexcept -> extent_value_type { return extent.width; }

		[[nodiscard]] constexpr auto height() const noexcept -> extent_value_type { return extent.height; }

		[[nodiscard]] constexpr auto depth() const noexcept -> extent_value_type { return extent.depth; }

		[[nodiscard]] constexpr auto size() const noexcept -> extent_type { return {width(), height()}; }

		[[nodiscard]] constexpr auto includes(const point_type& p) const noexcept -> bool
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not empty() and valid());

			return p.template between<DirectionCategory::ALL>(left_top_near(), right_bottom_near());
		}

		[[nodiscard]] constexpr auto includes(const basic_rect& rect) const noexcept -> bool
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not rect.empty() and rect.valid());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size().exact_greater_than(rect.size()));

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
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not empty() and valid());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not rect.empty() and rect.valid());

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

	template<typename PointValueType, typename ExtentValueType = PointValueType>
	using basic_rect_2d = basic_rect<2, PointValueType, ExtentValueType>;

	template<typename PointValueType, typename ExtentValueType = PointValueType>
	using basic_rect_3d = basic_rect<3, PointValueType, ExtentValueType>;
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_STD
{
	template<std::size_t Index, std::size_t N, typename PointValueType, typename ExtentValueType>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_rect<N, PointValueType, ExtentValueType>> // NOLINT(cert-dcl58-cpp)
	{
		using type = typename gal::prometheus::primitive::basic_rect<N, PointValueType, ExtentValueType>::template element_type<Index>;
	};

	template<std::size_t N, typename PointValueType, typename ExtentValueType>
	struct tuple_size<gal::prometheus::primitive::basic_rect<N, PointValueType, ExtentValueType>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, gal::prometheus::primitive::basic_rect<N, PointValueType, ExtentValueType>::element_size> {};

	template<std::size_t N, typename PointValueType, typename ExtentValueType>
	struct formatter<gal::prometheus::primitive::basic_rect<N, PointValueType, ExtentValueType>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_rect<N, PointValueType, ExtentValueType>& rect, FormatContext& context) const noexcept -> auto
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
