// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <vector>

#include <gfx_new/gfx.hpp>
#include <gfx_new/internal/texture.hpp>

#include <memory/reference_wrapper.hpp>

namespace gal::prometheus::gfx_new
{
	class TextureContext final
	{
	public:
		// index
		using texture_atlas_id_type = std::uint32_t;

		using texture_atlas_list_type = std::vector<Texture>;

	private:
		texture_atlas_list_type texture_atlas_list_;

		[[nodiscard]] auto root_id() const noexcept -> texture_atlas_id_type;

	public:
		TextureContext() noexcept;

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

		/**
		 * @brief Writes the given glyph data to any holdable texture
		 * @param data Glyph data to write
		 * @param size Size of data (rectangle area)
		 * @return The uv coordinate of the position where the data is written
		 */
		auto write(
			Texture::data_view_type data,
			Texture::size_type size
		) noexcept -> primitive::basic_rect_2d<Texture::uv_type::value_type>;

		/**
		 * @brief Upload all texture atlas (if it didn't upload or needs to be re-uploaded)
		 * @note This function is usually called every frame to upload the texture to the GPU, or to update the texture (if new glyph data is written)
		 */
		auto upload(Renderer& renderer) noexcept -> void;
	};

	/**
	 * @brief Proxy class for accessing the Renderer's private interface (this class helps us not to expose too many implementation details to the outside world)
	 */
	class Renderer::AccessorTexture final
	{
	public:
		using renderer_type = memory::RefWrapper<Renderer>;

		using texture_type = gfx_new::Texture;

	private:
		renderer_type renderer_;

	public:
		explicit AccessorTexture(Renderer& renderer) noexcept;

		/**
		 * @brief Upload texture atlas data to GPU and get GPU resource handle
		 */
		auto upload(texture_type& texture) noexcept -> void;

		/**
		 * @brief Update new texture atlas data to GPU (overwrite previous texture atlas data)
		 * @note Does not check for the need to update
		 */
		auto update(texture_type& texture) noexcept -> void;

		/**
		 * @brief Update new texture atlas data to GPU (overwrite previous texture atlas data)
		 * @note If no update is needed (not dirty) then do nothing
		 */
		auto update_if_dirty(texture_type& texture) noexcept -> void;
	};
}
