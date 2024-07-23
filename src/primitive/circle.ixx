// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:circle;

import std;

import :multidimensional;
import :point;
import :extent;
import :rect;

#else
#pragma once

#include <type_traits>
#include <utility>
#include <numbers>
#include <format>
#include <tuple>

#include <prometheus/macro.hpp>
#include <primitive/multidimensional.ixx>
#include <primitive/point.ixx>
#include <primitive/extent.ixx>
#include <primitive/rect.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::primitive)
{
	template<std::size_t N, typename PointValueType, typename RadiusType = PointValueType>
		requires (std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<RadiusType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_circle;

	template<typename>
	struct is_basic_circle : std::false_type {};

	template<std::size_t N, typename PointValueType, typename RadiusType>
	struct is_basic_circle<basic_circle<N, PointValueType, RadiusType>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_circle_v = is_basic_circle<T>::value;

	template<typename T>
	concept basic_circle_t = is_basic_circle_v<T>;

	template<typename, typename>
	struct is_circle_compatible : std::false_type {};

	template<typename T>
	struct is_circle_compatible<T, T> : std::false_type {};

	// point + radius
	template<std::size_t N, typename PointValueType, typename RadiusType, member_gettable_but_not_same_t<basic_circle<N, PointValueType, RadiusType>> U>
		requires(meta::member_size<U>() == 2)
	struct is_circle_compatible<U, basic_circle<N, PointValueType, RadiusType>> :
			std::bool_constant<
				is_point_compatible_v<meta::member_type_of_index_t<0, U>, basic_point<N, PointValueType>> and
				std::convertible_to<meta::member_type_of_index_t<1, U>, RadiusType>
			> {};

	// X + Y + radius
	template<typename PointValueType, typename RadiusType, std::size_t N, member_gettable_but_not_same_t<basic_circle<N, PointValueType, RadiusType>> U>
		requires(meta::member_size<U>() == 3)
	struct is_circle_compatible<U, basic_circle<N, PointValueType, RadiusType>> :
			std::bool_constant<
				std::convertible_to<meta::member_type_of_index_t<0, U>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<1, U>, PointValueType> and
				std::convertible_to<meta::member_type_of_index_t<2, U>, RadiusType>
			> {};

	template<typename OtherType, typename CircleType>
	constexpr auto is_circle_compatible_v = is_circle_compatible<OtherType, CircleType>::value;

	template<typename OtherType, typename CircleType>
	concept circle_compatible_t = is_circle_compatible_v<OtherType, CircleType>;

	template<std::size_t N, typename PointValueTypeL, typename RadiusTypeL, typename PointValueTypeR, typename RadiusTypeR>
	[[nodiscard]] constexpr auto operator==(
		const basic_circle<N, PointValueTypeL, RadiusTypeL>& lhs,
		const basic_circle<N, PointValueTypeR, RadiusTypeR>& rhs
	) noexcept -> bool
	{
		return lhs.center == rhs.center and lhs.radius == rhs.radius;
	}

	template<std::size_t N, typename PointValueType, typename RadiusType, circle_compatible_t<basic_circle<N, PointValueType, RadiusType>> R>
	[[nodiscard]] constexpr auto operator==(const basic_circle<N, PointValueType, RadiusType>& lhs, const R& rhs) noexcept -> bool
	{
		if constexpr (meta::member_size<R>() == 2)
		{
			// point + radius
			return lhs.center == meta::member_of_index<0>(rhs) and lhs.radius == meta::member_of_index<1>(rhs);
		}
		else if constexpr (meta::member_size<R>() == 3)
		{
			// X + Y + radius
			const auto x = meta::member_of_index<0>(rhs);
			const auto y = meta::member_of_index<1>(rhs);
			const auto radius = meta::member_of_index<2>(rhs);

			return x == lhs.center.x and y == lhs.center.y and radius == lhs.radius;
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<std::size_t N, typename PointValueType, typename RadiusType, circle_compatible_t<basic_circle<N, PointValueType, RadiusType>> L>
	[[nodiscard]] constexpr auto operator==(const L& lhs, const basic_circle<N, PointValueType, RadiusType>& rhs) noexcept -> bool
	{
		return rhs == lhs;
	}

	template<std::size_t N, typename PointValueType, typename RadiusType>
		requires(std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<RadiusType>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_circle final : multidimensional<basic_circle<N, PointValueType, RadiusType>, PointValueType, RadiusType>
	{
		using point_value_type = PointValueType;
		using radius_type = RadiusType;

		using point_type = basic_point<N, point_value_type>;

		constexpr static auto is_always_equal = true;

		constexpr static std::size_t element_size{2};
		template<std::size_t Index>
			requires(Index < element_size)
		using element_type = std::conditional_t<Index == 0, point_type, radius_type>;

		point_type center;
		radius_type radius;

		constexpr explicit(false) basic_circle(const point_value_type point_value = point_value_type{0}, const radius_type radius_value = radius_type{0}) noexcept
			: center{point_value},
			  radius{radius_value} {}

		constexpr basic_circle(const point_value_type x, const point_value_type y, const radius_type radius_value) noexcept
			: center{x, y},
			  radius{radius} {}

		constexpr basic_circle(const point_type& point_value, const radius_type radius_value) noexcept
			: center{point_value},
			  radius{radius_value} {}

		template<circle_compatible_t<basic_circle> U>
		constexpr explicit basic_circle(const U& value) noexcept
			: basic_circle{}
		{
			*this = value;
		}

		constexpr basic_circle(const basic_circle&) noexcept = default;
		constexpr basic_circle(basic_circle&&) noexcept = default;
		constexpr auto operator=(const basic_circle&) noexcept -> basic_circle& = default;
		constexpr auto operator=(basic_circle&&) noexcept -> basic_circle& = default;
		constexpr ~basic_circle() noexcept = default;

		template<circle_compatible_t<basic_circle> U>
		constexpr auto operator=(const U& value) noexcept -> basic_circle&
		{
			const auto [_center, _radius] = value;
			center = static_cast<point_type>(_center);
			radius = static_cast<radius_type>(_radius);

			return *this;
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> std::add_lvalue_reference_t<std::add_const_t<element_type<Index>>>
		{
			if constexpr (Index == 0) { return center; }
			else if constexpr (Index == 1) { return radius; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> std::add_lvalue_reference_t<element_type<Index>>
		{
			if constexpr (Index == 0) { return center; }
			else if constexpr (Index == 1) { return radius; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr auto empty() const noexcept -> bool { return radius == 0; }

		[[nodiscard]] constexpr auto includes(const point_type& point) const noexcept -> bool //
		{
			return center.distance(point) <= radius;
		}

		[[nodiscard]] constexpr auto includes(const basic_circle& circle) const noexcept -> bool
		{
			if (radius < circle.radius) { return false; }

			return center.distance(circle.center) <= (radius - circle.radius);
		}
	};

	template<typename PointValueType, typename RadiusType = PointValueType>
	using basic_circle_2d = basic_circle<2, PointValueType, RadiusType>;

	template<typename PointValueType, typename RadiusType = PointValueType>
	using basic_circle_3d = basic_circle<3, PointValueType, RadiusType>;

	template<std::size_t N, typename PointValueType, typename RadiusType>
	[[nodiscard]] constexpr auto inscribed_rect(const basic_circle<N, PointValueType, RadiusType>& circle) noexcept -> basic_rect<N, PointValueType, RadiusType>
	{
		const auto size = [r = circle.radius]
		{
			if constexpr (std::is_floating_point_v<RadiusType>) { return r * std::numbers::sqrt2_v<RadiusType>; }
			else if constexpr (sizeof(RadiusType) == sizeof(float)) { return static_cast<RadiusType>(static_cast<float>(r) * std::numbers::sqrt2_v<float>); }
			else { return static_cast<RadiusType>(static_cast<double>(r) * std::numbers::sqrt2_v<double>); }
		}();

		if constexpr (N == 2)
		{
			const auto extent = typename basic_rect<N, PointValueType, RadiusType>::extent_type{size, size};
			const auto offset = extent / 2;
			const auto left_top = circle.center - offset;

			return {left_top, extent};
		}
		else if constexpr (N == 3)
		{
			const auto extent = typename basic_rect<N, PointValueType, RadiusType>::extent_type{size, size, size};
			const auto offset = extent / 2;
			const auto left_top_near = circle.center - offset;

			return {left_top_near, extent};
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}

	template<std::size_t N, typename PointValueType, typename RadiusType>
	[[nodiscard]] constexpr auto circumscribed_rect(const basic_circle<N, PointValueType, RadiusType>& circle) noexcept -> basic_rect<N, PointValueType, RadiusType>
	{
		if constexpr (N == 2)
		{
			return
			{
					// left
					circle.center.x - circle.radius,
					// top
					circle.center.y - circle.radius,
					// right
					circle.center.x + circle.radius,
					// bottom
					circle.center.y + circle.radius
			};
		}
		else if constexpr (N == 3)
		{
			return
			{
					// left
					circle.center.x - circle.radius,
					// top
					circle.center.y - circle.radius,
					// near
					circle.center.z - circle.radius,
					// right
					circle.center.x + circle.radius,
					// bottom
					circle.center.y + circle.radius,
					// far
					circle.center.z + circle.radius
			};
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
			return {center, radius};
		}
		else if constexpr (N == 3)
		{
			const auto radius = std::ranges::min(rect.width(), rect.height(), rect.depth()) / 2;
			const auto center = rect.center();
			return {center, radius};
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}

	template<std::size_t N, typename PointValueType, typename RadiusType>
	[[nodiscard]] constexpr auto circumscribed_circle(const basic_rect<N, PointValueType, RadiusType>& rect) noexcept -> basic_circle<N, PointValueType, RadiusType>
	{
		if constexpr (N == 2)
		{
			const auto radius = rect.size().template to<typename basic_rect<N, PointValueType, RadiusType>::point_type>().distance({0, 0}) / 2;
			const auto center = rect.center();
			return {center, radius};
		}
		else if constexpr (N == 3)
		{
			const auto radius = rect.size().template to<typename basic_rect<N, PointValueType, RadiusType>::point_type>().distance({0, 0, 0}) / 2;
			const auto center = rect.center();
			return {center, radius};
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
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
				"[{}->{}]",
				circle.center,
				circle.radius
			);
		}
	};
}
