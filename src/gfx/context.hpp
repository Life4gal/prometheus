// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <filesystem>

#include <gfx/font.hpp>
#include <gfx/render_list.hpp>
#include <gfx/texture.hpp>
#include <gfx/type.hpp>

namespace gal::prometheus::gfx
{
	class TextureContext final
	{
	public:
		using texture_atlases_type = std::vector<Texture>;
		using font_faces_type = std::vector<FontFace>;

	private:
		class Territory final
		{
		public:
			using point_type = Texture::point_type;
			using size_type = Texture::size_type;

			texture_atlas_id_type id{invalid_texture_atlas_id};

			point_type point{0, 0};
			size_type size{0, 0};
		};

		using territories_type = std::vector<Territory>;

		using font_pending_load_datas = std::vector<FontPendingLoadData>;

		GlyphParser* parser_;

		texture_atlases_type texture_atlases_;
		territories_type territories_;

		font_faces_type font_faces_;
		font_pending_load_datas font_pending_load_datas_;

		/**
		 * @brief Retain at least one texture atlas (root)
		 */
		[[nodiscard]] auto root_atlas() noexcept -> Texture&;

		/**
		 * @brief Retain at least one texture atlas (root)
		 */
		[[nodiscard]] auto root_atlas() const noexcept -> const Texture&;

		/**
		 * @brief Get the texture atlas for the specified id
		 * @param id Texture atlas id
		 * @return Texture atlas
		 */
		[[nodiscard]] auto select_atlas(texture_atlas_id_type id) noexcept -> Texture&;

		/**
		 * @brief Gets a texture atlas large enough to hold the specified @c size sub texture
		 * @param size sub texture size
		 * @return Texture atlas id
		 */
		[[nodiscard]] auto select_atlas(Texture::size_type size) const noexcept -> texture_atlas_id_type;

		/**
		 * @brief Find a suitable texture atlas based on @c size, and then find a suitable region on it (for writing sub texture)
		 * @param size Sub texture size
		 * @return Region on the texture atlas
		 */
		auto make_territory(Texture::size_type size) noexcept -> BorrowTexture;

	public:
		TextureContext(const TextureContext&) noexcept = delete;
		TextureContext(TextureContext&&) noexcept = default;
		auto operator=(const TextureContext&) noexcept -> TextureContext& = delete;
		auto operator=(TextureContext&&) noexcept -> TextureContext& = default;
		~TextureContext() noexcept = default;

		TextureContext() noexcept;

		/**
		 * @brief Initialization, usually used to set up RenderListSharedData's anti-aliased lines (uv) and initialize all font faces
		 */
		auto initialize(RenderListSharedData& shared_data) noexcept -> void;

		/**
		 * @brief Bind parser, default parser is null pointer, must bind parser before loading fonts
		 */
		auto bind_parser(GlyphParser& parser) noexcept -> void;

		/**
		 * @brief Get the current parser, usually used by FontFace to load glyph data
		 */
		[[nodiscard]] auto parser() const noexcept -> GlyphParser&;

		/**
		 * @brief Load fonts from the specified path, assuming the path is a valid font file
		 * @param path Font path
		 */
		auto add_font(const std::filesystem::path& path) noexcept -> bool;

		/**
		 * @brief Load fonts from the specified path, assuming the path is a valid font file
		 * @param path Font path
		 */
		auto add_font(std::string_view path) noexcept -> bool;

		/**
		 * @brief Get root (default) texture
		 */
		[[nodiscard]] auto root_texture() const noexcept -> texture_id_type;

		/**
		 * @brief Get texture atlas for glyph information
		 */
		[[nodiscard]] auto atlas_of(const GlyphInfo& info) noexcept -> const Texture&;

		[[nodiscard]] auto glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> std::vector<const GlyphInfo*>;

		/**
		 * @brief The minimum space to be occupied if the specified codepoint is to be rendered in its entirety
		 * @param codepoint
		 * @param size
		 * @param flag
		 * @return
		 */
		[[nodiscard]] auto size_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;

		/**
		 * @brief The minimum space to be occupied if the specified text is to be rendered in its entirety
		 * @param text
		 * @param size
		 * @param flag
		 * @return
		 */
		[[nodiscard]] auto size_of(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;

		// =============================================================================================================================

		/**
		 * @brief Load the fonts previously added by @c add_font
		 */
		auto load_all_font() noexcept -> void;

		/**
		 * @brief Upload all used glyphs to the texture (if it is not already uploaded)
		 */
		auto upload_all_font_face() noexcept -> void;

		/**
		 * @brief Write FontFace uploaded glyph data to texture, also set the @c texture_atlas_id and @c uv coordinates for this glyph data
		 */
		auto upload_glyph_to_texture(GlyphInfo& info, const GlyphParsedInfo::data_type& data) noexcept -> void;

		/**
		 * @brief Upload all texture atlas (if it didn't upload or needs to be re-uploaded)
		 */
		auto upload_all_texture(Renderer& renderer) noexcept -> void;
	};

	class RendererContext final
	{
	public:
		using render_lists_type = std::vector<RenderList>;

	private:
		TextureContext texture_context_;
		RenderListSharedData render_list_shared_data_;

		render_lists_type render_lists_;

	public:
		RendererContext(const RendererContext&) noexcept = delete;
		RendererContext(RendererContext&&) noexcept = default;
		auto operator=(const RendererContext&) noexcept -> RendererContext& = delete;
		auto operator=(RendererContext&&) noexcept -> RendererContext& = default;

		~RendererContext() noexcept;

		RendererContext() noexcept;

		auto initialize() noexcept -> void;

		auto begin_frame(Renderer& renderer) noexcept -> void;

		auto end_frame() noexcept -> void;

		[[nodiscard]] auto texture_context() noexcept -> TextureContext&;
		[[nodiscard]] auto texture_context() const noexcept -> const TextureContext&;

		[[nodiscard]] auto render_list_shared_data() const noexcept -> const RenderListSharedData&;
	};
} // namespace gal::prometheus::gfx
