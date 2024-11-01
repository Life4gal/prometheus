// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.font;

import std;

import :primitive;

import :draw.def;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>
#include <span>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>

#include <draw/def.ixx>

#endif

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: draw
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
#endif
{
	class DrawList;

	class Font final
	{
	public:
		using rect_type = FontType::rect_type;
		using point_type = FontType::point_type;
		using extent_type = FontType::extent_type;

		using uv_rect_type = FontType::uv_rect_type;
		using uv_point_type = FontType::uv_point_type;
		using uv_extent_type = FontType::uv_extent_type;

		using char_type = FontType::char_type;

		using texture_id_type = FontType::texture_id_type;

		using glyph_type = FontType::glyph_type;
		using glyphs_type = FontType::glyphs_type;

		using glyph_value_type = FontGlyphRangeBuilder::glyph_value_type;
		using glyph_pair_type = FontGlyphRangeBuilder::glyph_pair_type;
		using glyph_range_type = FontGlyphRangeBuilder::glyph_range_type;

		using texture_type = FontType::texture_type;

		using baked_line_uv_type = FontType::baked_line_uv_type;

		constexpr static texture_id_type invalid_texture_id = 0;

	private:
		// FontOption
		std::string font_path_;
		std::uint32_t pixel_height_;
		std::uint32_t baked_line_max_width_;

		glyphs_type glyphs_;
		glyph_type fallback_glyph_;

		uv_point_type white_pixel_uv_;
		baked_line_uv_type baked_line_uv_;

		texture_id_type texture_id_;

		auto reset() noexcept -> void;

	public:
		explicit Font() noexcept;

		// ---------------------------------------------------------

		auto load(const FontOption& option) noexcept -> texture_type;

		// ---------------------------------------------------------

		[[nodiscard]] auto loaded() const noexcept -> bool;

		[[nodiscard]] auto font_path() const noexcept -> std::string_view;

		[[nodiscard]] auto pixel_height() const noexcept -> std::uint32_t;

		[[nodiscard]] auto baked_line_max_width() const noexcept -> std::uint32_t;

		[[nodiscard]] auto glyphs() const noexcept -> const glyphs_type&;

		[[nodiscard]] auto fallback_glyph() const noexcept -> const glyph_type&;

		[[nodiscard]] auto white_pixel_uv() const noexcept -> const uv_point_type&;

		[[nodiscard]] auto baked_line_uv() const noexcept -> const baked_line_uv_type&;

		[[nodiscard]] auto texture_id() const noexcept -> texture_id_type;

		// =========================================
		// DRAW TEXT

		[[nodiscard]] auto text_size(
			std::string_view utf8_text,
			float font_size,
			float wrap_width,
			std::basic_string<char_type>& out_text
		) const noexcept -> DrawListType::extent_type;

		[[nodiscard]] auto text_size(
			std::string_view utf8_text,
			float font_size,
			float wrap_width
		) const noexcept -> DrawListType::extent_type;

		auto draw_text(
			DrawList& draw_list,
			float font_size,
			const DrawListType::point_type& p,
			const DrawListType::color_type& color,
			std::string_view utf8_text,
			float wrap_width
		) const noexcept -> void;
	};
}
