// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/renderer.hpp>
#include <gfx/context.hpp>

namespace gal::prometheus::gfx
{
	Renderer::~Renderer() noexcept = default;

	auto Renderer::create() noexcept -> bool
	{
		return do_create();
	}

	auto Renderer::destroy() noexcept -> void
	{
		return do_destroy();
	}

	auto Renderer::ready() const noexcept -> bool
	{
		return do_ready();
	}

	auto Renderer::create_texture(const Texture::data_view_type data, const Texture::size_type size) noexcept -> texture_id_type
	{
		return do_create_texture(data, size);
	}

	auto Renderer::update_texture(const Texture& texture) noexcept -> void
	{
		do_update_texture(texture);
	}

	auto Renderer::destroy_texture(const texture_id_type texture_id) noexcept -> void
	{
		do_destroy_texture(texture_id);
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Renderer::present(RenderContext& renderer_context, const rect_type& display_area) noexcept -> void
	{
		// todo: We can explicitly upload all glyphs to texture here, so that we don't need to call TextureContext::upload_all_font_face in RenderContext::end_frame
		// renderer_context.upload_all_font_face();

		do_present(renderer_context, display_area);
	}
}
