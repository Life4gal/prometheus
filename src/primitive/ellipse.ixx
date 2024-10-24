// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:primitive.ellipse;

import std;

import :functional;

import :primitive.multidimensional;
import :primitive.point;
import :primitive.extent;
import :primitive.rect;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>
#include <utility>
#include <format>
#include <tuple>

#include <prometheus/macro.hpp>
#include <functional/functional.ixx>
#include <primitive/multidimensional.ixx>
#include <primitive/point.ixx>
#include <primitive/extent.ixx>
#include <primitive/rect.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(primitive)
{
	template<std::size_t N, typename PointValueType, typename RadiusValueType = PointValueType, typename RotationValueType = RadiusValueType>
		requires(std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<RadiusValueType> and std::is_arithmetic_v<RotationValueType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_ellipse;

	template<typename>
	struct is_basic_ellipse : std::false_type {};

	template<std::size_t N, typename PointValueType, typename RadiusValueType, typename RotationValueType>
	struct is_basic_ellipse<basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_ellipse_v = is_basic_ellipse<T>::value;

	template<typename T>
	concept basic_ellipse_t = is_basic_ellipse_v<T>;

	template<typename, typename>
	struct is_ellipse_compatible : std::false_type {};

	template<typename T>
	struct is_ellipse_compatible<T, T> : std::false_type {};

	// point + radius
	template<
		std::size_t N,
		typename PointValueType,
		typename RadiusValueType,
		typename RotationValueType,
		member_gettable_but_not_same_t<basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>> U
	>
		requires(meta::member_size<U>() == 2)
	struct is_ellipse_compatible<U, basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>> :
			std::bool_constant<
				is_point_compatible_v<meta::member_type_of_index_t<0, U>, basic_point<N, PointValueType>> and
				is_extent_compatible_v<meta::member_type_of_index_t<1, U>, basic_extent<N, RadiusValueType>>
			> {};

	// point + radius + rotation (2D)
	template<
		typename PointValueType,
		typename RadiusValueType,
		typename RotationValueType,
		member_gettable_but_not_same_t<basic_ellipse<2, PointValueType, RadiusValueType, RotationValueType>> U
	>
		requires(meta::member_size<U>() == 3)
	struct is_ellipse_compatible<U, basic_ellipse<2, PointValueType, RadiusValueType, RotationValueType>> :
			std::bool_constant<
				is_point_compatible_v<meta::member_type_of_index_t<0, U>, basic_point<2, PointValueType>> and
				is_extent_compatible_v<meta::member_type_of_index_t<1, U>, basic_extent<2, RadiusValueType>> and
				std::convertible_to<meta::member_type_of_index_t<2, U>, RotationValueType>
			> {};

	// point + radius + rotation (3D)
	template<
		typename PointValueType,
		typename RadiusValueType,
		typename RotationValueType,
		member_gettable_but_not_same_t<basic_ellipse<3, PointValueType, RadiusValueType, RotationValueType>> U
	>
		requires(meta::member_size<U>() == 3)
	struct is_ellipse_compatible<U, basic_ellipse<3, PointValueType, RadiusValueType, RotationValueType>> :
			std::bool_constant<
				is_point_compatible_v<meta::member_type_of_index_t<0, U>, basic_point<3, PointValueType>> and
				is_extent_compatible_v<meta::member_type_of_index_t<1, U>, basic_extent<3, RadiusValueType>> and
				is_extent_compatible_v<meta::member_type_of_index_t<2, U>, basic_extent<3, RotationValueType>>
			> {};

	template<typename OtherType, typename EllipseType>
	constexpr auto is_ellipse_compatible_v = is_ellipse_compatible<OtherType, EllipseType>::value;

	template<typename OtherType, typename EllipseType>
	concept ellipse_compatible_t = is_ellipse_compatible_v<OtherType, EllipseType>;

	template<
		std::size_t N,
		typename PointValueTypeL,
		typename RadiusValueTypeL,
		typename RotationValueTypeL,
		typename PointValueTypeR,
		typename RadiusValueTypeR,
		typename RotationValueTypeR
	>
	[[nodiscard]] constexpr auto operator==(
		const basic_ellipse<N, PointValueTypeL, RadiusValueTypeL, RotationValueTypeL>& lhs,
		const basic_ellipse<N, PointValueTypeR, RadiusValueTypeR, RotationValueTypeR>& rhs
	) noexcept -> bool
	{
		return lhs.center == rhs.center and lhs.radius == rhs.radius and lhs.rotation == rhs.rotation;
	}

	template<
		std::size_t N,
		typename PointValueType,
		typename RadiusValueType,
		typename RotationValueType,
		ellipse_compatible_t<basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>> R
	>
	[[nodiscard]] constexpr auto operator==(const basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>& lhs, const R& rhs) noexcept -> bool
	{
		if constexpr (meta::member_size<R>() == 2)
		{
			// point + radius
			return lhs.center == meta::member_of_index<0>(rhs) and lhs.radius == meta::member_of_index<1>(rhs);
		}
		else if constexpr (meta::member_size<R>() == 3)
		{
			// point + radius + rotation
			return
					lhs.center == meta::member_of_index<0>(rhs) and
					lhs.radius == meta::member_of_index<1>(rhs) and
					lhs.rotation == meta::member_of_index<2>(rhs);
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<
		std::size_t N,
		typename PointValueType,
		typename RadiusValueType,
		typename RotationValueType,
		ellipse_compatible_t<basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>> L
	>
	[[nodiscard]] constexpr auto operator==(const L& lhs, const basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>& rhs) noexcept -> bool
	{
		return rhs == lhs;
	}

	// 2D
	template<typename PointValueType, typename RadiusValueType, typename RotationValueType>
		requires(std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<RadiusValueType> and std::is_arithmetic_v<RotationValueType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_ellipse<2, PointValueType, RadiusValueType, RotationValueType> final
			: multidimensional<basic_ellipse<2, PointValueType, RadiusValueType, RotationValueType>>
	{
		using point_value_type = PointValueType;
		using radius_value_type = RadiusValueType;
		using rotation_value_type = RotationValueType;

		using point_type = basic_point<2, point_value_type>;
		using radius_type = basic_extent<2, radius_value_type>;
		using rotation_type = rotation_value_type;

		constexpr static std::size_t element_size{3};
		template<std::size_t Index>
			requires(Index < element_size)
		using element_type = std::conditional_t<Index == 0, point_type, std::conditional_t<Index == 1, radius_type, rotation_type>>;

		point_type center;
		radius_type radius;
		// multiples of PI
		// e.q. .5f * std::numbers::pi_v<rotation_value_type>
		rotation_type rotation;

		constexpr explicit(false) basic_ellipse(
			const point_value_type point_value = point_value_type{0},
			const radius_value_type radius_value = radius_value_type{0},
			const rotation_value_type rotation_value = radius_value_type{0}
		) noexcept
			: center{point_value},
			  radius{radius_value},
			  rotation{rotation_value} {}

		constexpr basic_ellipse(
			const point_value_type x,
			const point_value_type y,
			const radius_value_type radius_x,
			const radius_value_type radius_y,
			const rotation_value_type rotation_value = rotation_value_type{0}
		) noexcept
			: center{x, y},
			  radius{radius_x, radius_y},
			  rotation{rotation_value} {}

		constexpr basic_ellipse(const point_type& point_value, const radius_type& radius_value, const rotation_type rotation_value = rotation_value_type{0}) noexcept
			: center{point_value},
			  radius{radius_value},
			  rotation{rotation_value} {}

		template<ellipse_compatible_t<basic_ellipse> U>
		constexpr explicit basic_ellipse(const U& value) noexcept
			: basic_ellipse{}
		{
			*this = value;
		}

		constexpr basic_ellipse(const basic_ellipse&) noexcept = default;
		constexpr basic_ellipse(basic_ellipse&&) noexcept = default;
		constexpr auto operator=(const basic_ellipse&) noexcept -> basic_ellipse& = default;
		constexpr auto operator=(basic_ellipse&&) noexcept -> basic_ellipse& = default;
		constexpr ~basic_ellipse() noexcept = default;

		template<ellipse_compatible_t<basic_ellipse> U>
		constexpr auto operator=(const U& value) noexcept -> basic_ellipse&
		{
			if constexpr (meta::member_size<U>() == 2)
			{
				const auto& [_center, _radius] = value;
				center = static_cast<point_type>(_center);
				radius = static_cast<radius_type>(_radius);
			}
			else if constexpr (meta::member_size<U>() == 3)
			{
				const auto& [_center, _radius, _rotation] = value;
				center = static_cast<point_type>(_center);
				radius = static_cast<radius_type>(_radius);
				rotation = static_cast<rotation_type>(_rotation);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}

			return *this;
		}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() const noexcept -> std::add_lvalue_reference_t<std::add_const_t<element_type<Index>>>
		{
			if constexpr (Index == 0) { return center; }
			else if constexpr (Index == 1) { return radius; }
			else if constexpr (Index == 2) { return rotation; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() noexcept -> std::add_lvalue_reference_t<element_type<Index>>
		{
			if constexpr (Index == 0) { return center; }
			else if constexpr (Index == 1) { return radius; }
			else if constexpr (Index == 2) { return rotation; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr auto empty() const noexcept -> bool { return radius.width == 0 or radius.height == 0; }

		[[nodiscard]] constexpr auto includes(const point_type& point) const noexcept -> bool //
		{
			const auto dx = static_cast<rotation_type>(point.x - center.x);
			const auto dy = static_cast<rotation_type>(point.y - center.y);

			if (rotation == 0) // NOLINT(clang-diagnostic-float-equal)
			{
				return
						(
							functional::pow(dx, 2) / static_cast<rotation_type>(functional::pow(radius.width, 2)) +
							functional::pow(dy, 2) / static_cast<rotation_type>(functional::pow(radius.height, 2))
						)
						<= static_cast<rotation_type>(1);
			}

			// Rotate point back by ellipse's rotation
			const auto cos_theta = functional::cos(rotation);
			const auto sin_theta = functional::sin(rotation);
			const auto prime_x = dx * cos_theta + dy * sin_theta;
			const auto prime_y = -dx * sin_theta + dy * cos_theta;

			// Check if the point is inside the ellipse's standard form
			return
					(
						functional::pow(prime_x, 2) / static_cast<rotation_type>(functional::pow(radius.width, 2)) +
						functional::pow(prime_y, 2) / static_cast<rotation_type>(functional::pow(radius.height, 2))
					)
					<= static_cast<rotation_type>(1);
		}

		[[nodiscard]] constexpr auto includes(const basic_circle<2, point_value_type, radius_value_type>& circle) const noexcept -> bool
		{
			const auto dx = static_cast<rotation_type>(circle.center.x - center.x);
			const auto dy = static_cast<rotation_type>(circle.center.y - center.y);

			// Scale the circle's radius to ellipse's coordinate space
			const auto scaled_radius_x = static_cast<rotation_type>(circle.radius) / static_cast<rotation_type>(radius.width);
			const auto scaled_radius_y = static_cast<rotation_type>(circle.radius) / static_cast<rotation_type>(radius.height);

			if (rotation == 0) // NOLINT(clang-diagnostic-float-equal)
			{
				const auto distance = functional::sqrt(functional::pow(dx / static_cast<rotation_type>(radius.width), 2) + functional::pow(dy / static_cast<rotation_type>(radius.height), 2));
				return distance + std::ranges::max(scaled_radius_x, scaled_radius_y) <= static_cast<rotation_type>(1);
			}

			// Rotate point back by ellipse's rotation
			const auto cos_theta = functional::cos(rotation);
			const auto sin_theta = functional::sin(rotation);
			const auto prime_x = dx * cos_theta + dy * sin_theta;
			const auto prime_y = -dx * sin_theta + dy * cos_theta;

			// Check if the transformed circle is inside the unit circle
			const auto distance = functional::sqrt(functional::pow(prime_x / static_cast<rotation_type>(radius.width), 2) + functional::pow(prime_y / static_cast<rotation_type>(radius.height), 2));
			return distance + std::ranges::max(scaled_radius_x, scaled_radius_y) <= static_cast<rotation_type>(1);
		}

		[[nodiscard]] constexpr auto includes(const basic_ellipse& ellipse) const noexcept -> bool
		{
			const auto dx = static_cast<rotation_type>(ellipse.center.x - center.x);
			const auto dy = static_cast<rotation_type>(ellipse.center.y - center.y);

			// Scale the ellipse's radius to this ellipse's coordinate space
			const auto scaled_rx = static_cast<rotation_type>(ellipse.radius.width) / static_cast<rotation_type>(radius.width);
			const auto scaled_ry = static_cast<rotation_type>(ellipse.radius.height) / static_cast<rotation_type>(radius.height);

			if (rotation == 0) // NOLINT(clang-diagnostic-float-equal)
			{
				const auto distance = functional::sqrt(functional::pow(dx / static_cast<rotation_type>(radius.width), 2) + functional::pow(dy / static_cast<rotation_type>(radius.height), 2));
				return distance + std::ranges::max(scaled_rx, scaled_ry) <= static_cast<rotation_type>(1);
			}

			// Rotate point back by this ellipse's rotation
			const auto cos_theta = functional::cos(rotation);
			const auto sin_theta = functional::sin(rotation);
			const auto prime_x = dx * cos_theta + dy * sin_theta;
			const auto prime_y = -dx * sin_theta + dy * cos_theta;

			// Check if the transformed ellipse is inside the unit circle
			const auto distance = functional::sqrt(functional::pow(prime_x / static_cast<rotation_type>(radius.width), 2) + functional::pow(prime_y / static_cast<rotation_type>(radius.height), 2));
			return distance + std::ranges::max(scaled_rx, scaled_ry) <= static_cast<rotation_type>(1);
		}
	};

	template<typename PointValueType, typename RadiusValueType = PointValueType, typename RotationValueType = RadiusValueType>
	using basic_ellipse_2d = basic_ellipse<2, PointValueType, RadiusValueType, RotationValueType>;
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_STD
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
			: std::integral_constant<std::size_t, gal::prometheus::primitive::basic_ellipse<N, PointValueType, RadiusValueType, RotationValueType>::element_size> {};

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
				"[{}->{}({})]",
				ellipse.center,
				ellipse.radius,
				ellipse.rotation
			);
		}
	};
}
