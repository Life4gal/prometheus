// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:vertex;

import std;
import :point;
import :color;

export namespace gal::prometheus::primitive
{
	template<
		basic_point_t PointType,
		typename UvType = basic_point<typename PointType::value_type, 2>,
		typename ColorValueType = std::uint8_t>
		requires std::is_arithmetic_v<ColorValueType>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_vertex final
	{
		using point_type = PointType;
		using point_value_type = typename point_type::value_type;

		using uv_type = UvType;
		using color_type = basic_color<ColorValueType>;

		constexpr static auto is_always_equal = true;
		constexpr static auto default_uv = uv_type{0, 0};

		point_type position;
		uv_type uv;
		color_type color;

		constexpr basic_vertex(const point_type position, const uv_type uv, const color_type color) noexcept
			: position{position},
			  uv{uv},
			  color{color} {}

		constexpr basic_vertex(const point_type position, const color_type color) noexcept
			: basic_vertex{position, default_uv, color} {}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() const noexcept
			-> std::conditional_t<
				Index == 0,
				point_type,
				std::conditional_t<
					Index == 1,
					uv_type,
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
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() noexcept
			-> std::conditional_t<
				Index == 0,
				point_type,
				std::conditional_t<
					Index == 1,
					uv_type,
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

	template<typename>
	struct is_basic_vertex : std::false_type {};

	template<
		basic_point_t PointType,
		typename UvType,
		typename ColorValueType>
	struct is_basic_vertex<basic_vertex<PointType, UvType, ColorValueType>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_vertex_v = is_basic_vertex<T>::value;

	template<typename T>
	concept basic_vertex_t = is_basic_vertex_v<T>;
}

export namespace std
{
	template<std::size_t Index, typename PointType, typename UvType, typename ColorValueType>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_vertex<PointType, UvType, ColorValueType>> // NOLINT(cert-dcl58-cpp)
	{
		using type = std::conditional_t<
			Index == 0,
			typename gal::prometheus::primitive::basic_vertex<PointType, UvType, ColorValueType>::point_type,
			std::conditional_t<
				Index == 1,
				typename gal::prometheus::primitive::basic_vertex<PointType, UvType, ColorValueType>::uv_type,
				typename gal::prometheus::primitive::basic_vertex<PointType, UvType, ColorValueType>::color_type
			>
		>;
	};

	template<typename PointType, typename UvType, typename ColorValueType>
	struct tuple_size<gal::prometheus::primitive::basic_vertex<PointType, UvType, ColorValueType>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 3> {};

	template<typename PointType, typename UvType, typename ColorValueType>
	struct formatter<gal::prometheus::primitive::basic_vertex<PointType, UvType, ColorValueType>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(
				const gal::prometheus::primitive::basic_vertex<PointType, UvType, ColorValueType>& vertex,
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
}
