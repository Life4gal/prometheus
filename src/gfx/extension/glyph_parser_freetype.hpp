// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <vector>

#include <gfx/glyph.hpp>

#include <memory/unique_ptr.hpp>

namespace gal::prometheus::gfx::extension
{
	class GlyphParserFreeType final : public GlyphParser
	{
		class Library;
		class FontInfo;

	public:
		using infos_type = std::vector<FontInfo>;

	private:
		memory::UniquePointer<Library> library_;
		infos_type infos_;

	public:
		GlyphParserFreeType(const GlyphParserFreeType&) noexcept = delete;
		GlyphParserFreeType(GlyphParserFreeType&&) noexcept = default;
		auto operator=(const GlyphParserFreeType&) noexcept -> GlyphParserFreeType& = delete;
		auto operator=(GlyphParserFreeType&&) noexcept -> GlyphParserFreeType& = default;

		~GlyphParserFreeType() noexcept override;

		GlyphParserFreeType() noexcept;

		auto load(std::string_view path) noexcept -> FontLoadResult override;

		[[nodiscard]] auto has_glyph(std::uint32_t codepoint) const noexcept -> bool override;

		auto parse(const GlyphCode& code) noexcept -> GlyphDescriptor override;
	};
}
