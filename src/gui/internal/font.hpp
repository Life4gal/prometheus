// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <unordered_map>

#include <gui/internal/common.hpp>

namespace gal::prometheus::gui::internal
{
	class Font final
	{
	public:
		using value_type = FontOption::value_type;
		using char_type = FontOption::char_type;

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH
		// Variable '%1$s' is uninitialized. Always initialize a member variable (type.6).
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING(26495)
		#endif

		struct glyph_type
		{
			rect_type rect;
			rect_type uv;
			float advance_x;
		};

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_POP
		#endif

		using glyphs_type = std::unordered_map<char_type, glyph_type>;

		using glyph_value_type = FontOption::glyph_value_type;
		using glyph_ranges_type = FontOption::glyph_ranges_type;

		using baked_line_uv_type = std::vector<rect_type>;

		constexpr static texture_id_type invalid_texture_id = 0;

		// Line breaks only when a line break ('\n') is encountered (this means that text beyond the content area will be clipped)
		constexpr static value_type no_auto_wrap = 99999999.f;

		std::string font_path;
		std::uint32_t pixel_height;
		std::uint32_t baked_line_max_width;

		glyphs_type glyphs;
		glyph_type fallback_glyph;

		value_type scale;
		extent_type display_offset;
		point_type white_pixel_uv;
		baked_line_uv_type baked_line_uv;

		texture_id_type texture_id;

		Font() noexcept;

		auto load(const FontOption& option) noexcept -> Texture;

		// ---------------------------------------------------------

		[[nodiscard]] auto loaded() const noexcept -> bool;
	};

	// =========================================
	// DRAW TEXT

	[[nodiscard]] auto text_size(
		const Font& font,
		std::basic_string_view<char> utf8_text,
		float font_size,
		float wrap_width,
		std::basic_string<Font::char_type>& out_text
	) noexcept -> extent_type;

	[[nodiscard]] auto text_size(
		const Font& font,
		std::basic_string_view<char> utf8_text,
		float font_size,
		float wrap_width
	) noexcept -> extent_type;

	auto text_draw(
		const Font& font,
		std::basic_string_view<char> utf8_text,
		float font_size,
		float wrap_width,
		point_type point,
		Theme::color_type color,
		PrimitiveAppender appender
	) noexcept -> void;
}
