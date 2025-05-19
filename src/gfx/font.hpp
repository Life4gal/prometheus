// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include <unordered_map>

#include <gfx/type.hpp>
#include <gfx/glyph.hpp>

#include <memory/reference_wrapper.hpp>
#include <functional/function_ref.hpp>

namespace gal::prometheus::gfx
{
	class FontGlyphQueue final
	{
	public:
		struct element_type
		{
			memory::RefWrapper<GlyphInfo> info;
			GlyphParser::ParseResult result;
		};

		using list_type = std::vector<element_type>;

	private:
		list_type list_;

	public:
		FontGlyphQueue(const FontGlyphQueue&) noexcept = delete;
		FontGlyphQueue(FontGlyphQueue&&) noexcept = default;
		auto operator=(const FontGlyphQueue&) noexcept -> FontGlyphQueue& = delete;
		auto operator=(FontGlyphQueue&&) noexcept -> FontGlyphQueue& = default;

		~FontGlyphQueue() noexcept = default;

		FontGlyphQueue() noexcept = default;

		auto push(GlyphInfo& info, GlyphParser::ParseResult&& result) noexcept -> void;

		auto upload(TextureContext& context) noexcept -> void;
	};

	class Font final
	{
	public:
		using name_type = std::string;

	private:
		name_type name_;
		font_id_type id_;

		std::unordered_map<GlyphKey, GlyphInfo, GlyphKey::hasher> glyphs_;

	public:
		Font(const Font&) noexcept = delete;
		Font(Font&&) noexcept = default;
		auto operator=(const Font&) noexcept -> Font& = delete;
		auto operator=(Font&&) noexcept -> Font& = default;

		~Font() noexcept = default;

		Font(name_type name, font_id_type id) noexcept;

		/**
		 * @brief Font name (obtained on GlyphParser::load)
		 */
		[[nodiscard]] auto name() const noexcept -> std::string_view;

		/**
		 * @brief Font id (obtained on GlyphParser::load)
		 */
		[[nodiscard]] auto id() const noexcept -> font_id_type;

		/**
		 * @brief Get the glyph information of the specified codepoint, if it can't be found, then return a null pointer
		 * @param key {codepoint, size, flag}
		 * @return The glyph information of the specified codepoint, or a null pointer if it can't be found
		 */
		[[nodiscard]] auto get_glyph(const GlyphKey& key) const noexcept -> const GlyphInfo*;

		/**
		 * @brief Set the glyph information of the specified codepoint, override if it already exists
		 * @param key {codepoint, size, flag}
		 * @param result The parse result of the specified codepoint
		 * @return GlyphInfo after insertion
		 */
		[[nodiscard]] auto set_glyph(const GlyphKey& key, const GlyphParser::ParseResult& result) noexcept -> GlyphInfo&;
	};

	class FontLoadQueue final
	{
	public:
		struct font_data_type
		{
			using element_type = std::uint8_t;
			using data_type = std::unique_ptr<std::uint8_t>;
			using size_type = std::uint32_t;

			data_type data;
			size_type size;
		};

		using list_type = std::vector<font_data_type>;

	private:
		list_type list_;
		list_type::difference_type new_font_index_;

	public:
		FontLoadQueue(const FontLoadQueue&) noexcept = delete;
		FontLoadQueue(FontLoadQueue&&) noexcept = default;
		auto operator=(const FontLoadQueue&) noexcept -> FontLoadQueue& = delete;
		auto operator=(FontLoadQueue&&) noexcept -> FontLoadQueue& = default;

		~FontLoadQueue() noexcept = default;

		FontLoadQueue() noexcept;

		auto push(const std::filesystem::path& path) noexcept -> bool;

		auto upload(GlyphParser& parser, functional::function_reference_wrapper<void(Font&&)> font_dest) noexcept -> void;
	};

	class Fonts final
	{
	public:
		using font_list_type = std::vector<Font>;

		using font_glyph_queue_list_type = std::unordered_map<Font*, FontGlyphQueue>;

	private:
		font_list_type font_list_;
		FontLoadQueue font_load_queue_;

		const GlyphInfo* fallback_glyph_;
		font_glyph_queue_list_type font_glyph_queue_;

		GlyphParser* parser_;

	public:
		Fonts(const Fonts&) noexcept = delete;
		Fonts(Fonts&&) noexcept = default;
		auto operator=(const Fonts&) noexcept -> Fonts& = delete;
		auto operator=(Fonts&&) noexcept -> Fonts& = default;

		~Fonts() noexcept = default;

		Fonts() noexcept;

		/**
		 * @brief Bind parser, default parser is null pointer, must bind parser before loading fonts
		 * @return The previously bound parser, or nullptr if no parser is bound
		 */
		auto bind_parser(GlyphParser& parser) noexcept -> GlyphParser*;

		/**
		 * @brief Load fonts from the specified path, assuming the path is a valid font file
		 * @param path Font path
		 * @return Returns true if the file exists and was opened successfully (without checking if it is a valid font file), otherwise returns false
		 */
		auto add_font(const std::filesystem::path& path) noexcept -> bool;

		/**
		 * @brief Load the fonts previously added by @c add_font
		 * @note This function is usually called at initialization time (or at every frame if needed) to load all the required fonts
		 */
		auto load_all_font() noexcept -> void;

		auto set_fallback_glyph() noexcept -> void;

		/**
		 * @brief Set the fallback glyph, if we can't find the glyph of the specified codepoint, then use the fallback glyph
		 * @param key {codepoint, size, flag}
		 */
		auto set_fallback_glyph(const GlyphKey& key) noexcept -> void;

		/**
		 * @brief Set the fallback glyph, if we can't find the glyph of the specified codepoint, then use the fallback glyph
		 */
		auto set_fallback_glyph(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) noexcept -> void;

		[[nodiscard]] auto glyph_of(const GlyphKey& key) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>;

		[[nodiscard]] auto glyph_of_or_fallback(const GlyphKey& key) const noexcept -> const GlyphInfo&;
		[[nodiscard]] auto glyph_of_or_fallback(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) const noexcept -> const GlyphInfo&;
		[[nodiscard]] auto glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag) const noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>;

		/**
		 * @brief Upload all used glyphs to the texture (if it is not already uploaded)
		 * @note This function is usually called every frame (unless all the needed glyphs have been uploaded to the texture, but it can still be called) to upload all new (previously unused) glyphs to the texture
		 */
		auto load_all_glyph(TextureContext& context) noexcept -> void;
	};
} // namespace gal::prometheus::gfx
