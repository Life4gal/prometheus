// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <tuple>
#include <utility>
#include <format>

#include <prometheus/macro.hpp>

#include <primitive/point.hpp>
#include <primitive/extent.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::primitive
{
	template<std::size_t N, typename PointValueType, typename ExtentValueType>

	struct basic_rect;

	template<typename PointValueType, typename ExtentValueType>
		requires (std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<ExtentValueType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_rect<2, PointValueType, ExtentValueType> final : meta::dimension<basic_rect<2, PointValueType, ExtentValueType>>
	{
		using point_value_type = PointValueType;
		using extent_value_type = ExtentValueType;

		using point_type = basic_point<2, point_value_type>;
		using extent_type = basic_extent<2, extent_value_type>;

		template<std::size_t Index>
			requires (Index < 2)
		using element_type = std::conditional_t<Index == 0, point_type, extent_type>;

		point_type point;
		extent_type extent;

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> const element_type<Index>&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> element_type<Index>&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr explicit operator basic_rect<3, point_value_type, extent_value_type>() const noexcept
		{
			using r = basic_rect<3, point_value_type, extent_value_type>;
			return {.point = point.operator typename r::point_type(), .extent = extent.operator typename r::extent_type()};
		}

		[[nodiscard]] constexpr auto left_top() const noexcept -> point_type
		{
			return point;
		}

		[[nodiscard]] constexpr auto left_bottom() const noexcept -> point_type
		{
			return {point.x, point.y + extent.height};
		}

		[[nodiscard]] constexpr auto right_top() const noexcept -> point_type
		{
			return {point.x + extent.width, point.y};
		}

		[[nodiscard]] constexpr auto right_bottom() const noexcept -> point_type
		{
			return {point.x + extent.width, point.y + extent.height};
		}

		[[nodiscard]] constexpr auto center() const noexcept -> point_type
		{
			return {point.x + width() / 2, point.y + height() / 2};
		}

		[[nodiscard]] constexpr auto empty() const noexcept -> bool
		{
			return extent.width == 0 or extent.height == 0;
		}

		[[nodiscard]] constexpr auto valid() const noexcept -> bool
		{
			return extent.width >= 0 and extent.height >= 0;
		}

		[[nodiscard]] constexpr auto width() const noexcept -> extent_value_type
		{
			return extent.width;
		}

		[[nodiscard]] constexpr auto height() const noexcept -> extent_value_type
		{
			return extent.height;
		}

		[[nodiscard]] constexpr auto size() const noexcept -> extent_type
		{
			return {width(), height()};
		}

		[[nodiscard]] constexpr auto includes(const point_type& p) const noexcept -> bool
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not empty() and valid());

			return p.between(left_top(), right_bottom());
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
		requires (std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<ExtentValueType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_rect<3, PointValueType, ExtentValueType> final : meta::dimension<basic_rect<3, PointValueType, ExtentValueType>>
	{
		using point_value_type = PointValueType;
		using extent_value_type = ExtentValueType;

		using point_type = basic_point<3, point_value_type>;
		using extent_type = basic_extent<3, extent_value_type>;

		template<std::size_t Index>
			requires (Index < 2)
		using element_type = std::conditional_t<Index == 0, point_type, extent_type>;

		point_type point;
		extent_type extent;

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> const element_type<Index>&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> element_type<Index>&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return extent; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr explicit operator basic_rect<2, point_value_type, extent_value_type>() const noexcept
		{
			using r = basic_rect<2, point_value_type, extent_value_type>;
			return {.point = point.operator typename r::point_type(), .extent = extent.operator typename r::extent_type()};
		}

		[[nodiscard]] constexpr auto left_top_near() const noexcept -> point_type
		{
			return point;
		}

		[[nodiscard]] constexpr auto left_bottom_near() const noexcept -> point_type
		{
			return {point.x, point.y + extent.height, point.z};
		}

		[[nodiscard]] constexpr auto left_top_far() const noexcept -> point_type
		{
			return {point.x, point.y, point.z + extent.depth};
		}

		[[nodiscard]] constexpr auto left_bottom_far() const noexcept -> point_type
		{
			return {point.x, point.y + extent.height, point.z + extent.depth};
		}

		[[nodiscard]] constexpr auto right_top_near() const noexcept -> point_type
		{
			return {point.x + extent.width, point.y, point.z};
		}

		[[nodiscard]] constexpr auto right_bottom_near() const noexcept -> point_type
		{
			return {point.x + extent.width, point.y + extent.height, point.z};
		}

		[[nodiscard]] constexpr auto right_top_far() const noexcept -> point_type
		{
			return {point.x + extent.width, point.y, point.z + extent.depth};
		}

		[[nodiscard]] constexpr auto right_bottom_far() const noexcept -> point_type
		{
			return {point.x + extent.width, point.y + extent.height, point.z + extent.depth};
		}

		[[nodiscard]] constexpr auto center() const noexcept -> point_type
		{
			return {point.x + width() / 2, point.y + height() / 2, point.z + depth()};
		}

		[[nodiscard]] constexpr auto empty() const noexcept -> bool
		{
			return extent.width == 0 or extent.height == 0 or extent.depth == 0;
		}

		[[nodiscard]] constexpr auto valid() const noexcept -> bool
		{
			return extent.width >= 0 and extent.height >= 0 and extent.depth >= 0;
		}

		[[nodiscard]] constexpr auto width() const noexcept -> extent_value_type
		{
			return extent.width;
		}

		[[nodiscard]] constexpr auto height() const noexcept -> extent_value_type
		{
			return extent.height;
		}

		[[nodiscard]] constexpr auto depth() const noexcept -> extent_value_type
		{
			return extent.depth;
		}

		[[nodiscard]] constexpr auto size() const noexcept -> extent_type
		{
			return {width(), height(), depth()};
		}

		[[nodiscard]] constexpr auto includes(const point_type& p) const noexcept -> bool
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not empty() and valid());

			return p.between(left_top_near(), right_bottom_near());
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

namespace std
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
			: std::integral_constant<std::size_t, 2> {};

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
				"{}{}",
				rect.point,
				rect.extent
			);
		}
	};
}
