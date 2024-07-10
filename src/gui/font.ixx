// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

export module gal.prometheus.primitive:font;

import std;
import gal.prometheus.primitive;

#else
#pragma once

#include <memory>
#include <string>
#include <array>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::gui)
{
	struct glyph_type
	{
		using rect_type = primitive::basic_rect<float, 2>;

		rect_type rect;
		rect_type uv;
		float advance_x;
	};

	struct bitmap_font_type
	{
		using extent_type = primitive::basic_extent<float, 2>;

		// todo: ascii only
		constexpr static std::size_t glyphs_count = 128;

		float pixel_height;

		extent_type texture_size;
		// texture_size.width * texture_size.height (RGBA)
		std::unique_ptr<std::uint32_t[]> texture_data;

		std::array<glyph_type, glyphs_count> glyphs;
	};

	[[nodiscard]] auto load_font(std::string_view font_path, unsigned int pixel_height) noexcept -> bitmap_font_type;
}
