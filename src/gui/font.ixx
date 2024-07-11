// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:font;

import std;
import gal.prometheus.primitive;

#else
#pragma once

#include <type_traits>
#include <span>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::gui)
{
	struct standard_glyph_pack_type
	{
		primitive::basic_rect<float, 2> rect;
		primitive::basic_rect<float, 2> uv;
		float advance_x;
	};

	template<typename>
	struct is_glyph_type : std::false_type {};

	template<typename GlyphType>
		requires requires(const GlyphType& glyph)
		{
			// offset
			{ glyph.offset_x } -> std::convertible_to<float>;
			{ glyph.offset_y } -> std::convertible_to<float>;
			// size
			{ glyph.width } -> std::convertible_to<float>;
			{ glyph.height } -> std::convertible_to<float>;
			// uv_offset
			{ glyph.uv_offset_x } -> std::convertible_to<float>;
			{ glyph.uv_offset_y } -> std::convertible_to<float>;
			// uv_size
			{ glyph.uv_width } -> std::convertible_to<float>;
			{ glyph.uv_height } -> std::convertible_to<float>;
			// advance
			{ glyph.advance_x } -> std::convertible_to<float>;
		}
	struct is_glyph_type<GlyphType> : std::true_type
	{
		[[nodiscard]] constexpr static auto pack(const GlyphType& glyph) noexcept -> standard_glyph_pack_type
		{
			return
			{
					.rect = {primitive::basic_point<float, 2>{glyph.offset_x, glyph.offset_y}, primitive::basic_extent<float, 2>{glyph.width, glyph.height}},
					.uv = {primitive::basic_point<float, 2>{glyph.uv_offset_x, glyph.uv_offset_y}, primitive::basic_extent<float, 2>{glyph.uv_width, glyph.uv_height}},
					.advance_x = glyph.advance_x
			};
		}
	};

	template<typename GlyphType>
		requires requires(const GlyphType& glyph)
		{
			// offset
			std::is_constructible_v<primitive::basic_point<float, 2>, std::decay_t<decltype(glyph.offset)>>;
			// size
			std::is_constructible_v<primitive::basic_extent<float, 2>, std::decay_t<decltype(glyph.size)>>;
			// uv_offset
			std::is_constructible_v<primitive::basic_point<float, 2>, std::decay_t<decltype(glyph.uv_offset)>>;
			// uv_size
			std::is_constructible_v<primitive::basic_extent<float, 2>, std::decay_t<decltype(glyph.uv_size)>>;
			// advance
			{ glyph.advance_x } -> std::convertible_to<float>;
		}
	struct is_glyph_type<GlyphType> : std::true_type
	{
		[[nodiscard]] constexpr static auto pack(const GlyphType& glyph) noexcept -> standard_glyph_pack_type
		{
			return
			{
					.rect = {glyph.offset, glyph.size},
					.uv = {glyph.uv_offset, glyph.uv_size},
					.advance_x = glyph.advance_x
			};
		}
	};

	template<typename GlyphType>
		requires requires(const GlyphType& glyph)
		{
			// offset + size
			std::is_constructible_v<primitive::basic_rect<float, 2>, std::decay_t<decltype(glyph.rect)>>;
			// uv_offset + uv_size
			std::is_constructible_v<primitive::basic_rect<float, 2>, std::decay_t<decltype(glyph.uv)>>;
			// advance
			{ glyph.advance_x } -> std::convertible_to<float>;
		}
	struct is_glyph_type<GlyphType> : std::true_type
	{
		[[nodiscard]] constexpr static auto pack(const GlyphType& glyph) noexcept -> standard_glyph_pack_type
		{
			return
			{
					.rect = glyph.rect,
					.uv = glyph.uv,
					.advance_x = glyph.advance_x
			};
		}
	};

	template<typename GlyphType>
	constexpr auto is_glyph_type_v = is_glyph_type<GlyphType>::value;

	template<typename GlyphType>
	concept glyph_type_t = is_glyph_type_v<GlyphType>;

	template<typename>
	struct is_font_type : std::false_type {};

	template<typename FontType>
		requires requires(const FontType& font)
		{
			// pixel_height
			{ font.pixel_height } -> std::convertible_to<float>;
			// texture
			{ font.texture_width } -> std::convertible_to<float>;
			{ font.texture_height } -> std::convertible_to<float>;
			std::is_constructible_v<std::span<std::uint32_t>, std::decay_t<decltype(font.texture_data)>>;
			// glyphs
			{
				font.glyphs.find(std::declval<typename FontType::char_type>()).operator*().second
			} -> std::same_as<const typename FontType::glyph_type&>;
			{
				font.default_glyph
			} -> std::same_as<const typename FontType::glyph_type&>;
		}
	struct is_font_type<FontType> : std::true_type {};

	template<typename FontType>
		requires requires(const FontType& font)
		{
			// pixel_height
			{ font.pixel_height } -> std::convertible_to<float>;
			// texture
			std::is_constructible_v<primitive::basic_extent<float, 2>, std::decay_t<decltype(font.texture_size)>>;
			std::is_constructible_v<std::span<std::uint32_t>, std::decay_t<decltype(font.texture_data)>>;
			// glyphs
			{
				font.glyphs.find(std::declval<typename FontType::char_type>()).operator*().second
			} -> std::same_as<const typename FontType::glyph_type&>;
			{
				font.default_glyph
			} -> std::same_as<const typename FontType::glyph_type&>;
		}
	struct is_font_type<FontType> : std::true_type {};

	template<typename FontType>
	constexpr auto is_font_type_v = is_font_type<FontType>::value;

	template<typename FontType>
	concept font_type_t = is_font_type_v<FontType>;
}
