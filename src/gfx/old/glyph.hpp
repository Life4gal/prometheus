// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx/type.hpp>
#include <gfx/texture.hpp>

namespace gal::prometheus::gfx
{
	/**
	 * @brief Glyph key
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
	 * @brief Glyph info (bitmap info & texture info)
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
		// The uv coordinate of the glyph in the texture atlas
		uv_type uv{-1, -1, -1, -1};
	};

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
			// We could have just returned a GlyphInfo,
			// but to make the purpose of the interface clearer,
			// we specify that we only return part of the GlyphInfo (see GlyphInfo->'Data filled when loading glyph')
			// GlyphInfo info;

			// Bitmap infos of this glyph
			rect_type rect{-1, -1, -1, -1};
			GlyphInfo::value_type advance_x{-1};
			bool visible{false};
			bool colored{false};

			// todo: Borrows a memory region from the texture to write to, rather than having it allocated by the parser
			Texture::data_type data{nullptr};

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
		 * @brief Load font data from @c data, get all glyph data, return id of font
		 * @param data Font data
		 * @return id of the font, or invalid_font_id if failed to load
		 * @note Font data *should not* be released unless the target font is no longer needed (to call @c parse)
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
	};
}
