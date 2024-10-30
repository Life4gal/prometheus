// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H          // <freetype/freetype.h>
// #include FT_MODULE_H            // <freetype/ftmodapi.h>
// #include FT_GLYPH_H             // <freetype/ftglyph.h>
// #include FT_SYNTHESIS_H         // <freetype/ftsynth.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

export module gal.prometheus:draw.font.impl;

import std;

import :chars;

import :draw.def;
import :draw.font;
import :draw.draw_list;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#include <algorithm>
#include <ranges>
#include <numeric>
#include <vector>
#include <cmath>
#include <memory>
#include <format>

#include <ft2build.h>
#include FT_FREETYPE_H          // <freetype/freetype.h>
// #include FT_MODULE_H            // <freetype/ftmodapi.h>
// #include FT_GLYPH_H             // <freetype/ftglyph.h>
// #include FT_SYNTHESIS_H         // <freetype/ftsynth.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#include <prometheus/macro.hpp>
#include <chars/chars.ixx>
#include <draw/def.ixx>
#include <draw/font.ixx>
#include <draw/draw_list.ixx>

#endif

namespace
{
	[[nodiscard]] auto create_ft(const std::string_view font_path, const std::uint32_t pixel_height) noexcept -> std::pair<FT_Library, FT_Face>
	{
		FT_Library ft_library;
		if (FT_Init_FreeType(&ft_library))
		{
			// Could not initialize FreeType library
			return {nullptr, nullptr};
		}

		FT_Face ft_face;
		if (FT_New_Face(ft_library, font_path.data(), 0, &ft_face))
		{
			FT_Done_FreeType(ft_library);
			// Could not load font
			return {nullptr, nullptr};
		}

		FT_Set_Pixel_Sizes(ft_face, 0, pixel_height);
		return {ft_library, ft_face};
	}

	auto destroy_ft(std::pair<FT_Library, FT_Face> ft) noexcept -> void
	{
		auto [ft_library, ft_face] = ft;

		FT_Done_Face(ft_face);
		FT_Done_FreeType(ft_library);
	}
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT_IMPL(draw)
{
	auto Font::reset() noexcept -> void
	{
		font_path_.clear();
		pixel_height_ = std::numeric_limits<std::uint32_t>::min();
		baked_line_max_width_ = std::numeric_limits<std::uint32_t>::min();

		glyphs_.clear();
		fallback_glyph_ = {};

		white_pixel_uv_ = {};
		baked_line_uv_ = {};

		texture_id_ = invalid_texture_id;
	}

	Font::Font() noexcept
	{
		reset();
	}

	auto Font::load(const FontOption& option) noexcept -> texture_type
	{
		reset();

		texture_type texture
		{
				.size = {},
				.data = nullptr,
				.id = texture_id_
		};

		auto ft = create_ft(option.font_path, option.pixel_height);
		auto [ft_library, ft_face] = ft;

		if (ft_library == nullptr or ft_face == nullptr)
		{
			return texture;
		}

		// ===============================

		std::vector<stbrp_rect> rects;

		// baked line
		constexpr auto id_baked_line = std::numeric_limits<int>::min() + 0;
		{
			baked_line_uv_.reserve(baked_line_max_width_);
			rects.emplace_back(
				stbrp_rect
				{
						.id = id_baked_line,
						.w = static_cast<stbrp_coord>(baked_line_max_width_) + 1,
						.h = static_cast<stbrp_coord>(baked_line_max_width_) + 2,
						.x = 0,
						.y = 0,
						.was_packed = 0
				}
			);
		}

		// ===============================

		std::ranges::for_each(
			option.glyph_range,
			[&ft_face, &rects](const auto& pair) noexcept -> void
			{
				const auto [first, second] = pair;

				for (auto c = first; c <= second; ++c)
				{
					if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER))
					{
						continue;
					}

					const auto& g = ft_face->glyph;
					rects.emplace_back(
						stbrp_rect
						{
								.id = std::bit_cast<int>(c),
								.w = static_cast<stbrp_coord>(g->bitmap.width),
								.h = static_cast<stbrp_coord>(g->bitmap.rows),
								.x = static_cast<stbrp_coord>(g->bitmap_left),
								.y = static_cast<stbrp_coord>(g->bitmap_top),
								.was_packed = 0
						}
					);
				}
			}
		);

		// ===============================

		const auto size = [&rects]()
		{
			std::uint32_t total_area = 0;
			std::uint32_t max_width = 0;
			std::uint32_t max_height = 0;

			for (const auto& [id, w, h, x, y, was_packed]: rects)
			{
				total_area += w * h;
				max_width = std::ranges::max(max_width, static_cast<std::uint32_t>(w));
				max_height = std::ranges::max(max_height, static_cast<std::uint32_t>(h));
			}

			const auto min_side = static_cast<std::uint32_t>(std::sqrt(total_area));
			const auto max_side = std::ranges::max(max_width, max_height);

			return std::bit_ceil(std::ranges::max(min_side, max_side));
		}();
		auto atlas_width = size;
		auto atlas_height = size;

		stbrp_context context;
		std::vector<stbrp_node> nodes{atlas_width};
		while (true)
		{
			stbrp_init_target(&context, atlas_width, atlas_height, nodes.data(), static_cast<int>(nodes.size()));
			if (stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size())))
			{
				break;
			}

			atlas_width *= 2;
			atlas_height *= 2;
			nodes.resize(atlas_width);
		}

		// ===============================

		texture.size =
		{
				static_cast<extent_type::value_type>(atlas_width),
				static_cast<extent_type::value_type>(atlas_height)
		};
		// note: We don't necessarily overwrite all the memory, but it doesn't matter.
		// texture.data = std::make_unique<std::uint32_t[]>(static_cast<std::size_t>(atlas_width * atlas_height));
		texture.data = std::make_unique_for_overwrite<std::uint32_t[]>(static_cast<std::size_t>(atlas_width * atlas_height));

		const uv_extent_type texture_uv_scale{1.f / static_cast<float>(texture.size.width), 1.f / static_cast<float>(texture.size.height)};

		// ===============================

		for (const auto& [id, rect_width, rect_height, rect_x, rect_y, was_packed]: rects)
		{
			if (id == id_baked_line)
			{
				using value_type = uv_point_type::value_type;
				constexpr std::uint32_t white_color = 0xff'ff'ff'ff;

				// hacky: baked line rect area, one pixel
				{
					const auto x = rect_x + rect_y * atlas_width;
					texture.data[x] = white_color;

					const auto uv_x = static_cast<value_type>(static_cast<float>(rect_x) + .5f) * texture_uv_scale.width;
					const auto uv_y = static_cast<value_type>(static_cast<float>(rect_y) + .5f) * texture_uv_scale.height;

					white_pixel_uv_ = uv_point_type{uv_x, uv_y};
				}

				// ◿
				for (stbrp_coord y = 0; y < rect_height; ++y)
				{
					const auto line_width = y;
					const auto offset_y = (rect_y + y) * atlas_width;

					for (stbrp_coord x = line_width; x > 0; --x)
					{
						const auto offset_x = rect_x + (rect_width - x);
						const auto index = offset_x + offset_y;
						texture.data[index] = white_color;
					}

					const auto begin_x = rect_x + (rect_width - line_width);
					const auto end_x = rect_x + rect_width;
					const auto begin_y = rect_y + y;

					const auto uv0_x = static_cast<value_type>(begin_x) * texture_uv_scale.width;
					const auto uv1_x = static_cast<value_type>(end_x) * texture_uv_scale.width;
					const auto uv_y = static_cast<value_type>((static_cast<float>(begin_y) + .5f) * texture_uv_scale.height);

					baked_line_uv_.emplace_back(uv0_x, uv_y, uv1_x, uv_y);
				}

				continue;
			}

			const auto c = std::bit_cast<glyph_value_type>(id);

			if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER))
			{
				continue;
			}

			const auto& g = ft_face->glyph;

			for (std::uint32_t y = 0; y < g->bitmap.rows; ++y)
			{
				for (std::uint32_t x = 0; x < g->bitmap.width; ++x)
				{
					const auto index = rect_x + x + (rect_y + y) * atlas_width;
					const auto a = g->bitmap.buffer[x + y * g->bitmap.pitch] << 24;
					const auto color =
							// A
							a |
							// B
							std::uint32_t{0xff} << 16 |
							// G
							std::uint32_t{0xff} << 8 |
							// R
							std::uint32_t{0xff};
					texture.data[index] = color;
				}
			}

			const auto p0_x = static_cast<point_type::value_type>(g->bitmap_left);
			const auto p0_y = static_cast<point_type::value_type>(g->bitmap_top);
			const auto p1_x = p0_x + static_cast<point_type::value_type>(g->bitmap.width);
			const auto p1_y = p0_y + static_cast<point_type::value_type>(g->bitmap.rows);

			const auto uv0_x = static_cast<uv_point_type::value_type>(rect_x) * texture_uv_scale.width;
			const auto uv0_y = static_cast<uv_point_type::value_type>(rect_y) * texture_uv_scale.height;
			const auto uv1_x = uv0_x + static_cast<uv_extent_type::value_type>(g->bitmap.width) * texture_uv_scale.width;
			const auto uv1_y = uv0_y + static_cast<uv_extent_type::value_type>(g->bitmap.rows) * texture_uv_scale.height;

			glyphs_[c] = {
					.rect = {p0_x, p0_y, p1_x, p1_y},
					.uv = {uv0_x, uv0_y, uv1_x, uv1_y},
					.advance_x = static_cast<float>(g->advance.x) / 64.f
			};
		}

		font_path_ = std::format("{}-{}px", option.font_path, option.pixel_height);
		pixel_height_ = option.pixel_height;
		baked_line_max_width_ = option.baked_line_max_width == 0 ? FontOption::default_baked_line_max_width : option.baked_line_max_width;
		fallback_glyph_ = glyphs_[static_cast<char_type>('?')];

		// ===============================

		destroy_ft(ft);
		return texture;
	}

	auto Font::loaded() const noexcept -> bool
	{
		return not glyphs_.empty() and texture_id_ != invalid_texture_id;
	}

	auto Font::font_path() const noexcept -> std::string_view
	{
		return font_path_;
	}

	auto Font::pixel_height() const noexcept -> std::uint32_t
	{
		return pixel_height_;
	}

	auto Font::baked_line_max_width() const noexcept -> std::uint32_t
	{
		return baked_line_max_width_;
	}

	auto Font::glyphs() const noexcept -> const glyphs_type&
	{
		return glyphs_;
	}

	auto Font::fallback_glyph() const noexcept -> const glyph_type&
	{
		return fallback_glyph_;
	}

	auto Font::white_pixel_uv() const noexcept -> const uv_point_type&
	{
		return white_pixel_uv_;
	}

	auto Font::baked_line_uv() const noexcept -> const baked_line_uv_type&
	{
		return baked_line_uv_;
	}

	auto Font::texture_id() const noexcept -> texture_id_type
	{
		return texture_id_;
	}

	auto Font::text_size(
		const std::string_view utf8_text,
		const float font_size,
		const float wrap_width,
		std::basic_string<char_type>& out_text
	) const noexcept -> DrawListType::extent_type
	{
		const auto utf32_text = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32>(utf8_text);

		const auto line_height = font_size;
		const auto scale = line_height / pixel_height_;
		const auto& glyphs = glyphs_;
		const auto& fallback_glyph = fallback_glyph_;

		float max_width = 0;
		float current_width = 0;
		float total_height = 0;

		auto it_input_current = utf32_text.begin();
		const auto it_input_end = utf32_text.end();

		while (it_input_current != it_input_end)
		{
			const auto this_char = *it_input_current;
			it_input_current += 1;

			if (this_char == U'\n')
			{
				max_width = std::ranges::max(max_width, current_width);
				current_width = 0;
				total_height += line_height;
			}
			else
			{
				const auto& [glyph_rect, glyph_uv, glyph_advance_x] = [&glyphs, &fallback_glyph](const auto c) -> const auto& {
					if (const auto it = glyphs.find(c);
						it != glyphs.end())
					{
						return it->second;
					}

					return fallback_glyph;
				}(this_char);

				const auto advance_x = glyph_advance_x * scale;
				if (current_width + advance_x > wrap_width)
				{
					max_width = std::ranges::max(max_width, current_width);
					current_width = advance_x;
					total_height += line_height;
				}
				else
				{
					current_width += advance_x;
				}
			}
		}

		max_width = std::ranges::max(max_width, current_width);
		out_text = std::move(utf32_text);

		return {max_width, total_height};
	}

	auto Font::text_size(
		const std::string_view utf8_text,
		const float font_size,
		const float wrap_width
	) const noexcept -> DrawListType::extent_type
	{
		std::basic_string<char_type> out;
		return text_size(utf8_text, font_size, wrap_width, out);
	}

	auto Font::draw_text(
		DrawList& draw_list,
		const float font_size,
		const DrawListType::point_type& p,
		const DrawListType::color_type& color,
		const std::string_view utf8_text,
		const float wrap_width
	) const noexcept -> void
	{
		const auto utf32_text = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32>(utf8_text);

		const auto vertex_count = 4 * utf32_text.size();
		const auto index_count = 6 * utf32_text.size();
		draw_list.vertex_list_.reserve(draw_list.vertex_list_.size() + vertex_count);
		draw_list.index_list_.reserve(draw_list.index_list_.size() + index_count);

		draw_list.command_list_.back().element_count += index_count;

		const auto line_height = font_size;
		const auto scale = line_height / pixel_height_;
		const auto& glyphs = glyphs_;
		const auto& fallback_glyph = fallback_glyph_;

		auto cursor = p + DrawListSharedData::point_type{0, font_size};

		auto it_input_current = utf32_text.begin();
		const auto it_input_end = utf32_text.end();

		while (it_input_current != it_input_end)
		{
			const auto this_char = *it_input_current;
			it_input_current += 1;

			if (this_char == U'\n')
			{
				cursor.x = p.x;
				cursor.y += line_height;
				draw_list.command_list_.back().element_count -= 6;
				continue;
			}

			const auto& [glyph_rect, glyph_uv, glyph_advance_x] = [&glyphs, &fallback_glyph](const auto c) -> const auto& {
				if (const auto it = glyphs.find(c);
					it != glyphs.end())
				{
					return it->second;
				}

				return fallback_glyph;
			}(this_char);

			const auto advance_x = glyph_advance_x * scale;
			if (cursor.x + advance_x > wrap_width)
			{
				cursor.x = p.x;
				cursor.y += line_height;
			}

			using d_rect_type = DrawListType::rect_type;
			using d_point_type = DrawListType::point_type;
			using d_extent_type = DrawListType::extent_type;
			using d_index_type = DrawListType::index_type;

			const d_rect_type char_rect
			{
					cursor + d_point_type{static_cast<d_point_type::value_type>(glyph_rect.left_top().x), -static_cast<d_point_type::value_type>(glyph_rect.left_top().y)} * scale,
					static_cast<d_extent_type>(glyph_rect.size()) * scale
			};
			cursor.x += advance_x;

			const auto current_vertex_index = static_cast<d_index_type>(draw_list.vertex_list_.size());

			draw_list.vertex_list_.emplace_back(char_rect.left_top(), glyph_uv.left_top(), color);
			draw_list.vertex_list_.emplace_back(char_rect.right_top(), glyph_uv.right_top(), color);
			draw_list.vertex_list_.emplace_back(char_rect.right_bottom(), glyph_uv.right_bottom(), color);
			draw_list.vertex_list_.emplace_back(char_rect.left_bottom(), glyph_uv.left_bottom(), color);

			draw_list.index_list_.push_back(current_vertex_index + 0);
			draw_list.index_list_.push_back(current_vertex_index + 1);
			draw_list.index_list_.push_back(current_vertex_index + 2);
			draw_list.index_list_.push_back(current_vertex_index + 0);
			draw_list.index_list_.push_back(current_vertex_index + 2);
			draw_list.index_list_.push_back(current_vertex_index + 3);
		}
	}
}
