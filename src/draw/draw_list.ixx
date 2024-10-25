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
import :error;
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

#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
#include <string>
#include <format>
#include <span>
#endif

#include <prometheus/macro.hpp>

#if not defined(GAL_PROMETHEUS_DRAW_LIST_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 0
#endif
#endif

#include <functional/functional.ixx>
#include <primitive/primitive.ixx>
#include <chars/chars.ixx>

#include <draw/font.ixx>
#include <draw/draw_list.flag.ixx>
#include <draw/draw_list.shared_data.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
{
	class DrawList final
	{
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

		constexpr auto reset() noexcept -> void
		{
			command_list_.resize(0);
			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			command_message_.resize(0);
			#endif
			vertex_list_.resize(0);
			index_list_.resize(0);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.resize(0);
			debug_list_info_list_.resize(0);
			#endif

			// we don't know the size of the clip rect, so we need the user to set it
			this_command_clip_rect_ = {};
			// the first texture is always the (default) font texture
			this_command_texture_id_ = shared_data_->get_default_font().texture_id();

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(path_list_.empty());

			// we always have a command ready in the buffer
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
		}

		constexpr auto bind_debug_info() noexcept -> void
		{
			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(debug_range_info_list_.size() == debug_list_info_list_.size());

			for (size_type i = 0; i < debug_list_info_list_.size(); ++i)
			{
				auto& [range_vertices, range_indices] = debug_range_info_list_[i];
				auto& [what, list_vertices, list_indices] = debug_list_info_list_[i];
				list_vertices = {vertex_list_.begin() + range_vertices.first, vertex_list_.begin() + range_vertices.second};
				list_indices = {index_list_.begin() + range_indices.first, index_list_.begin() + range_indices.second};
			}
			#endif
		}

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

		constexpr auto push_clip_rect(const rect_type& rect, const bool intersect_with_current_clip_rect) noexcept -> rect_type&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not rect.empty() and rect.valid());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not command_list_.empty());

			const auto& [current_clip_rect, current_texture, current_index_offset, current_element_count] = command_list_.back();

			this_command_clip_rect_ = intersect_with_current_clip_rect ? rect.combine_min(current_clip_rect) : rect;

			on_element_changed<ChangedElement::CLIP_RECT>();
			return command_list_.back().clip_rect;
		}

		constexpr auto push_clip_rect(const point_type& left_top, const point_type& right_bottom, const bool intersect_with_current_clip_rect) noexcept -> rect_type&
		{
			return push_clip_rect({left_top, right_bottom}, intersect_with_current_clip_rect);
		}

		constexpr auto pop_clip_rect() noexcept -> void
		{
			// todo
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(command_list_.size() > 1);
			this_command_clip_rect_ = command_list_[command_list_.size() - 2].clip_rect;

			on_element_changed<ChangedElement::CLIP_RECT>();
		}

		constexpr auto push_texture_id(const texture_id_type texture) noexcept -> void
		{
			this_command_texture_id_ = texture;

			on_element_changed<ChangedElement::TEXTURE_ID>();
		}

		constexpr auto pop_texture_id() noexcept -> void
		{
			// todo
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(command_list_.size() > 1);
			this_command_texture_id_ = command_list_[command_list_.size() - 2].texture_id;

			on_element_changed<ChangedElement::TEXTURE_ID>();
		}

		constexpr auto line(const point_type& from, const point_type& to, const color_type& color, const float thickness = 1.f) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			// path_pin(from + point_type{.5f, .5f});
			// path_pin(to + point_type{.5f, .5f});
			path_pin(from);
			path_pin(to);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color, DrawFlag::NONE, thickness);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[LINE] [{} => {}]{}({:.3f})", from, to, color, thickness),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto triangle(const point_type& a, const point_type& b, const point_type& c, const color_type& color, const float thickness = 1.f) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			path_pin(a);
			path_pin(b);
			path_pin(c);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color, DrawFlag::CLOSED, thickness);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[TRIANGLE] [{} △ {} △ {}]{}({:.3f})", a, b, c, color, thickness),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto triangle_filled(const point_type& a, const point_type& b, const point_type& c, const color_type& color) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			path_pin(a);
			path_pin(b);
			path_pin(c);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[TRIANGLE FILLED] [{} ▲ {} ▲ {}]{}", a, b, c, color),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto rect(const rect_type& rect, const color_type& color, const float rounding = .0f, const DrawFlag flag = DrawFlag::NONE, const float thickness = 1.f) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			// path_rect(rect_type{rect.left_top() + point_type{.5f, .5f}, rect.right_bottom() - point_type{.5f, .5f}}, rounding, flag);
			path_rect(rect, rounding, flag);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color, DrawFlag::CLOSED, thickness);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[RECT] □ {}{}({:.3f})", rect, color, thickness),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto rect(
			const point_type& left_top,
			const point_type& right_bottom,
			const color_type& color,
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE,
			const float thickness = 1.f
		) noexcept -> void
		{
			return rect(rect_type{left_top, right_bottom}, color, rounding, flag, thickness);
		}

		constexpr auto rect_filled(const rect_type& rect, const color_type& color, const float rounding = .0f, const DrawFlag flag = DrawFlag::NONE) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			using functional::operators::operator&;
			if (rounding < .5f or (DrawFlag::ROUND_CORNER_MASK & flag) == DrawFlag::ROUND_CORNER_NONE)
			{
				draw_rect_filled(rect, color, color, color, color);
			}
			else
			{
				path_rect(rect, rounding, flag);
				path_stroke(color);
			}

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[RECT FILLED] ■ {}{}", rect, color),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto rect_filled(
			const point_type& left_top,
			const point_type& right_bottom,
			const color_type& color,
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE
		) noexcept -> void
		{
			return rect_filled(rect_type{left_top, right_bottom}, color, rounding, flag);
		}

		constexpr auto rect_filled(
			const rect_type& rect,
			const color_type& color_left_top,
			const color_type& color_right_top,
			const color_type& color_left_bottom,
			const color_type& color_right_bottom
		) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color_left_top.alpha == 0 or color_right_top.alpha == 0 or color_left_bottom.alpha == 0 or color_right_bottom.alpha == 0)
			{
				return;
			}

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			draw_rect_filled(rect, color_left_top, color_right_top, color_left_bottom, color_right_bottom);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[RECT FILLED] ■ {}({}/{}/{}/{})", rect, color_left_top, color_right_top, color_left_bottom, color_right_bottom),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto rect_filled(
			const point_type& left_top,
			const point_type& right_bottom,
			const color_type& color_left_top,
			const color_type& color_right_top,
			const color_type& color_left_bottom,
			const color_type& color_right_bottom
		) noexcept -> void
		{
			return rect_filled({left_top, right_bottom}, color_left_top, color_right_top, color_left_bottom, color_right_bottom);
		}

		constexpr auto quadrilateral(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4, const color_type& color, const float thickness = 1.f) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			path_quadrilateral(p1, p2, p3, p4);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color, DrawFlag::CLOSED, thickness);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[QUADRILATERAL] ▱ [{}-{}-{}-{}]{}({:.3f})", p1, p2, p3, p4, color, thickness),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto quadrilateral_filled(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4, const color_type& color) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			path_quadrilateral(p1, p2, p3, p4);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[QUADRILATERAL FILLED] ▰ [{}-{}-{}-{}]{}", p1, p2, p3, p4, color),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto circle_n(const circle_type& circle, const color_type& color, const std::uint32_t segments, const float thickness = 1.f) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0 or circle.radius < .5f or segments < 3)
			{
				return;
			}

			path_arc_n(circle, 0, std::numbers::pi_v<float> * 2, segments);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color, DrawFlag::CLOSED, thickness);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[CIRCLE] ○ {}{}({:.3f})", circle, color, thickness),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto circle_n(const point_type& center, const float radius, const color_type& color, const std::uint32_t segments, const float thickness = 1.f) noexcept -> void
		{
			return circle_n({center, radius}, color, segments, thickness);
		}

		constexpr auto ellipse_n(const ellipse_type& ellipse, const color_type& color, const std::uint32_t segments, const float thickness = 1.f) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f or segments < 3)
			{
				return;
			}

			path_arc_elliptical_n(ellipse, 0, std::numbers::pi_v<float> * 2, segments);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color, DrawFlag::CLOSED, thickness);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[ELLIPSE] ○ {}{}({:.3f})", ellipse, color, thickness),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto ellipse_n(
			const point_type& center,
			const extent_type& radius,
			const float rotation,
			const color_type& color,
			const std::uint32_t segments,
			const float thickness = 1.f
		) noexcept -> void
		{
			return ellipse_n({center, radius, rotation}, color, segments, thickness);
		}

		constexpr auto circle_n_filled(const circle_type& circle, const color_type& color, const std::uint32_t segments) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0 or circle.radius < .5f or segments < 3)
			{
				return;
			}

			path_arc_n(circle, 0, std::numbers::pi_v<float> * 2, segments);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[CIRCLE FILLED] ● {}{}", circle, color),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto circle_n_filled(const point_type& center, const float radius, const color_type& color, const std::uint32_t segments) noexcept -> void
		{
			return circle_n_filled({center, radius}, color, segments);
		}

		constexpr auto ellipse_n_filled(const ellipse_type& ellipse, const color_type& color, const std::uint32_t segments) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f or segments < 3)
			{
				return;
			}

			path_arc_elliptical_n(ellipse, 0, std::numbers::pi_v<float> * 2, segments);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[ELLIPSE FILLED] ● {}{}", ellipse, color),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto ellipse_n_filled(
			const point_type& center,
			const extent_type& radius,
			const float rotation,
			const color_type& color,
			const std::uint32_t segments
		) noexcept -> void
		{
			return ellipse_n_filled({center, radius, rotation}, color, segments);
		}

		constexpr auto circle(const circle_type& circle, const color_type& color, const std::uint32_t segments = 0, const float thickness = 1.f) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0 or circle.radius < .5f)
			{
				return;
			}

			if (segments == 0)
			{
				path_arc_fast(circle, 0, DrawListSharedData::vertex_sample_points_count - 1);

				#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
				const auto current_vertex_size = vertex_list_.size();
				const auto current_index_size = index_list_.size();
				#endif

				path_stroke(color, DrawFlag::CLOSED, thickness);

				#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
				debug_range_info_list_.emplace_back(
					debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
					debug_index_range_type{current_index_size, index_list_.size()}
				);
				debug_list_info_list_.emplace_back(
					std::format("[CIRCLE] ○ {}{}({:.3f})", circle, color, thickness),
					// the data stored now is unreliable
					debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
					// the data stored now is unreliable
					debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
				);
				#endif
			}
			else
			{
				const auto clamped_segments = std::ranges::clamp(segments, DrawListSharedData::circle_segments_min, DrawListSharedData::circle_segments_max);

				circle_n(circle, color, clamped_segments, thickness);
			}
		}

		constexpr auto circle(const point_type& center, const float radius, const color_type& color, const std::uint32_t segments = 0, const float thickness = 1.f) noexcept -> void
		{
			return circle({center, radius}, color, segments, thickness);
		}

		constexpr auto circle_filled(const circle_type& circle, const color_type& color, const std::uint32_t segments = 0) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0 or circle.radius < .5f)
			{
				return;
			}

			if (segments == 0)
			{
				path_arc_fast(circle, 0, DrawListSharedData::vertex_sample_points_count - 1);

				#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
				const auto current_vertex_size = vertex_list_.size();
				const auto current_index_size = index_list_.size();
				#endif

				path_stroke(color);

				#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
				debug_range_info_list_.emplace_back(
					debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
					debug_index_range_type{current_index_size, index_list_.size()}
				);
				debug_list_info_list_.emplace_back(
					std::format("[CIRCLE FILLED] ● {}{}", circle, color),
					// the data stored now is unreliable
					debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
					// the data stored now is unreliable
					debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
				);
				#endif
			}
			else
			{
				const auto clamped_segments = std::ranges::clamp(segments, DrawListSharedData::circle_segments_min, DrawListSharedData::circle_segments_max);

				circle_n_filled(circle, color, clamped_segments);
			}
		}

		constexpr auto circle_filled(const point_type& center, const float radius, const color_type& color, const std::uint32_t segments = 0) noexcept -> void
		{
			return circle_filled({center, radius}, color, segments);
		}

		constexpr auto ellipse(const ellipse_type& ellipse, const color_type& color, std::uint32_t segments = 0, const float thickness = 1.f) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f)
			{
				return;
			}

			if (segments == 0)
			{
				// fixme: maybe there's a better computation to do here
				segments = shared_data_->get_circle_auto_segment_count(std::ranges::max(ellipse.radius.width, ellipse.radius.height));
			}

			ellipse_n(ellipse, color, segments, thickness);
		}

		constexpr auto ellipse(
			const point_type& center,
			const extent_type& radius,
			const float rotation,
			const color_type& color,
			const std::uint32_t segments = 0,
			const float thickness = 1.f
		) noexcept -> void
		{
			return ellipse({center, radius, rotation}, color, segments, thickness);
		}

		constexpr auto ellipse_filled(const ellipse_type& ellipse, const color_type& color, std::uint32_t segments = 0) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f)
			{
				return;
			}

			if (segments == 0)
			{
				// fixme: maybe there's a better computation to do here
				segments = shared_data_->get_circle_auto_segment_count(std::ranges::max(ellipse.radius.width, ellipse.radius.height));
			}

			ellipse_n_filled(ellipse, color, segments);
		}

		constexpr auto ellipse_filled(
			const point_type& center,
			const extent_type& radius,
			const float rotation,
			const color_type& color,
			const std::uint32_t segments = 0
		) noexcept -> void
		{
			return ellipse_filled({center, radius, rotation}, color, segments);
		}

		constexpr auto bezier_cubic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			const color_type& color,
			const std::uint32_t segments = 0,
			const float thickness = 1.f
		) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			path_bezier_curve(p1, p2, p3, p4, segments);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color, DrawFlag::NONE, thickness);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[BEZIER CUBIC] ∫ [{}-{}-{}-{}]{}({:.3f})", p1, p2, p3, p4, color, thickness),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto bezier_quadratic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const color_type& color,
			const std::uint32_t segments = 0,
			const float thickness = 1.f
		) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			path_bezier_quadratic_curve(p1, p2, p3, segments);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			path_stroke(color, DrawFlag::NONE, thickness);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[BEZIER QUADRATIC] ৲ [{}-{}-{}]{}({:.3f})", p1, p2, p3, color, thickness),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto text(
			const Font& font,
			const float font_size,
			const point_type& p,
			const color_type& color,
			const std::string_view utf8_text,
			const float wrap_width = .0f
		) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0) { return; }

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			draw_text(font, font_size, p, color, utf8_text, wrap_width);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);

			debug_list_info_list_.emplace_back(
				std::format("[TEXT] [{}({:.3f}): {}]({})", p, font_size, utf8_text, color),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto text(
			const float font_size,
			const point_type& p,
			const color_type& color,
			const std::string_view utf8_text,
			const float wrap_width = .0f
		) noexcept -> void
		{
			this->text(shared_data_->get_default_font(), font_size, p, color, utf8_text, wrap_width);
		}

		//p1 ________ p2
		//     |          |
		//     |          |
		//p4 |__ ____| p3
		constexpr auto image(
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
		) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0) { return; }

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			draw_image(texture_id, display_p1, display_p2, display_p3, display_p4, uv_p1, uv_p2, uv_p3, uv_p4, color);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[IMAGE] [{}: {}-{}-{}-{}/{}-{}-{}-{}]{}", texture_id, display_p1, display_p2, display_p3, display_p4, uv_p1, uv_p2, uv_p3, uv_p4, color),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto image(
			const texture_id_type texture_id,
			const rect_type& display_rect,
			const rect_type& uv_rect = {0, 0, 1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void
		{
			image(
				texture_id,
				display_rect.left_top(),
				display_rect.right_top(),
				display_rect.right_bottom(),
				display_rect.left_bottom(),
				uv_rect.left_top(),
				uv_rect.right_top(),
				uv_rect.right_bottom(),
				uv_rect.left_bottom(),
				color
			);
		}

		constexpr auto image(
			const texture_id_type texture_id,
			const point_type& display_left_top,
			const point_type& display_right_bottom,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void
		{
			image(texture_id, {display_left_top, display_right_bottom}, {uv_left_top, uv_right_bottom}, color);
		}

		constexpr auto image_rounded(
			const texture_id_type texture_id,
			const rect_type& display_rect,
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE,
			const rect_type& uv_rect = {0, 0, 1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0)
			{
				return;
			}

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			draw_image_rounded(texture_id, display_rect, uv_rect, color, rounding, flag);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			debug_list_info_list_.emplace_back(
				std::format("[IMAGE] [{}: {}/{}]{}", texture_id, display_rect, uv_rect, color),
				// the data stored now is unreliable
				debug_vertex_list_type{vertex_list_.begin() + current_vertex_size, vertex_list_.end()},
				// the data stored now is unreliable
				debug_index_list_type{index_list_.begin() + current_index_size, index_list_.end()}
			);
			#endif
		}

		constexpr auto image_rounded(
			const texture_id_type texture_id,
			const point_type& display_left_top,
			const point_type& display_right_bottom,
			const float rounding = .0f,
			const DrawFlag flag = DrawFlag::NONE,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			const color_type& color = primitive::colors::white
		) noexcept -> void
		{
			image_rounded(texture_id, {display_left_top, display_right_bottom}, rounding, flag, {uv_left_top, uv_right_bottom}, color);
		}
	};
}
