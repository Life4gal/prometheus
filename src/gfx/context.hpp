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

		texture_atlases_type texture_atlases_;
		territories_type territories_;

		Fonts fonts_;

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
		 * @brief Get the texture atlas for the specified id
		 * @param id Texture atlas id
		 * @return Texture atlas
		 */
		[[nodiscard]] auto select_atlas(texture_atlas_id_type id) const noexcept -> const Texture&;

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
		 * @brief Get root (default) texture
		 */
		[[nodiscard]] auto root_texture() const noexcept -> texture_id_type;

		/**
		 * @brief Get texture atlas for glyph information
		 */
		[[nodiscard]] auto atlas_of(const GlyphInfo& info) const noexcept -> const Texture&;

		/**
		 * @brief Bind parser, default parser is null pointer, must bind parser before loading fonts
		 */
		auto bind_parser(GlyphParser& parser) noexcept -> void;

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
		[[nodiscard]] auto glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> std::vector<const GlyphInfo*>;
		// [[nodiscard]] auto glyph_of(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> std::vector<const GlyphInfo*>;

		[[nodiscard]] auto glyph_of_or_fallback(const GlyphKey& key) const noexcept -> const GlyphInfo&;
		[[nodiscard]] auto glyph_of_or_fallback(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) const noexcept -> const GlyphInfo&;
		[[nodiscard]] auto glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) const noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>;
		// [[nodiscard]] auto glyph_of_or_fallback(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) const noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>;

		// /**
		//  * @brief The minimum space to be occupied if the specified codepoint is to be rendered in its entirety
		//  * @param codepoint
		//  * @param size
		//  * @param flag
		//  * @return
		//  */
		// [[nodiscard]] auto size_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;
		//
		// /**
		//  * @brief The minimum space to be occupied if the specified text is to be rendered in its entirety
		//  * @param text
		//  * @param size
		//  * @param flag
		//  * @return
		//  */
		// [[nodiscard]] auto size_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;
		//
		// /**
		//  * @brief The minimum space to be occupied if the specified text is to be rendered in its entirety
		//  * @param text
		//  * @param size
		//  * @param flag
		//  * @return
		//  */
		// [[nodiscard]] auto size_of(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;

		/**
		 * @brief Upload all used glyphs to the texture (if it is not already uploaded)
		 * @note This function is usually called every frame (unless all the needed glyphs have been uploaded to the texture, but it can still be called) to upload all new (previously unused) glyphs to the texture
		 */
		auto load_all_glyph() noexcept -> void;

		struct parsed_info_upload_result_type
		{
			texture_atlas_id_type texture_atlas_id;
			GlyphInfo::uv_type uv;
		};

		/**
		 * @brief Write Font uploaded glyph data to texture
		 * @note @c FontGlyphQueue calls this function to upload glyph data to the texture and set its texture atlas ID and UV coordinates
		 */
		auto upload_parsed_info_to_texture(const GlyphParser::ParseResult& result) noexcept -> parsed_info_upload_result_type;

		/**
		 * @brief Upload all texture atlas (if it didn't upload or needs to be re-uploaded)
		 * @note This function is usually called every frame to upload the texture to the GPU, or to update the texture (if new glyph data is written)
		 */
		auto upload_all_texture(Renderer& renderer) noexcept -> void;
	};

	class RenderContext final
	{
	public:
		using render_lists_type = std::vector<RenderList>;

	private:
		TextureContext texture_context_;

		RenderListSharedData render_list_shared_data_;

		render_lists_type render_lists_;

	public:
		RenderContext(const RenderContext&) noexcept = delete;
		RenderContext(RenderContext&&) noexcept = default;
		auto operator=(const RenderContext&) noexcept -> RenderContext& = delete;
		auto operator=(RenderContext&&) noexcept -> RenderContext& = default;

		~RenderContext() noexcept;

		RenderContext() noexcept;

		auto initialize() noexcept -> void;

		auto begin_frame(Renderer& renderer) noexcept -> void;

		auto end_frame(Renderer& renderer) noexcept -> void;

		// ====================================================================
		// TextureContext
		// ====================================================================

		/**
		 * @brief Get root (default) texture
		 */
		[[nodiscard]] auto root_texture() const noexcept -> texture_id_type;

		/**
		 * @brief Get texture atlas for glyph information
		 */
		[[nodiscard]] auto atlas_of(const GlyphInfo& info) const noexcept -> const Texture&;

		/**
		 * @brief Bind parser, default parser is null pointer, must bind parser before loading fonts
		 */
		auto bind_parser(GlyphParser& parser) noexcept -> void;

		/**
		 * @brief Load fonts from the specified path, assuming the path is a valid font file
		 * @param path Font path
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
		[[nodiscard]] auto glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> std::vector<const GlyphInfo*>;
		// [[nodiscard]] auto glyph_of(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> std::vector<const GlyphInfo*>;

		[[nodiscard]] auto glyph_of_or_fallback(const GlyphKey& key) const noexcept -> const GlyphInfo&;
		[[nodiscard]] auto glyph_of_or_fallback(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) const noexcept -> const GlyphInfo&;
		[[nodiscard]] auto glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) const noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>;
		// [[nodiscard]] auto glyph_of_or_fallback(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) const noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>;

		// /**
		//  * @brief The minimum space to be occupied if the specified codepoint is to be rendered in its entirety
		//  * @param codepoint
		//  * @param size
		//  * @param flag
		//  * @return
		//  */
		// [[nodiscard]] auto size_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;
		//
		// /**
		//  * @brief The minimum space to be occupied if the specified text is to be rendered in its entirety
		//  * @param text
		//  * @param size
		//  * @param flag
		//  * @return
		//  */
		// [[nodiscard]] auto size_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;
		//
		// /**
		//  * @brief The minimum space to be occupied if the specified text is to be rendered in its entirety
		//  * @param text
		//  * @param size
		//  * @param flag
		//  * @return
		//  */
		// [[nodiscard]] auto size_of(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;

		/**
		 * @brief Upload all used glyphs to the texture (if it is not already uploaded)
		 * @note This function is usually called every frame (unless all the needed glyphs have been uploaded to the texture, but it can still be called) to upload all new (previously unused) glyphs to the texture
		 */
		auto load_all_glyph() noexcept -> void;

		/**
		 * @brief Upload all texture atlas (if it didn't upload or needs to be re-uploaded)
		 * @note This function is usually called every frame to upload the texture to the GPU, or to update the texture (if new glyph data is written)
		 */
		auto upload_all_texture(Renderer& renderer) noexcept -> void;

		// ====================================================================
		// RenderListSharedData
		// ====================================================================

		[[nodiscard]] auto render_list_shared_data() const noexcept -> const RenderListSharedData&;

		// ====================================================================
		// RenderList
		// ====================================================================

		[[nodiscard]] auto render_data() const noexcept -> std::vector<RenderData>;

		// test
		[[nodiscard]] auto test_render_list() noexcept -> RenderList&;
	};
} // namespace gal::prometheus::gfx
