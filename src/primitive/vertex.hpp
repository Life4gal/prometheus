// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:vertex;

import std;
import :point;
import :color;

#else

#include <type_traits>
#include <format>

#include <prometheus/macro.hpp>
#include <primitive/point.hpp>
#include <primitive/color.hpp>
#endif

namespace gal::prometheus::primitive
{
	template<typename PointValueType, typename ColorValueType>
		requires std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<ColorValueType>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_vertex final
	{
		using point_value_type = PointValueType;
		using color_value_type = ColorValueType;

		using point_type = basic_point<point_value_type>;
		using color_type = basic_color<color_value_type>;

		constexpr static auto is_always_equal = true;
		constexpr static auto default_uv = point_type{0, 0};

		point_type position;
		point_type uv;
		color_type color;

		constexpr basic_vertex(const point_type& position, const point_type uv, const color_type color) noexcept
			: position{position},
			  uv{uv},
			  color{color} {}

		constexpr basic_vertex(const point_type position, const color_type color) noexcept
			: basic_vertex{position, default_uv, color} {}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept
			-> std::conditional_t<
				Index == 0,
				point_type,
				std::conditional_t<
					Index == 1,
					point_type,
					color_type
				>
			>
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return uv; }
			else if constexpr (Index == 2) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept
			-> std::conditional_t<
				Index == 0,
				point_type,
				std::conditional_t<
					Index == 1,
					point_type,
					color_type
				>
			>
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return uv; }
			else if constexpr (Index == 2) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_BEGIN
	template<std::size_t Index, typename PointValueType, typename ColorValueType>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_vertex<PointValueType, ColorValueType>> // NOLINT(cert-dcl58-cpp)
	{
		using type = std::conditional_t<
			Index == 0,
			typename gal::prometheus::primitive::basic_vertex<PointValueType, ColorValueType>::point_type,
			std::conditional_t<
				Index == 1,
				typename gal::prometheus::primitive::basic_vertex<PointValueType, ColorValueType>::point_type,
				typename gal::prometheus::primitive::basic_vertex<PointValueType, ColorValueType>::color_type
			>
		>;
	};

	template<typename PointValueType, typename ColorValueType>
	struct tuple_size<gal::prometheus::primitive::basic_vertex<PointValueType, ColorValueType>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 3> {};

	template<typename PointValueType, typename ColorValueType>
	struct formatter<gal::prometheus::primitive::basic_vertex<PointValueType, ColorValueType>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(
				const gal::prometheus::primitive::basic_vertex<PointValueType, ColorValueType>& vertex,
				FormatContext& context
				) const noexcept -> auto
		{
			return std::format_to(
					context.out(),
					"[pos{}uv{}color{}]",
					vertex.position,
					vertex.uv,
					vertex.color
					);
		}
	};

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_END
