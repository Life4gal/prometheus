// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <vector>

#include <gfx/texture.hpp>

#include <gfx/internal/rect_pack.hpp>

namespace gal::prometheus::gfx
{
	// index
	using texture_atlas_id_type = std::uint32_t;
	constexpr texture_atlas_id_type invalid_texture_atlas_id{std::numeric_limits<texture_atlas_id_type>::max()};

	class RenderListSharedData;
	class Renderer;

	class TextureContext final
	{
	public:
		using element_type = Texture::element_type;
		using data_type = Texture::data_type;
		using data_view_type = Texture::data_view_type;

		using point_type = Texture::point_type;
		using size_type = Texture::size_type;

		static_assert(std::is_same_v<point_type, gfx::RectPackContext::point_type>);
		static_assert(std::is_same_v<size_type, gfx::RectPackContext::size_type>);

		using uv_scale_type = Texture::uv_scale_type;

		struct atlas_type
		{
			Texture texture;
			std::vector<TextureViewer> pending_update_data;

			gfx::RectPackContext rp_context;
		};

		using atlas_list_type = std::vector<atlas_type>;

	private:
		atlas_list_type atlas_list_;

		[[nodiscard]] auto root_id() const noexcept -> texture_atlas_id_type;

		[[nodiscard]] auto active_id() const noexcept -> texture_atlas_id_type;

		[[nodiscard]] auto select_atlas(texture_atlas_id_type texture_atlas_id) noexcept -> atlas_type&;

		[[nodiscard]] auto select_atlas(texture_atlas_id_type texture_atlas_id) const noexcept -> const atlas_type&;

		auto new_atlas(size_type size) noexcept -> atlas_type&;

		[[nodiscard]] auto write(texture_atlas_id_type texture_atlas_id, size_type size) noexcept -> TextureWriter;

	public:
		TextureContext(const TextureContext&) noexcept = delete;
		TextureContext(TextureContext&&) noexcept = default;
		auto operator=(const TextureContext&) noexcept -> TextureContext& = delete;
		auto operator=(TextureContext&&) noexcept -> TextureContext& = default;

		~TextureContext() noexcept = default;

		TextureContext() noexcept;

		auto initialize(RenderListSharedData& shared_data) noexcept -> void;

		/**
		 * @brief Upload all texture atlas if it didn't upload, or update them if it dirty
		 * @note This function is usually called every frame to upload the texture to the GPU, or to update the texture (if new glyph data is written)
		 */
		auto update_all_atlas(Renderer& renderer) noexcept -> void;

		/**
		 * @brief Get root (default) texture
		 */
		[[nodiscard]] auto root_texture() noexcept -> Texture&;

		/**
		 * @brief Get root (default) texture
		 */
		[[nodiscard]] auto root_texture() const noexcept -> const Texture&;

		/**
		 * @brief Get texture of id
		 */
		[[nodiscard]] auto select_texture(texture_atlas_id_type texture_atlas_id) noexcept -> Texture&;

		/**
		 * @brief Get texture of id
		 */
		[[nodiscard]] auto select_texture(texture_atlas_id_type texture_atlas_id) const noexcept -> const Texture&;

		struct write_result
		{
			TextureWriter writer;

			texture_atlas_id_type texture_atlas_id;
		};

		[[nodiscard]] auto write(size_type size) noexcept -> write_result;
	};
}
