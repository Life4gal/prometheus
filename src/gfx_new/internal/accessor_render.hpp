// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx_new/internal/type.hpp>

#include <memory/reference_wrapper.hpp>

namespace gal::prometheus::gfx_new
{
	class RenderContext final
	{
	public:
		RenderListSharedData render_list_shared_data;
		std::vector<RenderList> render_lists;
	};

	/**
	 * @brief Proxy class for accessing the Renderer's private interface (this class helps us not to expose too many implementation details to the outside world)
	 */
	class Renderer::AccessorRender final
	{
	public:
		using renderer_type = memory::RefWrapper<Renderer>;

	private:
		renderer_type renderer_;

	public:
		explicit AccessorRender(Renderer& renderer) noexcept;

		[[nodiscard]] auto context() const noexcept -> const RenderContext&;
	};
}
