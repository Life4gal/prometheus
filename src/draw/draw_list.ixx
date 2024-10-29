// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

#if not defined(GAL_PROMETHEUS_DRAW_LIST_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 0
#endif
#endif

export module gal.prometheus:draw.draw_list;

import std;

#if GAL_PROMETHEUS_COMPILER_DEBUG
import :platform;
#endif

import :draw.font;
export import :draw.draw_list.flag;
export import :draw.draw_list.shared_data;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <memory>
#include <vector>
#include <type_traits>
#include <utility>
#include <numbers>

#include <prometheus/macro.hpp>

#if not defined(GAL_PROMETHEUS_DRAW_LIST_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 0
#endif
#endif

#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
#include <string>
#include <format>
#include <span>
#endif

#include <functional/functional.ixx>
#include <primitive/primitive.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <draw/font.ixx>
#include <draw/draw_list.flag.ixx>
#include <draw/draw_list.shared_data.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
{
	class DrawList final
	{
		// @see `draw_text`
		friend Font;

	public:
		using rect_type = DrawListSharedData::rect_type;
		using point_type = DrawListSharedData::point_type;
		using extent_type = DrawListSharedData::extent_type;

		using circle_type = DrawListSharedData::circle_type;
		using ellipse_type = DrawListSharedData::ellipse_type;

		using uv_type = DrawListSharedData::uv_type;
		using color_type = DrawListSharedData::color_type;
		using vertex_type = DrawListSharedData::vertex_type;
		using index_type = DrawListSharedData::index_type;

		using texture_id_type = Font::texture_id_type;

		template<typename T>
		using list_type = std::vector<T>;

		using size_type = std::size_t;

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH
		// Variable '%1$s' is uninitialized. Always initialize a member variable (type.6).
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING(26495)
		#endif

		struct command_type
		{
			rect_type clip_rect;
			texture_id_type texture_id;
			// there may be additional data

			// =======================

			// set by index_list.size()
			// start offset in `index_list`
			size_type index_offset;
			// set by subsequent draw_xxx
			// number of indices (multiple of 3) to be rendered as triangles
			size_type element_count;
		};

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_POP
		#endif

		using command_list_type = list_type<command_type>;
		using vertex_list_type = list_type<vertex_type>;
		using index_list_type = list_type<index_type>;
		using path_list_type = list_type<point_type>;

	private:
		#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
		using debug_vertex_range_type = std::pair<vertex_list_type::size_type, vertex_list_type::size_type>;
		using debug_index_range_type = std::pair<index_list_type::size_type, index_list_type::size_type>;

		using debug_vertex_list_type = std::span<vertex_type>;
		using debug_index_list_type = std::span<index_type>;

		struct debug_range_info_type
		{
			debug_vertex_range_type vertices;
			debug_index_range_type indices;
		};

		struct debug_list_info_type
		{
			std::string what;

			debug_vertex_list_type vertices;
			debug_index_list_type indices;
		};

		using debug_range_info_list_type = list_type<debug_range_info_type>;
		using debug_list_info_list_type = list_type<debug_list_info_type>;
		#endif

		DrawListFlag draw_list_flag_;
		std::shared_ptr<DrawListSharedData> shared_data_;

		// vertex_list: v1-v2-v3-v4 + v5-v6-v7-v8 + v9-v10-v11 => rect0 + rect1(clipped by rect0) + triangle0(clipped by rect1)
		// index_list: 0/1/2-0/2/3 + 4/5/6-4/6/7 + 8/9/10
		// command_list: 
		//	0: .clip_rect = {0, 0, root_window_width, root_window_height}, .index_offset = 0, .element_count = root_window_element_count + 6 (two triangles => 0/1/2-0/2/3)
		// 1: .clip_rect = {max(rect0.left, rect1.left), max(rect0.top, rect1.top), min(rect0.right, rect1.right), min(rect0.bottom, rect1.bottom)}, .index_offset = root_window_element_count + 6, .element_count = 6 (two triangles => 4/5/6-4/6/7)
		// 2: .clip_rect = {...}, .index_offset = root_window_element_count + 12, .element_count = 3 (one triangle => 8/9/10)
		command_list_type command_list_;
		#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
		list_type<std::string> command_message_;
		#endif
		vertex_list_type vertex_list_;
		index_list_type index_list_;

		#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
		debug_range_info_list_type debug_range_info_list_;
		// If you want to get the exact data (and not just a [begin, end] of range), 
		// you have to `explicitly` call `bind_debug_info` after `all` operations are finished,
		// because only then can you be sure that the data will be valid.
		debug_list_info_list_type debug_list_info_list_;
		#endif

		rect_type this_command_clip_rect_;
		texture_id_type this_command_texture_id_;
		// there may be additional data

		path_list_type path_list_;

		constexpr auto push_command(
			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			std::string&& message
			#endif
		) noexcept -> void
		{
			// fixme: If the window boundary is smaller than the rect boundary, the rect will no longer be valid.
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not this_command_clip_rect_.empty() and this_command_clip_rect_.valid());

			command_list_.emplace_back(
				command_type
				{
						.clip_rect = this_command_clip_rect_,
						.texture_id = this_command_texture_id_,
						.index_offset = index_list_.size(),
						// set by subsequent draw_xxx
						.element_count = 0
				}
			);
			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			command_message_.emplace_back(std::move(message));
			#endif
		}

		enum class ChangedElement : std::uint8_t
		{
			CLIP_RECT,
			TEXTURE_ID,
		};

		template<ChangedElement Element>
		constexpr auto on_element_changed() noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not command_list_.empty());

			const auto& [current_clip_rect, current_texture_id, current_index_offset, current_element_count] = command_list_.back();
			if (current_element_count != 0)
			{
				if constexpr (Element == ChangedElement::CLIP_RECT)
				{
					if (current_clip_rect != this_command_clip_rect_)
					{
						push_command(
							#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
							std::format("[PUSH CLIP_RECT] {} -> {}", current_clip_rect, this_command_clip_rect_)
							#endif
						);

						return;
					}
				}
				else if constexpr (Element == ChangedElement::TEXTURE_ID)
				{
					if (current_texture_id != this_command_texture_id_)
					{
						push_command(
							#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
							std::format("[PUSH TEXTURE_ID] [{}] -> [{}]", current_texture_id, this_command_texture_id_)
							#endif
						);

						return;
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			// try to merge with previous command if it matches, else use current command
			if (command_list_.size() > 1)
			{
				if (
					const auto& [previous_clip_rect, previous_texture, previous_index_offset, previous_element_count] = command_list_[command_list_.size() - 2];
					current_element_count == 0 and
					(
						this_command_clip_rect_ == previous_clip_rect and
						this_command_texture_id_ == previous_texture
						// there may be additional data
					) and
					// sequential
					current_index_offset == previous_index_offset + previous_element_count
				)
				{
					command_list_.pop_back();
					#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
					command_message_.pop_back();
					#endif
					return;
				}
			}

			if constexpr (Element == ChangedElement::CLIP_RECT)
			{
				command_list_.back().clip_rect = this_command_clip_rect_;
			}
			else if constexpr (Element == ChangedElement::TEXTURE_ID)
			{
				command_list_.back().texture_id = this_command_texture_id_;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

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
			const texture_id_type texture_id,
			const rect_type& display_rect,
			const rect_type& uv_rect,
			const color_type& color,
			float rounding,
			DrawFlag flag
		) noexcept -> void;

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

		auto path_arc_fast(const circle_type& circle, ArcQuadrant quadrant) noexcept -> void;

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

		constexpr auto draw_list_flag(const DrawListFlag flag) noexcept -> void
		{
			draw_list_flag_ = flag;
		}

		constexpr auto draw_list_flag(const std::underlying_type_t<DrawListFlag> flag) noexcept -> void
		{
			draw_list_flag(static_cast<DrawListFlag>(flag));
		}

		auto shared_data(const std::shared_ptr<DrawListSharedData>& shared_data) noexcept -> void
		{
			shared_data_ = shared_data;
		}

		[[nodiscard]] auto shared_data() const noexcept -> std::shared_ptr<DrawListSharedData>
		{
			return shared_data_;
		}

		auto reset() noexcept -> void;

		auto bind_debug_info() noexcept -> void;

		[[nodiscard]] constexpr auto command_list() const noexcept -> auto
		{
			return command_list_ | std::views::all;
		}

		[[nodiscard]] constexpr auto vertex_list() const noexcept -> auto
		{
			return vertex_list_ | std::views::all;
		}

		[[nodiscard]] constexpr auto index_list() const noexcept -> auto
		{
			return index_list_ | std::views::all;
		}

		auto push_clip_rect(const rect_type& rect, bool intersect_with_current_clip_rect) noexcept -> rect_type&;

		auto push_clip_rect(const point_type& left_top, const point_type& right_bottom, bool intersect_with_current_clip_rect) noexcept -> rect_type&;

		auto pop_clip_rect() noexcept -> void;

		auto push_texture_id(const texture_id_type texture) noexcept -> void;

		auto pop_texture_id() noexcept -> void;

		auto line(
			const point_type& from,
			const point_type& to,
			const color_type& color,
			const float thickness = 1.f
		) noexcept -> void;

		auto triangle(
			const point_type& a,
			const point_type& b,
			const point_type& c,
			const color_type& color,
			const float thickness = 1.f
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
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE,
			const float thickness = 1.f
		) noexcept -> void;

		auto rect(
			const point_type& left_top,
			const point_type& right_bottom,
			const color_type& color,
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE,
			const float thickness = 1.f
		) noexcept -> void;

		auto rect_filled(
			const rect_type& rect,
			const color_type& color,
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE
		) noexcept -> void;

		auto rect_filled(
			const point_type& left_top,
			const point_type& right_bottom,
			const color_type& color,
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE
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
			const float thickness = 1.f
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
			const std::uint32_t segments,
			const float thickness = 1.f
		) noexcept -> void;

		auto circle_n(
			const point_type& center,
			const float radius,
			const color_type& color,
			const std::uint32_t segments,
			const float thickness = 1.f
		) noexcept -> void;

		auto ellipse_n(
			const ellipse_type& ellipse,
			const color_type& color,
			const std::uint32_t segments,
			const float thickness = 1.f
		) noexcept -> void;

		auto ellipse_n(
			const point_type& center,
			const extent_type& radius,
			const float rotation,
			const color_type& color,
			const std::uint32_t segments,
			const float thickness = 1.f
		) noexcept -> void;

		auto circle_n_filled(
			const circle_type& circle,
			const color_type& color,
			const std::uint32_t segments
		) noexcept -> void;

		auto circle_n_filled(
			const point_type& center,
			const float radius,
			const color_type& color,
			const std::uint32_t segments
		) noexcept -> void;

		auto ellipse_n_filled(
			const ellipse_type& ellipse,
			const color_type& color,
			const std::uint32_t segments
		) noexcept -> void;

		auto ellipse_n_filled(
			const point_type& center,
			const extent_type& radius,
			const float rotation,
			const color_type& color,
			const std::uint32_t segments
		) noexcept -> void;

		auto circle(
			const circle_type& circle,
			const color_type& color,
			const std::uint32_t segments = 0,
			const float thickness = 1.f
		) noexcept -> void;

		auto circle(
			const point_type& center,
			const float radius,
			const color_type& color,
			const std::uint32_t segments = 0,
			const float thickness = 1.f
		) noexcept -> void;

		auto circle_filled(
			const circle_type& circle,
			const color_type& color,
			const std::uint32_t segments = 0
		) noexcept -> void;

		auto circle_filled(
			const point_type& center,
			const float radius,
			const color_type& color,
			const std::uint32_t segments = 0
		) noexcept -> void;

		auto ellipse(
			const ellipse_type& ellipse,
			const color_type& color,
			std::uint32_t segments = 0,
			const float thickness = 1.f
		) noexcept -> void;

		auto ellipse(
			const point_type& center,
			const extent_type& radius,
			const float rotation,
			const color_type& color,
			const std::uint32_t segments = 0,
			const float thickness = 1.f
		) noexcept -> void;

		auto ellipse_filled(
			const ellipse_type& ellipse,
			const color_type& color,
			std::uint32_t segments = 0
		) noexcept -> void;

		auto ellipse_filled(
			const point_type& center,
			const extent_type& radius,
			const float rotation,
			const color_type& color,
			const std::uint32_t segments = 0
		) noexcept -> void;

		auto bezier_cubic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			const color_type& color,
			const std::uint32_t segments = 0,
			const float thickness = 1.f
		) noexcept -> void;

		auto bezier_quadratic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const color_type& color,
			const std::uint32_t segments = 0,
			const float thickness = 1.f
		) noexcept -> void;

		auto text(
			const Font& font,
			const float font_size,
			const point_type& p,
			const color_type& color,
			const std::string_view utf8_text,
			const float wrap_width = std::numeric_limits<float>::max()
		) noexcept -> void;

		auto text(
			const float font_size,
			const point_type& p,
			const color_type& color,
			const std::string_view utf8_text,
			const float wrap_width = std::numeric_limits<float>::max()
		) noexcept -> void;

		// p1 ________ p2
		//   |       |
		//   |       |
		// p4|_______| p3
		auto image(
			const texture_id_type texture_id,
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
			const texture_id_type texture_id,
			const rect_type& display_rect,
			const rect_type& uv_rect = {0, 0, 1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void;

		auto image(
			const texture_id_type texture_id,
			const point_type& display_left_top,
			const point_type& display_right_bottom,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void;

		auto image_rounded(
			const texture_id_type texture_id,
			const rect_type& display_rect,
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE,
			const rect_type& uv_rect = {0, 0, 1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void;

		auto image_rounded(
			const texture_id_type texture_id,
			const point_type& display_left_top,
			const point_type& display_right_bottom,
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void;
	};
}
