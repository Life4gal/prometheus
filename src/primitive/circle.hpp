// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:circle;

import std;
import :multi_dimension;
import :point;
import :extent;
import :rect;

#else

#include <type_traits>
#include <format>
#include <numbers>

#include <prometheus/macro.hpp>
#include <primitive/multi_dimension.hpp>
#include <primitive/point.hpp>
#include <primitive/extent.hpp>
#include <primitive/rect.hpp>
#endif

namespace gal::prometheus::primitive
{
	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_circle final : multi_dimension<T, basic_circle<T>>
	{
		using value_type = T;
		using point_type = basic_point<value_type>;

		constexpr static auto is_always_equal = true;

		point_type center;
		value_type radius;

		constexpr explicit(false) basic_circle(const value_type value = value_type{0}) noexcept
			: center{value},
			  radius{value} {}

		constexpr basic_circle(const point_type& p, const value_type r) noexcept
			: center{p},
			  radius{r} {}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> std::conditional_t<Index == 0, const point_type&, value_type>
		{
			if constexpr (Index == 0) { return center; }
			else if constexpr (Index == 1) { return radius; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> std::conditional_t<Index == 0, point_type&, value_type&>
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

	template<typename T>
	[[nodiscard]] constexpr auto inscribed_rect(const basic_circle<T>& circle) noexcept -> basic_rect<T>
	{
		const auto size = [r = circle.radius]
		{
			if constexpr (std::is_floating_point_v<T>) { return r * std::numbers::sqrt2_v<T>; }
			else if constexpr (sizeof(T) == sizeof(float)) { return static_cast<T>(static_cast<float>(r) * std::numbers::sqrt2_v<float>); }
			else { return static_cast<T>(static_cast<double>(r) * std::numbers::sqrt2_v<double>); }
		}();

		const auto extent = typename basic_rect<T>::extent_type{size, size};
		const auto offset = extent / 2;
		const auto top_left = circle.center - offset;

		return {top_left, extent};
	}

	template<typename T>
	[[nodiscard]] constexpr auto circumscribed_rect(const basic_circle<T>& circle) noexcept -> basic_rect<T>
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

	template<typename T>
	[[nodiscard]] constexpr auto inscribed_circle(const basic_rect<T>& rect) noexcept -> basic_circle<T>
	{
		const auto radius = std::ranges::min(rect.width(), rect.height()) / 2;
		const auto center = rect.center();
		return {center, radius};
	}

	template<typename T>
	[[nodiscard]] constexpr auto circumscribed_circle(const basic_rect<T>& rect) noexcept -> basic_circle<T>
	{
		const auto radius = rect.size().template to<typename basic_rect<T>::point_type>().distance({0, 0}) / 2;
		const auto center = rect.center();
		return {center, radius};
	}
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_BEGIN
	template<std::size_t Index, typename T>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_circle<T>> // NOLINT(cert-dcl58-cpp)
	{
		using type = std::conditional_t<Index == 0, typename gal::prometheus::primitive::basic_circle<T>::point_type, T>;
	};

	template<typename T>
	struct tuple_size<gal::prometheus::primitive::basic_circle<T>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 2> {};

	template<typename T>
	struct formatter<gal::prometheus::primitive::basic_circle<T>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_circle<T>& circle, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
					context.out(),
					"[{}->{}]",
					circle.center,
					circle.radius
					);
		}
	};

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_END
