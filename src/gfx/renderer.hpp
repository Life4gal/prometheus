// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <span>

#include <gfx/type.hpp>

#include <gfx/texture.hpp>
#include <gfx/render_list.hpp>

namespace gal::prometheus::gfx
{
	class Renderer
	{
	public:
		Renderer(const Renderer&) noexcept = delete;
		Renderer(Renderer&&) noexcept = default;
		auto operator=(const Renderer&) noexcept -> Renderer& = delete;
		auto operator=(Renderer&&) noexcept -> Renderer& = default;

		virtual ~Renderer() noexcept;

	protected:
		Renderer() noexcept;

	public:
		[[nodiscard]] virtual auto create_texture(const Texture::data_type& data, Texture::size_type size) noexcept -> texture_id_type = 0;
		virtual auto update_texture(texture_id_type id, std::span<TextureViewer> update_viewer) noexcept -> void = 0;
		virtual auto destroy_texture(texture_id_type id) noexcept -> void = 0;

		/**
		 * @brief
		 * @note @c new_frame -> @c present -> @c end_frame
		 */
		virtual auto present(const render_data_list_type& render_data_list, const extent_type& display_size) noexcept -> void = 0;
	};
}
