// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not defined(GAL_PROMETHEUS_DRAW_LIST_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 0
#endif
#endif

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>
#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

export module gal.prometheus.draw:draw_list;

import std;
import gal.prometheus.functional;
import gal.prometheus.primitive;
import gal.prometheus.chars;
GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

import :font;

#else
#pragma once

#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

#include <vector>
#include <type_traits>
#include <utility>
#include <limits>
#include <numbers>

#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
#include <string>
#include <format>
#include <span>
#endif

#include <prometheus/macro.hpp>
#include <functional/functional.ixx>
#include <primitive/primitive.ixx>
#include <chars/chars.ixx>
#include <draw/font.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

namespace gal::prometheus::draw
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	enum class DrawFlag : std::uint32_t
	{
		NONE = 0,
		// specify that shape should be closed
		// @see basic_draw_list::draw_polygon_line
		// @see basic_draw_list::draw_polygon_line_aa
		// @see basic_draw_list::path_stroke
		CLOSED = 1 << 0,
		// enable rounding left-top corner only (when rounding > 0.0f, we default to all corners)
		// @see basic_draw_list::path_rect
		// @see basic_draw_list::rect
		// @see basic_draw_list::rect_filled
		ROUND_CORNER_LEFT_TOP = 1 << 1,
		// enable rounding right_top corner only (when rounding > 0.0f, we default to all corners)
		// @see basic_draw_list::path_rect
		// @see basic_draw_list::rect
		// @see basic_draw_list::rect_filled
		ROUND_CORNER_RIGHT_TOP = 1 << 2,
		// enable rounding left-bottom corner only (when rounding > 0.0f, we default to all corners)
		// @see basic_draw_list::path_rect
		// @see basic_draw_list::rect
		// @see basic_draw_list::rect_filled
		ROUND_CORNER_LEFT_BOTTOM = 1 << 3,
		// enable rounding right-bottom corner only (when rounding > 0.0f, we default to all corners)
		// @see basic_draw_list::path_rect
		// @see basic_draw_list::rect
		// @see basic_draw_list::rect_filled
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

	enum class DrawListFlag : std::uint32_t
	{
		NONE = 0x0000'0000,
		ANTI_ALIASED_LINE = 0x0000'0001,
		ANTI_ALIASED_LINE_USE_TEXTURE = 0x0000'0010,
		ANTI_ALIASED_FILL = 0x0000'0100,
	};

	enum class ArcQuadrant : std::uint32_t
	{
		// [0~3)
		Q1 = 0x0000'0001,
		// [3~6)
		Q2 = 0x0000'0010,
		// [6~9)
		Q3 = 0x0000'0100,
		// [9~12)
		Q4 = 0x0000'1000,

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
		Q1_CLOCK_WISH = 0x0001'0000,
		// [6, 3)
		Q2_CLOCK_WISH = 0x0010'0000,
		// [9, 6)
		Q3_CLOCK_WISH = 0x0100'0000,
		// [12, 9)
		Q4_CLOCK_WISH = 0x1000'0000,

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

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace draw_list_detail
	{
		[[nodiscard]] constexpr auto to_fixed_rect_corner_flag(const DrawFlag flag) noexcept -> DrawFlag
		{
			if (functional::exclude(flag, DrawFlag::ROUND_CORNER_MASK))
			{
				using functional::operators::operator|;
				return DrawFlag::ROUND_CORNER_ALL | flag;
			}

			return flag;
		}

		[[nodiscard]] constexpr auto to_fixed_normal(const float x, const float y) noexcept -> std::pair<float, float>
		{
			if (const auto d = functional::pow(x, 2) + functional::pow(y, 2);
				d > 1e-6f)
			{
				// fixme
				const auto inv_len = [d]
				{
					#if defined(__AVX512F__)
					__m512 d_v = _mm512_set1_ps(d);
					__m512 inv_len_v = _mm512_rcp14_ps(d_v);
					return _mm512_cvtss_f32(inv_len_v);
					#elif defined(__AVX__)
					__m256 d_v = _mm256_set1_ps(d);
					__m256 inv_len_v = _mm256_rcp_ps(d_v);
					return _mm256_cvtss_f32(inv_len_v);
					#elif defined(__SSE4_1__) or defined(__SSE3__) or defined(__SSE__)
					__m128 d_v = _mm_set_ss(d);
					__m128 inv_len_v = _mm_rcp_ss(d_v);
					return _mm_cvtss_f32(inv_len_v);
					#else
					return 1.0f / d;
					#endif
				}();

				return {x * inv_len, y * inv_len};
			}

			return {x, y};
		}

		using point_type = primitive::basic_point_2d<float>;
		using uv_type = primitive::basic_point_2d<float>;
		using color_type = primitive::basic_color<std::uint8_t>;

		using extent_type = primitive::basic_extent_2d<float>;
		using circle_type = primitive::basic_circle_2d<float>;
		using ellipse_type = primitive::basic_ellipse_2d<float>;
		using rect_type = primitive::basic_rect_2d<float>;

		using vertex_type = primitive::basic_vertex<point_type, uv_type, color_type>;
		using index_type = std::uint16_t;

		// @see https://stackoverflow.com/a/2244088/15194693
		// Number of segments (N) is calculated using equation:
		//	N = ceil ( pi / acos(1 - error / r) ) where r > 0 and error <= r
		constexpr std::uint32_t circle_segments_min = 4;
		constexpr std::uint32_t circle_segments_max = 512;
		constexpr auto circle_segments_calc = [](const float radius, const float max_error) noexcept -> auto
		{
			constexpr auto circle_segments_roundup_to_even = [](const auto v) noexcept -> auto
			{
				return (v + 1) / 2 * 2;
			};

			return std::ranges::clamp(
				circle_segments_roundup_to_even(static_cast<std::uint32_t>(std::ceil(std::numbers::pi_v<float> / std::acos(1 - std::ranges::min(radius, max_error) / radius)))),
				circle_segments_min,
				circle_segments_max
			);
		};
		constexpr auto circle_segments_calc_radius = [](const std::size_t n, const float max_error) noexcept -> auto
		{
			return max_error / (1 - std::cos(std::numbers::pi_v<float> / std::ranges::max(static_cast<float>(n), std::numbers::pi_v<float>)));
		};
		constexpr auto circle_segments_calc_error = [](const std::size_t n, const float radius) noexcept -> auto
		{
			return (1 - std::cos(std::numbers::pi_v<float> / std::ranges::max(static_cast<float>(n), std::numbers::pi_v<float>))) / radius;
		};

		constexpr std::size_t vertex_sample_points_count = 48;
		using vertex_sample_points_type = std::array<point_type, vertex_sample_points_count>;
		constexpr std::size_t circle_segment_counts_count = 64;
		using circle_segment_counts_type = std::array<std::uint8_t, circle_segment_counts_count>;

		constexpr auto vertex_sample_points = []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> vertex_sample_points_type
		{
			constexpr auto make_point = []<std::size_t I>() noexcept -> point_type
			{
				const auto a = static_cast<float>(I) / static_cast<float>(vertex_sample_points_type{}.size()) * 2 * std::numbers::pi_v<float>;
				return {functional::cos(a), -functional::sin(a)};
			};

			return {{make_point.template operator()<Index>()...}};
		}(std::make_index_sequence<vertex_sample_points_count>{});

		constexpr auto range_of_quadrant = [](const ArcQuadrant quadrant) noexcept -> std::pair<int, int>
		{
			constexpr auto factor = static_cast<int>(vertex_sample_points.size() / 12);

			switch (quadrant)
			{
				case ArcQuadrant::Q1: { return std::make_pair(0 * factor, 3 * factor); }
				case ArcQuadrant::Q2: { return std::make_pair(3 * factor, 6 * factor); }
				case ArcQuadrant::Q3: { return std::make_pair(6 * factor, 9 * factor); }
				case ArcQuadrant::Q4: { return std::make_pair(9 * factor, 12 * factor); }
				case ArcQuadrant::TOP: { return std::make_pair(0 * factor, 6 * factor); }
				case ArcQuadrant::BOTTOM: { return std::make_pair(6 * factor, 12 * factor); }
				case ArcQuadrant::LEFT: { return std::make_pair(3 * factor, 9 * factor); }
				case ArcQuadrant::RIGHT: { return std::make_pair(9 * factor, 15 * factor); }
				case ArcQuadrant::ALL: { return std::make_pair(0 * factor, 12 * factor); }
				case ArcQuadrant::Q1_CLOCK_WISH: { return std::make_pair(3 * factor, 0 * factor); }
				case ArcQuadrant::Q2_CLOCK_WISH: { return std::make_pair(6 * factor, 3 * factor); }
				case ArcQuadrant::Q3_CLOCK_WISH: { return std::make_pair(9 * factor, 6 * factor); }
				case ArcQuadrant::Q4_CLOCK_WISH: { return std::make_pair(12 * factor, 9 * factor); }
				case ArcQuadrant::TOP_CLOCK_WISH: { return std::make_pair(6 * factor, 0 * factor); }
				case ArcQuadrant::BOTTOM_CLOCK_WISH: { return std::make_pair(12 * factor, 6 * factor); }
				case ArcQuadrant::LEFT_CLOCK_WISH: { return std::make_pair(9 * factor, 3 * factor); }
				case ArcQuadrant::RIGHT_CLOCK_WISH: { return std::make_pair(15 * factor, 9 * factor); }
				case ArcQuadrant::ALL_CLOCK_WISH: { return std::make_pair(12 * factor, 0 * factor); }
			}

			std::unreachable();
		};

		constexpr auto bezier_cubic_calc = [](const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4, const float tolerance) noexcept -> point_type
		{
			const auto u = 1.f - tolerance;

			const auto w1 = functional::pow(u, 3);
			const auto w2 = 3 * functional::pow(u, 2) * tolerance;
			const auto w3 = 3 * u * functional::pow(tolerance, 2);
			const auto w4 = functional::pow(tolerance, 3);

			return {
					p1.x * w1 + p2.x * w2 + p3.x * w3 + p4.x * w4,
					p1.y * w1 + p2.y * w2 + p3.y * w3 + p4.y * w4
			};
		};

		constexpr auto bezier_quadratic_calc = [](const point_type& p1, const point_type& p2, const point_type& p3, const float tolerance) noexcept -> point_type
		{
			const auto u = 1.f - tolerance;

			const auto w1 = functional::pow(u, 2);
			const auto w2 = 2 * u * tolerance;
			const auto w3 = functional::pow(tolerance, 2);

			return {
					p1.x * w1 + p2.x * w2 + p3.x * w3,
					p1.y * w1 + p2.y * w2 + p3.y * w3
			};
		};

		constexpr auto draw_list_texture_line_max_width{63};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	class DrawListSharedData final
	{
	public:
		using vertex_type = draw_list_detail::vertex_type;
		using uv_type = vertex_type::uv_type;

		using circle_segment_counts_type = draw_list_detail::circle_segment_counts_type;
		using vertex_sample_points_type = draw_list_detail::vertex_sample_points_type;

		constexpr static auto vertex_sample_points_count = draw_list_detail::vertex_sample_points_count;

	private:
		circle_segment_counts_type circle_segment_counts_;
		// Maximum error (in pixels) allowed when using `circle`/`circle_filled` or drawing rounded corner rectangles with no explicit segment count specified.
		// Decrease for higher quality but more geometry.
		float circle_segment_max_error_;
		// Cutoff radius after which arc drawing will fall back to slower `path_arc`
		float arc_fast_radius_cutoff_;
		// Tessellation tolerance when using `path_bezier_curve` without a specific number of segments.
		// Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
		float curve_tessellation_tolerance_;

		uv_type texture_uv_of_white_pixel_;

		font_type default_font_;

	public:
		DrawListSharedData() noexcept
			:
			circle_segment_counts_{},
			circle_segment_max_error_{},
			arc_fast_radius_cutoff_{},
			curve_tessellation_tolerance_{1.25f},
			texture_uv_of_white_pixel_{0},
			default_font_{}
		{
			set_circle_tessellation_max_error(.3f);
		}

		constexpr auto set_circle_tessellation_max_error(const float max_error) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(max_error > .0f);

			if (circle_segment_max_error_ == max_error) // NOLINT(clang-diagnostic-float-equal)
			{
				return;
			}

			for (decltype(circle_segment_counts_.size()) i = 0; i < circle_segment_counts_.size(); ++i)
			{
				const auto radius = static_cast<float>(i);
				circle_segment_counts_[i] = static_cast<std::uint8_t>(draw_list_detail::circle_segments_calc(radius, max_error));
			}
			circle_segment_max_error_ = max_error;
			arc_fast_radius_cutoff_ = draw_list_detail::circle_segments_calc_radius(vertex_sample_points_count, max_error);
		}

		[[nodiscard]] constexpr auto get_circle_tessellation_max_error() const noexcept -> float
		{
			return circle_segment_max_error_;
		}

		[[nodiscard]] constexpr auto get_arc_fast_radius_cutoff() const noexcept -> float
		{
			return arc_fast_radius_cutoff_;
		}

		constexpr auto set_curve_tessellation_tolerance(const float tolerance) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(tolerance > .0f);

			curve_tessellation_tolerance_ = tolerance;
		}

		[[nodiscard]] constexpr auto get_curve_tessellation_tolerance() const noexcept -> float
		{
			return curve_tessellation_tolerance_;
		}

		[[nodiscard]] constexpr auto get_circle_auto_segment_count(const float radius) const noexcept -> auto
		{
			// ceil to never reduce accuracy
			if (const auto radius_index = static_cast<std::uintptr_t>(radius + .999999f); radius_index < circle_segment_counts_.size())
			{
				return circle_segment_counts_[radius_index];
			}
			return static_cast<circle_segment_counts_type::value_type>(draw_list_detail::circle_segments_calc(radius, circle_segment_max_error_));
		}

		constexpr auto set_texture_uv_of_white_pixel(const uv_type& uv) noexcept -> void
		{
			texture_uv_of_white_pixel_ = uv;
		}

		[[nodiscard]] constexpr auto get_texture_uv_of_white_pixel() const noexcept -> const uv_type&
		{
			return texture_uv_of_white_pixel_;
		}

		auto set_default_font(font_type&& font) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(font.texture_data);

			default_font_ = std::move(font);
		}

		[[nodiscard]] constexpr auto get_default_font() noexcept -> font_type&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(default_font_.texture_data);

			return default_font_;
		}

		[[nodiscard]] constexpr auto get_default_font() const noexcept -> const font_type&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(default_font_.texture_data);

			return default_font_;
		}
	};

	class DrawList final
	{
	public:
		using point_type = draw_list_detail::point_type;
		using uv_type = draw_list_detail::uv_type;
		using color_type = draw_list_detail::color_type;

		using extent_type = draw_list_detail::extent_type;
		using circle_type = draw_list_detail::circle_type;
		using ellipse_type = draw_list_detail::ellipse_type;
		using rect_type = draw_list_detail::rect_type;

		using texture_id_type = font_type::texture_id_type;

		using vertex_type = draw_list_detail::vertex_type;
		using index_type = draw_list_detail::index_type;

		template<typename T>
		using list_type = std::vector<T>;

		using size_type = std::size_t;

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

		enum class ChangedElement
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

		constexpr auto draw_polygon_line(const color_type& color, const DrawFlag draw_flag, const float thickness) noexcept -> void
		{
			const auto path_point_count = path_list_.size();
			const auto& path_point = path_list_;

			if (path_point_count < 2 or color.alpha == 0)
			{
				return;
			}

			const auto is_closed = not functional::exclude(draw_flag, DrawFlag::CLOSED);
			const auto segments_count = is_closed ? path_point_count : path_point_count - 1;

			const auto vertex_count = segments_count * 4;
			const auto index_count = segments_count * 6;
			vertex_list_.reserve(vertex_list_.size() + vertex_count);
			index_list_.reserve(index_list_.size() + index_count);

			command_list_.back().element_count += index_count;

			for (std::decay_t<decltype(segments_count)> i = 0; i < segments_count; ++i)
			{
				const auto n = (i + 1) % path_point_count;

				const auto& p1 = path_point[i];
				const auto& p2 = path_point[n];

				auto [normalized_x, normalized_y] = functional::normalize(p2.x - p1.x, p2.y - p1.y);
				normalized_x *= (thickness * .5f);
				normalized_y *= (thickness * .5f);

				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(vertex_list_.size() + 3 <= std::numeric_limits<index_type>::max());

				const auto current_vertex_index = static_cast<index_type>(vertex_list_.size());

				const auto& opaque_uv = shared_data_->get_texture_uv_of_white_pixel();

				vertex_list_.emplace_back(p1 + point_type{normalized_y, -normalized_x}, opaque_uv, color);
				vertex_list_.emplace_back(p2 + point_type{normalized_y, -normalized_x}, opaque_uv, color);
				vertex_list_.emplace_back(p2 + point_type{-normalized_y, normalized_x}, opaque_uv, color);
				vertex_list_.emplace_back(p1 + point_type{-normalized_y, normalized_x}, opaque_uv, color);

				index_list_.push_back(current_vertex_index + 0);
				index_list_.push_back(current_vertex_index + 1);
				index_list_.push_back(current_vertex_index + 2);
				index_list_.push_back(current_vertex_index + 0);
				index_list_.push_back(current_vertex_index + 2);
				index_list_.push_back(current_vertex_index + 3);
			}
		}

		constexpr auto draw_polygon_line_aa(const color_type& color, const DrawFlag draw_flag, float thickness) noexcept -> void
		{
			const auto path_point_count = path_list_.size();
			const auto& path_point = path_list_;

			if (path_point_count < 2 or color.alpha == 0)
			{
				return;
			}

			const auto& opaque_uv = shared_data_->get_texture_uv_of_white_pixel();
			const auto transparent_color = color.transparent();

			const auto is_closed = not functional::exclude(draw_flag, DrawFlag::CLOSED);
			const auto segments_count = is_closed ? path_point_count : path_point_count - 1;
			const auto is_thick_line = thickness > 1.f;

			thickness = std::ranges::max(thickness, 1.f);
			const auto thickness_integer = static_cast<int>(thickness);
			const auto thickness_fractional = thickness - static_cast<float>(thickness_integer);

			const auto is_use_texture =
			(
				functional::contains<functional::EnumCheckPolicy::ANY_BIT>(draw_list_flag_, DrawListFlag::ANTI_ALIASED_LINE_USE_TEXTURE) and
				(thickness_integer < draw_list_detail::draw_list_texture_line_max_width) and
				(thickness_fractional <= .00001f));

			const auto vertex_cont = is_use_texture ? (path_point_count * 2) : (is_thick_line ? path_point_count * 4 : path_point_count * 3);
			const auto index_count = is_use_texture ? (segments_count * 6) : (is_thick_line ? segments_count * 18 : segments_count * 12);
			vertex_list_.reserve(vertex_list_.size() + vertex_cont);
			index_list_.reserve(index_list_.size() + index_count);

			command_list_.back().element_count += index_count;

			// The first <path_point_count> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
			list_type<point_type> temp_buffer{};
			temp_buffer.resize(path_point_count * ((is_use_texture or not is_thick_line) ? 3 : 5));
			auto temp_buffer_normals = std::span{temp_buffer.begin(), path_point_count};
			auto temp_buffer_points = std::span{temp_buffer.begin() + static_cast<std::ptrdiff_t>(path_point_count), temp_buffer.end()};

			// Calculate normals (tangents) for each line segment
			for (std::decay_t<decltype(segments_count)> i = 0; i < segments_count; ++i)
			{
				const auto n = (i + 1) % path_point_count;
				const auto d = path_point[n] - path_point[i];

				const auto [normalized_x, normalized_y] = functional::normalize(d.x, d.y);
				temp_buffer_normals[i].x = normalized_y;
				temp_buffer_normals[i].y = -normalized_x;
			}

			if (not is_closed)
			{
				temp_buffer_normals[temp_buffer_normals.size() - 1] = temp_buffer_normals[temp_buffer_normals.size() - 2];
			}

			// If we are drawing a one-pixel-wide line without a texture, or a textured line of any width, we only need 2 or 3 vertices per point
			if (is_use_texture or not is_thick_line)
			{
				// [PATH 1] Texture-based lines (thick or non-thick)

				// The width of the geometry we need to draw - this is essentially <thickness> pixels for the line itself, plus "one pixel" for AA.
				const auto half_draw_size = is_use_texture ? ((thickness * .5f) + 1) : 1.f;

				// If line is not closed, the first and last points need to be generated differently as there are no normals to blend
				if (not is_closed)
				{
					temp_buffer_points[0] = path_point[0] + temp_buffer_normals[0] * half_draw_size;
					temp_buffer_points[1] = path_point[0] - temp_buffer_normals[0] * half_draw_size;
					temp_buffer_points[(path_point_count - 1) * 2 + 0] = path_point[path_point_count - 1] + temp_buffer_normals[path_point_count - 1] * half_draw_size;
					temp_buffer_points[(path_point_count - 1) * 2 + 1] = path_point[path_point_count - 1] - temp_buffer_normals[path_point_count - 1] * half_draw_size;
				}

				const auto current_vertex_index = static_cast<index_type>(vertex_list_.size());

				// Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
				// This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
				auto vertex_index_for_start = current_vertex_index;
				for (std::decay_t<decltype(segments_count)> first_point_of_segment = 0; first_point_of_segment < segments_count; ++first_point_of_segment)
				{
					const auto second_point_of_segment = (first_point_of_segment + 1) % path_point_count;
					const auto vertex_index_for_end = static_cast<index_type>(
						// closed
						(first_point_of_segment + 1) == path_point_count
							? current_vertex_index
							: (vertex_index_for_start + (is_use_texture ? 2 : 3))
					);

					// Average normals
					const auto d = (temp_buffer_normals[first_point_of_segment] + temp_buffer_normals[second_point_of_segment]) * .5f;
					// dm_x, dm_y are offset to the outer edge of the AA area
					auto [dm_x, dm_y] = draw_list_detail::to_fixed_normal(d.x, d.y);
					dm_x *= half_draw_size;
					dm_y *= half_draw_size;

					// Add temporary vertexes for the outer edges
					temp_buffer_points[second_point_of_segment * 2 + 0] = path_point[second_point_of_segment] + point_type{dm_x, dm_y};
					temp_buffer_points[second_point_of_segment * 2 + 1] = path_point[second_point_of_segment] - point_type{dm_x, dm_y};

					if (is_use_texture)
					{
						// Add indices for two triangles

						// right
						index_list_.push_back(vertex_index_for_end + 0);
						index_list_.push_back(vertex_index_for_start + 0);
						index_list_.push_back(vertex_index_for_start + 1);
						// left
						index_list_.push_back(vertex_index_for_end + 1);
						index_list_.push_back(vertex_index_for_start + 1);
						index_list_.push_back(vertex_index_for_end + 0);
					}
					else
					{
						// Add indexes for four triangles

						// right 1
						index_list_.push_back(vertex_index_for_end + 0);
						index_list_.push_back(vertex_index_for_start + 0);
						index_list_.push_back(vertex_index_for_start + 2);
						// right 2
						index_list_.push_back(vertex_index_for_start + 2);
						index_list_.push_back(vertex_index_for_end + 2);
						index_list_.push_back(vertex_index_for_end + 0);
						// left 1
						index_list_.push_back(vertex_index_for_end + 1);
						index_list_.push_back(vertex_index_for_start + 1);
						index_list_.push_back(vertex_index_for_start + 0);
						// left 2
						index_list_.push_back(vertex_index_for_start + 0);
						index_list_.push_back(vertex_index_for_end + 0);
						index_list_.push_back(vertex_index_for_end + 1);
					}

					vertex_index_for_start = vertex_index_for_end;
				}

				// Add vertexes for each point on the line
				if (is_use_texture)
				{
					// todo: get the texture
					GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();
				}
				else
				{
					// If we're not using a texture, we need the center vertex as well
					for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
					{
						// center of line
						vertex_list_.emplace_back(path_point[i], opaque_uv, color);
						// left-side outer edge
						vertex_list_.emplace_back(temp_buffer_points[i * 2 + 0], opaque_uv, transparent_color);
						// right-side outer edge
						vertex_list_.emplace_back(temp_buffer_points[i * 2 + 1], opaque_uv, transparent_color);
					}
				}
			}
			else
			{
				// [PATH 2] Non-texture-based lines (non-thick)

				// we need to draw the solid line core and thus require four vertices per point
				const auto half_inner_thickness = (thickness - 1.f) * .5f;

				// If line is not closed, the first and last points need to be generated differently as there are no normals to blend
				if (not is_closed)
				{
					const auto point_last = path_point_count - 1;
					temp_buffer_points[0] = path_point[0] + temp_buffer_normals[0] * (half_inner_thickness + 1.f);
					temp_buffer_points[1] = path_point[0] + temp_buffer_normals[0] * (half_inner_thickness + 0.f);
					temp_buffer_points[2] = path_point[0] - temp_buffer_normals[0] * (half_inner_thickness + 0.f);
					temp_buffer_points[3] = path_point[0] - temp_buffer_normals[0] * (half_inner_thickness + 1.f);
					temp_buffer_points[point_last * 4 + 0] = path_point[point_last] + temp_buffer_normals[point_last] * (half_inner_thickness + 1.f);
					temp_buffer_points[point_last * 4 + 1] = path_point[point_last] + temp_buffer_normals[point_last] * (half_inner_thickness + 0.f);
					temp_buffer_points[point_last * 4 + 2] = path_point[point_last] - temp_buffer_normals[point_last] * (half_inner_thickness + 0.f);
					temp_buffer_points[point_last * 4 + 3] = path_point[point_last] - temp_buffer_normals[point_last] * (half_inner_thickness + 1.f);
				}

				const auto current_vertex_index = static_cast<index_type>(vertex_list_.size());

				// Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
				// This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
				auto vertex_index_for_start = current_vertex_index;
				for (std::decay_t<decltype(segments_count)> first_point_of_segment = 0; first_point_of_segment < segments_count; ++first_point_of_segment)
				{
					const auto second_point_of_segment = (first_point_of_segment + 1) % path_point_count;
					const auto vertex_index_for_end = static_cast<index_type>(
						(first_point_of_segment + 1) == path_point_count
							? current_vertex_index
							: (vertex_index_for_start + 4)
					);

					// Average normals
					const auto d = (temp_buffer_normals[first_point_of_segment] + temp_buffer_normals[second_point_of_segment]) * .5f;
					const auto [dm_x, dm_y] = draw_list_detail::to_fixed_normal(d.x, d.y);
					const auto dm_out_x = dm_x * (half_inner_thickness + 1.f);
					const auto dm_out_y = dm_y * (half_inner_thickness + 1.f);
					const auto dm_in_x = dm_x * (half_inner_thickness + 0.f);
					const auto dm_in_y = dm_y * (half_inner_thickness + 0.f);

					// Add temporary vertices
					temp_buffer_points[second_point_of_segment * 4 + 0] = path_point[second_point_of_segment] + point_type{dm_out_x, dm_out_y};
					temp_buffer_points[second_point_of_segment * 4 + 1] = path_point[second_point_of_segment] + point_type{dm_in_x, dm_in_y};
					temp_buffer_points[second_point_of_segment * 4 + 2] = path_point[second_point_of_segment] - point_type{dm_in_x, dm_in_y};
					temp_buffer_points[second_point_of_segment * 4 + 3] = path_point[second_point_of_segment] - point_type{dm_out_x, dm_out_y};

					// Add indexes
					index_list_.push_back(vertex_index_for_end + 1);
					index_list_.push_back(vertex_index_for_end + 1);
					index_list_.push_back(vertex_index_for_start + 2);

					index_list_.push_back(vertex_index_for_start + 2);
					index_list_.push_back(vertex_index_for_end + 2);
					index_list_.push_back(vertex_index_for_end + 1);

					index_list_.push_back(vertex_index_for_end + 1);
					index_list_.push_back(vertex_index_for_start + 1);
					index_list_.push_back(vertex_index_for_start + 0);

					index_list_.push_back(vertex_index_for_start + 0);
					index_list_.push_back(vertex_index_for_end + 0);
					index_list_.push_back(vertex_index_for_end + 1);

					index_list_.push_back(vertex_index_for_end + 2);
					index_list_.push_back(vertex_index_for_start + 2);
					index_list_.push_back(vertex_index_for_start + 3);

					index_list_.push_back(vertex_index_for_start + 3);
					index_list_.push_back(vertex_index_for_end + 3);
					index_list_.push_back(vertex_index_for_end + 2);

					vertex_index_for_start = vertex_index_for_end;
				}

				// Add vertices
				for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
				{
					vertex_list_.emplace_back(temp_buffer_points[i * 4 + 0], opaque_uv, transparent_color);
					vertex_list_.emplace_back(temp_buffer_points[i * 4 + 1], opaque_uv, color);
					vertex_list_.emplace_back(temp_buffer_points[i * 4 + 2], opaque_uv, color);
					vertex_list_.emplace_back(temp_buffer_points[i * 4 + 2], opaque_uv, transparent_color);
				}
			}
		}

		constexpr auto draw_convex_polygon_line_filled(const color_type& color) noexcept -> void
		{
			const auto path_point_count = path_list_.size();
			const auto& path_point = path_list_;

			if (path_point_count < 3 or color.alpha == 0)
			{
				return;
			}

			const auto vertex_count = path_point_count;
			const auto index_count = (path_point_count - 2) * 3;
			vertex_list_.reserve(vertex_list_.size() + vertex_count);
			index_list_.reserve(index_list_.size() + index_count);

			command_list_.back().element_count += index_count;

			const auto current_vertex_index = vertex_list_.size();

			const auto& opaque_uv = shared_data_->get_texture_uv_of_white_pixel();

			std::ranges::transform(path_point, std::back_inserter(vertex_list_), [opaque_uv, color](const point_type& point) noexcept -> vertex_type { return {point, opaque_uv, color}; });
			for (index_type i = 2; std::cmp_less(i, path_point_count); ++i)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_index + 0 <= std::numeric_limits<index_type>::max());
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_index + i - 1 <= std::numeric_limits<index_type>::max());
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_index + i <= std::numeric_limits<index_type>::max());

				index_list_.push_back(static_cast<index_type>(current_vertex_index + 0));
				index_list_.push_back(static_cast<index_type>(current_vertex_index + i - 1));
				index_list_.push_back(static_cast<index_type>(current_vertex_index + i));
			}
		}

		constexpr auto draw_convex_polygon_line_filled_aa(const color_type& color) noexcept -> void
		{
			const auto path_point_count = path_list_.size();
			const auto& path_point = path_list_;

			if (path_point_count < 3 or color.alpha == 0)
			{
				return;
			}

			const auto& opaque_uv = shared_data_->get_texture_uv_of_white_pixel();
			const auto transparent_color = color.transparent();

			const auto vertex_count = path_point_count * 2;
			const auto index_count = (path_point_count - 2) * 3 + path_point_count * 6;
			vertex_list_.reserve(vertex_list_.size() + vertex_count);
			index_list_.reserve(index_list_.size() + index_count);

			command_list_.back().element_count += index_count;

			const auto current_vertex_inner_index = vertex_list_.size();
			const auto current_vertex_outer_index = vertex_list_.size() + 1;

			// Add indexes for fill
			for (index_type i = 2; std::cmp_less(i, path_point_count); ++i)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + 0 <= std::numeric_limits<index_type>::max());
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + ((i - 1) << 1) <= std::numeric_limits<index_type>::max());
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + (i << 1) <= std::numeric_limits<index_type>::max());

				index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + 0));
				index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + ((i - 1) << 1)));
				index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + (i << 1)));
			}

			list_type<point_type> temp_buffer{};
			temp_buffer.resize(path_point_count);
			auto temp_buffer_normals = std::span{temp_buffer.begin(), path_point_count};

			for (auto i = path_point_count - 1, n = static_cast<decltype(i)>(0); n < path_point_count; i = n++)
			{
				const auto d = path_point[n] - path_point[i];

				const auto [normalized_x, normalized_y] = functional::normalize(d.x, d.y);
				temp_buffer_normals[i].x = normalized_y;
				temp_buffer_normals[i].y = -normalized_x;
			}
			for (auto i = path_point_count - 1, n = static_cast<decltype(i)>(0); n < path_point_count; i = n++)
			{
				// Average normals
				const auto d = (temp_buffer_normals[n] + temp_buffer_normals[i]) * .5f;
				auto [dm_x, dm_y] = draw_list_detail::to_fixed_normal(d.x, d.y);
				dm_x *= .5f;
				dm_y *= .5f;

				// inner
				vertex_list_.emplace_back(path_point[n] - point_type{dm_x, dm_y}, opaque_uv, color);
				// outer
				vertex_list_.emplace_back(path_point[n] + point_type{dm_x, dm_y}, opaque_uv, transparent_color);

				// Add indexes for fringes
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + (n << 1) <= std::numeric_limits<index_type>::max());
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + (i << 1) <= std::numeric_limits<index_type>::max());
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + (i << 1) <=std::numeric_limits<index_type>::max());
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + (i << 1) <= std::numeric_limits<index_type>::max());
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + (n << 1) <= std::numeric_limits<index_type>::max());
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + (n << 1) <= std::numeric_limits<index_type>::max());

				index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + (n << 1)));
				index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + (i << 1)));
				index_list_.push_back(static_cast<index_type>(current_vertex_outer_index + (i << 1)));
				index_list_.push_back(static_cast<index_type>(current_vertex_outer_index + (i << 1)));
				index_list_.push_back(static_cast<index_type>(current_vertex_outer_index + (n << 1)));
				index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + (n << 1)));
			}
		}

		constexpr auto draw_rect_filled(
			const rect_type& rect,
			const color_type& color_left_top,
			const color_type& color_right_top,
			const color_type& color_left_bottom,
			const color_type& color_right_bottom
		) noexcept -> void
		{
			// two triangle without path
			constexpr auto vertex_count = 4;
			constexpr auto index_count = 6;
			vertex_list_.reserve(vertex_list_.size() + vertex_count);
			index_list_.reserve(index_list_.size() + index_count);

			command_list_.back().element_count += index_count;

			const auto& opaque_uv = shared_data_->get_texture_uv_of_white_pixel();

			const auto current_vertex_index = static_cast<index_type>(vertex_list_.size());

			vertex_list_.emplace_back(rect.left_top(), opaque_uv, color_left_top);
			vertex_list_.emplace_back(rect.right_top(), opaque_uv, color_right_top);
			vertex_list_.emplace_back(rect.right_bottom(), opaque_uv, color_right_bottom);
			vertex_list_.emplace_back(rect.left_bottom(), opaque_uv, color_left_bottom);

			index_list_.push_back(current_vertex_index + 0);
			index_list_.push_back(current_vertex_index + 1);
			index_list_.push_back(current_vertex_index + 2);
			index_list_.push_back(current_vertex_index + 0);
			index_list_.push_back(current_vertex_index + 2);
			index_list_.push_back(current_vertex_index + 3);
		}

		constexpr auto draw_text(
			const font_type& font,
			const float font_size,
			const point_type& p,
			const color_type& color,
			const std::u32string_view text,
			const float wrap_width
		) noexcept -> void
		{
			const auto new_texture = this_command_texture_id_ != font.texture_id;

			if (new_texture)
			{
				push_texture_id(font.texture_id);
			}

			const auto vertex_count = 4 * text.size();
			const auto index_count = 6 * text.size();
			vertex_list_.reserve(vertex_list_.size() + vertex_count);
			index_list_.reserve(index_list_.size() + index_count);

			command_list_.back().element_count += index_count;

			const float scale = font_size / font.pixel_height;

			auto it_input_current = text.begin();
			const auto it_input_end = text.end();

			auto cursor = p + point_type{0, font_size};

			while (it_input_current != it_input_end)
			{
				const auto c = *it_input_current;
				it_input_current += 1;

				if (c == U'\n' or (wrap_width > 0 and cursor.x - p.x > wrap_width))
				{
					cursor.x = p.x;
					cursor.y += font.pixel_height * scale;
					if (c == U'\n')
					{
						command_list_.back().element_count -= 6;
						continue;
					}
				}

				const auto& [glyph_rect, glyph_uv, glyph_advance_x] = [&]
				{
					if (const auto it = font.glyphs.find(c);
						it != font.glyphs.end())
					{
						return it->second;
					}

					return font.fallback_glyph;
				}();

				const auto advance_x = glyph_advance_x * scale;
				const rect_type char_rect{
						cursor + point_type{static_cast<point_type::value_type>(glyph_rect.left_top().x), -static_cast<point_type::value_type>(glyph_rect.left_top().y)} * scale,
						static_cast<rect_type::extent_type>(glyph_rect.size()) * scale
				};
				cursor.x += advance_x;

				const auto current_vertex_index = static_cast<index_type>(vertex_list_.size());

				vertex_list_.emplace_back(char_rect.left_top(), glyph_uv.left_top(), color);
				vertex_list_.emplace_back(char_rect.right_top(), glyph_uv.right_top(), color);
				vertex_list_.emplace_back(char_rect.right_bottom(), glyph_uv.right_bottom(), color);
				vertex_list_.emplace_back(char_rect.left_bottom(), glyph_uv.left_bottom(), color);

				index_list_.push_back(current_vertex_index + 0);
				index_list_.push_back(current_vertex_index + 1);
				index_list_.push_back(current_vertex_index + 2);
				index_list_.push_back(current_vertex_index + 0);
				index_list_.push_back(current_vertex_index + 2);
				index_list_.push_back(current_vertex_index + 3);
			}

			if (new_texture)
			{
				pop_texture_id();
			}
		}

		constexpr auto draw_image(
			const texture_id_type texture_id,
			const point_type& display_p1,
			const point_type& display_p2,
			const point_type& display_p3,
			const point_type& display_p4,
			const uv_type& uv_p1,
			const uv_type& uv_p2,
			const uv_type& uv_p3,
			const uv_type& uv_p4,
			const color_type& color
		) noexcept -> void
		{
			const auto new_texture = this_command_texture_id_ != texture_id;

			if (new_texture)
			{
				push_texture_id(texture_id);
			}

			// two triangle without path
			constexpr auto vertex_count = 4;
			constexpr auto index_count = 6;
			vertex_list_.reserve(vertex_list_.size() + vertex_count);
			index_list_.reserve(index_list_.size() + index_count);

			command_list_.back().element_count += index_count;

			const auto current_vertex_index = static_cast<index_type>(vertex_list_.size());

			vertex_list_.emplace_back(display_p1, uv_p1, color);
			vertex_list_.emplace_back(display_p2, uv_p2, color);
			vertex_list_.emplace_back(display_p3, uv_p3, color);
			vertex_list_.emplace_back(display_p4, uv_p4, color);

			index_list_.push_back(current_vertex_index + 0);
			index_list_.push_back(current_vertex_index + 1);
			index_list_.push_back(current_vertex_index + 2);
			index_list_.push_back(current_vertex_index + 0);
			index_list_.push_back(current_vertex_index + 2);
			index_list_.push_back(current_vertex_index + 3);

			if (new_texture)
			{
				pop_texture_id();
			}
		}

		constexpr auto draw_image_rounded(
			const texture_id_type texture_id,
			const rect_type& display_rect,
			const rect_type& uv_rect,
			const color_type& color,
			float rounding,
			DrawFlag flag
		) noexcept -> void
		{
			// @see `path_rect`
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(display_rect.valid() and not display_rect.empty());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(uv_rect.valid() and not uv_rect.empty());

			if (rounding >= .5f)
			{
				flag = draw_list_detail::to_fixed_rect_corner_flag(flag);

				const auto v = functional::contains<functional::EnumCheckPolicy::ALL_BITS, functional::EnumFoldPolicy::LOGICAL_OR>(flag, DrawFlag::ROUND_CORNER_TOP, DrawFlag::ROUND_CORNER_BOTTOM);
				const auto h = functional::contains<functional::EnumCheckPolicy::ALL_BITS, functional::EnumFoldPolicy::LOGICAL_OR>(flag, DrawFlag::ROUND_CORNER_LEFT, DrawFlag::ROUND_CORNER_RIGHT);

				rounding = std::ranges::min(rounding, display_rect.width() * (v ? .5f : 1.f) - 1.f);
				rounding = std::ranges::min(rounding, display_rect.height() * (h ? .5f : 1.f) - 1.f);
			}

			using functional::operators::operator&;
			if (rounding < .5f or (DrawFlag::ROUND_CORNER_MASK & flag) == DrawFlag::ROUND_CORNER_NONE)
			{
				draw_image(
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
			else
			{
				const auto new_texture = this_command_texture_id_ != texture_id;

				if (new_texture)
				{
					push_texture_id(texture_id);
				}

				const auto rounding_left_top = functional::contains<functional::EnumCheckPolicy::ANY_BIT>(flag, DrawFlag::ROUND_CORNER_LEFT_TOP) ? rounding : 0;
				const auto rounding_right_top = functional::contains<functional::EnumCheckPolicy::ANY_BIT>(flag, DrawFlag::ROUND_CORNER_RIGHT_TOP) ? rounding : 0;
				const auto rounding_left_bottom = functional::contains<functional::EnumCheckPolicy::ANY_BIT>(flag, DrawFlag::ROUND_CORNER_LEFT_BOTTOM) ? rounding : 0;
				const auto rounding_right_bottom = functional::contains<functional::EnumCheckPolicy::ANY_BIT>(flag, DrawFlag::ROUND_CORNER_RIGHT_BOTTOM) ? rounding : 0;

				path_arc_fast({display_rect.left_top() + point_type{rounding_left_top, rounding_left_top}, rounding_left_top}, ArcQuadrant::Q2_CLOCK_WISH);
				path_arc_fast({display_rect.right_top() + point_type{-rounding_right_top, rounding_right_top}, rounding_right_top}, ArcQuadrant::Q1_CLOCK_WISH);
				path_arc_fast({display_rect.right_bottom() + point_type{-rounding_right_bottom, -rounding_right_bottom}, rounding_right_bottom}, ArcQuadrant::Q4_CLOCK_WISH);
				path_arc_fast({display_rect.left_bottom() + point_type{rounding_left_bottom, -rounding_left_bottom}, rounding_left_bottom}, ArcQuadrant::Q3_CLOCK_WISH);

				const auto before_vertex_count = vertex_list_.size();
				path_stroke(color);
				const auto after_vertex_count = vertex_list_.size();

				const auto display_size = display_rect.size();
				const auto uv_size = uv_rect.size();
				const auto scale = uv_size / display_size;

				auto it = vertex_list_.begin() + static_cast<vertex_list_type::difference_type>(before_vertex_count);
				const auto end = vertex_list_.begin() + static_cast<vertex_list_type::difference_type>(after_vertex_count);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it < end);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(end == vertex_list_.end());

				// note: linear uv
				const auto uv_min = uv_rect.left_top();
				// const auto uv_max = uv_rect.right_bottom();
				while (it != end)
				{
					const auto v = uv_min + (it->position - display_rect.left_top()) * scale;

					it->uv =
					{
							// std::ranges::clamp(v.x, uv_min.x, uv_max.x),
							v.x,
							// std::ranges::clamp(v.y, uv_min.y, uv_max.y)
							v.y
					};
					it += 1;
				}

				if (new_texture)
				{
					pop_texture_id();
				}
			}
		}

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

		constexpr auto path_stroke(const color_type& color, const DrawFlag flag, const float thickness) noexcept -> void
		{
			if (functional::contains<functional::EnumCheckPolicy::ANY_BIT>(draw_list_flag_, DrawListFlag::ANTI_ALIASED_LINE))
			{
				draw_polygon_line_aa(color, flag, thickness);
			}
			else
			{
				draw_polygon_line(color, flag, thickness);
			}

			path_clear();
		}

		constexpr auto path_stroke(const color_type& color) noexcept -> void
		{
			if (functional::contains<functional::EnumCheckPolicy::ANY_BIT>(draw_list_flag_, DrawListFlag::ANTI_ALIASED_FILL))
			{
				draw_convex_polygon_line_filled_aa(color);
			}
			else
			{
				draw_convex_polygon_line_filled(color);
			}

			path_clear();
		}

		constexpr auto path_arc_fast(const circle_type& circle, const int from, const int to) noexcept -> void
		{
			const auto& [center, radius] = circle;

			if (radius < .5f)
			{
				path_pin(center);
				return;
			}

			// Calculate arc auto segment step size
			auto step = DrawListSharedData::vertex_sample_points_count / shared_data_->get_circle_auto_segment_count(radius);
			// Make sure we never do steps larger than one quarter of the circle
			step = std::clamp(step, static_cast<decltype(step)>(1), DrawListSharedData::vertex_sample_points_count / 4);

			const auto sample_range = functional::abs(to - from);
			const auto next_step = step;

			auto extra_max_sample = false;
			if (step > 1)
			{
				const auto overstep = sample_range % step;
				if (overstep > 0)
				{
					extra_max_sample = true;

					// When we have overstepped to avoid awkwardly looking one long line and one tiny one at the end,
					// distribute first step range evenly between them by reducing first step size.
					step -= (step - overstep) / 2;
				}

				path_reserve_extra(sample_range / step + 1 + (overstep > 0));
			}
			else
			{
				path_reserve_extra(sample_range + 1);
			}

			auto sample_index = from;
			if (sample_index < 0 or std::cmp_greater_equal(sample_index, DrawListSharedData::vertex_sample_points_count))
			{
				sample_index = sample_index % static_cast<int>(DrawListSharedData::vertex_sample_points_count);
				if (sample_index < 0)
				{
					sample_index += static_cast<int>(DrawListSharedData::vertex_sample_points_count);
				}
			}

			if (to >= from)
			{
				for (int i = from; i <= to; i += static_cast<int>(step), sample_index += static_cast<int>(step), step = next_step)
				{
					// a_step is clamped to vertex_sample_points_count, so we have guaranteed that it will not wrap over range twice or more
					if (std::cmp_greater_equal(sample_index, DrawListSharedData::vertex_sample_points_count))
					{
						sample_index -= static_cast<int>(DrawListSharedData::vertex_sample_points_count);
					}

					const auto& sample_point = draw_list_detail::vertex_sample_points[sample_index];

					path_pin({center + sample_point * radius});
				}
			}
			else
			{
				for (int i = from; i >= to; i -= static_cast<int>(step), sample_index -= static_cast<int>(step), step = next_step)
				{
					// a_step is clamped to vertex_sample_points_count, so we have guaranteed that it will not wrap over range twice or more
					if (sample_index < 0)
					{
						sample_index += static_cast<int>(DrawListSharedData::vertex_sample_points_count);
					}

					const auto& sample_point = draw_list_detail::vertex_sample_points[sample_index];

					path_pin({center + sample_point * radius});
				}
			}

			if (extra_max_sample)
			{
				auto normalized_max_sample_index = to % static_cast<int>(DrawListSharedData::vertex_sample_points_count);
				if (normalized_max_sample_index < 0)
				{
					normalized_max_sample_index += DrawListSharedData::vertex_sample_points_count;
				}

				const auto& sample_point = draw_list_detail::vertex_sample_points[normalized_max_sample_index];

				path_pin({center + sample_point * radius});
			}
		}

		constexpr auto path_arc_fast(const circle_type& circle, const ArcQuadrant quadrant) noexcept -> void
		{
			const auto [from, to] = draw_list_detail::range_of_quadrant(quadrant);

			return path_arc_fast(circle, from, to);
		}

		constexpr auto path_arc_n(const circle_type& circle, const float from, const float to, const std::uint32_t segments) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(to > from);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(from >= 0);

			const auto& [center, radius] = circle;

			if (radius < .5f)
			{
				path_pin(center);
				return;
			}

			path_reserve_extra(segments);
			for (std::uint32_t i = 0; i < segments; ++i)
			{
				const auto a = from + static_cast<float>(i) / static_cast<float>(segments) * (to - from);
				path_pin({center + point_type{functional::cos(a), functional::sin(a)} * radius});
			}
		}

		constexpr auto path_arc(const circle_type& circle, const float from, const float to) noexcept -> void
		{
			const auto& [center, radius] = circle;

			if (radius < .5f)
			{
				path_pin(center);
				return;
			}

			// Automatic segment count
			if (radius <= shared_data_->get_arc_fast_radius_cutoff())
			{
				const auto is_reversed = to < from;

				// We are going to use precomputed values for mid-samples.
				// Determine first and last sample in lookup table that belong to the arc
				const auto sample_from_f = DrawListSharedData::vertex_sample_points_count * from / (std::numbers::pi_v<float> * 2);
				const auto sample_to_f = DrawListSharedData::vertex_sample_points_count * to / (std::numbers::pi_v<float> * 2);

				const auto sample_from = is_reversed ? static_cast<int>(functional::floor(sample_from_f)) : static_cast<int>(functional::ceil(sample_from_f));
				const auto sample_to = is_reversed ? static_cast<int>(functional::ceil(sample_to_f)) : static_cast<int>(functional::floor(sample_to_f));
				const auto sample_mid = is_reversed ? static_cast<int>(std::ranges::max(sample_from - sample_to, 0)) : static_cast<int>(std::ranges::max(sample_to - sample_from, 0));

				const auto segment_from_angle = static_cast<float>(sample_from) * std::numbers::pi_v<float> * 2 / DrawListSharedData::vertex_sample_points_count;
				const auto segment_to_angle = static_cast<float>(sample_to) * std::numbers::pi_v<float> * 2 / DrawListSharedData::vertex_sample_points_count;

				const auto emit_start = functional::abs(segment_from_angle - from) >= 1e-5f;
				const auto emit_end = functional::abs(to - segment_to_angle) >= 1e-5f;

				if (emit_start)
				{
					// The quadrant must be the same, otherwise it is not continuous with the path drawn by `path_arc_fast`.
					path_pin({center + point_type{functional::cos(from), -functional::sin(from)} * radius});
				}
				if (sample_mid > 0)
				{
					path_arc_fast(circle, sample_from, sample_to);
				}
				if (emit_end)
				{
					// The quadrant must be the same, otherwise it is not continuous with the path drawn by `path_arc_fast`.
					path_pin({center + point_type{functional::cos(to), -functional::sin(to)} * radius});
				}
			}
			else
			{
				const auto arc_length = to - from;
				const auto circle_segment_count = shared_data_->get_circle_auto_segment_count(radius);
				const auto arc_segment_count = std::ranges::max(
					static_cast<unsigned>(functional::ceil(static_cast<float>(circle_segment_count) * arc_length / (std::numbers::pi_v<float> * 2))),
					static_cast<unsigned>(std::numbers::pi_v<float> * 2 / arc_length)
				);
				path_arc_n(circle, from, to, arc_segment_count);
			}
		}

		constexpr auto path_arc_elliptical_n(const ellipse_type& ellipse, const float from, const float to, const std::uint32_t segments) noexcept -> void
		{
			const auto& [center, radius, rotation] = ellipse;
			const auto cos_theta = functional::cos(rotation);
			const auto sin_theta = functional::sin(rotation);

			path_reserve_extra(segments);
			for (std::uint32_t i = 0; i < segments; ++i)
			{
				const auto a = from + static_cast<float>(i) / static_cast<float>(segments) * (to - from);
				const auto offset = point_type{functional::cos(a), functional::sin(a)} * radius;
				const auto prime_x = offset.x * cos_theta - offset.y * sin_theta;
				const auto prime_y = offset.x * sin_theta + offset.y * cos_theta;
				path_pin({center + point_type{prime_x, prime_y}});
			}
		}

		constexpr auto path_quadrilateral(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4) noexcept -> void
		{
			path_pin(p1);
			path_pin(p2);
			path_pin(p3);
			path_pin(p4);
		}

		constexpr auto path_rect(const rect_type& rect, float rounding, DrawFlag flag) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(rect.valid() and not rect.empty());

			if (rounding >= .5f)
			{
				flag = draw_list_detail::to_fixed_rect_corner_flag(flag);

				const auto v = functional::contains<functional::EnumCheckPolicy::ALL_BITS, functional::EnumFoldPolicy::LOGICAL_OR>(flag, DrawFlag::ROUND_CORNER_TOP, DrawFlag::ROUND_CORNER_BOTTOM);
				const auto h = functional::contains<functional::EnumCheckPolicy::ALL_BITS, functional::EnumFoldPolicy::LOGICAL_OR>(flag, DrawFlag::ROUND_CORNER_LEFT, DrawFlag::ROUND_CORNER_RIGHT);

				rounding = std::ranges::min(rounding, rect.width() * (v ? .5f : 1.f) - 1.f);
				rounding = std::ranges::min(rounding, rect.height() * (h ? .5f : 1.f) - 1.f);
			}

			using functional::operators::operator&;
			if (rounding < .5f or (DrawFlag::ROUND_CORNER_MASK & flag) == DrawFlag::ROUND_CORNER_NONE)
			{
				path_quadrilateral(rect.left_top(), rect.right_top(), rect.right_bottom(), rect.left_bottom());
			}
			else
			{
				const auto rounding_left_top = functional::contains<functional::EnumCheckPolicy::ANY_BIT>(flag, DrawFlag::ROUND_CORNER_LEFT_TOP) ? rounding : 0;
				const auto rounding_right_top = functional::contains<functional::EnumCheckPolicy::ANY_BIT>(flag, DrawFlag::ROUND_CORNER_RIGHT_TOP) ? rounding : 0;
				const auto rounding_left_bottom = functional::contains<functional::EnumCheckPolicy::ANY_BIT>(flag, DrawFlag::ROUND_CORNER_LEFT_BOTTOM) ? rounding : 0;
				const auto rounding_right_bottom = functional::contains<functional::EnumCheckPolicy::ANY_BIT>(flag, DrawFlag::ROUND_CORNER_RIGHT_BOTTOM) ? rounding : 0;

				path_arc_fast({rect.left_top() + point_type{rounding_left_top, rounding_left_top}, rounding_left_top}, ArcQuadrant::Q2_CLOCK_WISH);
				path_arc_fast({rect.right_top() + point_type{-rounding_right_top, rounding_right_top}, rounding_right_top}, ArcQuadrant::Q1_CLOCK_WISH);
				path_arc_fast({rect.right_bottom() + point_type{-rounding_right_bottom, -rounding_right_bottom}, rounding_right_bottom}, ArcQuadrant::Q4_CLOCK_WISH);
				path_arc_fast({rect.left_bottom() + point_type{rounding_left_bottom, -rounding_left_bottom}, rounding_left_bottom}, ArcQuadrant::Q3_CLOCK_WISH);
			}
		}

		// fixme
		constexpr static std::size_t bezier_curve_casteljau_max_level = 10;

		constexpr auto path_bezier_cubic_curve_casteljau(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			const float tessellation_tolerance,
			const std::size_t level
		) noexcept -> void
		{
			const auto dx = p4.x - p1.x;
			const auto dy = p4.y - p1.y;
			const auto d2 = functional::abs((p2.x - p4.x) * dy - (p2.y - p4.y) * dx);
			const auto d3 = functional::abs((p3.x - p4.x) * dy - (p3.y - p4.y) * dx);

			if (functional::pow(d2 + d3, 2) < tessellation_tolerance * (functional::pow(dx, 2) + functional::pow(dy, 2)))
			{
				path_pin(p4);
			}
			else if (level < bezier_curve_casteljau_max_level)
			{
				const auto p_12 = (p1 + p2) * .5f;
				const auto p_23 = (p2 + p3) * .5f;
				const auto p_34 = (p3 + p4) * .5f;
				const auto p_123 = (p_12 + p_23) * .5f;
				const auto p_234 = (p_23 + p_34) * .5f;
				const auto p_1234 = (p_123 + p_234) * .5f;

				path_bezier_cubic_curve_casteljau(p1, p_12, p_123, p_1234, tessellation_tolerance, level + 1);
				path_bezier_cubic_curve_casteljau(p_1234, p_234, p_34, p4, tessellation_tolerance, level + 1);
			}
		}

		constexpr auto path_bezier_quadratic_curve_casteljau(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const float tessellation_tolerance,
			const std::size_t level
		) noexcept -> void
		{
			const auto dx = p3.x - p1.x;
			const auto dy = p3.y - p1.y;
			const auto det = (p2.x - p3.x) * dy - (p2.y - p3.y) * dx;

			if (functional::pow(det, 2) * 4.f < tessellation_tolerance * (functional::pow(dx, 2) + functional::pow(dy, 2)))
			{
				path_pin(p3);
			}
			else if (level < bezier_curve_casteljau_max_level)
			{
				const auto p_12 = (p1 + p2) * .5f;
				const auto p_23 = (p2 + p3) * .5f;
				const auto p_123 = (p_12 + p_23) * .5f;

				path_bezier_quadratic_curve_casteljau(p1, p_12, p_123, tessellation_tolerance, level + 1);
				path_bezier_quadratic_curve_casteljau(p_123, p_23, p3, tessellation_tolerance, level + 1);
			}
		}

		constexpr auto path_bezier_curve(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			const std::uint32_t segments
		) noexcept -> void
		{
			path_pin(p1);
			if (segments == 0)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_->get_curve_tessellation_tolerance() > 0);

				path_reserve_extra(bezier_curve_casteljau_max_level * 2);
				// auto-tessellated
				path_bezier_cubic_curve_casteljau(p1, p2, p3, p4, shared_data_->get_curve_tessellation_tolerance(), 0);
			}
			else
			{
				path_reserve_extra(segments);
				const auto step = 1.f / static_cast<float>(segments);
				for (std::uint32_t i = 1; i <= segments; ++i)
				{
					path_pin(draw_list_detail::bezier_cubic_calc(p1, p2, p3, p4, step * static_cast<float>(i)));
				}
			}
		}

		constexpr auto path_bezier_quadratic_curve(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const std::uint32_t segments
		) noexcept -> void
		{
			path_pin(p1);
			if (segments == 0)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_->get_curve_tessellation_tolerance() > 0);

				path_reserve_extra(bezier_curve_casteljau_max_level * 2);
				// auto-tessellated
				path_bezier_quadratic_curve_casteljau(p1, p2, p3, shared_data_->get_curve_tessellation_tolerance(), 0);
			}
			else
			{
				path_reserve_extra(segments);
				const auto step = 1.f / static_cast<float>(segments);
				for (std::uint32_t i = 1; i <= segments; ++i)
				{
					path_pin(draw_list_detail::bezier_quadratic_calc(p1, p2, p3, step * static_cast<float>(i)));
				}
			}
		}

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
			this_command_texture_id_ = shared_data_->get_default_font().texture_id;

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

		#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
		constexpr auto bind_debug_info() noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(debug_range_info_list_.size() == debug_list_info_list_.size());

			for (size_type i = 0; i < debug_list_info_list_.size(); ++i)
			{
				auto& [range_vertices, range_indices] = debug_range_info_list_[i];
				auto& [what, list_vertices, list_indices] = debug_list_info_list_[i];
				list_vertices = {vertex_list_.begin() + range_vertices.first, vertex_list_.begin() + range_vertices.second};
				list_indices = {index_list_.begin() + range_indices.first, index_list_.begin() + range_indices.second};
			}
		}
		#endif

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

			path_pin(from + point_type{.5f, .5f});
			path_pin(to + point_type{.5f, .5f});

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

			path_rect(rect_type{rect.left_top() + point_type{.5f, .5f}, rect.right_bottom() - point_type{.5f, .5f}}, rounding, flag);

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
				const auto clamped_segments = std::ranges::clamp(segments, draw_list_detail::circle_segments_min, draw_list_detail::circle_segments_max);

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
				const auto clamped_segments = std::ranges::clamp(segments, draw_list_detail::circle_segments_min, draw_list_detail::circle_segments_max);

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
			const font_type& font,
			const float font_size,
			const point_type& p,
			const color_type& color,
			const std::u32string_view text,
			const float wrap_width = .0f
		) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data_ != nullptr);

			if (color.alpha == 0) { return; }

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			const auto current_vertex_size = vertex_list_.size();
			const auto current_index_size = index_list_.size();
			#endif

			draw_text(font, font_size, p, color, text, wrap_width);

			#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
			debug_range_info_list_.emplace_back(
				debug_vertex_range_type{current_vertex_size, vertex_list_.size()},
				debug_index_range_type{current_index_size, index_list_.size()}
			);
			const auto utf8_text = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF8_CHAR>(text);
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
			const std::u32string_view text,
			const float wrap_width = .0f
		) noexcept -> void
		{
			this->text(shared_data_->get_default_font(), font_size, p, color, text, wrap_width);
		}

		constexpr auto text(
			const font_type& font,
			const float font_size,
			const point_type& p,
			const color_type& color,
			const std::string_view text,
			const float wrap_width = .0f
		) noexcept -> void
		{
			const auto utf32_text = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32>(text);
			this->text(font, font_size, p, color, utf32_text, wrap_width);
		}

		constexpr auto text(
			const float font_size,
			const point_type& p,
			const color_type& color,
			const std::string_view text,
			const float wrap_width = .0f
		) noexcept -> void
		{
			this->text(shared_data_->get_default_font(), font_size, p, color, text, wrap_width);
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

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
