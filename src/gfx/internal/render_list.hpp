// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx/gfx.hpp>

namespace gal::prometheus::gfx
{
	class RenderList::RenderListContext final
	{
	public:
		using size_type = RenderData::size_type;

		using command_type = RenderData::command_type;

		using vertex_list_type = RenderData::vertex_list_type;
		using index_list_type = RenderData::index_list_type;
		using command_list_type = RenderData::command_list_type;

		mutable memory::RefWrapper<Context> context;

		RenderListFlag render_list_flag;

		// vertex_list: v1-v2-v3-v4 + v5-v6-v7-v8 + v9-v10-v11 => rect0 + rect1(clipped by rect0) + triangle0(clipped by rect1)
		// index_list: 0/1/2-0/2/3 + 4/5/6-4/6/7 + 8/9/10
		// command_list:
		//	0: .scissor = {0, 0, root_window_width, root_window_height}, .index_offset = 0, .element_count = root_window_element_count + 6 (two triangles => 0/1/2-0/2/3)
		// 1: .scissor = {max(rect0.left, rect1.left), max(rect0.top, rect1.top), min(rect0.right, rect1.right), min(rect0.bottom, rect1.bottom)}, .index_offset = root_window_element_count + 6, .element_count = 6 (two triangles => 4/5/6-4/6/7)
		// 2: .scissor = {...}, .index_offset = root_window_element_count + 12, .element_count = 3 (one triangle => 8/9/10)
		command_list_type command_list;
		vertex_list_type vertex_list;
		index_list_type index_list;

		rect_type this_command_scissor;
		texture_id_type this_command_texture;

	private:
		auto push_command() noexcept -> void;

		auto on_scissor_changed() noexcept -> void;
		auto on_texture_changed() noexcept -> void;

	public:
		[[nodiscard]] auto shared_data() const noexcept -> const RenderListSharedData&;

		[[nodiscard]] auto default_texture() const noexcept -> texture_id_type;

		auto reset() noexcept -> void;

		// ----------------------------------------------------------------------------
		// SCISSOR & TEXTURE

		auto push_scissor(const rect_type& rect, bool intersect_with_current_scissor) noexcept -> rect_type&;

		auto pop_scissor() noexcept -> void;

		auto push_texture(texture_id_type texture) noexcept -> void;

		auto pop_texture() noexcept -> void;
	};
}
