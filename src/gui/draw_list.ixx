// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:draw_list;

import std;
import gal.prometheus.functional;
import gal.prometheus.primitive;

#else
#pragma once

#include <vector>
#include <type_traits>
#include <utility>
#include <limits>
#include <numbers>

#include <prometheus/macro.hpp>
#include <functional/functional.ixx>
#include <primitive/primitive.ixx>

#endif

namespace gal::prometheus::gui
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	enum class ArcQuadrant : unsigned
	{
		// [0~3)
		Q1 = 0x0001,
		// [3~6)
		Q2 = 0x0010,
		// [6~9)
		Q3 = 0x0100,
		// [9~12)
		Q4 = 0x1000,

		RIGHT_TOP = Q1,
		LEFT_TOP = Q2,
		LEFT_BOTTOM = Q3,
		RIGHT_BOTTOM = Q4,
		TOP = Q1 | Q2,
		BOTTOM = Q3 | Q4,
		LEFT = Q2 | Q3,
		RIGHT = Q1 | Q4,
		ALL = Q1 | Q2 | Q3 | Q4,
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace draw_list_detail
	{
		template<typename> struct detect_vertex_uv
		{
			using type = void;
			using value_type = void;
		};

		template<typename PositionType, typename UvType, typename ColorType>
			requires(not std::is_same_v<UvType, void>)
		struct detect_vertex_uv<primitive::basic_vertex<PositionType, UvType, ColorType>>
		{
			using type = typename primitive::basic_vertex<PositionType, UvType, ColorType>::uv_type;
			using value_type = typename primitive::basic_vertex<PositionType, UvType, ColorType>::uv_value_type;
		};

		constexpr auto draw_list_texture_line_max_width{63};

		// @see https://stackoverflow.com/a/2244088/15194693
		// Number of segments (N) is calculated using equation:
		//	N = ceil ( pi / acos(1 - error / r) ) where r > 0 and error <= r
		constexpr auto circle_segments_min = 4;
		constexpr auto circle_segments_max = 512;
		constexpr auto circle_roundup_to_even = [](const auto v) noexcept -> auto { return (v + 1) / 2 * 2; };
		constexpr auto circle_segments_maker = [](const float radius, const float max_error) noexcept -> auto
		{
			return std::ranges::clamp(
				circle_roundup_to_even(static_cast<int>(std::ceil(std::numbers::pi_v<float> / std::acos(1 - std::ranges::min(radius, max_error) / radius)))),
				circle_segments_min,
				circle_segments_max
			);
		};
		constexpr auto circle_segments_maker_radius = [](const std::size_t n, const float max_error) noexcept -> auto
		{
			return max_error / (1 - std::cos(std::numbers::pi_v<float> / std::ranges::max(static_cast<float>(n), std::numbers::pi_v<float>)));
		};
		constexpr auto circle_segments_maker_error = [](const std::size_t n, const float radius) noexcept -> auto
		{
			return (1 - std::cos(std::numbers::pi_v<float> / std::ranges::max(static_cast<float>(n), std::numbers::pi_v<float>))) / radius;
		};

		using circle_segment_counts_type = std::array<std::uint8_t, 64>;
		using vertex_sample_points_type = std::array<primitive::basic_point<float, 2>, 48>;
		constexpr auto vertex_sample_point_maker = []<std::size_t... Index>(std::index_sequence<Index...>) noexcept //
		{
			const auto make_point = []<std::size_t I>() noexcept //
			{
				const auto a = static_cast<float>(I) / static_cast<float>(vertex_sample_points_type{}.size()) * 2 * std::numbers::pi_v<float>;
				return primitive::basic_point<float, 2>{functional::cos(a), -functional::sin(a)};
			};

			return vertex_sample_points_type{{make_point.template operator()<Index>()...}};
		};
		constexpr auto vertex_sample_points{vertex_sample_point_maker.operator()(std::make_index_sequence<vertex_sample_points_type{}.size()>{})};

		constexpr auto range_of_quadrant = [](const ArcQuadrant quadrant) noexcept -> auto
		{
			constexpr auto factor = vertex_sample_points.size() / 12;

			if (quadrant == ArcQuadrant::Q1) { return std::make_pair(0 * factor, 3 * factor); }
			if (quadrant == ArcQuadrant::Q2) { return std::make_pair(3 * factor, 6 * factor); }
			if (quadrant == ArcQuadrant::Q3) { return std::make_pair(6 * factor, 9 * factor); }
			if (quadrant == ArcQuadrant::Q4) { return std::make_pair(9 * factor, 12 * factor); }

			std::unreachable();
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	enum class DrawFlag: std::uint32_t
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
		ROUND_CORNERS_LEFT_TOP = 1 << 1,
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

		ROUND_CORNER_LEFT = ROUND_CORNERS_LEFT_TOP | ROUND_CORNER_LEFT_BOTTOM,
		ROUND_CORNER_TOP = ROUND_CORNERS_LEFT_TOP | ROUND_CORNER_RIGHT_TOP,
		ROUND_CORNER_RIGHT = ROUND_CORNER_RIGHT_TOP | ROUND_CORNER_RIGHT_BOTTOM,
		ROUND_CORNER_BOTTOM = ROUND_CORNER_LEFT_BOTTOM | ROUND_CORNER_RIGHT_BOTTOM,

		ROUND_CORNER_ALL = ROUND_CORNERS_LEFT_TOP | ROUND_CORNER_RIGHT_TOP | ROUND_CORNER_LEFT_BOTTOM | ROUND_CORNER_RIGHT_BOTTOM,
		ROUND_CORNER_DEFAULT = ROUND_CORNER_ALL,
		ROUND_CORNER_MASK = ROUND_CORNER_ALL | ROUND_CORNER_NONE,
	};

	enum class DrawListFlag : std::uint32_t
	{
		NONE = 0,
		ANTI_ALIASED_LINE,
		ANTI_ALIASED_LINE_USE_TEXTURE,
		ANTI_ALIASED_FILL,
	};

	template<
		primitive::basic_vertex_t VertexType,
		typename IndexType,
		template<typename> typename ContainerType = std::vector
	>
		requires std::is_arithmetic_v<IndexType>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_draw_list
	{
		using vertex_type = VertexType;
		using index_type = IndexType;

		using position_type = typename vertex_type::position_type;
		using uv_type = typename draw_list_detail::detect_vertex_uv<vertex_type>::type;
		using color_type = typename vertex_type::color_type;

		using position_value_type = typename vertex_type::position_value_type;
		using uv_value_type = typename draw_list_detail::detect_vertex_uv<vertex_type>::value_type;
		using color_value_type = typename vertex_type::color_value_type;

		using vertex_list_type = ContainerType<vertex_type>;
		using index_list_type = ContainerType<index_type>;

		using vertex_list_iterator = typename vertex_list_type::iterator;
		using index_list_iterator = typename index_list_type::iterator;

		using circle_type = primitive::basic_circle<position_value_type, std::tuple_size_v<position_type>>;
		using rect_type = primitive::basic_rect<position_value_type, std::tuple_size_v<position_type>>;

		constexpr static auto vertex_has_uv = not std::is_same_v<uv_type, void> and not std::is_same_v<uv_value_type, void>;

		vertex_list_type vertex_list;
		index_list_type index_list;

		DrawListFlag draw_list_flag;

	private:
		using path_list_type = ContainerType<position_type>;
		using circle_segment_counts_type = draw_list_detail::circle_segment_counts_type;
		using vertex_sample_points_type = draw_list_detail::vertex_sample_points_type;

		constexpr static auto vertex_sample_points_count = vertex_sample_points_type{}.size();

		circle_segment_counts_type circle_segment_counts_;
		// Maximum error (in pixels) allowed when using `circle`/`circle_filled` or drawing rounded corner rectangles with no explicit segment count specified.
		// Decrease for higher quality but more geometry.
		float circle_segment_max_error_;
		// Cutoff radius after which arc drawing will fall back to slower `path_arc`
		float arc_fast_radius_cutoff_;

		path_list_type path_list_;

		constexpr auto get_circle_auto_segment_count(const float radius) const noexcept -> auto
		{
			// ceil to never reduce accuracy
			if (const auto radius_index = static_cast<std::ptrdiff_t>(radius + .999999f);
				radius_index >= 0 and radius_index < circle_segment_counts_.size())
			{
				return circle_segment_counts_[radius_index];
			}
			return static_cast<circle_segment_counts_type::value_type>(draw_list_detail::circle_segments_maker(radius, circle_segment_max_error_));
		}

		constexpr auto draw_polygon_line(const color_type& color, const DrawFlag draw_flag, const float thickness) noexcept -> void
		{
			const auto path_point_count = path_list_.size();
			const auto& path_point = path_list_;

			if (path_point_count < 2 or color.alpha == 0)
			{
				return;
			}

			const auto draw_flag_value = std::to_underlying(draw_flag);
			const auto is_closed = (draw_flag_value & std::to_underlying(DrawFlag::CLOSED)) != 0;
			const auto segments_count = is_closed ? path_point_count : path_point_count - 1;

			const auto vertex_count = segments_count * 4;
			const auto index_count = segments_count * 6;
			vertex_list.reserve(vertex_list.size() + vertex_count);
			index_list.reserve(index_list.size() + index_count);

			for (std::decay_t<decltype(segments_count)> i = 0; i < segments_count; ++i)
			{
				const auto n = (i + 1) % path_point_count;

				const auto& p1 = path_point[i];
				const auto& p2 = path_point[n];

				auto normalized = functional::normalize(p2.x - p1.x, p2.y - p1.y);
				normalized.first *= (thickness * .5f);
				normalized.second *= (thickness * .5f);

				const auto current_vertex_index = vertex_list.size();

				if constexpr (vertex_has_uv)
				{
					// todo
					constexpr auto opaque_uv = vertex_type::default_uv;

					vertex_list.emplace_back(p1 + position_type{normalized.second, -normalized.first}, opaque_uv, color);
					vertex_list.emplace_back(p2 + position_type{normalized.second, -normalized.first}, opaque_uv, color);
					vertex_list.emplace_back(p2 + position_type{-normalized.second, normalized.first}, opaque_uv, color);
					vertex_list.emplace_back(p1 + position_type{-normalized.second, normalized.first}, opaque_uv, color);
				}
				else
				{
					vertex_list.emplace_back(p1 + position_type{normalized.second, -normalized.first}, color);
					vertex_list.emplace_back(p2 + position_type{normalized.second, -normalized.first}, color);
					vertex_list.emplace_back(p2 + position_type{-normalized.second, normalized.first}, color);
					vertex_list.emplace_back(p1 + position_type{-normalized.second, normalized.first}, color);
				}

				index_list.push_back(current_vertex_index + 0);
				index_list.push_back(current_vertex_index + 1);
				index_list.push_back(current_vertex_index + 2);
				index_list.push_back(current_vertex_index + 0);
				index_list.push_back(current_vertex_index + 2);
				index_list.push_back(current_vertex_index + 3);
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

			// todo
			constexpr auto opaque_uv = vertex_type::default_uv;
			const auto transparent_color = color.transparent();

			const auto draw_list_flag_value = std::to_underlying(draw_list_flag);
			const auto draw_flag_value = std::to_underlying(draw_flag);
			const auto is_closed = (draw_flag_value & std::to_underlying(DrawFlag::CLOSED)) != 0;
			const auto segments_count = is_closed ? path_point_count : path_point_count - 1;
			const auto is_thick_line = thickness > 1.f;

			thickness = std::ranges::max(thickness, 1.f);
			const auto thickness_integer = static_cast<int>(thickness);
			const auto thickness_fractional = thickness - static_cast<float>(thickness_integer);

			const auto is_use_texture =
			(draw_list_flag_value & std::to_underlying(DrawListFlag::ANTI_ALIASED_LINE_USE_TEXTURE) and
			 (thickness_integer < draw_list_detail::draw_list_texture_line_max_width) and
			 (thickness_fractional <= .00001f));

			const auto vertex_cont = is_use_texture ? (path_point_count * 2) : (is_thick_line ? path_point_count * 4 : path_point_count * 3);
			const auto index_count = is_use_texture ? (segments_count * 6) : (is_thick_line ? segments_count * 18 : segments_count * 12);
			vertex_list.reserve(vertex_list.size() + vertex_cont);
			index_list.reserve(index_list.size() + index_count);

			// The first <path_point_count> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
			ContainerType<position_type> temp_buffer{};
			temp_buffer.resize(path_point_count * ((is_use_texture or not is_thick_line) ? 3 : 5));
			auto temp_buffer_normals = std::span{temp_buffer.begin(), path_point_count};
			auto temp_buffer_points = std::span{temp_buffer.begin() + path_point_count, temp_buffer.end()};

			// Calculate normals (tangents) for each line segment
			for (std::decay_t<decltype(segments_count)> i = 0; i < segments_count; ++i)
			{
				const auto n = (i + 1) % path_point_count;
				const auto d = path_point[n] - path_point[i];
				const auto normalized = functional::normalize(d.x, d.y);
				temp_buffer_normals[i].x = normalized.first;
				temp_buffer_normals[i].y = -normalized.second;
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

				const auto current_vertex_index = vertex_list.size();

				// Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
				// This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
				auto vertex_index_for_start = current_vertex_index;
				for (std::decay_t<decltype(segments_count)> first_point_of_segment = 0; first_point_of_segment < segments_count; ++first_point_of_segment)
				{
					const auto second_point_of_segment = (first_point_of_segment + 1) % path_point_count;
					const auto vertex_index_for_end = (first_point_of_segment + 1) == path_point_count ? current_vertex_index : (vertex_index_for_start + (is_use_texture ? 2 : 3));

					// Average normals
					const auto d = (temp_buffer_normals[first_point_of_segment] + temp_buffer_normals[second_point_of_segment]) * .5f;
					auto normalized = functional::normalize(d.x, d.y);
					normalized.first *= half_draw_size;
					normalized.second *= half_draw_size;

					// Add temporary vertexes for the outer edges
					temp_buffer_points[second_point_of_segment * 2 + 0] = path_point[second_point_of_segment] + position_type{normalized.first, normalized.second};
					temp_buffer_points[second_point_of_segment * 2 + 1] = path_point[second_point_of_segment] - position_type{normalized.first, normalized.second};

					if (is_use_texture)
					{
						// Add indices for two triangles

						// right
						index_list.push_back(vertex_index_for_end + 0);
						index_list.push_back(vertex_index_for_start + 0);
						index_list.push_back(vertex_index_for_start + 1);
						// left
						index_list.push_back(vertex_index_for_end + 1);
						index_list.push_back(vertex_index_for_start + 1);
						index_list.push_back(vertex_index_for_end + 0);
					}
					else
					{
						// Add indexes for four triangles

						// right 1
						index_list.push_back(vertex_index_for_end + 0);
						index_list.push_back(vertex_index_for_start + 0);
						index_list.push_back(vertex_index_for_start + 2);
						// right 2
						index_list.push_back(vertex_index_for_start + 2);
						index_list.push_back(vertex_index_for_end + 2);
						index_list.push_back(vertex_index_for_end + 0);
						// left 1
						index_list.push_back(vertex_index_for_end + 1);
						index_list.push_back(vertex_index_for_start + 1);
						index_list.push_back(vertex_index_for_start + 0);
						// left 2
						index_list.push_back(vertex_index_for_start + 0);
						index_list.push_back(vertex_index_for_end + 0);
						index_list.push_back(vertex_index_for_end + 1);
					}

					vertex_index_for_start = vertex_index_for_end;
				}

				// Add vertexes for each point on the line
				if (is_use_texture)
				{
					// todo: get the texture
					GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED();
				}
				else
				{
					// If we're not using a texture, we need the center vertex as well
					for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
					{
						// center of line
						vertex_list.emplace_back(path_point[i], opaque_uv, color);
						// left-side outer edge
						vertex_list.emplace_back(temp_buffer_points[i * 2 + 0], opaque_uv, transparent_color);
						// right-side outer edge
						vertex_list.emplace_back(temp_buffer_points[i * 2 + 1], opaque_uv, transparent_color);
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

				const auto current_vertex_index = vertex_list.size();

				// Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
				// This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
				auto vertex_index_for_start = current_vertex_index;
				for (std::decay_t<decltype(segments_count)> first_point_of_segment = 0; first_point_of_segment < segments_count; ++first_point_of_segment)
				{
					const auto second_point_of_segment = (first_point_of_segment + 1) % path_point_count;
					const auto vertex_index_for_end = (first_point_of_segment + 1) == path_point_count ? current_vertex_index : (vertex_index_for_start + 4);

					// Average normals
					const auto d = (temp_buffer_normals[first_point_of_segment] + temp_buffer_normals[second_point_of_segment]) * .5f;
					auto normalized = functional::normalize(d.x, d.y);

					const auto normalized_out_x = normalized.first * (half_inner_thickness + 1.f);
					const auto normalized_out_y = normalized.second * (half_inner_thickness + 1.f);
					const auto normalized_in_x = normalized.first * (half_inner_thickness + 0.f);
					const auto normalized_in_y = normalized.second * (half_inner_thickness + 0.f);

					// Add temporary vertices
					temp_buffer_points[second_point_of_segment * 4 + 0] = path_point[second_point_of_segment] + position_type{normalized_out_x, normalized_out_y};
					temp_buffer_points[second_point_of_segment * 4 + 1] = path_point[second_point_of_segment] + position_type{normalized_in_x, normalized_in_y};
					temp_buffer_points[second_point_of_segment * 4 + 2] = path_point[second_point_of_segment] - position_type{normalized_in_x, normalized_in_y};
					temp_buffer_points[second_point_of_segment * 4 + 3] = path_point[second_point_of_segment] - position_type{normalized_out_x, normalized_out_y};

					// Add indexes
					index_list.push_back(vertex_index_for_end + 1);
					index_list.push_back(vertex_index_for_end + 1);
					index_list.push_back(vertex_index_for_start + 2);

					index_list.push_back(vertex_index_for_start + 2);
					index_list.push_back(vertex_index_for_end + 2);
					index_list.push_back(vertex_index_for_end + 1);

					index_list.push_back(vertex_index_for_end + 1);
					index_list.push_back(vertex_index_for_start + 1);
					index_list.push_back(vertex_index_for_start + 0);

					index_list.push_back(vertex_index_for_start + 0);
					index_list.push_back(vertex_index_for_end + 0);
					index_list.push_back(vertex_index_for_end + 1);

					index_list.push_back(vertex_index_for_end + 2);
					index_list.push_back(vertex_index_for_start + 2);
					index_list.push_back(vertex_index_for_start + 3);

					index_list.push_back(vertex_index_for_start + 3);
					index_list.push_back(vertex_index_for_end + 3);
					index_list.push_back(vertex_index_for_end + 2);

					vertex_index_for_start = vertex_index_for_end;
				}

				// Add vertices
				for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
				{
					vertex_list.emplace_back(temp_buffer_points[i * 4 + 0], opaque_uv, transparent_color);
					vertex_list.emplace_back(temp_buffer_points[i * 4 + 1], opaque_uv, color);
					vertex_list.emplace_back(temp_buffer_points[i * 4 + 2], opaque_uv, color);
					vertex_list.emplace_back(temp_buffer_points[i * 4 + 2], opaque_uv, transparent_color);
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

		constexpr auto path_pin(const position_type& point) noexcept -> void
		{
			path_list_.push_back(point);
		}

		constexpr auto path_pin_merge_duplicate(const position_type& point) noexcept -> void
		{
			if (path_list_.empty() or path_list_.back() != point)
			{
				path_list_.emplace_back(point);
			}
		}

		constexpr auto path_stroke(const color_type& color, const DrawFlag flag = DrawFlag::NONE, const float thickness = 1.f) noexcept -> void
		{
			if constexpr (vertex_has_uv)
			{
				const auto draw_list_flag_value = std::to_underlying(draw_list_flag);

				if (const auto is_aa = draw_list_flag_value & std::to_underlying(DrawListFlag::ANTI_ALIASED_LINE);
					is_aa)
				{
					draw_polygon_line_aa(color, flag, thickness);
				}
				else
				{
					draw_polygon_line(color, flag, thickness);
				}
			}
			else
			{
				draw_polygon_line(color, flag, thickness);
			}

			draw_polygon_line(color, flag, thickness);
			path_clear();
		}

		// Use precomputed angles for a 12 steps circle
		constexpr auto path_arc_fast(const circle_type& circle, const unsigned from, const unsigned to) noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_AXIOM(to > from);

			const auto& [center, radius] = circle;

			if (radius < static_cast<position_value_type>(.5f))
			{
				path_pin(center);
				return;
			}

			// Calculate arc auto segment step size
			auto step = vertex_sample_points_count / get_circle_auto_segment_count(radius);
			// Make sure we never do steps larger than one quarter of the circle
			step = std::clamp(step, static_cast<decltype(step)>(1), vertex_sample_points_count / 4);

			const auto sample_range = to - from;
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

			for (auto i = from; i <= to; i += step, step = next_step)
			{
				const auto& sample_point = draw_list_detail::vertex_sample_points[i % vertex_sample_points_count];

				path_pin({center + sample_point * radius});
			}

			if (extra_max_sample)
			{
				const auto& sample_point = draw_list_detail::vertex_sample_points[to % vertex_sample_points_count];

				path_pin({center + sample_point * radius});
			}
		}

		// Use precomputed angles for a 12 steps circle
		constexpr auto path_arc_fast(const circle_type& circle, const ArcQuadrant quadrant) noexcept -> void
		{
			const auto [from, to] = draw_list_detail::range_of_quadrant(quadrant);

			return path_arc_fast(circle, from, to);
		}

		constexpr auto path_arc_n(const circle_type& circle, const float from, const float to, const int segments) noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_AXIOM(to > from);
			GAL_PROMETHEUS_DEBUG_AXIOM(from >= 0);
			GAL_PROMETHEUS_DEBUG_AXIOM(segments > 0);

			const auto& [center, radius] = circle;

			if (radius < static_cast<position_value_type>(.5f))
			{
				path_pin(center);
				return;
			}

			path_reserve_extra(segments + 1);
			for (int i = 0; i < segments; ++i)
			{
				const auto a = from + static_cast<float>(i) / static_cast<float>(segments) * (to - from);
				path_pin({center + position_type{functional::cos(a), functional::sin(a)} * radius});
			}
		}

		constexpr auto path_arc(const circle_type& circle, const float from, const float to) noexcept -> void
		{
			const auto& [center, radius] = circle;

			if (radius < static_cast<position_value_type>(.5f))
			{
				path_pin(center);
				return;
			}

			// Automatic segment count
			if (radius <= arc_fast_radius_cutoff_)
			{
				const auto is_reversed = to < from;

				// We are going to use precomputed values for mid-samples.
				// Determine first and last sample in lookup table that belong to the arc
				const auto sample_from_f = vertex_sample_points_count * from / (std::numbers::pi_v<float> * 2);
				const auto sample_to_f = vertex_sample_points_count * to / (std::numbers::pi_v<float> * 2);

				const auto sample_from = is_reversed ? static_cast<int>(functional::floor(sample_from_f)) : static_cast<int>(functional::ceil(sample_from_f));
				const auto sample_to = is_reversed ? static_cast<int>(functional::ceil(sample_to_f)) : static_cast<int>(functional::floor(sample_to_f));
				const auto sample_mid = is_reversed ? static_cast<int>(std::ranges::max(sample_from - sample_to, 0)) : static_cast<int>(std::ranges::max(sample_to - sample_from, 0));

				const auto segment_from_angle = static_cast<float>(sample_from) * std::numbers::pi_v<float> * 2 / vertex_sample_points_count;
				const auto segment_to_angle = static_cast<float>(sample_to) * std::numbers::pi_v<float> * 2 / vertex_sample_points_count;

				const auto emit_start = functional::abs(segment_from_angle - from) >= 1e-5f;
				const auto emit_end = functional::abs(to - segment_to_angle) >= 1e-5f;

				if (emit_start)
				{
					// The quadrant must be the same, otherwise it is not continuous with the path drawn by `path_arc_fast`.
					path_pin({center + position_type{functional::cos(from), -functional::sin(from)} * radius});
				}
				if (sample_mid > 0)
				{
					path_arc_fast(circle, sample_from, sample_to);
				}
				if (emit_end)
				{
					// The quadrant must be the same, otherwise it is not continuous with the path drawn by `path_arc_fast`.
					path_pin({center + position_type{functional::cos(to), -functional::sin(to)} * radius});
				}
			}
			else
			{
				const auto arc_length = to - from;
				const auto circle_segment_count = get_circle_auto_segment_count(radius);
				const auto arc_segment_count = std::ranges::max(
					static_cast<unsigned>(functional::ceil(static_cast<float>(circle_segment_count) * arc_length / (std::numbers::pi_v<float> * 2))),
					static_cast<unsigned>(std::numbers::pi_v<float> * 2 / arc_length)
				);
				path_arc_n(circle, from, to, arc_segment_count);
			}
		}

	public:
		constexpr basic_draw_list() noexcept
			: draw_list_flag{DrawListFlag::NONE},
			  circle_segment_counts_{},
			  circle_segment_max_error_{},
			  arc_fast_radius_cutoff_{}
		{
			set_circle_tessellation_max_error(.3f);
		}

		constexpr auto set_circle_tessellation_max_error(const float max_error) noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_AXIOM(max_error > .0f);

			if (circle_segment_max_error_ == max_error) // NOLINT(clang-diagnostic-float-equal)
			{
				return;
			}

			for (decltype(circle_segment_counts_.size()) i = 0; i < circle_segment_counts_.size(); ++i)
			{
				const auto radius = static_cast<float>(i);
				circle_segment_counts_[i] = static_cast<std::uint8_t>(draw_list_detail::circle_segments_maker(radius, max_error));
			}
			circle_segment_max_error_ = max_error;
			arc_fast_radius_cutoff_ = draw_list_detail::circle_segments_maker_radius(vertex_sample_points_count, max_error);
		}

		constexpr auto line(const position_type& from, const position_type& to, const color_type& color) noexcept -> void
		{
			if (color.alpha == 0)
			{
				return;
			}

			path_pin(from + position_type{.5f, .5f});
			path_pin(to + position_type{.5f, .5f});
			path_stroke(color);
		}
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
