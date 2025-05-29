// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <unordered_map>

#include <gfx/glyph.hpp>

#include <gfx/internal/texture.hpp>

#include <memory/reference_wrapper.hpp>
#include <functional/function_ref.hpp>

namespace gal::prometheus::gfx
{
	static_assert(std::is_same_v<GlyphDescriptor::data_type, Texture::data_type>);

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

		[[nodiscard]] constexpr explicit(false) operator GlyphCode() const noexcept
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
		using rect_type = GlyphDescriptor::rect_type;
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
		// Data filled when writing texture (if and only if the glyph is visible)
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

			Texture::data_type data;
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

		auto push(GlyphInfo& info, Texture::data_type&& data) noexcept -> void;

		auto upload(TextureContext& context) noexcept -> void;
	};

	class GlyphContext final
	{
	public:
		using cached_glyphs_type = std::unordered_map<GlyphKey, GlyphInfo, GlyphKey::hasher>;

		GlyphParser** glyph_parser;

	private:
		cached_glyphs_type cached_glyphs_;

		GlyphUploadQueue upload_queue_;

	public:
		GlyphContext(const GlyphContext&) noexcept = delete;
		GlyphContext(GlyphContext&&) noexcept = default;
		auto operator=(const GlyphContext&) noexcept -> GlyphContext& = delete;
		auto operator=(GlyphContext&&) noexcept -> GlyphContext& = default;

		~GlyphContext() noexcept = default;

		GlyphContext() noexcept = default;

		[[nodiscard]] auto glyph_of(const GlyphKey& key) noexcept -> const GlyphInfo&;
		[[nodiscard]] auto glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) noexcept -> const GlyphInfo&;
		[[nodiscard]] auto glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>;

		/**
		 * @brief Upload all used glyphs to the texture (if it is not already uploaded)
		 * @note This function is usually called every frame (unless all the needed glyphs have been uploaded to the texture, but it can still be called) to upload all new (previously unused) glyphs to the texture
		 */
		auto upload_all_glyph(TextureContext& context) noexcept -> void;
	};
}
