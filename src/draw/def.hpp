// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if not defined(GAL_PROMETHEUS_DRAW_LIST_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 0
#endif
#endif

#include <primitive/rect.hpp>
#include <primitive/circle.hpp>
#include <primitive/ellipse.hpp>
#include <primitive/color.hpp>
#include <primitive/vertex.hpp>

namespace gal::prometheus::draw
{
	class DrawListDef
	{
	public:
		template<typename T>
		using container_type = std::vector<T>;

		// ----------------------------------------------------

		using rect_type = primitive::basic_rect_2d<float, float>;
		using point_type = rect_type::point_type;
		using extent_type = rect_type::extent_type;

		using circle_type = primitive::basic_circle_2d<float, float>;
		using ellipse_type = primitive::basic_ellipse_2d<float, float, float>;

		// ----------------------------------------------------

		using uv_type = primitive::basic_point_2d<float>;
		using color_type = primitive::basic_color;
		using vertex_type = primitive::basic_vertex<point_type, uv_type, color_type>;
		using index_type = std::uint16_t;

		// ----------------------------------------------------

		using path_list_type = container_type<point_type>;
		using vertex_list_type = container_type<vertex_type>;
		using index_list_type = container_type<index_type>;

		// ----------------------------------------------------

		using texture_id_type = std::uintptr_t;
		using size_type = vertex_list_type::size_type;

		struct command_type
		{
			rect_type clip_rect;
			texture_id_type texture_id;

			// =======================

			// set by DrawList::index_list.size()
			// start offset in `DrawList::index_list`
			size_type index_offset;
			// set by subsequent `DrawList::draw_xxx`
			// number of indices (multiple of 3) to be rendered as triangles
			size_type element_count;
		};

		using command_list_type = container_type<command_type>;

		// ----------------------------------------------------

		class Accessor
		{
			// command_type::element_count
			std::reference_wrapper<size_type> element_count_;
			// DrawList::vertex_list
			std::reference_wrapper<vertex_list_type> vertex_list_;
			// DrawList::index_list_;
			std::reference_wrapper<index_list_type> index_list_;

		public:
			constexpr Accessor(command_type& command, vertex_list_type& vertex_list, index_list_type& index_list) noexcept
				: element_count_{command.element_count},
				  vertex_list_{vertex_list},
				  index_list_{index_list} {}

			Accessor(const Accessor& other) = delete;
			Accessor(Accessor&& other) noexcept = delete;
			auto operator=(const Accessor& other) -> Accessor& = delete;
			auto operator=(Accessor&& other) noexcept -> Accessor& = delete;

			~Accessor() noexcept;

			constexpr auto reserve(const size_type vertex_count, const size_type index_count) const noexcept -> void
			{
				auto& element_count = element_count_.get();
				auto& vertex = vertex_list_.get();
				auto& index = index_list_.get();

				element_count += index_count;
				vertex.reserve(vertex.size() + vertex_count);
				index.reserve(index.size() + index_count);
			}

			[[nodiscard]] constexpr auto size() const noexcept -> size_type
			{
				const auto& list = vertex_list_.get();

				return list.size();
			}

			constexpr auto add_vertex(const point_type& position, const uv_type& uv, const color_type& color) const noexcept -> void
			{
				auto& list = vertex_list_.get();

				list.emplace_back(position, uv, color);
			}

			constexpr auto add_index(const index_type a, const index_type b, const index_type c) const noexcept -> void
			{
				auto& list = index_list_.get();

				list.push_back(a);
				list.push_back(b);
				list.push_back(c);
			}
		};
	};
}
