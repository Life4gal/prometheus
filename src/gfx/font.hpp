// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <filesystem>
#include <memory>

#include <gfx/texture.hpp>
#include <gfx/type.hpp>

#include <functional/enumeration.hpp>
#include <memory/reference_wrapper.hpp>

namespace gal::prometheus::gfx
{
	enum class GlyphFlag : std::uint8_t
	{
		NONE = 0,
		BOLD = 1 << 0,
		ITALIC = 1 << 1,
	};

	/**
	 * @brief Glyph
	 */
	class GlyphKey final
	{
	public:
		std::uint32_t codepoint;
		std::uint32_t size : 26;
		GlyphFlag flag : 6;

		[[nodiscard]] constexpr auto operator==(const GlyphKey& other) const noexcept -> bool
		{
			return codepoint == other.codepoint and size == other.size and flag == other.flag;
		}

		struct hasher
		{
			[[nodiscard]] auto operator()(const GlyphKey& key) const noexcept -> std::size_t
			{
				return std::hash<std::uint32_t>{}(key.codepoint) ^ std::hash<std::uint32_t>{}(key.size) ^ std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(key.flag));
			}
		};
	};

	/**
	 * @brief Information about a glyph
	 */
	class GlyphInfo final
	{
	public:
		using value_type = extent_type::value_type;
		using uv_type = primitive::basic_rect_2d<uv_type::value_type>;

		// =============
		// Data filled when loading glyph
		// =============

		// Bitmap infos of this glyph
		rect_type rect{-1, -1, -1, -1};
		value_type advance_x{-1};
		bool visible{false};
		bool colored{false};

		// =============
		// Data filled when writing texture
		// =============

		// The id of the texture atlas where the glyph is located
		// This id is present if and only if the glyph is in a texture atlas, otherwise it is invalid_texture_atlas_id
		texture_atlas_id_type texture_atlas_id{invalid_texture_atlas_id};
		uv_type uv{-1, -1, -1, -1};
	};

	/**
	 * @brief Glyph data parsed by @c GlyphParser::parse,
	 * which references @c GlyphInfo (to set its @c texture_atlas_id and @c uv) and holds the bitmap data for the glyph (which is automatically released after writing it to the texture atlas)
	 */
	class GlyphParsedInfo final
	{
	public:
		using info_type = memory::RefWrapper<GlyphInfo>;
		using data_type = Texture::data_type;

	private:
		info_type info_;
		data_type data_;

	public:
		/**
		 * @param info Glyph info
		 * @param data Glyph bitmap data
		 *
		 * @link FontFace::find_or_parse_glyph
		 * @endlink 
		 */
		GlyphParsedInfo(GlyphInfo& info, data_type data) noexcept;

		/**
		 * @brief Upload the glyph data to the texture atlas, set its @c texture_atlas_id and @c uv
		 */
		auto upload(TextureContext& texture_context) noexcept -> void;
	};

	/**
	 * @brief Parse glyph data from (binary) font data
	 */
	class GlyphParser
	{
	public:
		class [[nodiscard]] LoadResult final
		{
		public:
			std::string name;
			font_id_type id;

			[[nodiscard]] explicit operator bool() const noexcept
			{
				return id != invalid_font_id;
			}

			[[nodiscard]] auto valid() const noexcept -> bool
			{
				return operator bool();
			}
		};

		class [[nodiscard]] ParseResult final
		{
		public:
			GlyphInfo info;

			// todo: Borrows a memory region from the texture to write to, rather than having it allocated by the parser
			GlyphParsedInfo::data_type data;

			[[nodiscard]] explicit operator bool() const noexcept
			{
				return data != nullptr;
			}

			[[nodiscard]] auto valid() const noexcept -> bool
			{
				return operator bool();
			}
		};

		GlyphParser(const GlyphParser&) noexcept = delete;
		GlyphParser(GlyphParser&&) noexcept = default;
		auto operator=(const GlyphParser&) noexcept -> GlyphParser& = delete;
		auto operator=(GlyphParser&&) noexcept -> GlyphParser& = default;

		virtual ~GlyphParser() noexcept;

		GlyphParser() noexcept = default;

		[[nodiscard]] virtual auto ready() noexcept -> bool = 0;

		/**
		 * @brief Load font data from file, get all glyph data, return id of font
		 * @param path Font file path
		 * @return id of the font, or invalid_font_id if failed to load
		 */
		[[nodiscard]] virtual auto load(const std::filesystem::path& path) noexcept -> LoadResult = 0;

		/**
		 * @brief Load font data from @c data, get all glyph data, return id of font
		 * @param data Font data
		 * @param size Font data length
		 * @return id of the font, or invalid_font_id if failed to load
		 * @note Transfer ownership of the font data, the caller does not need to free memory
		 */
		[[nodiscard]] virtual auto load(std::unique_ptr<std::uint8_t> data, std::size_t size) noexcept -> LoadResult = 0;

		/**
		 * @brief Load font data from @c data, get all glyph data, return id of font
		 * @param data Font data
		 * @return id of the font, or invalid_font_id if failed to load
		 * @note Copy font data, caller needs to free memory
		 */
		[[nodiscard]] virtual auto load(std::span<std::uint8_t> data) noexcept -> LoadResult = 0;

		/**
		 * @brief Determines whether the target font contains the glyphs of the specified codepoint
		 * @param id The id returned by loading the font from the previous load
		 * @param codepoint The codepoint of the glyph to be checked
		 * @return Exists or not
		 */
		[[nodiscard]] virtual auto has_glyph(font_id_type id, std::uint32_t codepoint) const noexcept -> bool = 0;

		/**
		 * @brief Get the glyph information of the specified size (and style) of the target codepoint, if it does not exist then data is a null pointer
		 * @param id The id returned by loading the font from the previous load
		 * @param key {codepoint, size, flag}
		 * @return The glyph information of the specified size (and style) of the target codepoint
		 */
		[[nodiscard]] virtual auto parse(font_id_type id, const GlyphKey& key) noexcept -> ParseResult = 0;

		/**
		 * @brief Get the glyph information of the specified size (and style) of the target codepoint, if it does not exist then data is a null pointer
		 * @param id The id returned by loading the font from the previous load
		 * @param codepoint The codepoint of the glyph
		 * @param size The size of the glyph
		 * @param flag The style of the glyph (bold, italic, etc.)
		 * @return The glyph information of the specified size (and style) of the target codepoint
		 */
		[[nodiscard]] auto parse(font_id_type id, std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> ParseResult;
	};

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

		std::vector<GlyphParsedInfo> parsed_infos_upload_queue_;

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

// ReSharper disable once CppRedundantNamespaceDefinition
namespace gal::prometheus::meta::user_defined
{
	template<>
	struct enum_is_flag<gfx::GlyphFlag> : std::true_type {};
} // namespace gal::prometheus::meta::user_defined
