// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx/render_list.hpp>

namespace gal::prometheus::gfx
{
	class GlyphParser;
	class Renderer;

	class Context final
	{
		friend RenderList;

	public:
		class ContextPrivate;

		RenderListSharedData render_list_shared_data;
		std::vector<RenderList> render_lists;

		GlyphParser* glyph_parser;
		Renderer* renderer;

	private:
		memory::UniquePointer<ContextPrivate> private_;

	public:
		Context(const Context&) noexcept = delete;
		Context(Context&&) noexcept; // = default;
		auto operator=(const Context&) noexcept -> Context& = delete;
		auto operator=(Context&&) noexcept -> Context&; // = default;

		~Context() noexcept; // = default;

		Context() noexcept; // = default;

		auto initialize() noexcept -> void;

		auto new_render_list() noexcept -> RenderList&;

		auto new_frame() noexcept -> void;

		auto present(const extent_type& display_size) noexcept -> void;

		auto end_frame() noexcept -> void;
	};
}
