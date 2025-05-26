// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/extension/glyph_parser_freetype.hpp>

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
	class GlyphParserFreeType::Library final
	{
	public:
		FT_Library library{nullptr};
	};

	class GlyphParserFreeType::FontInfo final
	{
	public:
		std::filesystem::path path;
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

	GlyphParserFreeType::~GlyphParserFreeType() noexcept
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

	GlyphParserFreeType::GlyphParserFreeType() noexcept
		: library_{memory::make_unique<Library>()}
	{
		if (const auto error = FT_Init_FreeType(&library_->library); error != FT_Err_Ok)
		{
			// todo: error handling
			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
		}
	}

	auto GlyphParserFreeType::load(const std::filesystem::path& path) noexcept -> FontDescriptor
	{
		if (const auto it = std::ranges::find(infos_, path, &FontInfo::path); it != infos_.end())
		{
			const auto& info = it.operator*();
			const auto index = std::ranges::distance(infos_.begin(), it);

			return {.identifier = info.face->family_name, .id = static_cast<font_id_type>(index)};
		}

		const auto path_string = path.string();

		FT_Face face = nullptr;
		if (const auto error = FT_New_Face(library_->library, path_string.data(), 0, &face); error != FT_Err_Ok)
		{
			return FontDescriptor::error();
		}

		if (const auto error = FT_Select_Charmap(face, FT_ENCODING_UNICODE); error != FT_Err_Ok)
		{
			FT_Done_Face(face);
			return FontDescriptor::error();
		}

		const auto id = infos_.size();
		auto& info = infos_.emplace_back();
		info.face = face;

		return {.identifier = face->family_name, .id = static_cast<font_id_type>(id)};
	}

	// auto GlyphParserFreeType::load(const std::span<std::uint8_t> data) noexcept -> FontDescriptor
	// {
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.data() != nullptr);
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not data.empty());
	//
	// 	FT_Face face = nullptr;
	// 	if (const auto error = FT_New_Memory_Face(library_->library, data.data(), static_cast<FT_Long>(data.size()), 0, &face); error != FT_Err_Ok)
	// 	{
	// 		return FontDescriptor::error();
	// 	}
	//
	// 	if (const auto error = FT_Select_Charmap(face, FT_ENCODING_UNICODE); error != FT_Err_Ok)
	// 	{
	// 		FT_Done_Face(face);
	// 		return FontDescriptor::error();
	// 	}
	//
	// 	const auto id = infos_.size();
	//
	// 	auto& info = infos_.emplace_back();
	// 	info.face = face;
	//
	// 	return {.identifier = face->family_name, .id = static_cast<font_id_type>(id)};
	// }

	auto GlyphParserFreeType::has_glyph(const font_id_type id, const std::uint32_t codepoint) const noexcept -> bool
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

	auto GlyphParserFreeType::parse(const font_id_type id, const GlyphCode& code) noexcept -> GlyphDescriptor
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id < infos_.size());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(code.codepoint != 0);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(code.size != 0);

		auto& info = infos_[id];
		const auto& face = info.face;

		const auto char_index = FT_Get_Char_Index(face, code.codepoint);
		if (char_index == 0)
		{
			return GlyphDescriptor::error();
		}

		info.set_pixel_height(code.size);

		if (const auto error = FT_Load_Glyph(face, char_index, FT_LOAD_DEFAULT); error != FT_Err_Ok)
		{
			return GlyphDescriptor::error();
		}

		const auto& slot = face->glyph;

		if (std::to_underlying(code.flag) & GlyphFlag::BOLD)
		{
			FT_GlyphSlot_Embolden(slot);
		}
		if (std::to_underlying(code.flag) & GlyphFlag::ITALIC)
		{
			FT_GlyphSlot_Oblique(slot);
		}

		if (const auto error = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL); error != FT_Err_Ok)
		{
			return GlyphDescriptor::error();
		}

		const auto& bitmap = face->glyph->bitmap;

		const GlyphDescriptor::rect_type::point_type point{slot->bitmap_left, slot->bitmap_top};
		const GlyphDescriptor::rect_type::extent_type size{bitmap.width, bitmap.rows};
		const std::size_t data_length = static_cast<std::size_t>(bitmap.width) * bitmap.rows;

		GlyphDescriptor result{
				.rect = {point, size},
				.advance_x = ft_size_to_float(slot->advance.x),
				.visible = size.width > 0 and size.height > 0,
				.colored = bitmap.pixel_mode == FT_PIXEL_MODE_BGRA,
				.data = std::make_unique_for_overwrite<TextureDescriptor::element_type[]>(data_length)
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
}
