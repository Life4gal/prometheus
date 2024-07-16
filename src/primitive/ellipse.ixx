// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:ellipse;

import std;
import gal.prometheus.functional;

import :multidimensional;
import :point;
import :extent;
import :rect;

#else
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

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::primitive)
{
	template<typename T, std::size_t N>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_ellipse;

	template<typename>
	struct is_basic_ellipse : std::false_type {};

	template<typename T, std::size_t N>
	struct is_basic_ellipse<basic_ellipse<T, N>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_ellipse_v = is_basic_ellipse<T>::value;

	template<typename T>
	concept basic_ellipse_t = is_basic_ellipse_v<T>;

	template<typename, typename>
	struct is_ellipse_compatible : std::false_type {};

	template<typename T>
	struct is_ellipse_compatible<T, T> : std::false_type {};

	template<typename EllipseValueType, std::size_t N, member_gettable_but_not_same_t<basic_ellipse<EllipseValueType, N>> U>
		requires(meta::member_size<U>() == 2)
	struct is_ellipse_compatible<U, basic_ellipse<EllipseValueType, N>> :
			std::bool_constant<
				is_point_compatible_v<meta::member_type_of_index_t<0, U>, basic_point<EllipseValueType, N>> and
				is_extent_compatible_v<meta::member_type_of_index_t<1, U>, basic_extent<EllipseValueType, N>>
			> {};

	template<typename EllipseValueType, std::size_t N, member_gettable_but_not_same_t<basic_ellipse<EllipseValueType, N>> U>
		requires(meta::member_size<U>() == 3)
	struct is_ellipse_compatible<U, basic_ellipse<EllipseValueType, N>> :
			std::bool_constant<
				is_point_compatible_v<meta::member_type_of_index_t<0, U>, basic_point<EllipseValueType, N>> and
				is_extent_compatible_v<meta::member_type_of_index_t<1, U>, basic_extent<EllipseValueType, N>> and
				is_extent_compatible_v<meta::member_type_of_index_t<2, U>, basic_extent<float, N>>
			> {};

	template<typename OtherType, typename EllipseType>
	constexpr auto is_ellipse_compatible_v = is_ellipse_compatible<OtherType, EllipseType>::value;

	template<typename OtherType, typename EllipseType>
	concept ellipse_compatible_t = is_ellipse_compatible_v<OtherType, EllipseType>;

	template<typename L, typename R, std::size_t N>
	[[nodiscard]] constexpr auto operator==(const basic_ellipse<L, N>& lhs, const basic_ellipse<R, N>& rhs) noexcept -> bool
	{
		return lhs.center == rhs.center and lhs.radius == rhs.radius and lhs.rotation == rhs.rotation;
	}

	template<typename T, std::size_t N, ellipse_compatible_t<basic_ellipse<T, N>> R>
	[[nodiscard]] constexpr auto operator==(const basic_ellipse<T, N>& lhs, const R& rhs) noexcept -> bool
	{
		if constexpr (meta::member_size<R>() == 2)
		{
			return lhs.center == meta::member_of_index<0>(rhs) and lhs.radius == meta::member_of_index<1>(rhs);
		}
		else if constexpr (meta::member_size<R>() == 3)
		{
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

	template<typename T, std::size_t N, ellipse_compatible_t<basic_ellipse<T, N>> L>
	[[nodiscard]] constexpr auto operator==(const L& lhs, const basic_ellipse<T, N>& rhs) noexcept -> bool
	{
		if constexpr (meta::member_size<L>() == 2)
		{
			return rhs.center == meta::member_of_index<0>(lhs) and rhs.radius == meta::member_of_index<1>(lhs);
		}
		else if constexpr (meta::member_size<L>() == 3)
		{
			return
					rhs.center == meta::member_of_index<0>(lhs) and
					rhs.radius == meta::member_of_index<1>(lhs) and
					rhs.rotation == meta::member_of_index<2>(lhs);
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_ellipse<T, 2> final : multidimensional<T, basic_ellipse<T, 2>>
	{
		using value_type = T;

		using point_type = basic_point<value_type, 2>;
		using radius_type = basic_extent<value_type, 2>;
		using rotation_type = float;

		constexpr static auto is_always_equal = true;

		constexpr static std::size_t element_size{3};
		template<std::size_t Index>
			requires(Index < element_size)
		using element_type = std::conditional_t<Index == 0, point_type, std::conditional_t<Index == 1, radius_type, rotation_type>>;

		point_type center;
		radius_type radius;
		// Multiples of PI
		// e.q. .5f * std::numbers::pi_v<rotation_type>
		rotation_type rotation;

		constexpr explicit(false) basic_ellipse(const value_type value = value_type{0}) noexcept
			: center{value},
			  radius{value},
			  rotation{0} {}

		constexpr basic_ellipse(const point_type& p, const radius_type& r, const rotation_type rotation = 0) noexcept
			: center{p},
			  radius{r},
			  rotation{rotation} {}

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

		[[nodiscard]] constexpr auto includes(const basic_circle<value_type, 2>& circle) const noexcept -> bool
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
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
{
	template<std::size_t Index, typename T, std::size_t N>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_ellipse<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		using type = typename gal::prometheus::primitive::basic_ellipse<T, N>::template element_type<Index>;
	};

	template<typename T, std::size_t N>
	struct tuple_size<gal::prometheus::primitive::basic_ellipse<T, N>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, gal::prometheus::primitive::basic_ellipse<T, N>::element_size> {};

	template<typename T, std::size_t N>
	struct formatter<gal::prometheus::primitive::basic_ellipse<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_ellipse<T, N>& ellipse, FormatContext& context) const noexcept -> auto
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
