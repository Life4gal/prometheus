// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H          // <freetype/freetype.h>
// #include FT_MODULE_H            // <freetype/ftmodapi.h>
// #include FT_GLYPH_H             // <freetype/ftglyph.h>
// #include FT_SYNTHESIS_H         // <freetype/ftsynth.h>

export module gal.prometheus.gui:font.impl;

import std;

import :font;

#else
#pragma once

#include <algorithm>
#include <ranges>
#include <numeric>

#include <ft2build.h>
#include FT_FREETYPE_H          // <freetype/freetype.h>
// #include FT_MODULE_H            // <freetype/ftmodapi.h>
// #include FT_GLYPH_H             // <freetype/ftglyph.h>
// #include FT_SYNTHESIS_H         // <freetype/ftsynth.h>

#include <prometheus/macro.hpp>
#include <gui/font.ixx>

#endif

namespace
{
	using namespace gal::prometheus::gui;

	// ReSharper disable once CppInconsistentNaming
	constexpr glyph_range_value_type simplified_chinese_common_accumulative_offsets_from_0x4e00[]
	{
			0, 1, 2, 4, 1, 1, 1, 1, 2, 1, 3, 2, 1, 2, 2, 1, 1, 1, 1, 1, 5, 2, 1, 2, 3, 3, 3, 2, 2, 4, 1, 1, 1, 2, 1, 5, 2, 3, 1, 2, 1, 2,
			1, 1, 2, 1, 1, 2, 2, 1, 4, 1, 1, 1, 1, 5, 10, 1, 2, 19, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 5, 1, 6, 3, 2, 1, 2, 2, 1, 1, 1, 4, 8, 5,
			1, 1, 4, 1, 1, 3, 1, 2, 1, 5, 1, 2, 1, 1, 1, 10, 1, 1, 5, 2, 4, 6, 1, 4, 2, 2, 2, 12, 2, 1, 1, 6, 1, 1, 1, 4, 1, 1, 4, 6, 5, 1,
			4, 2, 2, 4, 10, 7, 1, 1, 4, 2, 4, 2, 1, 4, 3, 6, 10, 12, 5, 7, 2, 14, 2, 9, 1, 1, 6, 7, 10, 4, 7, 13, 1, 5, 4, 8, 4, 1, 1, 2, 28, 5,
			6, 1, 1, 5, 2, 5, 20, 2, 2, 9, 8, 11, 2, 9, 17, 1, 8, 6, 8, 27, 4, 6, 9, 20, 11, 27, 6, 68, 2, 2, 1, 1, 1, 2, 1, 2, 2, 7, 6, 11, 3, 3,
			1, 1, 3, 1, 2, 1, 1, 1, 1, 1, 3, 1, 1, 8, 3, 4, 1, 5, 7, 2, 1, 4, 4, 8, 4, 2, 1, 2, 1, 1, 4, 5, 6, 3, 6, 2, 12, 3, 1, 3, 9, 2,
			4, 3, 4, 1, 5, 3, 3, 1, 3, 7, 1, 5, 1, 1, 1, 1, 2, 3, 4, 5, 2, 3, 2, 6, 1, 1, 2, 1, 7, 1, 7, 3, 4, 5, 15, 2, 2, 1, 5, 3, 22, 19,
			2, 1, 1, 1, 1, 2, 5, 1, 1, 1, 6, 1, 1, 12, 8, 2, 9, 18, 22, 4, 1, 1, 5, 1, 16, 1, 2, 7, 10, 15, 1, 1, 6, 2, 4, 1, 2, 4, 1, 6, 1, 1,
			3, 2, 4, 1, 6, 4, 5, 1, 2, 1, 1, 2, 1, 10, 3, 1, 3, 2, 1, 9, 3, 2, 5, 7, 2, 19, 4, 3, 6, 1, 1, 1, 1, 1, 4, 3, 2, 1, 1, 1, 2, 5,
			3, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1, 2, 1, 3, 1, 1, 1, 3, 7, 1, 4, 1, 1, 2, 1, 1, 2, 1, 2, 4, 4, 3, 8, 1, 1, 1, 2, 1, 3, 5, 1, 3,
			1, 3, 4, 6, 2, 2, 14, 4, 6, 6, 11, 9, 1, 15, 3, 1, 28, 5, 2, 5, 5, 3, 1, 3, 4, 5, 4, 6, 14, 3, 2, 3, 5, 21, 2, 7, 20, 10, 1, 2, 19, 2,
			4, 28, 28, 2, 3, 2, 1, 14, 4, 1, 26, 28, 42, 12, 40, 3, 52, 79, 5, 14, 17, 3, 2, 2, 11, 3, 4, 6, 3, 1, 8, 2, 23, 4, 5, 8, 10, 4, 2, 7, 3, 5,
			1, 1, 6, 3, 1, 2, 2, 2, 5, 28, 1, 1, 7, 7, 20, 5, 3, 29, 3, 17, 26, 1, 8, 4, 27, 3, 6, 11, 23, 5, 3, 4, 6, 13, 24, 16, 6, 5, 10, 25, 35, 7,
			3, 2, 3, 3, 14, 3, 6, 2, 6, 1, 4, 2, 3, 8, 2, 1, 1, 3, 3, 3, 4, 1, 1, 13, 2, 2, 4, 5, 2, 1, 14, 14, 1, 2, 2, 1, 4, 5, 2, 3, 1, 14,
			3, 12, 3, 17, 2, 16, 5, 1, 2, 1, 8, 9, 3, 19, 4, 2, 2, 4, 17, 25, 21, 20, 28, 75, 1, 10, 29, 103, 4, 1, 2, 1, 1, 4, 2, 4, 1, 2, 3, 24, 2, 2,
			2, 1, 1, 2, 1, 3, 8, 1, 1, 1, 2, 1, 1, 3, 1, 1, 1, 6, 1, 5, 3, 1, 1, 1, 3, 4, 1, 1, 5, 2, 1, 5, 6, 13, 9, 16, 1, 1, 1, 1, 3, 2,
			3, 2, 4, 5, 2, 5, 2, 2, 3, 7, 13, 7, 2, 2, 1, 1, 1, 1, 2, 3, 3, 2, 1, 6, 4, 9, 2, 1, 14, 2, 14, 2, 1, 18, 3, 4, 14, 4, 11, 41, 15, 23,
			15, 23, 176, 1, 3, 4, 1, 1, 1, 1, 5, 3, 1, 2, 3, 7, 3, 1, 1, 2, 1, 2, 4, 4, 6, 2, 4, 1, 9, 7, 1, 10, 5, 8, 16, 29, 1, 1, 2, 2, 3, 1,
			3, 5, 2, 4, 5, 4, 1, 1, 2, 2, 3, 3, 7, 1, 6, 10, 1, 17, 1, 44, 4, 6, 2, 1, 1, 6, 5, 4, 2, 10, 1, 6, 9, 2, 8, 1, 24, 1, 2, 13, 7, 8,
			8, 2, 1, 4, 1, 3, 1, 3, 3, 5, 2, 5, 10, 9, 4, 9, 12, 2, 1, 6, 1, 10, 1, 1, 7, 7, 4, 10, 8, 3, 1, 13, 4, 3, 1, 6, 1, 3, 5, 2, 1, 2,
			17, 16, 5, 2, 16, 6, 1, 4, 2, 1, 3, 3, 6, 8, 5, 11, 11, 1, 3, 3, 2, 4, 6, 10, 9, 5, 7, 4, 7, 4, 7, 1, 1, 4, 2, 1, 3, 6, 8, 7, 1, 6,
			11, 5, 5, 3, 24, 9, 4, 2, 7, 13, 5, 1, 8, 82, 16, 61, 1, 1, 1, 4, 2, 2, 16, 10, 3, 8, 1, 1, 6, 4, 2, 1, 3, 1, 1, 1, 4, 3, 8, 4, 2, 2,
			1, 1, 1, 1, 1, 6, 3, 5, 1, 1, 4, 6, 9, 2, 1, 1, 1, 2, 1, 7, 2, 1, 6, 1, 5, 4, 4, 3, 1, 8, 1, 3, 3, 1, 3, 2, 2, 2, 2, 3, 1, 6,
			1, 2, 1, 2, 1, 3, 7, 1, 8, 2, 1, 2, 1, 5, 2, 5, 3, 5, 10, 1, 2, 1, 1, 3, 2, 5, 11, 3, 9, 3, 5, 1, 1, 5, 9, 1, 2, 1, 5, 7, 9, 9,
			8, 1, 3, 3, 3, 6, 8, 2, 3, 2, 1, 1, 32, 6, 1, 2, 15, 9, 3, 7, 13, 1, 3, 10, 13, 2, 14, 1, 13, 10, 2, 1, 3, 10, 4, 15, 2, 15, 15, 10, 1, 3,
			9, 6, 9, 32, 25, 26, 47, 7, 3, 2, 3, 1, 6, 3, 4, 3, 2, 8, 5, 4, 1, 9, 4, 2, 2, 19, 10, 6, 2, 3, 8, 1, 2, 2, 4, 2, 1, 9, 4, 4, 4, 6,
			4, 8, 9, 2, 3, 1, 1, 1, 1, 3, 5, 5, 1, 3, 8, 4, 6, 2, 1, 4, 12, 1, 5, 3, 7, 13, 2, 5, 8, 1, 6, 1, 2, 5, 14, 6, 1, 5, 2, 4, 8, 15,
			5, 1, 23, 6, 62, 2, 10, 1, 1, 8, 1, 2, 2, 10, 4, 2, 2, 9, 2, 1, 1, 3, 2, 3, 1, 5, 3, 3, 2, 1, 3, 8, 1, 1, 1, 11, 3, 1, 1, 4, 3, 7,
			1, 14, 1, 2, 3, 12, 5, 2, 5, 1, 6, 7, 5, 7, 14, 11, 1, 3, 1, 8, 9, 12, 2, 1, 11, 8, 4, 4, 2, 6, 10, 9, 13, 1, 1, 3, 1, 5, 1, 3, 2, 4,
			4, 1, 18, 2, 3, 14, 11, 4, 29, 4, 2, 7, 1, 3, 13, 9, 2, 2, 5, 3, 5, 20, 7, 16, 8, 5, 72, 34, 6, 4, 22, 12, 12, 28, 45, 36, 9, 7, 39, 9, 191, 1,
			1, 1, 4, 11, 8, 4, 9, 2, 3, 22, 1, 1, 1, 1, 4, 17, 1, 7, 7, 1, 11, 31, 10, 2, 4, 8, 2, 3, 2, 1, 4, 2, 16, 4, 32, 2, 3, 19, 13, 4, 9, 1,
			5, 2, 14, 8, 1, 1, 3, 6, 19, 6, 5, 1, 16, 6, 2, 10, 8, 5, 1, 2, 3, 1, 5, 5, 1, 11, 6, 6, 1, 3, 3, 2, 6, 3, 8, 1, 1, 4, 10, 7, 5, 7,
			7, 5, 8, 9, 2, 1, 3, 4, 1, 1, 3, 1, 3, 3, 2, 6, 16, 1, 4, 6, 3, 1, 10, 6, 1, 3, 15, 2, 9, 2, 10, 25, 13, 9, 16, 6, 2, 2, 10, 11, 4, 3,
			9, 1, 2, 6, 6, 5, 4, 30, 40, 1, 10, 7, 12, 14, 33, 6, 3, 6, 7, 3, 1, 3, 1, 11, 14, 4, 9, 5, 12, 11, 49, 18, 51, 31, 140, 31, 2, 2, 1, 5, 1, 8,
			1, 10, 1, 4, 4, 3, 24, 1, 10, 1, 3, 6, 6, 16, 3, 4, 5, 2, 1, 4, 2, 57, 10, 6, 22, 2, 22, 3, 7, 22, 6, 10, 11, 36, 18, 16, 33, 36, 2, 5, 5, 1,
			1, 1, 4, 10, 1, 4, 13, 2, 7, 5, 2, 9, 3, 4, 1, 7, 43, 3, 7, 3, 9, 14, 7, 9, 1, 11, 1, 1, 3, 7, 4, 18, 13, 1, 14, 1, 3, 6, 10, 73, 2, 2,
			30, 6, 1, 11, 18, 19, 13, 22, 3, 46, 42, 37, 89, 7, 3, 16, 34, 2, 2, 3, 9, 1, 7, 1, 1, 1, 2, 2, 4, 10, 7, 3, 10, 3, 9, 5, 28, 9, 2, 6, 13, 7,
			3, 1, 3, 10, 2, 7, 2, 11, 3, 6, 21, 54, 85, 2, 1, 4, 2, 2, 1, 39, 3, 21, 2, 2, 5, 1, 1, 1, 4, 1, 1, 3, 4, 15, 1, 3, 2, 4, 4, 2, 3, 8,
			2, 20, 1, 8, 7, 13, 4, 1, 26, 6, 2, 9, 34, 4, 21, 52, 10, 4, 4, 1, 5, 12, 2, 11, 1, 7, 2, 30, 12, 44, 2, 30, 1, 1, 3, 6, 16, 9, 17, 39, 82, 2,
			2, 24, 7, 1, 7, 3, 16, 9, 14, 44, 2, 1, 2, 1, 2, 3, 5, 2, 4, 1, 6, 7, 5, 3, 2, 6, 1, 11, 5, 11, 2, 1, 18, 19, 8, 1, 3, 24, 29, 2, 1, 3,
			5, 2, 2, 1, 13, 6, 5, 1, 46, 11, 3, 5, 1, 1, 5, 8, 2, 10, 6, 12, 6, 3, 7, 11, 2, 4, 16, 13, 2, 5, 1, 1, 2, 2, 5, 2, 28, 5, 2, 23, 10, 8,
			4, 4, 22, 39, 95, 38, 8, 14, 9, 5, 1, 13, 5, 4, 3, 13, 12, 11, 1, 9, 1, 27, 37, 2, 5, 4, 4, 63, 211, 95, 2, 2, 2, 1, 3, 5, 2, 1, 1, 2, 2, 1,
			1, 1, 3, 2, 4, 1, 2, 1, 1, 5, 2, 2, 1, 1, 2, 3, 1, 3, 1, 1, 1, 3, 1, 4, 2, 1, 3, 6, 1, 1, 3, 7, 15, 5, 3, 2, 5, 3, 9, 11, 4, 2,
			22, 1, 6, 3, 8, 7, 1, 4, 28, 4, 16, 3, 3, 25, 4, 4, 27, 27, 1, 4, 1, 2, 2, 7, 1, 3, 5, 2, 28, 8, 2, 14, 1, 8, 6, 16, 25, 3, 3, 3, 14, 3,
			3, 1, 1, 2, 1, 4, 6, 3, 8, 4, 1, 1, 1, 2, 3, 6, 10, 6, 2, 3, 18, 3, 2, 5, 5, 4, 3, 1, 5, 2, 5, 4, 23, 7, 6, 12, 6, 4, 17, 11, 9, 5,
			1, 1, 10, 5, 12, 1, 1, 11, 26, 33, 7, 3, 6, 1, 17, 7, 1, 5, 12, 1, 11, 2, 4, 1, 8, 14, 17, 23, 1, 2, 1, 7, 8, 16, 11, 9, 6, 5, 2, 6, 4, 16,
			2, 8, 14, 1, 11, 8, 9, 1, 1, 1, 9, 25, 4, 11, 19, 7, 2, 15, 2, 12, 8, 52, 7, 5, 19, 2, 16, 4, 36, 8, 1, 16, 8, 24, 26, 4, 6, 2, 9, 5, 4, 36,
			3, 28, 12, 25, 15, 37, 27, 17, 12, 59, 38, 5, 32, 127, 1, 2, 9, 17, 14, 4, 1, 2, 1, 1, 8, 11, 50, 4, 14, 2, 19, 16, 4, 17, 5, 4, 5, 26, 12, 45, 2, 23,
			45, 104, 30, 12, 8, 3, 10, 2, 2, 3, 3, 1, 4, 20, 7, 2, 9, 6, 15, 2, 20, 1, 3, 16, 4, 11, 15, 6, 134, 2, 5, 59, 1, 2, 2, 2, 1, 9, 17, 3, 26, 137,
			10, 211, 59, 1, 2, 4, 1, 4, 1, 1, 1, 2, 6, 2, 3, 1, 1, 2, 3, 2, 3, 1, 3, 4, 4, 2, 3, 3, 1, 4, 3, 1, 7, 2, 2, 3, 1, 2, 1, 3, 3, 3,
			2, 2, 3, 2, 1, 3, 14, 6, 1, 3, 2, 9, 6, 15, 27, 9, 34, 145, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 2, 3, 1, 2, 1, 1, 1, 2, 3, 5,
			8, 3, 5, 2, 4, 1, 3, 2, 2, 2, 12, 4, 1, 1, 1, 10, 4, 5, 1, 20, 4, 16, 1, 15, 9, 5, 12, 2, 9, 2, 5, 4, 2, 26, 19, 7, 1, 26, 4, 30, 12, 15,
			42, 1, 6, 8, 172, 1, 1, 4, 2, 1, 1, 11, 2, 2, 4, 2, 1, 2, 1, 10, 8, 1, 2, 1, 4, 5, 1, 2, 5, 1, 8, 4, 1, 3, 4, 2, 1, 6, 2, 1, 3, 4,
			1, 2, 1, 1, 1, 1, 12, 5, 7, 2, 4, 3, 1, 1, 1, 3, 3, 6, 1, 2, 2, 3, 3, 3, 2, 1, 2, 12, 14, 11, 6, 6, 4, 12, 2, 8, 1, 7, 10, 1, 35, 7,
			4, 13, 15, 4, 3, 23, 21, 28, 52, 5, 26, 5, 6, 1, 7, 10, 2, 7, 53, 3, 2, 1, 1, 1, 2, 163, 532, 1, 10, 11, 1, 3, 3, 4, 8, 2, 8, 6, 2, 2, 23, 22,
			4, 2, 2, 4, 2, 1, 3, 1, 3, 3, 5, 9, 8, 2, 1, 2, 8, 1, 10, 2, 12, 21, 20, 15, 105, 2, 3, 1, 1, 3, 2, 3, 1, 1, 2, 5, 1, 4, 15, 11, 19, 1,
			1, 1, 1, 5, 4, 5, 1, 1, 2, 5, 3, 5, 12, 1, 2, 5, 1, 11, 1, 1, 15, 9, 1, 4, 5, 3, 26, 8, 2, 1, 3, 1, 1, 15, 19, 2, 12, 1, 2, 5, 2, 7,
			2, 19, 2, 20, 6, 26, 7, 5, 2, 2, 7, 34, 21, 13, 70, 2, 128, 1, 1, 2, 1, 1, 2, 1, 1, 3, 2, 2, 2, 15, 1, 4, 1, 3, 4, 42, 10, 6, 1, 49, 85, 8,
			1, 2, 1, 1, 4, 4, 2, 3, 6, 1, 5, 7, 4, 3, 211, 4, 1, 2, 1, 2, 5, 1, 2, 4, 2, 2, 6, 5, 6, 10, 3, 4, 48, 100, 6, 2, 16, 296, 5, 27, 387, 2,
			2, 3, 7, 16, 8, 5, 38, 15, 39, 21, 9, 10, 3, 7, 59, 13, 27, 21, 47, 5, 21, 6
	};

	[[nodiscard]] auto calculate_texture_size(const FT_Face ft_face, const glyph_ranges_view_type glyph_ranges) noexcept -> std::uint32_t
	{
		std::uint32_t total_area = 0;

		for (const auto [first, second]: glyph_ranges)
		{
			for (auto c = first; c <= second; ++c)
			{
				if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER))
				{
					continue;
				}
				const auto& g = ft_face->glyph;

				total_area += g->bitmap.width * g->bitmap.rows;
			}
		}

		const auto side = static_cast<std::uint32_t>(std::sqrt(total_area));
		return std::bit_ceil(side + 1);
	}
}

namespace gal::prometheus::gui
{
	[[nodiscard]] auto glyph_range_latin() noexcept -> glyph_ranges_view_type
	{
		constexpr static std::array<glyph_range_type, 1> range{{
				{0x0020, 0x00ff}, // Basic Latin + Latin Supplement
		}};

		return range;
	}

	[[nodiscard]] auto glyph_range_greek() noexcept -> glyph_ranges_view_type
	{
		constexpr static std::array<glyph_range_type, 2> range{{
				{0x0020, 0x00ff}, // Basic Latin + Latin Supplement
				{0x0370, 0x3ff}, // Greek and Coptic
		}};

		return range;
	}

	[[nodiscard]] auto glyph_range_simplified_chinese_common() noexcept -> glyph_ranges_view_type
	{
		const auto unpack = []<std::size_t I>() constexpr noexcept -> glyph_range_type
		{
			const auto codepoint =
					0x4e00 +
					// // fixme: fatal error C1128: number of sections exceeded object file format limit : compile with /bigobj
					// std::ranges::fold_left(
					// 	std::ranges::subrange{
					// 			std::ranges::begin(simplified_chinese_common_accumulative_offsets_from_0x4e00),
					// 			std::ranges::begin(simplified_chinese_common_accumulative_offsets_from_0x4e00) + 1 + I
					// 	},
					// 	glyph_range_value_type{0},
					// 	[](const glyph_range_value_type total, const glyph_range_value_type current) noexcept -> glyph_range_value_type
					// 	{
					// 		return total + current;
					// 	}
					// );
					std::accumulate(
						std::ranges::begin(simplified_chinese_common_accumulative_offsets_from_0x4e00),
						std::ranges::begin(simplified_chinese_common_accumulative_offsets_from_0x4e00) + 1 + I,
						glyph_range_value_type{0}
					);
			return {codepoint, codepoint};
		};

		const static auto range = [unpack]<std::size_t... Index>(std::index_sequence<Index...>) constexpr noexcept ->
			std::array<glyph_range_type,
			           1 + // Basic Latin + Latin Supplement
			           1 + // General Punctuation
			           1 + // CJK Symbols and Punctuations, Hiragana, Katakana
			           1 + // Katakana Phonetic Extensions
			           1 + // Half-width characters
			           1 + // Invalid
			           std::ranges::size(simplified_chinese_common_accumulative_offsets_from_0x4e00)
			>
				{
					return {
							{{0x0020, 0x00ff}, // Basic Latin + Latin Supplement
							 {0x2000, 0x206f}, // General Punctuation
							 {0x3000, 0x30ff}, // CJK Symbols and Punctuations, Hiragana, Katakana
							 {0x31f0, 0x31ff}, // Katakana Phonetic Extensions
							 {0xff00, 0xffef}, // Half-width characters
							 {0xfffd, 0xfffd}, // Invalid
							 unpack.operator()<Index>()...}
					};
				}(std::make_index_sequence<std::ranges::size(simplified_chinese_common_accumulative_offsets_from_0x4e00)>{});

		return range;
	}

	[[nodiscard]] auto glyph_range_simplified_chinese_all() noexcept -> glyph_ranges_view_type
	{
		constexpr static std::array<glyph_range_type, 7> range{{
				{0x0020, 0x00ff}, // Basic Latin + Latin Supplement
				{0x2000, 0x206f}, // General Punctuation
				{0x3000, 0x30ff}, // CJK Symbols and Punctuations, Hiragana, Katakana
				{0x31f0, 0x31ff}, // Katakana Phonetic Extensions
				{0xff00, 0xffef}, // Half-width characters
				{0xfffd, 0xfffd}, // Invalid
				{0x4e00, 0x9faf}, // CJK Ideograms
		}};

		return range;
	}

	auto load_font(const std::string_view font_path, const std::uint32_t pixel_height, const glyph_ranges_view_type glyph_ranges) noexcept -> font_type
	{
		FT_Library ft_library;
		if (FT_Init_FreeType(&ft_library))
		{
			// Could not initialize FreeType library
			return font_type{};
		}

		FT_Face ft_face;
		if (FT_New_Face(ft_library, font_path.data(), 0, &ft_face))
		{
			FT_Done_FreeType(ft_library);
			// Could not load font
			return font_type{};
		}

		FT_Set_Pixel_Sizes(ft_face, 0, pixel_height);

		const auto size = calculate_texture_size(ft_face, glyph_ranges);
		const auto atlas_width = size;
		const auto atlas_height = size;

		font_type font
		{
				.pixel_height = static_cast<float>(pixel_height),
				.texture_size = {atlas_width, atlas_height},
				.texture_data = std::make_unique_for_overwrite<std::uint32_t[]>(static_cast<std::size_t>(atlas_width * atlas_height)),
				.glyphs = {},
				.fallback_glyph = {}
		};

		std::uint32_t pen_x = 0;
		std::uint32_t pen_y = 0;
		std::uint32_t max_row_height = 0;

		for (const auto [first, second]: glyph_ranges)
		{
			for (auto c = first; c <= second; ++c)
			{
				if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER))
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

					return font_type{};
				}

				for (std::uint32_t y = 0; y < g->bitmap.rows; ++y)
				{
					for (std::uint32_t x = 0; x < g->bitmap.width; ++x)
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

				font.glyphs[c] = {
						.rect =
						{
								glyph_type::rect_type::point_type
								{
										static_cast<std::uint32_t>(g->bitmap_left),
										static_cast<std::uint32_t>(g->bitmap_top)
								},
								glyph_type::rect_type::extent_type
								{
										static_cast<std::uint32_t>(g->bitmap.width),
										static_cast<std::uint32_t>(g->bitmap.rows)
								}
						},
						.uv = {
								glyph_type::uv_type::point_type
								{
										static_cast<float>(pen_x) / static_cast<float>(atlas_width),
										static_cast<float>(pen_y) / static_cast<float>(atlas_height)
								},
								glyph_type::uv_type::extent_type
								{
										static_cast<float>(g->bitmap.width) / static_cast<float>(atlas_width),
										static_cast<float>(g->bitmap.rows) / static_cast<float>(atlas_height)
								}
						},
						.advance_x = static_cast<float>(g->advance.x) / 64.f
				};

				pen_x += g->bitmap.width;
				if (g->bitmap.rows > max_row_height)
				{
					max_row_height = g->bitmap.rows;
				}
			}
		}

		font.fallback_glyph = font.glyphs[U'?'];

		FT_Done_Face(ft_face);
		FT_Done_FreeType(ft_library);

		return font;
	}
}
