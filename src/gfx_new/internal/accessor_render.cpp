// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx_new/internal/accessor_render.hpp>

#include <gfx_new/internal/renderer_context.hpp>

namespace gal::prometheus::gfx_new
{
	Renderer::AccessorRender::AccessorRender(Renderer& renderer) noexcept
		: renderer_{renderer} {}

	auto Renderer::AccessorRender::context() const noexcept -> const RenderContext&
	{
		return renderer_.get().context_->render_context;
	}
}
