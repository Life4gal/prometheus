// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx/type.hpp>
#include <gfx/texture.hpp>
#include <gfx/glyph.hpp>

#include <memory/reference_wrapper.hpp>

namespace gal::prometheus::gfx
{
	/**
	 * @brief All the glyph data used in a font
	 */
	class FontFace final
	{
	public:
		using value_type = extent_type::value_type;
		using uv_type = primitive::basic_rect_2d<value_type>;

	private:
		memory::RefWrapper<TextureContext> context_;

		std::string name_;
		font_id_type id_;

		struct parsed_info_type
		{
			memory::RefWrapper<GlyphInfo> info;
			GlyphParser::ParseResult result;
		};

		std::vector<parsed_info_type> parsed_info_queue_;

		std::unordered_map<GlyphKey, GlyphInfo, GlyphKey::hasher> glyphs_;
		const GlyphInfo* fallback_glyph_;

		[[nodiscard]] auto find_or_parse_glyph(const GlyphKey& key) noexcept -> GlyphInfo*;

	public:
		FontFace(const FontFace&) noexcept = delete;
		FontFace(FontFace&&) noexcept = default;
		auto operator=(const FontFace&) noexcept -> FontFace& = delete;
		auto operator=(FontFace&&) noexcept -> FontFace& = default;

		~FontFace() noexcept = default;

		FontFace(TextureContext& context, std::string name, font_id_type id) noexcept;

		FontFace(TextureContext& context, std::string_view name, font_id_type id) noexcept;

		/**
		 * @brief Font name (obtained on GlyphParser::load)
		 */
		[[nodiscard]] auto name() const noexcept -> std::string_view;

		/**
		 * @brief Font id (obtained on GlyphParser::load)
		 */
		[[nodiscard]] auto id() const noexcept -> font_id_type;

		/**
		 * @brief Initialization, usually used to load default glyph data (for fallbacks when the desired glyph is not found)
		 */
		auto initialize() noexcept -> void;

		/**
		 * @brief Upload the glyph data used and not uploaded to the texture before to the texture
		 */
		auto upload() noexcept -> void;

		/**
		 * @brief Get the glyph information of the specified codepoint, if it can't be found, then return the fallback glyph information
		 * @param key {codepoint, size, flag}
		 * @return The glyph information of the specified codepoint, or the fallback glyph information if it can't be found
		 */
		[[nodiscard]] auto find_glyph(const GlyphKey& key) noexcept -> const GlyphInfo&;

		/**
		 * @brief Get the glyph information of the specified codepoint, if it can't be found, then return a null pointer
		 * @param key {codepoint, size, flag}
		 * @return The glyph information of the specified codepoint, or a null pointer if it can't be found
		 */
		[[nodiscard]] auto find_glyph_no_fallback(const GlyphKey& key) noexcept -> const GlyphInfo*;
	};
} // namespace gal::prometheus::gfx
