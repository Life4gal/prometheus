// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx/texture.hpp>

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
		Renderer() noexcept = default;

	public:
		auto create() noexcept -> bool;
		auto destroy() noexcept -> void;

		[[nodiscard]] auto ready() const noexcept -> bool;

		auto create_texture(Texture::data_view_type data, Texture::size_type size) noexcept -> texture_id_type;
		auto update_texture(const Texture& texture) noexcept -> void;
		auto destroy_texture(texture_id_type texture_id) noexcept -> void;

		auto present(RenderContext& renderer_context, const rect_type& display_area) noexcept -> void;

	private:
		[[nodiscard]] virtual auto do_create() noexcept -> bool = 0;
		virtual auto do_destroy() noexcept -> void = 0;

		[[nodiscard]] virtual auto do_ready() const noexcept -> bool = 0;

		virtual auto do_create_texture(Texture::data_view_type data, Texture::size_type size) noexcept -> texture_id_type = 0;
		virtual auto do_update_texture(const Texture& texture) noexcept -> void = 0;
		virtual auto do_destroy_texture(texture_id_type texture_id) noexcept -> void = 0;

		virtual auto do_present(const RenderContext& render_context, const rect_type& display_area) noexcept -> void = 0;
	};
} // namespace gal::prometheus::gfx
