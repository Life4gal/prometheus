// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

export module gal.prometheus.gui:font.impl;

import std;

import :font;

#else
#pragma once

#include <prometheus/macro.hpp>
#include <gui/font.ixx>

#include <ft2build.h>
#include FT_FREETYPE_H

#endif

namespace
{
	using namespace gal::prometheus::gui;

	[[nodiscard]] auto calculate_texture_size(const FT_Face ft_face) noexcept -> unsigned int
	{
		unsigned int total_area = 0;
		unsigned int max_width = 0;
		unsigned int max_height = 0;

		for (unsigned int c = 0; c < bitmap_font_type::glyphs_count; ++c)
		{
			if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER))
			{
				continue;
			}
			const auto& g = ft_face->glyph;

			total_area += (g->bitmap.width + 1) * (g->bitmap.rows + 1); // Add padding
			if (g->bitmap.width > max_width)
			{
				max_width = g->bitmap.width;
			}
			if (g->bitmap.rows > max_height)
			{
				max_height = g->bitmap.rows;
			}
		}

		// Ensure the texture size is large enough to accommodate the largest glyph
		auto side = static_cast<unsigned int>(std::sqrt(total_area));
		if (side < max_width)
		{
			side = max_width;
		}
		if (side < max_height)
		{
			side = max_height;
		}

		// Round up to the next power of 2 for better texture alignment
		unsigned int texture_size = 1;
		while (texture_size < side)
		{
			texture_size *= 2;
		}

		return texture_size;
	}
}

namespace gal::prometheus::gui
{
	[[nodiscard]] auto load_font(const std::string_view font_path, const unsigned int pixel_height) noexcept -> bitmap_font_type
	{
		FT_Library ft_library;
		if (FT_Init_FreeType(&ft_library))
		{
			// Could not initialize FreeType library
			return bitmap_font_type{};
		}

		FT_Face ft_face;
		if (FT_New_Face(ft_library, font_path.data(), 0, &ft_face))
		{
			FT_Done_FreeType(ft_library);
			// Could not load font
			return bitmap_font_type{};
		}

		FT_Set_Pixel_Sizes(ft_face, 0, pixel_height);

		const auto size = calculate_texture_size(ft_face);
		const auto atlas_width = size;
		const auto atlas_height = size;

		bitmap_font_type font
		{
				.pixel_height = static_cast<float>(pixel_height),
				.texture_size = {static_cast<float>(atlas_width), static_cast<float>(atlas_height)},
				.texture_data = std::make_unique<std::uint32_t[]>(atlas_width * atlas_height),
				.glyphs = {},
		};

		unsigned int pen_x = 0;
		unsigned int pen_y = 0;
		unsigned int max_row_height = 0;
		for (unsigned int i = 0; i < bitmap_font_type::glyphs_count; ++i)
		{
			if (FT_Load_Char(ft_face, i, FT_LOAD_RENDER))
			{
				continue;
			}

			const auto& g = ft_face->glyph;

			if (pen_x + g->bitmap.width >= atlas_width)
			{
				pen_x = 0;
				pen_y += max_row_height;
				max_row_height = 0;
			}

			if (pen_y + g->bitmap.rows >= atlas_height)
			{
				// Texture atlas is too small
				FT_Done_Face(ft_face);
				FT_Done_FreeType(ft_library);

				return bitmap_font_type{};
			}

			for (unsigned int y = 0; y < g->bitmap.rows; ++y)
			{
				for (unsigned int x = 0; x < g->bitmap.width; ++x)
				{
					const auto index = pen_x + x + (pen_y + y) * atlas_width;
					font.texture_data[index] =
							// A
							g->bitmap.buffer[x + y * g->bitmap.pitch] << 24 |
							// B
							std::uint32_t{255} << 16 |
							// G
							std::uint32_t{255} << 8 |
							// R
							std::uint32_t{255};
				}
			}

			font.glyphs[i] = {
					.rect =
					{
							glyph_type::rect_type::point_type
							{
									static_cast<float>(g->bitmap_left),
									static_cast<float>(g->bitmap_top)
							},
							glyph_type::rect_type::extent_type
							{
									static_cast<float>(g->bitmap.width),
									static_cast<float>(g->bitmap.rows)
							}
					},
					.uv = {
							glyph_type::rect_type::point_type
							{
									static_cast<float>(pen_x) / static_cast<float>(atlas_width),
									static_cast<float>(pen_y) / static_cast<float>(atlas_height)
							},
							glyph_type::rect_type::extent_type
							{
									static_cast<float>(g->bitmap.width) / static_cast<float>(atlas_width),
									static_cast<float>(g->bitmap.rows) / static_cast<float>(atlas_height)
							}
					},
					.advance_x = g->advance.x / 64.f
			};

			pen_x += g->bitmap.width;
			if (g->bitmap.rows > max_row_height)
			{
				max_row_height = g->bitmap.rows;
			}
		}

		FT_Done_Face(ft_face);
		FT_Done_FreeType(ft_library);

		return font;
	}
}
