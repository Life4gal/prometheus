// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <draw/flag.hpp>
#include <draw/def.hpp>

namespace gal::prometheus::draw
{
	class Font;

	class DrawList final
	{
	public:
		template<typename T>
		using container_type = DrawListDef::container_type<T>;

		// ----------------------------------------------------

		using rect_type = DrawListDef::rect_type;
		using point_type = DrawListDef::point_type;
		using extent_type = DrawListDef::extent_type;

		using circle_type = DrawListDef::circle_type;
		using ellipse_type = DrawListDef::ellipse_type;

		// ----------------------------------------------------

		using uv_type = DrawListDef::uv_type;
		using color_type = DrawListDef::color_type;
		using vertex_type = DrawListDef::vertex_type;
		using index_type = DrawListDef::index_type;

		// ----------------------------------------------------

		using path_list_type = DrawListDef::path_list_type;
		using vertex_list_type = DrawListDef::vertex_list_type;
		using index_list_type = DrawListDef::index_list_type;

		// ----------------------------------------------------

		using texture_id_type = DrawListDef::texture_id_type;
		using size_type = DrawListDef::size_type;

		using command_type = DrawListDef::command_type;
		using command_list_type = DrawListDef::command_list_type;

	private:
		DrawListFlag draw_list_flag_;

		/**
		 * @brief This wrapper structure expects to limit the data writes to a limited number of functions to avoid unintended data writes.
		 * @see make_accessor
		 * @see push_command
		 * @see on_element_changed
		 * @see reset
		 * @see push_clip_rect
		 * @see draw_image_rounded (fill image uv)
		 */
		struct private_data_type
		{
			// vertex_list: v1-v2-v3-v4 + v5-v6-v7-v8 + v9-v10-v11 => rect0 + rect1(clipped by rect0) + triangle0(clipped by rect1)
			// index_list: 0/1/2-0/2/3 + 4/5/6-4/6/7 + 8/9/10
			// command_list: 
			//	0: .clip_rect = {0, 0, root_window_width, root_window_height}, .index_offset = 0, .element_count = root_window_element_count + 6 (two triangles => 0/1/2-0/2/3)
			// 1: .clip_rect = {max(rect0.left, rect1.left), max(rect0.top, rect1.top), min(rect0.right, rect1.right), min(rect0.bottom, rect1.bottom)}, .index_offset = root_window_element_count + 6, .element_count = 6 (two triangles => 4/5/6-4/6/7)
			// 2: .clip_rect = {...}, .index_offset = root_window_element_count + 12, .element_count = 3 (one triangle => 8/9/10)
			command_list_type command_list;
			vertex_list_type vertex_list;
			index_list_type index_list;
		};

		private_data_type private_data_;

		rect_type this_command_clip_rect_;
		texture_id_type this_command_texture_id_;

		path_list_type path_list_;

		[[nodiscard]] auto make_accessor() noexcept -> DrawListDef::Accessor;

		auto push_command() noexcept -> void;

		enum class ChangedElement : std::uint8_t
		{
			CLIP_RECT,
			TEXTURE_ID,
		};

		auto on_element_changed(ChangedElement element) noexcept -> void;

		// ----------------------------------------------------------------------------
		// DRAW

		auto draw_polygon_line(const color_type& color, DrawFlag draw_flag, float thickness) noexcept -> void;

		auto draw_polygon_line_aa(const color_type& color, DrawFlag draw_flag, float thickness) noexcept -> void;

		auto draw_convex_polygon_line_filled(const color_type& color) noexcept -> void;

		auto draw_convex_polygon_line_filled_aa(const color_type& color) noexcept -> void;

		auto draw_rect_filled(
			const rect_type& rect,
			const color_type& color_left_top,
			const color_type& color_right_top,
			const color_type& color_left_bottom,
			const color_type& color_right_bottom
		) noexcept -> void;

		auto draw_text(
			const Font& font,
			float font_size,
			const point_type& p,
			const color_type& color,
			std::string_view utf8_text,
			float wrap_width
		) noexcept -> void;

		auto draw_image(
			texture_id_type texture_id,
			const point_type& display_p1,
			const point_type& display_p2,
			const point_type& display_p3,
			const point_type& display_p4,
			const uv_type& uv_p1,
			const uv_type& uv_p2,
			const uv_type& uv_p3,
			const uv_type& uv_p4,
			const color_type& color
		) noexcept -> void;

		auto draw_image_rounded(
			texture_id_type texture_id,
			const rect_type& display_rect,
			const rect_type& uv_rect,
			const color_type& color,
			float rounding,
			DrawFlag flag
		) noexcept -> void;

		// ----------------------------------------------------------------------------
		// PATH

		constexpr auto path_clear() noexcept -> void
		{
			path_list_.clear();
		}

		constexpr auto path_reserve(const std::size_t size) noexcept -> void
		{
			path_list_.reserve(size);
		}

		constexpr auto path_reserve_extra(const std::size_t size) noexcept -> void
		{
			path_reserve(path_list_.size() + size);
		}

		constexpr auto path_pin(const point_type& point) noexcept -> void
		{
			path_list_.push_back(point);
		}

		auto path_stroke(const color_type& color, DrawFlag flag, float thickness) noexcept -> void;

		auto path_stroke(const color_type& color) noexcept -> void;

		auto path_arc_fast(const circle_type& circle, int from, int to) noexcept -> void;

		auto path_arc_fast(const circle_type& circle, DrawArcFlag flag) noexcept -> void;

		auto path_arc_n(const circle_type& circle, float from, float to, std::uint32_t segments) noexcept -> void;

		auto path_arc(const circle_type& circle, float from, float to) noexcept -> void;

		auto path_arc_elliptical_n(const ellipse_type& ellipse, float from, float to, std::uint32_t segments) noexcept -> void;

		auto path_quadrilateral(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4) noexcept -> void;

		auto path_rect(const rect_type& rect, float rounding, DrawFlag flag) noexcept -> void;

		auto path_bezier_cubic_curve_casteljau(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			float tessellation_tolerance,
			std::size_t level
		) noexcept -> void;

		auto path_bezier_quadratic_curve_casteljau(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			float tessellation_tolerance,
			std::size_t level
		) noexcept -> void;

		auto path_bezier_curve(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			std::uint32_t segments
		) noexcept -> void;

		auto path_bezier_quadratic_curve(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			std::uint32_t segments
		) noexcept -> void;

	public:
		constexpr DrawList() noexcept
			: draw_list_flag_{DrawListFlag::NONE},
			  this_command_clip_rect_{0, 0, 0, 0},
			  this_command_texture_id_{0} {}

		// ----------------------------------------------------------------------------
		// FLAG

		constexpr auto draw_list_flag(const DrawListFlag flag) noexcept -> void
		{
			draw_list_flag_ = flag;
		}

		constexpr auto draw_list_flag(const std::underlying_type_t<DrawListFlag> flag) noexcept -> void
		{
			draw_list_flag(static_cast<DrawListFlag>(flag));
		}

		// ----------------------------------------------------------------------------
		// RESET

		auto reset() noexcept -> void;

		// ----------------------------------------------------------------------------
		// DRAW DATA

		[[nodiscard]] constexpr auto command_list() const noexcept -> auto
		{
			return private_data_.command_list | std::views::all;
		}

		[[nodiscard]] constexpr auto vertex_list() const noexcept -> auto
		{
			return private_data_.vertex_list | std::views::all;
		}

		[[nodiscard]] constexpr auto index_list() const noexcept -> auto
		{
			return private_data_.index_list | std::views::all;
		}

		// ----------------------------------------------------------------------------
		// CLIP RECT & TEXTURE

		auto push_clip_rect(const rect_type& rect, bool intersect_with_current_clip_rect) noexcept -> rect_type&;

		auto push_clip_rect(const point_type& left_top, const point_type& right_bottom, bool intersect_with_current_clip_rect) noexcept -> rect_type&;

		auto pop_clip_rect() noexcept -> void;

		auto push_texture_id(texture_id_type texture) noexcept -> void;

		auto pop_texture_id() noexcept -> void;

		// ----------------------------------------------------------------------------
		// PRIMITIVE

		auto line(
			const point_type& from,
			const point_type& to,
			const color_type& color,
			float thickness = 1.f
		) noexcept -> void;

		auto triangle(
			const point_type& a,
			const point_type& b,
			const point_type& c,
			const color_type& color,
			float thickness = 1.f
		) noexcept -> void;

		auto triangle_filled(
			const point_type& a,
			const point_type& b,
			const point_type& c,
			const color_type& color
		) noexcept -> void;

		auto rect(
			const rect_type& rect,
			const color_type& color,
			float rounding = .0f,
			DrawFlag flag = DrawFlag::NONE,
			float thickness = 1.f
		) noexcept -> void;

		auto rect(
			const point_type& left_top,
			const point_type& right_bottom,
			const color_type& color,
			float rounding = .0f,
			DrawFlag flag = DrawFlag::NONE,
			float thickness = 1.f
		) noexcept -> void;

		auto rect_filled(
			const rect_type& rect,
			const color_type& color,
			float rounding = .0f,
			DrawFlag flag = DrawFlag::NONE
		) noexcept -> void;

		auto rect_filled(
			const point_type& left_top,
			const point_type& right_bottom,
			const color_type& color,
			float rounding = .0f,
			DrawFlag flag = DrawFlag::NONE
		) noexcept -> void;

		auto rect_filled(
			const rect_type& rect,
			const color_type& color_left_top,
			const color_type& color_right_top,
			const color_type& color_left_bottom,
			const color_type& color_right_bottom
		) noexcept -> void;

		auto rect_filled(
			const point_type& left_top,
			const point_type& right_bottom,
			const color_type& color_left_top,
			const color_type& color_right_top,
			const color_type& color_left_bottom,
			const color_type& color_right_bottom
		) noexcept -> void;

		auto quadrilateral(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			const color_type& color,
			float thickness = 1.f
		) noexcept -> void;

		auto quadrilateral_filled(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			const color_type& color
		) noexcept -> void;

		auto circle_n(
			const circle_type& circle,
			const color_type& color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto circle_n(
			const point_type& center,
			float radius,
			const color_type& color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse_n(
			const ellipse_type& ellipse,
			const color_type& color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse_n(
			const point_type& center,
			const extent_type& radius,
			float rotation,
			const color_type& color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto circle_n_filled(
			const circle_type& circle,
			const color_type& color,
			std::uint32_t segments
		) noexcept -> void;

		auto circle_n_filled(
			const point_type& center,
			float radius,
			const color_type& color,
			std::uint32_t segments
		) noexcept -> void;

		auto ellipse_n_filled(
			const ellipse_type& ellipse,
			const color_type& color,
			std::uint32_t segments
		) noexcept -> void;

		auto ellipse_n_filled(
			const point_type& center,
			const extent_type& radius,
			float rotation,
			const color_type& color,
			std::uint32_t segments
		) noexcept -> void;

		auto circle(
			const circle_type& circle,
			const color_type& color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto circle(
			const point_type& center,
			float radius,
			const color_type& color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto circle_filled(
			const circle_type& circle,
			const color_type& color,
			std::uint32_t segments = 0
		) noexcept -> void;

		auto circle_filled(
			const point_type& center,
			float radius,
			const color_type& color,
			std::uint32_t segments = 0
		) noexcept -> void;

		auto ellipse(
			const ellipse_type& ellipse,
			const color_type& color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse(
			const point_type& center,
			const extent_type& radius,
			float rotation,
			const color_type& color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse_filled(
			const ellipse_type& ellipse,
			const color_type& color,
			std::uint32_t segments = 0
		) noexcept -> void;

		auto ellipse_filled(
			const point_type& center,
			const extent_type& radius,
			float rotation,
			const color_type& color,
			std::uint32_t segments = 0
		) noexcept -> void;

		auto bezier_cubic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			const color_type& color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		auto bezier_quadratic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const color_type& color,
			std::uint32_t segments = 0,
			float thickness = 1.f
		) noexcept -> void;

		// ----------------------------------------------------------------------------
		// TEXT

		auto text(
			const Font& font,
			float font_size,
			const point_type& p,
			const color_type& color,
			std::string_view utf8_text,
			float wrap_width = std::numeric_limits<float>::max()
		) noexcept -> void;

		auto text(
			float font_size,
			const point_type& p,
			const color_type& color,
			std::string_view utf8_text,
			float wrap_width = std::numeric_limits<float>::max()
		) noexcept -> void;

		// ----------------------------------------------------------------------------
		// IMAGE

		// p1________ p2
		//     |           |
		//     |           |
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
			const color_type& color = primitive::colors::white
		) noexcept -> void;

		auto image(
			texture_id_type texture_id,
			const rect_type& display_rect,
			const rect_type& uv_rect = {0, 0, 1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void;

		auto image(
			texture_id_type texture_id,
			const point_type& display_left_top,
			const point_type& display_right_bottom,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void;

		auto image_rounded(
			texture_id_type texture_id,
			const rect_type& display_rect,
			float rounding = .0f,
			DrawFlag flag = DrawFlag::NONE,
			const rect_type& uv_rect = {0, 0, 1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void;

		auto image_rounded(
			texture_id_type texture_id,
			const point_type& display_left_top,
			const point_type& display_right_bottom,
			float rounding = .0f,
			DrawFlag flag = DrawFlag::NONE,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void;
	};
}
