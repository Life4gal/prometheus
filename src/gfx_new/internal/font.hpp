// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <unordered_map>
#include <filesystem>

#include <gfx_new/gfx.hpp>

#include <memory/reference_wrapper.hpp>
#include <functional/function_ref.hpp>

namespace gal::prometheus::gfx_new
{
	// index
	using texture_atlas_id_type = std::uint32_t;
	constexpr texture_atlas_id_type invalid_texture_atlas_id{std::numeric_limits<texture_atlas_id_type>::max()};

	class TextureContext;

	/**
	 * @brief This class is essentially the same as @c GlyphParser::GlyphCode, but takes up less memory space.
	 */
	class GlyphKey final
	{
	public:
		std::uint32_t codepoint;
		std::uint32_t size : 26;
		std::uint32_t flag : 6;

		constexpr GlyphKey(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept
			: codepoint{codepoint},
			  size{size},
			  flag{static_cast<std::uint32_t>(flag)} {}

		[[nodiscard]] constexpr auto operator==(const GlyphKey& other) const noexcept -> bool
		{
			return codepoint == other.codepoint and size == other.size and flag == other.flag;
		}

		[[nodiscard]] constexpr explicit(false) operator GlyphParser::GlyphCode() const noexcept
		{
			return {codepoint, size, static_cast<GlyphFlag>(flag)};
		}

		struct hasher
		{
			[[nodiscard]] auto operator()(const GlyphKey& key) const noexcept -> std::size_t
			{
				return std::hash<std::uint32_t>{}(key.codepoint) ^ std::hash<std::uint32_t>{}(key.size) ^ std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(key.flag));
			}
		};
	};

	static_assert(sizeof(GlyphKey) == sizeof(std::uint32_t) + sizeof(std::uint32_t));

	/**
	 * @brief Glyph info (bitmap info & texture info)
	 */
	class GlyphInfo final
	{
	public:
		using rect_type = GlyphParser::GlyphDescriptor::rect_type;
		using uv_type = primitive::basic_rect_2d<uv_type::value_type>;

		// =============
		// Data filled when loading glyph
		// =============

		// Bitmap infos of this glyph
		rect_type rect;
		float advance_x;
		bool visible;
		bool colored;

		// =============
		// Data filled when writing texture
		// =============

		// The id of the texture atlas where the glyph is located
		// This id is present if and only if the glyph is in a texture atlas, otherwise it is invalid_texture_atlas_id
		texture_atlas_id_type texture_atlas_id{invalid_texture_atlas_id};
		// The uv coordinate of the glyph in the texture atlas
		uv_type uv{-1, -1, -1, -1};
	};

	/**
	 * @brief Queue of glyph data to be uploaded to the texture
	 */
	class GlyphUploadQueue final
	{
	public:
		struct element_type
		{
			memory::RefWrapper<GlyphInfo> info;
			GlyphParser::GlyphDescriptor descriptor;
		};

		using list_type = std::vector<element_type>;

	private:
		list_type list_;

	public:
		GlyphUploadQueue(const GlyphUploadQueue&) noexcept = delete;
		GlyphUploadQueue(GlyphUploadQueue&&) noexcept = default;
		auto operator=(const GlyphUploadQueue&) noexcept -> GlyphUploadQueue& = delete;
		auto operator=(GlyphUploadQueue&&) noexcept -> GlyphUploadQueue& = default;

		~GlyphUploadQueue() noexcept = default;

		GlyphUploadQueue() noexcept = default;

		auto push(GlyphInfo& info, GlyphParser::GlyphDescriptor&& descriptor) noexcept -> void;

		auto upload(TextureContext& context) noexcept -> void;
	};

	class Font final
	{
	public:
		GlyphParser::FontDescriptor descriptor;
		std::unordered_map<GlyphKey, GlyphInfo, GlyphKey::hasher> cached_glyphs;

		/**
		 * @brief Get the glyph information of the specified codepoint, if it can't be found, then return a null pointer
		 * @param key {codepoint, size, flag}
		 * @return The glyph information of the specified codepoint, or a null pointer if it can't be found
		 */
		[[nodiscard]] auto get_glyph(const GlyphKey& key) const noexcept -> const GlyphInfo*;

		/**
		 * @brief Set the glyph information of the specified codepoint, override if it already exists
		 * @param key {codepoint, size, flag}
		 * @param glyph_descriptor The parse result of the specified codepoint
		 * @return GlyphInfo after insertion
		 */
		[[nodiscard]] auto set_glyph(const GlyphKey& key, const GlyphParser::GlyphDescriptor& glyph_descriptor) noexcept -> GlyphInfo&;
	};

	/**
	 * @brief Queue of font data to be loaded to the context
	 */
	class FontLoadQueue final
	{
		using descriptor = GlyphParser::FontDescriptor;

	public:
		using element_type = descriptor::element_type;
		using data_type = descriptor::data_type;
		using data_view_type = descriptor::data_view_type;

		using size_type = descriptor::size_type;

	private:
		class Descriptor final
		{
		public:
			data_type data;
			size_type size;
		};

		using list_type = std::vector<Descriptor>;

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
}
