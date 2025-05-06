// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx/type.hpp>

namespace gal::prometheus::gfx
{
	class Renderer
	{
	public:
		Renderer(const Renderer&) noexcept = delete;
		Renderer(Renderer&&) noexcept = default;
		auto operator=(const Renderer&) noexcept -> Renderer& = delete;
		auto operator=(Renderer&&) noexcept -> Renderer& = default;

		virtual ~Renderer() noexcept = default;

	protected:
		Renderer() noexcept = default;

	public:
		[[nodiscard]] virtual auto create() noexcept -> bool = 0;
		virtual auto destroy() noexcept -> void = 0;

		[[nodiscard]] virtual auto ready() const noexcept -> bool = 0;

		virtual auto begin_frame(const RendererContext& context) noexcept -> void = 0;
		virtual auto end_frame(const RendererContext& context) noexcept -> void = 0;

		// todo
		virtual auto create_texture() noexcept -> void = 0;
		virtual auto destroy_texture(texture_id_type texture_id) noexcept -> void = 0;
	};
}
