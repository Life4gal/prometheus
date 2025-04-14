// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gui/internal/font.hpp>

#include <algorithm>
#include <ranges>
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

#include <chars/chars.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <gui/internal/context.hpp>

namespace
{
	struct ft_type
	{
		FT_Library library;
		FT_Face face;

		[[nodiscard]] constexpr auto valid() const noexcept -> bool
		{
			return library and face;
		}
	};

	[[nodiscard]] auto create_ft(const std::string_view font_path, const std::uint32_t pixel_height) noexcept -> ft_type
	{
		FT_Library ft_library;
		if (FT_Init_FreeType(&ft_library))
		{
			// Could not initialize FreeType library
			return {.library = nullptr, .face = nullptr};
		}

		FT_Face ft_face;
		if (FT_New_Face(ft_library, font_path.data(), 0, &ft_face))
		{
			FT_Done_FreeType(ft_library);
			// Could not load font
			return {.library = nullptr, .face = nullptr};
		}

		FT_Set_Pixel_Sizes(ft_face, 0, pixel_height);
		return {.library = ft_library, .face = ft_face};
	}

	auto destroy_ft(const ft_type ft) noexcept -> void
	{
		auto [ft_library, ft_face] = ft;

		FT_Done_Face(ft_face);
		FT_Done_FreeType(ft_library);
	}

	using namespace gal::prometheus;
	using namespace gui;

	auto do_load_font(const FontOption& option, internal::Font& font) noexcept -> Texture
	{
		font.font_path = std::format("{}-{}px", option.font_path, option.pixel_height);
		font.pixel_height = option.pixel_height;
		if (option.baked_line_max_width == 0 or option.baked_line_max_width > FontOption::default_baked_line_max_width)
		{
			font.baked_line_max_width = FontOption::default_baked_line_max_width;
		}
		else
		{
			font.baked_line_max_width = option.baked_line_max_width;
		}
		font.scale = option.scale;
		font.display_offset = option.display_offset;
		font.white_pixel_uv = option.white_pixel_uv;

		Texture texture{font.texture_id};

		auto ft = create_ft(option.font_path, option.pixel_height);
		if (not ft.valid())
		{
			return texture;
		}

		auto [library, face] = ft;

		// ===============================

		std::vector<stbrp_rect> rects;

		// baked line
		constexpr auto id_baked_line = std::numeric_limits<int>::min() + 0;
		{
			const auto baked_line_uv_width = font.baked_line_max_width + 1;
			const auto baked_line_uv_height = font.baked_line_max_width + 2;

			font.baked_line_uv.reserve(baked_line_uv_height);

			rects.emplace_back(
				stbrp_rect
				{
						.id = id_baked_line,
						.w = static_cast<stbrp_coord>(baked_line_uv_width),
						.h = static_cast<stbrp_coord>(baked_line_uv_height),
						.x = 0,
						.y = 0,
						.was_packed = 0
				}
			);
		}

		// ===============================

		std::ranges::for_each(
			option.glyph_ranges,
			[&face, &rects](const auto& pair) noexcept -> void
			{
				const auto [from, to] = pair;

				for (auto c = from; c <= to; ++c)
				{
					if (FT_Load_Char(face, c, FT_LOAD_RENDER))
					{
						continue;
					}

					const auto& g = face->glyph;
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

		// todo: texture size?
		const auto size = [&rects]()
		{
			Texture::size_type total_area = 0;
			Texture::size_type max_width = 0;
			Texture::size_type max_height = 0;

			for (const auto& [id, w, h, x, y, was_packed]: rects)
			{
				total_area += w * h;
				max_width = std::ranges::max(max_width, static_cast<Texture::size_type>(w));
				max_height = std::ranges::max(max_height, static_cast<Texture::size_type>(h));
			}

			const auto min_side = static_cast<Texture::size_type>(std::sqrt(total_area));
			const auto max_side = std::ranges::max(max_width, max_height);

			return std::bit_ceil(std::ranges::max(min_side, max_side));
		}();
		auto atlas_width = size;
		auto atlas_height = size;

		stbrp_context rp_context;
		std::vector<stbrp_node> nodes{atlas_width};
		while (true)
		{
			stbrp_init_target(&rp_context, static_cast<int>(atlas_width), static_cast<int>(atlas_height), nodes.data(), static_cast<int>(nodes.size()));
			if (stbrp_pack_rects(&rp_context, rects.data(), static_cast<int>(rects.size())))
			{
				break;
			}

			atlas_width *= 2;
			atlas_height *= 2;
			nodes.resize(atlas_width);
		}

		// ===============================

		// note: We don't necessarily overwrite all the memory, but it doesn't matter.
		// auto texture_data = std::make_unique<std::uint32_t[]>(static_cast<std::size_t>(atlas_width * atlas_height));
		auto texture_data = std::make_unique_for_overwrite<std::uint32_t[]>(static_cast<std::size_t>(atlas_width * atlas_height));

		const extent_type texture_uv_scale{1.f / static_cast<float>(atlas_width), 1.f / static_cast<float>(atlas_height)};

		// ===============================

		for (const auto& [id, rect_width, rect_height, rect_x, rect_y, was_packed]: rects)
		{
			if (id == id_baked_line)
			{
				using value_type = point_type::value_type;
				constexpr std::uint32_t white_color = 0xff'ff'ff'ff;

				if (font.white_pixel_uv.x < 0 or font.white_pixel_uv.y < 0)
				{
					// hacky: baked line rect area, one pixel
					{
						const auto x = rect_x + rect_y * atlas_width;
						texture_data[x] = white_color;

						const auto uv_x = static_cast<value_type>(static_cast<float>(rect_x) + .5f) * texture_uv_scale.width;
						const auto uv_y = static_cast<value_type>(static_cast<float>(rect_y) + .5f) * texture_uv_scale.height;

						font.white_pixel_uv = point_type{uv_x, uv_y};
					}
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
						texture_data[index] = white_color;
					}

					const auto p_x = rect_x + (rect_width - line_width);
					const auto p_y = rect_y + y;
					const auto width = line_width;
					constexpr auto height = .5f;

					const auto uv_x = static_cast<value_type>(p_x) * texture_uv_scale.width;
					const auto uv_y = static_cast<value_type>(p_y) * texture_uv_scale.height;
					const auto uv_width = static_cast<value_type>(width) * texture_uv_scale.width;
					const auto uv_height = static_cast<value_type>(height) * texture_uv_scale.height;

					font.baked_line_uv.emplace_back(uv_x, uv_y, uv_width, uv_height);
				}

				continue;
			}

			const auto c = std::bit_cast<internal::Font::glyph_value_type>(id);

			// todo
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(c <= std::numeric_limits<internal::Font::char_type>::max());

			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				continue;
			}

			const auto& g = face->glyph;

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
					texture_data[index] = color;
				}
			}

			const auto p_x = static_cast<point_type::value_type>(g->bitmap_left);
			const auto p_y = static_cast<point_type::value_type>(g->bitmap_top);
			const auto p_width = static_cast<point_type::value_type>(g->bitmap.width);
			const auto p_height = static_cast<point_type::value_type>(g->bitmap.rows);

			const auto uv_x = static_cast<point_type::value_type>(rect_x) * texture_uv_scale.width;
			const auto uv_y = static_cast<point_type::value_type>(rect_y) * texture_uv_scale.height;
			const auto uv_width = static_cast<extent_type::value_type>(g->bitmap.width) * texture_uv_scale.width;
			const auto uv_height = static_cast<extent_type::value_type>(g->bitmap.rows) * texture_uv_scale.height;

			const internal::Font::glyph_type glyph{
					.rect = {p_x, p_y, p_width, p_height},
					.uv = {uv_x, uv_y, uv_width, uv_height},
					.advance_x = static_cast<float>(g->advance.x) / 64.f
			};

			font.glyphs.insert_or_assign(static_cast<internal::Font::char_type>(c), glyph);
		}

		font.fallback_glyph = font.glyphs[option.fallback_char];

		// ===============================

		destroy_ft(ft);

		texture.width = atlas_width;
		texture.height = atlas_height;
		texture.data = std::move(texture_data);
		return texture;
	}
}

namespace gal::prometheus::gui
{
	auto set_default_font(Context& context, const FontOption& option) noexcept -> Texture
	{
		// if (context.font_current == Context::stack_pointer_default)
		// {
		// 	return push_font(context, option);
		// }
		//
		// auto font = memory::make_unique<internal::Font>();
		// auto texture = do_load_font(option, *font);
		//
		// context.font_stack[0] = std::move(font);
		//
		// return texture;
		auto texture = do_load_font(option, context.font);
		return texture;
	}

	// [[nodiscard]] auto push_font(Context& context, const FontOption& option) noexcept -> Texture
	// {
	// 	auto font = memory::make_unique<internal::Font>();
	// 	auto texture = do_load_font(option, *font);
	//
	// 	internal::push_font(context, std::move(font));
	//
	// 	return texture;
	// }
	//
	// auto pop_font(Context& context) noexcept -> void
	// {
	// 	internal::pop_font(context);
	// }

	Texture::Texture(texture_id_type& texture_id) noexcept
		: width{0},
		  height{0},
		  data{nullptr},
		  id{texture_id} {}

	Texture::~Texture() noexcept
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
			id != internal::Font::invalid_texture_id,
			"Texture is not bound to a GPU resource id!"
		);
	}

	auto Texture::valid() const noexcept -> bool
	{
		return data != nullptr;
	}

	// ReSharper disable once CppMemberFunctionMayBeConst
	auto Texture::bind(const texture_id_type texture_id) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
			this->valid(),
			"Only valid textures can be bound to a GPU resource id"
		);

		this->id.get() = texture_id;
	}

	namespace internal
	{
		Font::Font() noexcept
			:
			pixel_height{std::numeric_limits<std::uint32_t>::min()},
			baked_line_max_width{std::numeric_limits<std::uint32_t>::min()},
			fallback_glyph{},
			scale{1},
			display_offset{0, 0},
			texture_id{invalid_texture_id} {}

		auto Font::loaded() const noexcept -> bool
		{
			return not glyphs.empty() and texture_id != invalid_texture_id;
		}

		auto text_size(
			const Font& font,
			const std::basic_string_view<char> utf8_text,
			const float font_size,
			const float wrap_width,
			std::basic_string<Font::char_type>& out_text
		) noexcept -> extent_type
		{
			// todo
			auto utf16_text = chars::convert<chars::CharsType::UTF8_CHAR, chars::CharsType::UTF16_LE>(utf8_text);
			static_assert(std::is_same_v<decltype(utf16_text)::value_type, Font::char_type>);

			const auto line_height = font_size;
			const auto scale = line_height / (static_cast<float>(font.pixel_height) * font.scale);
			const auto& glyphs = font.glyphs;
			const auto& fallback_glyph = font.fallback_glyph;

			float max_width = 0;
			float current_width = 0;
			float total_height = line_height;

			auto it_input_current = utf16_text.begin();
			const auto it_input_end = utf16_text.end();

			while (it_input_current != it_input_end)
			{
				const auto this_char = *it_input_current;
				it_input_current += 1;

				if (this_char == u'\n')
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

					if (const auto advance_x = glyph_advance_x * scale;
						current_width + advance_x > wrap_width)
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
			out_text = std::move(utf16_text);

			return {max_width, total_height};
		}

		auto text_size(
			const Font& font,
			const std::basic_string_view<char> utf8_text,
			const float font_size,
			const float wrap_width
		) noexcept -> extent_type
		{
			std::basic_string<Font::char_type> out;
			return text_size(font, utf8_text, font_size, wrap_width, out);
		}

		auto text_draw(
			const Font& font,
			const std::basic_string_view<char> utf8_text,
			const float font_size,
			const float wrap_width,
			const point_type point,
			const Theme::color_type color,
			PrimitiveAppender appender
		) noexcept -> void
		{
			// todo
			auto utf16_text = chars::convert<chars::CharsType::UTF8_CHAR, chars::CharsType::UTF16_LE>(utf8_text);
			static_assert(std::is_same_v<decltype(utf16_text)::value_type, Font::char_type>);

			const auto newline_count = std::ranges::count(utf16_text, u'\n');
			const auto text_size_exclude_newline = utf16_text.size() - newline_count;

			const auto vertex_count = 4 * text_size_exclude_newline;
			const auto index_count = 6 * text_size_exclude_newline;
			appender.reserve(vertex_count, index_count);

			const auto line_height = font_size;
			const auto scale = line_height / static_cast<float>(font.pixel_height) * font.scale;
			const auto& glyphs = font.glyphs;
			const auto& fallback_glyph = font.fallback_glyph;

			auto cursor = point + point_type{0, line_height};

			auto it_input_current = utf16_text.begin();
			const auto it_input_end = utf16_text.end();

			while (it_input_current != it_input_end)
			{
				const auto this_char = *it_input_current;
				it_input_current += 1;

				if (this_char == u'\n')
				{
					cursor.x = point.x;
					cursor.y += line_height;
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
				if (cursor.x + advance_x > point.x + wrap_width)
				{
					cursor.x = point.x;
					cursor.y += line_height;
				}

				const rect_type char_rect
				{
						cursor + point_type{glyph_rect.left_top().x, -glyph_rect.left_top().y} * scale,
						glyph_rect.size() * scale
				};
				cursor.x += advance_x;

				const auto current_vertex_index = static_cast<index_type>(appender.vertex_count());

				appender.add_vertex(char_rect.left_top(), glyph_uv.left_top(), color);
				appender.add_vertex(char_rect.right_top(), glyph_uv.right_top(), color);
				appender.add_vertex(char_rect.right_bottom(), glyph_uv.right_bottom(), color);
				appender.add_vertex(char_rect.left_bottom(), glyph_uv.left_bottom(), color);

				appender.add_index(current_vertex_index + 0, current_vertex_index + 1, current_vertex_index + 2);
				appender.add_index(current_vertex_index + 0, current_vertex_index + 2, current_vertex_index + 3);
			}
		}
	}
}
