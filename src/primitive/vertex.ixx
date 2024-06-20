// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:vertex;

import std;
import :point;
import :color;

#else
#pragma once

#include <tuple>
#include <utility>
#include <format>

#include <prometheus/macro.hpp>
#include <primitive/point.ixx>
#include <primitive/color.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::primitive)
{
	template<
		typename, // point type
		typename, // uv type
		typename // color type
	>
	struct basic_vertex;

	template<typename>
	struct is_basic_vertex : std::false_type {};

	template<
		typename PositionType,
		typename UvType,
		typename ColorType
	>
	struct is_basic_vertex<basic_vertex<PositionType, UvType, ColorType>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_vertex_v = is_basic_vertex<T>::value;

	template<typename T>
	concept basic_vertex_t = is_basic_vertex_v<T>;

	template<
		basic_point_t PositionType,
		basic_point_t UvType,
		basic_color_t ColorType
	>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_vertex<PositionType, UvType, ColorType> final
	{
		using position_type = PositionType;
		using uv_type = UvType;
		using color_type = ColorType;

		using position_value_type = typename position_type::value_type;
		using uv_value_type = typename uv_type::value_type;
		using color_value_type = typename color_type::value_type;

		constexpr static std::size_t element_size{3};
		template<std::size_t Index>
			requires(Index < element_size)
		using element_type = std::conditional_t<Index == 0, position_type, std::conditional_t<Index == 1, uv_type, color_type>>;

		constexpr static uv_type default_uv{};

		position_type position;
		uv_type uv;
		color_type color;

		constexpr basic_vertex(const position_type position, const uv_type uv, const color_type color) noexcept
			: position{position},
			  uv{uv},
			  color{color} {}

		constexpr basic_vertex(const position_type position, const color_type color) noexcept
			: basic_vertex{position, default_uv, color} {}

		template<std::size_t Index>
			requires(Index < element_size)
		[[nodiscard]] constexpr auto get() const noexcept -> std::add_const_t<std::add_lvalue_reference_t<element_type<Index>>>
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return uv; }
			else if constexpr (Index == 2) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < element_size)
		[[nodiscard]] constexpr auto get() noexcept -> std::add_lvalue_reference_t<element_type<Index>>
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return uv; }
			else if constexpr (Index == 2) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};

	template<
		basic_point_t PositionType,
		basic_color_t ColorType
	>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_vertex<PositionType, void, ColorType> final
	{
		using position_type = PositionType;
		using color_type = ColorType;

		using position_value_type = typename position_type::value_type;
		using color_value_type = typename color_type::value_type;

		constexpr static std::size_t element_size{2};

		position_type position;
		color_type color;

		constexpr basic_vertex(const position_type position, const color_type color) noexcept
			: position{position},
			  color{color} {}

		template<std::size_t Index>
			requires(Index < element_size)
		[[nodiscard]] constexpr auto get() const noexcept
			-> std::conditional_t<
				Index == 0,
				const position_type&,
				const color_type&>
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < element_size)
		[[nodiscard]] constexpr auto get() noexcept
			-> std::conditional_t<
				Index == 0,
				position_type&,
				color_type&>
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
{
	template<std::size_t Index, typename PositionType, typename UvType, typename ColorType>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_vertex<PositionType, UvType, ColorType>> // NOLINT(cert-dcl58-cpp)
	{
		using type = typename gal::prometheus::primitive::basic_vertex<PositionType, UvType, ColorType>::template element_type<Index>;
	};

	template<typename PositionType, typename UvType, typename ColorType>
	struct tuple_size<gal::prometheus::primitive::basic_vertex<PositionType, UvType, ColorType>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, gal::prometheus::primitive::basic_vertex<PositionType, UvType, ColorType>::element_size> {};

	template<typename PositionType, typename UvType, typename ColorType>
	struct formatter<gal::prometheus::primitive::basic_vertex<PositionType, UvType, ColorType>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(
			const gal::prometheus::primitive::basic_vertex<PositionType, UvType, ColorType>& vertex,
			FormatContext& context
		) const noexcept -> auto
		{
			if constexpr (gal::prometheus::primitive::basic_vertex<PositionType, UvType, ColorType>::element_size == 2)
			{
				return std::format_to(context.out(), "[pos{}color{}]", vertex.position, vertex.color);
			}
			else if constexpr (gal::prometheus::primitive::basic_vertex<PositionType, UvType, ColorType>::element_size == 3)
			{
				return std::format_to(context.out(), "[pos{}uv{}color{}]", vertex.position, vertex.uv, vertex.color);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
	};
}
