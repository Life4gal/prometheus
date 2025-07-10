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

#include <memory/unique_ptr.hpp>

namespace gal::prometheus::gfx
{
	class FreeTypeGlyphParser final : public GlyphParser
	{
		class Library;
		class FontInfo;

	public:
		using infos_type = std::vector<FontInfo>;

	private:
		memory::UniquePointer<Library> library_;
		infos_type infos_;

	public:
		FreeTypeGlyphParser(const FreeTypeGlyphParser&) noexcept = delete;
		FreeTypeGlyphParser(FreeTypeGlyphParser&&) noexcept = default;
		auto operator=(const FreeTypeGlyphParser&) noexcept -> FreeTypeGlyphParser& = delete;
		auto operator=(FreeTypeGlyphParser&&) noexcept -> FreeTypeGlyphParser& = default;
		~FreeTypeGlyphParser() noexcept override;

		FreeTypeGlyphParser() noexcept;

		auto ready() noexcept -> bool override;

		auto load(std::span<std::uint8_t> data) noexcept -> LoadResult override;

		[[nodiscard]] auto has_glyph(font_id_type id, std::uint32_t codepoint) const noexcept -> bool override;

		auto parse(font_id_type id, const GlyphKey& key) noexcept -> ParseResult override;
	};
} // namespace gal::prometheus::gfx

#endif
