// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
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
#include <primitive/rect.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::primitive
{
	template<std::size_t N, typename PointValueType, typename RadiusValueType>
		requires(std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<RadiusValueType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_circle final : meta::dimension<basic_circle<N, PointValueType, RadiusValueType>>
	{
		using point_value_type = PointValueType;
		using radius_value_type = RadiusValueType;

		using point_type = basic_point<N, point_value_type>;

		template<std::size_t Index>
			requires (Index < 2)
		using element_type = std::conditional_t<Index == 0, point_type, radius_value_type>;

		point_type point;
		radius_value_type radius;

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> const element_type<Index>&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return radius; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> element_type<Index>&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return radius; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr auto center() const noexcept -> point_type
		{
			return point;
		}

		[[nodiscard]] constexpr auto empty() const noexcept -> bool
		{
			return radius == 0;
		}

		[[nodiscard]] constexpr auto valid() const noexcept -> bool
		{
			return radius >= 0;
		}

		[[nodiscard]] constexpr auto includes(const point_type& p) const noexcept -> bool
		{
			return point.distance(p) <= radius;
		}

		[[nodiscard]] constexpr auto includes(const basic_circle& circle) const noexcept -> bool
		{
			if (radius < circle.radius) { return false; }

			return point.distance(circle.center) <= (radius - circle.radius);
		}
	};

	template<std::size_t N, typename PointValueType, typename RadiusValueType>
	[[nodiscard]] constexpr auto inscribed_rect(
		const basic_circle<N, PointValueType, RadiusValueType>& circle
	) noexcept -> basic_rect<N, PointValueType, RadiusValueType>
	{
		const auto radius = [r = circle.radius]
		{
			if constexpr (std::is_floating_point_v<RadiusValueType>) { return r * std::numbers::sqrt2_v<RadiusValueType>; }
			else if constexpr (sizeof(RadiusValueType) == sizeof(float)) { return static_cast<RadiusValueType>(static_cast<float>(r) * std::numbers::sqrt2_v<float>); }
			else { return static_cast<RadiusValueType>(static_cast<double>(r) * std::numbers::sqrt2_v<double>); }
		}();

		using rect_type = basic_rect<N, PointValueType, RadiusValueType>;
		using extent_type = typename rect_type::extent_type;

		if constexpr (N == 2)
		{
			const auto extent = extent_type{radius, radius};
			const auto offset = extent / 2;
			const auto left_top = circle.center - offset;

			return {.point = left_top, .extent = extent};
		}
		else if constexpr (N == 3)
		{
			const auto extent = extent_type{radius, radius, radius};
			const auto offset = extent / 2;
			const auto left_top_near = circle.center - offset;

			return {.point = left_top_near, .extent = extent};
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}

	template<std::size_t N, typename PointValueType, typename RadiusValueType>
	[[nodiscard]] constexpr auto circumscribed_rect(
		const basic_circle<N, PointValueType, RadiusValueType>& circle
	) noexcept -> basic_rect<N, PointValueType, RadiusValueType>
	{
		using rect_type = basic_rect<N, PointValueType, RadiusValueType>;
		using point_type = typename rect_type::point_type;
		using extent_type = typename rect_type::extent_type;

		if constexpr (N == 2)
		{
			const point_type left_top{circle.center.x - circle.radius, circle.center.y - circle.radius};
			const extent_type extent{circle.radius * 2, circle.radius * 2};

			return {.point = left_top, .extent = extent};
		}
		else if constexpr (N == 3)
		{
			const point_type left_top_near{circle.center.x - circle.radius, circle.center.y - circle.radius, circle.center.z - circle.radius};
			const extent_type extent{circle.radius * 2, circle.radius * 2, circle.radius * 2};

			return {.point = left_top_near, .extent = extent};
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}

	template<std::size_t N, typename PointValueType, typename RadiusType>
	[[nodiscard]] constexpr auto inscribed_circle(const basic_rect<N, PointValueType, RadiusType>& rect) noexcept -> basic_circle<N, PointValueType, RadiusType>
	{
		if constexpr (N == 2)
		{
			const auto radius = std::ranges::min(rect.width(), rect.height()) / 2;
			const auto center = rect.center();
			return {.point = center, .radius = radius};
		}
		else if constexpr (N == 3)
		{
			const auto radius = std::ranges::min(rect.width(), rect.height(), rect.depth()) / 2;
			const auto center = rect.center();
			return {.point = center, .radius = radius};
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}

	template<std::size_t N, typename PointValueType, typename RadiusType>
	[[nodiscard]] constexpr auto circumscribed_circle(const basic_rect<N, PointValueType, RadiusType>& rect) noexcept -> basic_circle<N, PointValueType, RadiusType>
	{
		using rect_type = basic_rect<N, PointValueType, RadiusType>;
		using point_type = typename rect_type::point_type;

		if constexpr (N == 2)
		{
			const auto radius = rect.size().template to<point_type>().distance({0, 0}) / 2;
			const auto center = rect.center();
			return {center, radius};
		}
		else if constexpr (N == 3)
		{
			const auto radius = rect.size().template to<point_type>().distance({0, 0, 0}) / 2;
			const auto center = rect.center();
			return {center, radius};
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}

	template<typename PointValueType, typename RadiusValueType = PointValueType>
	using basic_circle_2d = basic_circle<2, PointValueType, RadiusValueType>;

	template<typename PointValueType, typename RadiusValueType = PointValueType>
	using basic_circle_3d = basic_circle<3, PointValueType, RadiusValueType>;
}

namespace std
{
	template<std::size_t Index, std::size_t N, typename PointValueType, typename RadiusType>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_circle<N, PointValueType, RadiusType>> // NOLINT(cert-dcl58-cpp)
	{
		using type = typename gal::prometheus::primitive::basic_circle<N, PointValueType, RadiusType>::template element_type<Index>;
	};

	template<std::size_t N, typename PointValueType, typename RadiusType>
	struct tuple_size<gal::prometheus::primitive::basic_circle<N, PointValueType, RadiusType>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 2> {};

	template<std::size_t N, typename PointValueType, typename RadiusType>
	struct formatter<gal::prometheus::primitive::basic_circle<N, PointValueType, RadiusType>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_circle<N, PointValueType, RadiusType>& circle, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
				context.out(),
				"({}->{})",
				circle.center,
				circle.radius
			);
		}
	};
}
