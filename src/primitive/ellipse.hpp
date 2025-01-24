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
#include <primitive/rect.hpp>
#include <primitive/circle.hpp>

#include <math/cmath.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::primitive
{
	template<std::size_t N, typename PointValueType, typename RadiusValueType, typename RotationValueType>
	struct basic_ellipse;

	// 2D
	template<typename PointValueType, typename RadiusValueType, typename RotationValueType>
		requires(std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<RadiusValueType> and std::is_arithmetic_v<RotationValueType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_ellipse<2, PointValueType, RadiusValueType, RotationValueType> final
			: meta::dimension<basic_ellipse<2, PointValueType, RadiusValueType, RotationValueType>>
	{
		using point_value_type = PointValueType;
		using radius_value_type = RadiusValueType;
		using rotation_value_type = RotationValueType;

		using point_type = basic_point<2, point_value_type>;
		using radius_type = basic_extent<2, radius_value_type>;

		template<std::size_t Index>
			requires (Index < 3)
		using element_type = std::conditional_t<Index == 0, point_type, std::conditional_t<Index == 1, radius_type, rotation_value_type>>;

		point_type point;
		radius_type radius;
		// multiples of PI
		// e.q. .5f * std::numbers::pi_v<rotation_value_type>
		rotation_value_type rotation;

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() const noexcept -> const element_type<Index>&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return radius; }
			else if constexpr (Index == 2) { return rotation; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() noexcept -> element_type<Index>&
		{
			if constexpr (Index == 0) { return point; }
			else if constexpr (Index == 1) { return radius; }
			else if constexpr (Index == 2) { return rotation; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr auto center() const noexcept -> point_type
		{
			return point;
		}

		[[nodiscard]] constexpr auto empty() const noexcept -> bool
		{
			return radius.width == 0 and radius.height == 0;
		}

		[[nodiscard]] constexpr auto valid() const noexcept -> bool
		{
			return radius.width >= 0 and radius.height >= 0;
		}

		[[nodiscard]] constexpr auto includes(const point_type& p) const noexcept -> bool //
		{
			const auto dx = static_cast<rotation_value_type>(p.x - point.x);
			const auto dy = static_cast<rotation_value_type>(p.y - point.y);

			if (rotation == 0) // NOLINT(clang-diagnostic-float-equal)
			{
				return
						(
							math::pow(dx, 2) / static_cast<rotation_value_type>(math::pow(radius.width, 2)) +
							math::pow(dy, 2) / static_cast<rotation_value_type>(math::pow(radius.height, 2))
						)
						<= static_cast<rotation_value_type>(1);
			}

			// Rotate point back by ellipse's rotation
			const auto cos_theta = math::cos(rotation);
			const auto sin_theta = math::sin(rotation);
			const auto prime_x = dx * cos_theta + dy * sin_theta;
			const auto prime_y = -dx * sin_theta + dy * cos_theta;

			// Check if the point is inside the ellipse's standard form
			return
					(
						math::pow(prime_x, 2) / static_cast<rotation_value_type>(math::pow(radius.width, 2)) +
						math::pow(prime_y, 2) / static_cast<rotation_value_type>(math::pow(radius.height, 2))
					)
					<= static_cast<rotation_value_type>(1);
		}

		[[nodiscard]] constexpr auto includes(const basic_circle<2, point_value_type, radius_value_type>& circle) const noexcept -> bool
		{
			const auto dx = static_cast<rotation_value_type>(circle.point.x - point.x);
			const auto dy = static_cast<rotation_value_type>(circle.point.y - point.y);

			// Scale the circle's radius to ellipse's coordinate space
			const auto scaled_radius_x = static_cast<rotation_value_type>(circle.radius) / static_cast<rotation_value_type>(radius.width);
			const auto scaled_radius_y = static_cast<rotation_value_type>(circle.radius) / static_cast<rotation_value_type>(radius.height);

			if (rotation == 0) // NOLINT(clang-diagnostic-float-equal)
			{
				const auto distance =
						math::hypot(
							dx / static_cast<rotation_value_type>(radius.width),
							dy / static_cast<rotation_value_type>(radius.height)
						);
				return distance + std::ranges::max(scaled_radius_x, scaled_radius_y) <= static_cast<rotation_value_type>(1);
			}

			// Rotate point back by ellipse's rotation
			const auto cos_theta = math::cos(rotation);
			const auto sin_theta = math::sin(rotation);
			const auto prime_x = dx * cos_theta + dy * sin_theta;
			const auto prime_y = -dx * sin_theta + dy * cos_theta;

			// Check if the transformed circle is inside the unit circle
			const auto distance =
					math::hypot(
						prime_x / static_cast<rotation_value_type>(radius.width),
						prime_y / static_cast<rotation_value_type>(radius.height)
					);
			return distance + std::ranges::max(scaled_radius_x, scaled_radius_y) <= static_cast<rotation_value_type>(1);
		}

		[[nodiscard]] constexpr auto includes(const basic_ellipse& ellipse) const noexcept -> bool
		{
			const auto dx = static_cast<rotation_value_type>(ellipse.point.x - point.x);
			const auto dy = static_cast<rotation_value_type>(ellipse.point.y - point.y);

			// Scale the ellipse's radius to this ellipse's coordinate space
			const auto scaled_rx = static_cast<rotation_value_type>(ellipse.radius.width) / static_cast<rotation_value_type>(radius.width);
			const auto scaled_ry = static_cast<rotation_value_type>(ellipse.radius.height) / static_cast<rotation_value_type>(radius.height);

			if (rotation == 0) // NOLINT(clang-diagnostic-float-equal)
			{
				const auto distance =
						math::hypot(
							dx / static_cast<rotation_value_type>(radius.width),
							dy / static_cast<rotation_value_type>(radius.height)
						);
				return distance + std::ranges::max(scaled_rx, scaled_ry) <= static_cast<rotation_value_type>(1);
			}

			// Rotate point back by this ellipse's rotation
			const auto cos_theta = math::cos(rotation);
			const auto sin_theta = math::sin(rotation);
			const auto prime_x = dx * cos_theta + dy * sin_theta;
			const auto prime_y = -dx * sin_theta + dy * cos_theta;

			// Check if the transformed ellipse is inside the unit circle
			const auto distance =
					math::hypot(
						prime_x / static_cast<rotation_value_type>(radius.width),
						prime_y / static_cast<rotation_value_type>(radius.height)
					);
			return distance + std::ranges::max(scaled_rx, scaled_ry) <= static_cast<rotation_value_type>(1);
		}
	};

	template<typename PointValueType, typename RadiusValueType = PointValueType, typename RotationValueType = RadiusValueType>
	using basic_ellipse_2d = basic_ellipse<2, PointValueType, RadiusValueType, RotationValueType>;
}

namespace std
{
	template<std::size_t Index, std::size_t N, typename PointValueType, typename RadiusValueType, typename RotationValueType>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>> // NOLINT(cert-dcl58-cpp)
	{
		using type = typename gal::prometheus::primitive::basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>::template element_type<Index>;
	};

	template<std::size_t N, typename PointValueType, typename RadiusValueType, typename RotationValueType>
	struct tuple_size<gal::prometheus::primitive::basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 3> {};

	template<std::size_t N, typename PointValueType, typename RadiusValueType, typename RotationValueType>
	struct formatter<gal::prometheus::primitive::basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>& ellipse, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
				context.out(),
				"({}->{}[{} °])",
				ellipse.center,
				ellipse.radius,
				ellipse.rotation * 180
			);
		}
	};
}
