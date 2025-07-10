// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gui/internal/common.hpp>

namespace gal::prometheus
{
	namespace gui::internal
	{
		enum class DrawFlag : std::uint8_t
		{
			NONE = 0,
			// specify that shape should be closed
			// @see DrawList::draw_polygon_line
			// @see DrawList::draw_polygon_line_aa
			// @see DrawList::path_stroke
			CLOSED = 1 << 0,
			// enable rounding left-top corner only (when rounding > 0.0f, we default to all corners)
			// @see DrawList::path_rect
			// @see DrawList::rect
			// @see DrawList::rect_filled
			ROUND_CORNER_LEFT_TOP = 1 << 1,
			// enable rounding right_top corner only (when rounding > 0.0f, we default to all corners)
			// @see DrawList::path_rect
			// @see DrawList::rect
			// @see DrawList::rect_filled
			ROUND_CORNER_RIGHT_TOP = 1 << 2,
			// enable rounding left-bottom corner only (when rounding > 0.0f, we default to all corners)
			// @see DrawList::path_rect
			// @see DrawList::rect
			// @see DrawList::rect_filled
			ROUND_CORNER_LEFT_BOTTOM = 1 << 3,
			// enable rounding right-bottom corner only (when rounding > 0.0f, we default to all corners)
			// @see DrawList::path_rect
			// @see DrawList::rect
			// @see DrawList::rect_filled
			ROUND_CORNER_RIGHT_BOTTOM = 1 << 4,
			// disable rounding on all corners (when rounding > 0.0f)
			ROUND_CORNER_NONE = 1 << 5,

			ROUND_CORNER_LEFT = ROUND_CORNER_LEFT_TOP | ROUND_CORNER_LEFT_BOTTOM,
			ROUND_CORNER_TOP = ROUND_CORNER_LEFT_TOP | ROUND_CORNER_RIGHT_TOP,
			ROUND_CORNER_RIGHT = ROUND_CORNER_RIGHT_TOP | ROUND_CORNER_RIGHT_BOTTOM,
			ROUND_CORNER_BOTTOM = ROUND_CORNER_LEFT_BOTTOM | ROUND_CORNER_RIGHT_BOTTOM,

			ROUND_CORNER_ALL = ROUND_CORNER_LEFT_TOP | ROUND_CORNER_RIGHT_TOP | ROUND_CORNER_LEFT_BOTTOM | ROUND_CORNER_RIGHT_BOTTOM,
			ROUND_CORNER_DEFAULT = ROUND_CORNER_ALL,
			ROUND_CORNER_MASK = ROUND_CORNER_ALL | ROUND_CORNER_NONE,
		};

		enum class DrawArcFlag : std::uint8_t
		{
			// [0~3)
			Q1 = 1 << 0,
			// [3~6)
			Q2 = 1 << 1,
			// [6~9)
			Q3 = 1 << 2,
			// [9~12)
			Q4 = 1 << 3,

			RIGHT_TOP = Q1,
			LEFT_TOP = Q2,
			LEFT_BOTTOM = Q3,
			RIGHT_BOTTOM = Q4,
			TOP = Q1 | Q2,
			BOTTOM = Q3 | Q4,
			LEFT = Q2 | Q3,
			RIGHT = Q1 | Q4,
			ALL = Q1 | Q2 | Q3 | Q4,

			// [3, 0)
			Q1_CLOCK_WISH = 1 << 4,
			// [6, 3)
			Q2_CLOCK_WISH = 1 << 5,
			// [9, 6)
			Q3_CLOCK_WISH = 1 << 6,
			// [12, 9)
			Q4_CLOCK_WISH = 1 << 7,

			RIGHT_TOP_CLOCK_WISH = Q1_CLOCK_WISH,
			LEFT_TOP_CLOCK_WISH = Q2_CLOCK_WISH,
			LEFT_BOTTOM_CLOCK_WISH = Q3_CLOCK_WISH,
			RIGHT_BOTTOM_CLOCK_WISH = Q4_CLOCK_WISH,
			TOP_CLOCK_WISH = Q1_CLOCK_WISH | Q2_CLOCK_WISH,
			BOTTOM_CLOCK_WISH = Q3_CLOCK_WISH | Q4_CLOCK_WISH,
			LEFT_CLOCK_WISH = Q2_CLOCK_WISH | Q3_CLOCK_WISH,
			RIGHT_CLOCK_WISH = Q1_CLOCK_WISH | Q4_CLOCK_WISH,
			ALL_CLOCK_WISH = Q1_CLOCK_WISH | Q2_CLOCK_WISH | Q3_CLOCK_WISH | Q4_CLOCK_WISH,
		};
	}

	namespace meta::user_defined
	{
		template<>
		struct enum_is_flag<gui::internal::DrawFlag> : std::true_type {};

		template<>
		struct enum_is_flag<gui::internal::DrawArcFlag> : std::true_type {};
	}

	namespace gui::internal
	{
		[[nodiscard]] auto range_of_arc(DrawArcFlag flag) noexcept -> std::pair<int, int>;

		class Font;

		class DrawList
		{
		public:
			using size_type = DrawData::size_type;

			using command_type = DrawData::command_type;

			using vertex_list_type = DrawData::vertex_list_type;
			using index_list_type = DrawData::index_list_type;
			using command_list_type = DrawData::command_list_type;

			// not set ==> Font::no_auto_wrap
			constexpr static float text_wrap_width_not_set = -1.f;

		private:
			class Drawer;

			Context* context_;

			DrawListFlag draw_list_flag_;

			// vertex_list: v1-v2-v3-v4 + v5-v6-v7-v8 + v9-v10-v11 => rect0 + rect1(clipped by rect0) + triangle0(clipped by rect1)
			// index_list: 0/1/2-0/2/3 + 4/5/6-4/6/7 + 8/9/10
			// command_list: 
			//	0: .clip_rect = {0, 0, root_window_width, root_window_height}, .index_offset = 0, .element_count = root_window_element_count + 6 (two triangles => 0/1/2-0/2/3)
			// 1: .clip_rect = {max(rect0.left, rect1.left), max(rect0.top, rect1.top), min(rect0.right, rect1.right), min(rect0.bottom, rect1.bottom)}, .index_offset = root_window_element_count + 6, .element_count = 6 (two triangles => 4/5/6-4/6/7)
			// 2: .clip_rect = {...}, .index_offset = root_window_element_count + 12, .element_count = 3 (one triangle => 8/9/10)
			command_list_type command_list_;
			vertex_list_type vertex_list_;
			index_list_type index_list_;

			rect_type this_command_clip_rect_;
			texture_id_type this_command_texture_id_;

			auto push_command() noexcept -> void;

			enum class ChangedElement : std::uint8_t
			{
				CLIP_RECT,
				TEXTURE_ID,
			};

			auto on_element_changed(ChangedElement element) noexcept -> void;

		public:
			DrawList() noexcept;

			// ----------------------------------------------------------------------------
			// CONTEXT

			auto bind_context(Context& context) noexcept -> void;

			// ----------------------------------------------------------------------------
			// RESET

			auto reset() noexcept -> void;

			// ----------------------------------------------------------------------------
			// DRAW DATA

			[[nodiscard]] auto command_list() const noexcept -> const command_list_type&;

			[[nodiscard]] auto vertex_list() const noexcept -> const vertex_list_type&;

			[[nodiscard]] auto index_list() const noexcept -> const index_list_type&;

			// ----------------------------------------------------------------------------
			// CLIP RECT & TEXTURE

			auto push_clip_rect(const rect_type& rect, bool intersect_with_current_clip_rect) noexcept -> rect_type&;

			auto pop_clip_rect() noexcept -> void;

			auto push_texture_id(texture_id_type texture) noexcept -> void;

			auto pop_texture_id() noexcept -> void;

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
				DrawFlag flag = DrawFlag::ROUND_CORNER_ALL,
				float thickness = 1.f
			) noexcept -> void;

			auto rect(
				const point_type& left_top,
				const point_type& right_bottom,
				color_type color,
				float rounding = .0f,
				DrawFlag flag = DrawFlag::ROUND_CORNER_ALL,
				float thickness = 1.f
			) noexcept -> void;

			auto rect_filled(
				const rect_type& rect,
				color_type color,
				float rounding = .0f,
				DrawFlag flag = DrawFlag::ROUND_CORNER_ALL
			) noexcept -> void;

			auto rect_filled(
				const point_type& left_top,
				const point_type& right_bottom,
				color_type color,
				float rounding = .0f,
				DrawFlag flag = DrawFlag::ROUND_CORNER_ALL
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
				const Font& font,
				float font_size,
				const point_type& point,
				color_type color,
				std::string_view utf8_text,
				float wrap_width = text_wrap_width_not_set
			) noexcept -> void;

			auto text(
				float font_size,
				const point_type& point,
				color_type color,
				std::string_view utf8_text,
				float wrap_width = text_wrap_width_not_set
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
				DrawFlag flag = DrawFlag::NONE,
				const rect_type& uv_rect = {0, 0, 1, 1},
				color_type color = primitive::colors::white
			) noexcept -> void;

			auto image_rounded(
				texture_id_type texture_id,
				const point_type& display_left_top,
				const point_type& display_right_bottom,
				float rounding = .0f,
				DrawFlag flag = DrawFlag::NONE,
				const uv_type& uv_left_top = {0, 0},
				const uv_type& uv_right_bottom = {1, 1},
				color_type color = primitive::colors::white
			) noexcept -> void;
		};
	}
}
