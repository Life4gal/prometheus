// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <tuple>
#include <utility>
#include <format>

#include <prometheus/macro.hpp>

#include <primitive/point.hpp>
#include <primitive/color.hpp>

namespace gal::prometheus::primitive
{
	template<typename PositionType, typename UvType, typename ColorType>
	struct basic_vertex final
	{
		using position_type = PositionType;
		using uv_type = UvType;
		using color_type = ColorType;

		using position_value_type = typename position_type::value_type;
		using uv_value_type = typename uv_type::value_type;
		using color_value_type = typename color_type::value_type;

		template<std::size_t Index>
		using element_type = std::conditional_t<Index == 0, position_type, std::conditional_t<Index == 1, uv_type, color_type>>;

		position_type position;
		uv_type uv;
		color_type color;

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() const noexcept -> const element_type<Index>&
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return uv; }
			else if constexpr (Index == 2) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() noexcept -> element_type<Index>&
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return uv; }
			else if constexpr (Index == 2) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};

	template<typename PositionType, typename ColorType>
	struct basic_vertex<PositionType, void, ColorType> final
	{
		using position_type = PositionType;
		using color_type = ColorType;

		using position_value_type = typename position_type::value_type;
		using color_value_type = typename color_type::value_type;

		template<std::size_t Index>
		using element_type = std::conditional_t<Index == 0, position_type, color_type>;

		position_type position;
		color_type color;

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> const element_type<Index>&
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> element_type<Index>&
		{
			if constexpr (Index == 0) { return position; }
			else if constexpr (Index == 1) { return color; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};
}

namespace std
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
			: std::integral_constant<std::size_t, 2 + std::is_same_v<UvType, void> ? 0 : 1> {};

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
			using vertex_type = gal::prometheus::primitive::basic_vertex<PositionType, UvType, ColorType>;

			if constexpr (std::tuple_size_v<vertex_type> == 2)
			{
				return std::format_to(context.out(), "[pos{}color{}]", vertex.position, vertex.color);
			}
			else if constexpr (std::tuple_size_v<vertex_type> == 3)
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
