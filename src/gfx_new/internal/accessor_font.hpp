// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <vector>

#include <gfx_new/internal/type.hpp>
#include <gfx_new/internal/font.hpp>

namespace gal::prometheus::gfx_new
{
	class FontContext final
	{
	public:
		using font_list_type = std::vector<Font>;
		using glyph_upload_queue_list_type = std::unordered_map<Font*, GlyphUploadQueue>;

	private:
		font_list_type font_list_;
		FontLoadQueue font_load_queue_;

		glyph_upload_queue_list_type glyph_upload_queue_list_;

		GlyphParser* parser_;
		const GlyphInfo* fallback_glyph_;

	public:
		FontContext(const FontContext&) noexcept = delete;
		FontContext(FontContext&&) noexcept = default;
		auto operator=(const FontContext&) noexcept -> FontContext& = delete;
		auto operator=(FontContext&&) noexcept -> FontContext& = default;

		~FontContext() noexcept = default;

		FontContext() noexcept = default;

		auto set_glyph_parser(GlyphParser& parser) noexcept -> GlyphParser*;

		auto set_fallback_glyph() noexcept -> void;

		/**
		 * @brief Load font from the specified path, assuming the path is a valid font file
		 * @param path Font path
		 * @return Returns true if the file exists and was opened successfully (without checking if it is a valid font file), otherwise returns false
		 */
		auto add_font(const std::filesystem::path& path) noexcept -> bool;

		/**
		 * @brief Load the fonts previously added by @c add_font
		 * @note This function is usually called at initialization time (or at every frame if needed) to load all the required fonts
		 */
		auto load_all_font() noexcept -> void;

		[[nodiscard]] auto glyph_of(const GlyphKey& key) const noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) const noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) const noexcept -> std::vector<const GlyphInfo*>;

		[[nodiscard]] auto glyph_of_or_fallback(const GlyphKey& key) const noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of_or_fallback(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) const noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag) const noexcept -> std::vector<const GlyphInfo*>;

		[[nodiscard]] auto glyph_of(const GlyphKey& key) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>;

		[[nodiscard]] auto glyph_of_or_fallback(const GlyphKey& key) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of_or_fallback(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>;

		/**
		 * @brief Upload all used glyphs to the texture (if it is not already uploaded)
		 * @note This function is usually called every frame (unless all the needed glyphs have been uploaded to the texture, but it can still be called) to upload all new (previously unused) glyphs to the texture
		 */
		auto upload_all_glyph(TextureContext& context) noexcept -> void;
	};

	/**
	 * @brief Proxy class for accessing the Renderer's private interface (this class helps us not to expose too many implementation details to the outside world)
	 */
	class Renderer::AccessorFont final
	{
	public:
		using renderer_type = memory::RefWrapper<Renderer>;

	private:
		renderer_type renderer_;

	public:
		explicit AccessorFont(Renderer& renderer) noexcept;

		[[nodiscard]] auto context() noexcept -> FontContext&;
	};
}
