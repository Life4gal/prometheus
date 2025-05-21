// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx_new/gfx.hpp>
#include <gfx_new/internal/renderer_context.hpp>

namespace gal::prometheus::gfx_new
{
	GlyphParser::~GlyphParser() noexcept = default;

	Renderer::~Renderer() noexcept = default;

	Renderer::Renderer() noexcept
		: context_{memory::make_unique<RendererContext>()} {}

	auto Renderer::construct() noexcept -> bool
	{
		return do_construct();
	}

	auto Renderer::destruct() noexcept -> void
	{
		return do_destruct();
	}

	auto Renderer::ready() const noexcept -> bool
	{
		return do_ready();
	}

	auto Renderer::new_frame() noexcept -> void
	{
		// font
		{
			context_->font_context.load_all_font();
			context_->font_context.set_fallback_glyph();
		}
		// texture
		{
			AccessorTexture accessor{*this};
			context_->texture_context.upload(accessor);
		}
	}

	auto Renderer::present() noexcept -> void
	{
		// glyphs
		{
			// note: newly added glyph information is not available until the next frame
			context_->font_context.upload_all_glyph(context_->texture_context);
		}

		// todo
		// do_present()
	}

	auto Renderer::end_frame() noexcept -> void
	{
		//
	}

	auto Renderer::set_glyph_parser(GlyphParser& parser) noexcept -> GlyphParser*
	{
		return context_->font_context.set_glyph_parser(parser);
	}

	auto Renderer::add_font(const std::filesystem::path& path) noexcept -> bool
	{
		return context_->font_context.add_font(path);
	}

	auto Renderer::new_render_list() noexcept -> RenderList&
	{
		RenderList new_render_list{*this};
		return context_->render_context.render_lists.emplace_back(std::move(new_render_list));
	}
}
