// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/glyph_parser_freetype.hpp>

#if defined(GAL_PROMETHEUS_GFX_GLYPH_PARSER_FREETYPE)

#include <freetype/freetype.h>
#include <freetype/ftsynth.h>

namespace
{
	[[nodiscard]] auto ft_size_to_float(const FT_Pos size) noexcept -> float
	{
		return static_cast<float>((size + 63) >> 6);
	}
} // namespace

namespace gal::prometheus::gfx
{
	class FreeTypeGlyphParser::Library final
	{
	public:
		FT_Library library{nullptr};
	};

	class FreeTypeGlyphParser::FontInfo final
	{
	public:
		std::unique_ptr<std::uint8_t> font_data{nullptr};
		FT_Face face{nullptr};

		float ascender{0};
		float descender{0};
		float line_spacing{0};
		float line_gap{0};
		float max_advance_width{0};

		auto set_pixel_height(const std::size_t height) noexcept -> bool
		{
			FT_Size_RequestRec request{.type = FT_SIZE_REQUEST_TYPE_NOMINAL, .width = 0, .height = static_cast<FT_Long>(height) * 64, .horiResolution = 0, .vertResolution = 0};

			if (const auto error = FT_Request_Size(face, &request); error != FT_Err_Ok)
			{
				return false;
			}

			const auto& metrics = face->size->metrics;

			ascender = ft_size_to_float(metrics.ascender);
			descender = ft_size_to_float(metrics.descender);
			line_spacing = ft_size_to_float(metrics.height);
			line_gap = ft_size_to_float(metrics.height - metrics.ascender + metrics.descender);
			max_advance_width = ft_size_to_float(metrics.max_advance);

			return true;
		}
	};

	FreeTypeGlyphParser::~FreeTypeGlyphParser() noexcept
	{
		std::ranges::for_each(
			infos_,
			[](auto& info) noexcept -> void
			{
				::FT_Done_Face(info.face);
			}
		);

		FT_Done_FreeType(library_->library);
	}

	FreeTypeGlyphParser::FreeTypeGlyphParser() noexcept
		: library_{memory::make_unique<Library>()}
	{
		if (const auto error = FT_Init_FreeType(&library_->library); error != FT_Err_Ok)
		{
			// todo: error handling
			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
		}
	}

	auto FreeTypeGlyphParser::ready() noexcept -> bool
	{
		return library_ != nullptr;
	}

	auto FreeTypeGlyphParser::load(const std::filesystem::path& path) noexcept -> LoadResult
	{
		const auto path_string = path.string();

		LoadResult invalid_result{.name = {}, .id = invalid_font_id};

		FT_Face face = nullptr;
		if (const auto error = FT_New_Face(library_->library, path_string.data(), 0, &face); error != FT_Err_Ok)
		{
			return invalid_result;
		}

		if (const auto error = FT_Select_Charmap(face, FT_ENCODING_UNICODE); error != FT_Err_Ok)
		{
			FT_Done_Face(face);
			return invalid_result;
		}

		const auto id = infos_.size();

		auto& info = infos_.emplace_back();
		info.face = face;

		return {.name = face->family_name, .id = static_cast<font_id_type>(id)};
	}

	auto FreeTypeGlyphParser::load(std::unique_ptr<std::uint8_t> data, const std::size_t size) noexcept -> LoadResult
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.get() != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size != 0);

		LoadResult invalid_result{.name = {}, .id = invalid_font_id};

		FT_Face face = nullptr;
		if (const auto error = FT_New_Memory_Face(library_->library, data.get(), static_cast<FT_Long>(size), 0, &face); error != FT_Err_Ok)
		{
			return invalid_result;
		}

		if (const auto error = FT_Select_Charmap(face, FT_ENCODING_UNICODE); error != FT_Err_Ok)
		{
			FT_Done_Face(face);
			return invalid_result;
		}

		const auto id = infos_.size();

		auto& info = infos_.emplace_back();
		info.font_data = std::move(data);
		info.face = face;

		return {.name = face->family_name, .id = static_cast<font_id_type>(id)};
	}

	auto FreeTypeGlyphParser::load(const std::span<std::uint8_t> data) noexcept -> LoadResult
	{
		auto* copy = new std::uint8_t[data.size()];
		std::ranges::copy(data, copy);

		return this->load(std::unique_ptr<std::uint8_t>{copy}, data.size());
	}

	auto FreeTypeGlyphParser::has_glyph(const font_id_type id, const std::uint32_t codepoint) const noexcept -> bool
	{
		if (id >= infos_.size())
		{
			return false;
		}

		const auto& info = infos_[id];
		if (const auto char_index = FT_Get_Char_Index(info.face, codepoint); char_index == 0)
		{
			return false;
		}

		return true;
	}

	auto FreeTypeGlyphParser::parse(const font_id_type id, const GlyphKey& key) noexcept -> ParseResult
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id < infos_.size());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(key.codepoint != 0);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(key.size != 0);

		ParseResult invalid_result{.info = {}, .data = nullptr};

		auto& info = infos_[id];
		const auto& face = info.face;

		const auto char_index = FT_Get_Char_Index(face, key.codepoint);
		if (char_index == 0)
		{
			return invalid_result;
		}

		info.set_pixel_height(key.size);

		if (const auto error = FT_Load_Glyph(face, char_index, FT_LOAD_DEFAULT); error != FT_Err_Ok)
		{
			return invalid_result;
		}

		const auto& slot = face->glyph;

		if (std::to_underlying(key.flag) & GlyphFlag::BOLD)
		{
			FT_GlyphSlot_Embolden(slot);
		}
		if (std::to_underlying(key.flag) & GlyphFlag::ITALIC)
		{
			FT_GlyphSlot_Oblique(slot);
		}

		if (const auto error = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL); error != FT_Err_Ok)
		{
			return invalid_result;
		}

		const auto& bitmap = info.face->glyph->bitmap;

		const point_type point{static_cast<point_type::value_type>(slot->bitmap_left), static_cast<point_type::value_type>(slot->bitmap_top)};
		const extent_type size{static_cast<extent_type::value_type>(bitmap.width), static_cast<extent_type::value_type>(bitmap.rows)};
		const std::size_t data_length = static_cast<std::size_t>(bitmap.width) * bitmap.rows;

		ParseResult result{
				.info =
				{
						.rect = {point, size},
						.advance_x = ft_size_to_float(slot->advance.x),
						.visible = size.width > 0 and size.height > 0,
						.colored = bitmap.pixel_mode == FT_PIXEL_MODE_BGRA,
						.texture_atlas_id = invalid_texture_atlas_id,
						.uv = {},
				},
				.data = std::make_unique_for_overwrite<Texture::element_type[]>(data_length)
		};

		{
			const auto* source = bitmap.buffer;
			auto* dest = result.data.get();

			if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
			{
				for (std::uint32_t y = 0; y < bitmap.rows; ++y)
				{
					for (std::uint32_t x = 0; x < bitmap.width; ++x)
					{
						const auto a = source[x];
						const auto color =
								// A
								a << 24 |
								// B
								std::uint32_t{0xff} << 16 |
								// G
								std::uint32_t{0xff} << 8 |
								// R
								std::uint32_t{0xff};
						dest[x] = color;
					}

					source += bitmap.pitch;
					dest += bitmap.width;
				}
			}
			else if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
			{
				for (std::uint32_t y = 0; y < bitmap.rows; ++y)
				{
					const std::uint8_t* p = source;
					std::uint8_t bits = 0;

					for (std::uint32_t x = 0; x < bitmap.width; ++x)
					{
						if ((x & 7) == 0)
						{
							bits = *p;
							p += 1;
						}

						const auto a = (bits & 0x80) ? std::uint32_t{0xff} : std::uint32_t{0};
						const auto color =
								// A
								a << 24 |
								// B
								std::uint32_t{0xff} << 16 |
								// G
								std::uint32_t{0xff} << 8 |
								// R
								std::uint32_t{0xff};
						dest[x] = color;

						bits <<= 1;
					}

					source += bitmap.pitch;
					dest += bitmap.width;
				}
			}
		}

		return result;
	}
} // namespace gal::prometheus::gfx

#endif
