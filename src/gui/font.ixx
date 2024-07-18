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
#include <vector>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>

#endif

namespace gal::prometheus::gui
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	using glyph_range_value_type = std::uint32_t;
	using glyph_range_type = std::pair<glyph_range_value_type, glyph_range_value_type>;
	using glyph_ranges_type = std::vector<glyph_range_type>;
	using glyph_ranges_view_type = std::span<const glyph_range_type>;

	// Latin
	[[nodiscard]] auto glyph_range_latin() noexcept -> glyph_ranges_view_type;

	// Latin + Greek and Coptic
	[[nodiscard]] auto glyph_range_greek() noexcept -> glyph_ranges_view_type;

	// Latin + Half-Width + Japanese Hiragana/Katakana + set of 2500 CJK Unified Ideographs for common simplified Chinese
	[[nodiscard]] auto glyph_range_simplified_chinese_common() noexcept -> glyph_ranges_view_type;

	// Latin + Half-Width + Japanese Hiragana/Katakana + full set of about 21000 CJK Unified Ideographs
	[[nodiscard]] auto glyph_range_simplified_chinese_all() noexcept -> glyph_ranges_view_type;

	struct glyph_type
	{
		using rect_type = primitive::basic_rect<std::uint32_t, 2>;
		using uv_type = primitive::basic_rect<float, 2>;

		rect_type rect;
		uv_type uv;
		float advance_x;
	};

	struct font_type
	{
		using extent_type = primitive::basic_extent<std::uint32_t, 2>;
		using char_type = char32_t;
		using glyph_type = glyph_type;

		float pixel_height;

		extent_type texture_size;
		// texture_size.width * texture_size.height (RGBA)
		std::unique_ptr<std::uint32_t[]> texture_data;

		std::unordered_map<char_type, glyph_type> glyphs;
		glyph_type fallback_glyph;
	};

	[[nodiscard]] auto load_font(std::string_view font_path, std::uint32_t pixel_height, glyph_ranges_view_type glyph_ranges) noexcept -> font_type;

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
