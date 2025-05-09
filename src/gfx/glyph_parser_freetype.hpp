// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

// todo
#define GAL_PROMETHEUS_GFX_GLYPH_PARSER_FREETYPE

#if defined(GAL_PROMETHEUS_GFX_GLYPH_PARSER_FREETYPE)

#include <vector>

#include <gfx/font.hpp>

#include <freetype/freetype.h>

namespace gal::prometheus::gfx
{
	class FreeTypeGlyphParser final : public GlyphParser
	{
		class FontInfo final
		{
		public:
			FT_Face face{nullptr};

			float ascender{0};
			float descender{0};
			float line_spacing{0};
			float line_gap{0};
			float max_advance_width{0};

			auto set_pixel_height(std::size_t height) noexcept -> bool;
		};

	public:
		using infos_type = std::vector<FontInfo>;

	private:
		FT_Library library_;

		infos_type infos_;

	public:
		FreeTypeGlyphParser(const FreeTypeGlyphParser&) noexcept = delete;
		FreeTypeGlyphParser(FreeTypeGlyphParser&&) noexcept = default;
		auto operator=(const FreeTypeGlyphParser&) noexcept -> FreeTypeGlyphParser& = delete;
		auto operator=(FreeTypeGlyphParser&&) noexcept -> FreeTypeGlyphParser& = default;
		~FreeTypeGlyphParser() noexcept override;

		FreeTypeGlyphParser() noexcept;

		auto initialize() noexcept -> bool override;

		auto load(FontPendingLoadData::data_view_type data) noexcept -> LoadResult override;

		[[nodiscard]] auto has_glyph(font_id_type id, std::uint32_t codepoint) const noexcept -> bool override;

		auto parse(font_id_type id, const GlyphKey& key) noexcept -> ParseResult override;
	};
} // namespace gal::prometheus::gfx

#endif
