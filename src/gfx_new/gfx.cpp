// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx_new/gfx.hpp>
#include <gfx_new/internal/renderer_context.hpp>

namespace gal::prometheus::gfx_new
{
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
		do_before_new_frame();

		//

		do_after_new_frame();
	}

	auto Renderer::present() noexcept -> void
	{
		do_before_present();

		//

		do_after_present();
	}

	auto Renderer::end_frame() noexcept -> void
	{
		do_before_end_frame();

		//

		do_after_end_frame();
	}

	auto Renderer::do_before_new_frame() noexcept -> void
	{
		//
	}

	auto Renderer::do_after_new_frame() noexcept -> void
	{
		//
	}

	auto Renderer::do_before_present() noexcept -> void
	{
		//
	}

	auto Renderer::do_after_present() noexcept -> void
	{
		//
	}

	auto Renderer::do_before_end_frame() noexcept -> void
	{
		//
	}

	auto Renderer::do_after_end_frame() noexcept -> void
	{
		//
	}
}
