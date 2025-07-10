// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx/type.hpp>

#include <memory/reference_wrapper.hpp>

namespace gal::prometheus::gfx
{
	class RenderListSharedData
	{
	public:
		using circle_segment_count_type = std::uint8_t;
		constexpr static std::size_t circle_segment_counts_count = 64;
		using circle_segment_counts_type = std::array<circle_segment_count_type, circle_segment_counts_count>;

		constexpr static std::uint32_t circle_segments_min = 4;
		constexpr static std::uint32_t circle_segments_max = 512;

		constexpr static std::size_t vertex_sample_points_count = 48;
		using vertex_sample_points_type = std::array<point_type, vertex_sample_points_count>;

		constexpr static std::size_t baked_line_uv_count = 64;
		using baked_line_uvs_type = std::array<rect_type, baked_line_uv_count>;

		circle_segment_counts_type circle_segment_counts;

		vertex_sample_points_type vertex_sample_points;

		baked_line_uvs_type baked_line_uvs;
		point_type white_pixel_uv;

		// Maximum error (in pixels) allowed when using `circle`/`circle_filled` or drawing rounded corner rectangles with no explicit segment count specified.
		// Decrease for higher quality but more geometry.
		float circle_segment_max_error;
		// Cutoff radius after which arc drawing will fall back to slower `path_arc`
		float arc_fast_radius_cutoff;
		// Tessellation tolerance when using `path_bezier_curve` without a specific number of segments.
		// Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
		float curve_tessellation_tolerance;

		// --------------------------------------------------

		RenderListSharedData() noexcept;

		// --------------------------------------------------

		[[nodiscard]] auto circle_auto_segment_count(float radius) const noexcept -> circle_segment_count_type;

		[[nodiscard]] auto vertex_sample_point(std::size_t index) const noexcept -> const point_type&;

		// --------------------------------------------------

		auto set_circle_tessellation_max_error(float max_error) noexcept -> void;

		auto set_curve_tessellation_tolerance(float tolerance) noexcept -> void;
	};

	class RenderData final
	{
	public:
		using size_type = std::uint32_t;

		struct [[nodiscard]] command_type
		{
			rect_type scissor;
			texture_id_type texture;

			// =======================

			// set by RenderList::index_list.size()
			// start offset in @c RenderList::index_list
			size_type index_offset;
			// set by RenderList::draw_xxx
			// number of indices (multiple of 3) to be rendered as triangles
			size_type element_count;
		};

		using vertex_list_type = std::vector<vertex_type>;
		using index_list_type = std::vector<index_type>;
		using command_list_type = std::vector<command_type>;

		std::reference_wrapper<const vertex_list_type> vertex_list;
		std::reference_wrapper<const index_list_type> index_list;
		std::reference_wrapper<const command_list_type> command_list;
	};

	class RenderList final
	{
	public:
		using size_type = RenderData::size_type;

		using command_type = RenderData::command_type;

		using vertex_list_type = RenderData::vertex_list_type;
		using index_list_type = RenderData::index_list_type;
		using command_list_type = RenderData::command_list_type;

		constexpr static float text_no_auto_wrap = 9999999.f;

	private:
		class Drawer;

		RenderListFlag render_list_flag_;
		memory::RefWrapper<RenderContext> render_context_;

		// vertex_list: v1-v2-v3-v4 + v5-v6-v7-v8 + v9-v10-v11 => rect0 + rect1(clipped by rect0) + triangle0(clipped by rect1)
		// index_list: 0/1/2-0/2/3 + 4/5/6-4/6/7 + 8/9/10
		// command_list:
		//	0: .scissor = {0, 0, root_window_width, root_window_height}, .index_offset = 0, .element_count = root_window_element_count + 6 (two triangles => 0/1/2-0/2/3)
		// 1: .scissor = {max(rect0.left, rect1.left), max(rect0.top, rect1.top), min(rect0.right, rect1.right), min(rect0.bottom, rect1.bottom)}, .index_offset = root_window_element_count + 6, .element_count = 6 (two triangles => 4/5/6-4/6/7)
		// 2: .scissor = {...}, .index_offset = root_window_element_count + 12, .element_count = 3 (one triangle => 8/9/10)
		command_list_type command_list_;
		vertex_list_type vertex_list_;
		index_list_type index_list_;

		rect_type this_command_scissor_;
		texture_id_type this_command_texture_;

		auto push_command() noexcept -> void;

		auto on_scissor_changed() noexcept -> void;
		auto on_texture_changed() noexcept -> void;

		[[nodiscard]] auto shared_data() const noexcept -> const RenderListSharedData&;

		[[nodiscard]] auto default_texture() const noexcept -> texture_id_type;

	public:
		RenderList(RenderListFlag flag, RenderContext& render_context) noexcept;

		auto reset() noexcept -> void;

		// ----------------------------------------------------------------------------
		// RENDER DATA

		[[nodiscard]] auto render_data() const noexcept -> RenderData;

		// ----------------------------------------------------------------------------
		// SCISSOR & TEXTURE

		auto push_scissor(const rect_type& rect, bool intersect_with_current_scissor) noexcept -> rect_type&;

		auto pop_scissor() noexcept -> void;

		auto push_texture(texture_id_type texture) noexcept -> void;

		auto pop_texture() noexcept -> void;

		// clang-format off

		// ----------------------------------------------------------------------------
		// PRIMITIVE

		auto line(
			const point_type& from,
			const point_type& to,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto triangle(
			const point_type& a,
			const point_type& b,
			const point_type& c,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto triangle_filled(
			const point_type& a,
			const point_type& b,
			const point_type& c,
			color_type color
		) noexcept -> void;

		auto rect(
			const rect_type& rect,
			color_type color,
			float rounding = .0f,
			RenderFlag flag = RenderFlag::ROUND_CORNER_ALL,
			float thickness = 1.f
		) noexcept -> void;

		auto rect(
			const point_type& left_top,
			const point_type& right_bottom,
			color_type color,
			float rounding = .0f,
			RenderFlag flag = RenderFlag::ROUND_CORNER_ALL,
			float thickness = 1.f
		) noexcept -> void;

		auto rect_filled(
			const rect_type& rect,
			color_type color,
			float rounding = .0f,
			RenderFlag flag = RenderFlag::ROUND_CORNER_ALL
		) noexcept -> void;

		auto rect_filled(
			const point_type& left_top,
			const point_type& right_bottom,
			color_type color,
			float rounding = .0f,
			RenderFlag flag = RenderFlag::ROUND_CORNER_ALL
		) noexcept -> void;

		auto rect_filled(
			const rect_type& rect,
			color_type color_left_top,
			color_type color_right_top,
			color_type color_left_bottom,
			color_type color_right_bottom
		) noexcept -> void;

		auto rect_filled(
			const point_type& left_top,
			const point_type& right_bottom,
			color_type color_left_top,
			color_type color_right_top,
			color_type color_left_bottom,
			color_type color_right_bottom
		) noexcept -> void;

		auto quadrilateral(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto quadrilateral_filled(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			color_type color
		) noexcept -> void;

		auto circle_n(
			const circle_type& circle,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto circle_n(
			const point_type& center,
			float radius,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse_n(
			const ellipse_type& ellipse,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse_n(
			const point_type& center,
			const extent_type& radius,
			float rotation,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto circle_n_filled(
			const circle_type& circle,
			color_type color,
			std::uint32_t segments
		) noexcept -> void;

		auto circle_n_filled(
			const point_type& center,
			float radius,
			color_type color,
			std::uint32_t segments
		) noexcept -> void;

		auto ellipse_n_filled(
			const ellipse_type& ellipse,
			color_type color,
			std::uint32_t segments
		) noexcept -> void;

		auto ellipse_n_filled(
			const point_type& center,
			const extent_type& radius,
			float rotation,
			color_type color,
			std::uint32_t segments
		) noexcept -> void;

		auto circle(
			const circle_type& circle,
			color_type color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto circle(
			const point_type& center,
			float radius,
			color_type color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto circle_filled(
			const circle_type& circle,
			color_type color,
			std::uint32_t segments = 0
		) noexcept -> void;

		auto circle_filled(
			const point_type& center,
			float radius,
			color_type color,
			std::uint32_t segments = 0
		) noexcept -> void;

		auto ellipse(
			const ellipse_type& ellipse,
			color_type color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse(
			const point_type& center,
			const extent_type& radius,
			float rotation,
			color_type color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse_filled(
			const ellipse_type& ellipse,
			color_type color,
			std::uint32_t segments = 0
		) noexcept -> void;

		auto ellipse_filled(
			const point_type& center,
			const extent_type& radius,
			float rotation,
			color_type color,
			std::uint32_t segments = 0
		) noexcept -> void;

		auto bezier_cubic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			color_type color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto bezier_quadratic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			color_type color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		// ----------------------------------------------------------------------------
		// TEXT

		auto text(
			std::string_view utf8_text,
			std::uint32_t font_size,
			const point_type& point,
			color_type color,
			float wrap_width = text_no_auto_wrap
		) noexcept -> void;
		
		auto text(
			std::string_view utf8_text,
			std::uint32_t font_size,
			const point_type& point,
			color_type color,
			GlyphFlag flag,
			float wrap_width = text_no_auto_wrap
		) noexcept -> void;

		// fixme: There is no suitable place to implement this function, this function must be highly consistent with the implementation when drawing text
		auto text_size(
			std::string_view utf8_text,
			std::uint32_t font_size,
			float wrap_width = text_no_auto_wrap
		) noexcept -> extent_type;
		
		// fixme: There is no suitable place to implement this function, this function must be highly consistent with the implementation when drawing text
		auto text_size(
			std::string_view utf8_text,
			std::uint32_t font_size,
			GlyphFlag flag,
			float wrap_width = text_no_auto_wrap
		) noexcept -> extent_type;

		// ----------------------------------------------------------------------------
		// IMAGE

		// p1________ p2
		//    |            |
		//    |            |
		// p4|_______| p3
		auto image(
			texture_id_type texture_id,
			const point_type& display_p1,
			const point_type& display_p2,
			const point_type& display_p3,
			const point_type& display_p4,
			const uv_type& uv_p1 = {0, 0},
			const uv_type& uv_p2 = {1, 0},
			const uv_type& uv_p3 = {1, 1},
			const uv_type& uv_p4 = {0, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;

		auto image(
			texture_id_type texture_id,
			const rect_type& display_rect,
			const rect_type& uv_rect = {0, 0, 1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;

		auto image(
			texture_id_type texture_id,
			const point_type& display_left_top,
			const point_type& display_right_bottom,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;

		auto image_rounded(
			texture_id_type texture_id,
			const rect_type& display_rect,
			float rounding = .0f,
			RenderFlag flag = RenderFlag::NONE,
			const rect_type& uv_rect = {0, 0, 1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;

		auto image_rounded(
			texture_id_type texture_id,
			const point_type& display_left_top,
			const point_type& display_right_bottom,
			float rounding = .0f,
			RenderFlag flag = RenderFlag::NONE,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;

		// clang-format on
	};
} // namespace gal::prometheus::gfx
