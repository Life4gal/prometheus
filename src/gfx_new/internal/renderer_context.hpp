// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx_new/internal/accessor_font.hpp>
#include <gfx_new/internal/accessor_texture.hpp>
#include <gfx_new/internal/accessor_render.hpp>

namespace gal::prometheus::gfx_new
{
	class Renderer::RendererContext
	{
	public:
		FontContext font_context;
		TextureContext texture_context;
		RenderContext render_context;
	};
}
