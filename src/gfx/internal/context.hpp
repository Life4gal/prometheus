// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx/gfx.hpp>
#include <gfx/internal/texture.hpp>
#include <gfx/internal/font.hpp>

#include <memory/reference_wrapper.hpp>

namespace gal::prometheus::gfx
{
	// =========================================================
	// TEXTURE
	// =========================================================

	class TextureContext final
	{
	public:
		using texture_atlas_list_type = std::vector<Texture>;

	private:
		texture_atlas_list_type texture_atlas_list_;

		[[nodiscard]] auto root_id() const noexcept -> texture_atlas_id_type;

	public:
		TextureContext(const TextureContext&) noexcept = delete;
		TextureContext(TextureContext&&) noexcept = default;
		auto operator=(const TextureContext&) noexcept -> TextureContext& = delete;
		auto operator=(TextureContext&&) noexcept -> TextureContext& = default;

		~TextureContext() noexcept = default;

		TextureContext() noexcept;

		auto initialize(RenderListSharedData& shared_data) noexcept -> void;

		/**
		 * @brief Get root (default) texture
		 */
		[[nodiscard]] auto root() noexcept -> Texture&;

		/**
		 * @brief Get root (default) texture
		 */
		[[nodiscard]] auto root() const noexcept -> const Texture&;

		/**
		 * @brief Get texture of id
		 */
		[[nodiscard]] auto select(texture_atlas_id_type texture_atlas_id) noexcept -> Texture&;

		/**
		 * @brief Get texture of id
		 */
		[[nodiscard]] auto select(texture_atlas_id_type texture_atlas_id) const noexcept -> const Texture&;

		/**
		 * @brief Writes the given glyph data to the specified texture
		 * @param texture_atlas_id id of the specified texture
		 * @param data Glyph data to write
		 * @param size Size of data (rectangle area)
		 * @return The uv coordinate of the position where the data is written
		 */
		auto write(
			texture_atlas_id_type texture_atlas_id,
			Texture::data_view_type data,
			Texture::size_type size
		) noexcept -> primitive::basic_rect_2d<Texture::uv_type::value_type>;

		struct random_write_result_type
		{
			texture_atlas_id_type texture_atlas_id;
			primitive::basic_rect_2d<Texture::uv_type::value_type> uv;
		};

		/**
		 * @brief Writes the given glyph data to any holdable texture
		 * @param data Glyph data to write
		 * @param size Size of data (rectangle area)
		 * @return The id of the written texture atlas and the uv coordinate of the position where the data is written
		 */
		auto write(
			Texture::data_view_type data,
			Texture::size_type size
		) noexcept -> random_write_result_type;

		/**
		 * @brief Upload all texture atlas (if it didn't upload or needs to be re-uploaded)
		 * @note This function is usually called every frame to upload the texture to the GPU, or to update the texture (if new glyph data is written)
		 */
		auto upload(const Context& context) noexcept -> void;
	};

	// =========================================================
	// FONT
	// =========================================================

	class FontContext final
	{
	public:
		using font_list_type = std::vector<Font>;
		using glyph_upload_queue_list_type = std::unordered_map<Font*, GlyphUploadQueue>;

	private:
		font_list_type font_list_;
		FontLoadQueue font_load_queue_;

		glyph_upload_queue_list_type glyph_upload_queue_list_;

		GlyphParser* glyph_parser_;
		const GlyphInfo* fallback_glyph_;

	public:
		FontContext(const FontContext&) noexcept = delete;
		FontContext(FontContext&&) noexcept = default;
		auto operator=(const FontContext&) noexcept -> FontContext& = delete;
		auto operator=(FontContext&&) noexcept -> FontContext& = default;

		~FontContext() noexcept = default;

		FontContext() noexcept = default;

		auto set_glyph_parser(GlyphParser& glyph_parser) noexcept -> void;

		auto set_fallback_glyph() noexcept -> void;

		/**
		 * @brief Load font from the specified path, assuming the path is a valid font file
		 * @param path Font path
		 */
		auto add_font(const std::filesystem::path& path) noexcept -> void;

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

	// =========================================================
	// CONTEXT
	// =========================================================

	class Context final
	{
		// xxx_context
		friend Renderer;

	public:
		class RendererAccessor final
		{
			// TextureContext::upload
			friend TextureContext;

		public:
			using renderer_type = memory::RefWrapper<Renderer>;

		private:
			renderer_type renderer_;

		public:
			explicit RendererAccessor(Renderer& renderer) noexcept;

		private:
			/**
			 * @brief Upload texture atlas data to GPU and get GPU resource handle
			 */
			auto upload(Texture& texture) noexcept -> void;

			/**
			 * @brief Update new texture atlas data to GPU (overwrite previous texture atlas data)
			 * @note Does not check for the need to update
			 */
			auto update(Texture& texture) noexcept -> void;

			/**
			 * @brief Update new texture atlas data to GPU (overwrite previous texture atlas data)
			 * @note If no update is needed (not dirty) then do nothing
			 */
			auto update_if_dirty(Texture& texture) noexcept -> void;
		};

		class TextureAccessor final
		{
			// RenderList::RenderListContext::default_texture
			friend RenderList::RenderListContext;
			// RenderList::text (select texture atlas for text rendering)
			friend RenderList;

		public:

		private:
			memory::RefWrapper<const TextureContext> texture_context_;

		public:
			explicit TextureAccessor(TextureContext& texture_context) noexcept;

		private:
			[[nodiscard]] auto texture_context() const noexcept -> const TextureContext&;
		};

		class FontAccessor final
		{
			// RenderList::text
			// RenderList::text_size
			friend RenderList;
			friend auto add_font(Context& context, const std::filesystem::path& path) noexcept -> void;

		public:

		private:
			memory::RefWrapper<FontContext> font_context_;

		public:
			explicit FontAccessor(FontContext& font_context) noexcept;

		private:
			[[nodiscard]] auto font_context() noexcept -> FontContext&;
			[[nodiscard]] auto font_context() const noexcept -> const FontContext&;
		};

		class RenderListAccessor final
		{
			// RenderList::RenderListContext::shared_data
			friend RenderList::RenderListContext;

		public:

		private:
			memory::RefWrapper<const RenderListSharedData> render_list_shared_data_;

		public:
			explicit RenderListAccessor(RenderListSharedData& render_list_shared_data) noexcept;

		private:
			[[nodiscard]] auto shared_data() const noexcept -> const RenderListSharedData&;
		};

	private:
		std::shared_ptr<GlyphParser> glyph_parser_;
		std::shared_ptr<Renderer> renderer_;

		TextureContext texture_context_;
		FontContext font_context_;

		RenderListSharedData render_list_shared_data_;
		std::vector<RenderList> render_lists_;

	public:
		Context() noexcept;

		auto set_glyph_parser(std::shared_ptr<GlyphParser> glyph_parser) noexcept -> std::shared_ptr<GlyphParser>;
		auto set_renderer(std::shared_ptr<Renderer> renderer) noexcept -> std::shared_ptr<Renderer>;

		// todo: set RenderListSharedData

		[[nodiscard]] auto renderer_accessor() const noexcept -> RendererAccessor;
		[[nodiscard]] auto texture_accessor() noexcept -> TextureAccessor;
		[[nodiscard]] auto font_accessor() noexcept -> FontAccessor;
		[[nodiscard]] auto render_list_accessor() noexcept -> RenderListAccessor;

		[[nodiscard]] auto new_render_list(RenderListFlag flag) noexcept -> RenderList&;
		[[nodiscard]] auto render_data() const noexcept -> render_data_list_type;
	};
}
