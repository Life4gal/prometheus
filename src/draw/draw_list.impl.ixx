// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

export module gal.prometheus:draw.draw_list.impl;

import std;

import :functional;
import :primitive;
import :chars;
#if GAL_PROMETHEUS_COMPILER_DEBUG
import :error;
#endif

import :draw.draw_list;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#include <vector>
#include <type_traits>
#include <utility>
#include <numbers>
#include <span>
#include <algorithm>
#include <iterator>

#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

#include <prometheus/macro.hpp>
#include <functional/functional.ixx>
#include <primitive/primitive.ixx>
#include <chars/chars.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <draw/draw_list.ixx>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

namespace
{
	using namespace gal::prometheus;

	[[nodiscard]] constexpr auto to_fixed_rect_corner_flag(const draw::DrawFlag flag) noexcept -> draw::DrawFlag
	{
		if (functional::exclude(flag, draw::DrawFlag::ROUND_CORNER_MASK))
		{
			using functional::operators::operator|;
			return draw::DrawFlag::ROUND_CORNER_ALL | flag;
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

	// fixme
	constexpr static std::size_t bezier_curve_casteljau_max_level = 10;

	using point_type = draw::DrawListSharedData::point_type;

	constexpr auto bezier_cubic_calc = [](const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4, const float tolerance) noexcept -> point_type
	{
		const auto u = 1.f - tolerance;

		const auto w1 = functional::pow(u, 3);
		const auto w2 = 3 * functional::pow(u, 2) * tolerance;
		const auto w3 = 3 * u * functional::pow(tolerance, 2);
		const auto w4 = functional::pow(tolerance, 3);

		return
		{
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

		return
		{
				p1.x * w1 + p2.x * w2 + p3.x * w3,
				p1.y * w1 + p2.y * w2 + p3.y * w3
		};
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
{
	auto DrawList::draw_polygon_line(const color_type& color, const DrawFlag draw_flag, const float thickness) noexcept -> void
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

			const auto current_vertex_index = static_cast<index_type>(vertex_list_.size());
			const auto& opaque_uv = shared_data_->get_default_font().white_pixel_uv();

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

	auto DrawList::draw_polygon_line_aa(const color_type& color, const DrawFlag draw_flag, float thickness) noexcept -> void
	{
		const auto path_point_count = path_list_.size();
		const auto& path_point = path_list_;

		if (path_point_count < 2 or color.alpha == 0)
		{
			return;
		}

		const auto& opaque_uv = shared_data_->get_default_font().white_pixel_uv();
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
			(thickness_integer < shared_data_->get_default_font().baked_line_max_width()) and
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

			// The width of the geometry we need to draw - this is essentially <thickness> pixels for the line itself, plus "one pixel" for AA
			const auto half_draw_size = is_use_texture ? ((thickness * .5f) + 1.f) : 1.f;

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
				auto [dm_x, dm_y] = to_fixed_normal(d.x, d.y);
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
				GAL_PROMETHEUS_ERROR_ASSUME(not shared_data_->get_default_font().baked_line_uv().empty(), "draw::FontAtlasFlag::NO_BAKED_LINE");

				const auto& uv = shared_data_->get_default_font().baked_line_uv()[thickness_integer];

				const auto uv0 = uv.left_top();
				const auto uv1 = uv.right_bottom();
				for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
				{
					// left-side outer edge
					vertex_list_.emplace_back(temp_buffer_points[i * 2 + 0], uv0, color);
					// right-side outer edge
					vertex_list_.emplace_back(temp_buffer_points[i * 2 + 1], uv1, color);
				}
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
				const auto [dm_x, dm_y] = to_fixed_normal(d.x, d.y);
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

	auto DrawList::draw_convex_polygon_line_filled(const color_type& color) noexcept -> void
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
		const auto& opaque_uv = shared_data_->get_default_font().white_pixel_uv();

		std::ranges::transform(path_point, std::back_inserter(vertex_list_), [opaque_uv, color](const point_type& point) noexcept -> vertex_type { return {point, opaque_uv, color}; });
		for (index_type i = 2; std::cmp_less(i, path_point_count); ++i)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_index + i - 1 >= current_vertex_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_index + i >= current_vertex_index);

			index_list_.push_back(static_cast<index_type>(current_vertex_index + 0));
			index_list_.push_back(static_cast<index_type>(current_vertex_index + i - 1));
			index_list_.push_back(static_cast<index_type>(current_vertex_index + i));
		}
	}

	auto DrawList::draw_convex_polygon_line_filled_aa(const color_type& color) noexcept -> void
	{
		const auto path_point_count = path_list_.size();
		const auto& path_point = path_list_;

		if (path_point_count < 3 or color.alpha == 0)
		{
			return;
		}

		const auto& opaque_uv = shared_data_->get_default_font().white_pixel_uv();
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
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING(6297)
			#endif

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + ((i - 1) << 1) >= current_vertex_inner_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + (i << 1) >= current_vertex_inner_index);

			index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + 0));
			index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + ((i - 1) << 1)));
			index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + (i << 1)));

			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_POP
			#endif
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
			auto [dm_x, dm_y] = to_fixed_normal(d.x, d.y);
			dm_x *= .5f;
			dm_y *= .5f;

			// inner
			vertex_list_.emplace_back(path_point[n] - point_type{dm_x, dm_y}, opaque_uv, color);
			// outer
			vertex_list_.emplace_back(path_point[n] + point_type{dm_x, dm_y}, opaque_uv, transparent_color);

			// Add indexes for fringes
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + (n << 1) >= current_vertex_inner_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + (i << 1) >= current_vertex_inner_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + (i << 1) >= current_vertex_outer_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + (i << 1) >= current_vertex_outer_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + (n << 1) >= current_vertex_outer_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + (n << 1) >= current_vertex_inner_index);

			index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + (n << 1)));
			index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + (i << 1)));
			index_list_.push_back(static_cast<index_type>(current_vertex_outer_index + (i << 1)));
			index_list_.push_back(static_cast<index_type>(current_vertex_outer_index + (i << 1)));
			index_list_.push_back(static_cast<index_type>(current_vertex_outer_index + (n << 1)));
			index_list_.push_back(static_cast<index_type>(current_vertex_inner_index + (n << 1)));
		}
	}

	auto DrawList::draw_rect_filled(
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

		const auto& opaque_uv = shared_data_->get_default_font().white_pixel_uv();

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

	auto DrawList::draw_text(
		const Font& font,
		const float font_size,
		const point_type& p,
		const color_type& color,
		const std::string_view utf8_text,
		const float wrap_width
	) noexcept -> void
	{
		const auto new_texture = this_command_texture_id_ != font.texture_id();

		if (new_texture)
		{
			push_texture_id(font.texture_id());
		}

		const auto utf32_text = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32>(utf8_text);

		const auto vertex_count = 4 * utf32_text.size();
		const auto index_count = 6 * utf32_text.size();
		vertex_list_.reserve(vertex_list_.size() + vertex_count);
		index_list_.reserve(index_list_.size() + index_count);

		command_list_.back().element_count += index_count;

		const float scale = font_size / font.pixel_height();
		const auto& glyphs = font.glyphs();
		const auto& fallback_glyph = font.fallback_glyph();

		auto it_input_current = utf32_text.begin();
		const auto it_input_end = utf32_text.end();

		auto cursor = p + point_type{0, font_size};

		while (it_input_current != it_input_end)
		{
			const auto c = *it_input_current;
			it_input_current += 1;

			if (c == U'\n' or (wrap_width > 0 and cursor.x - p.x > wrap_width))
			{
				cursor.x = p.x;
				cursor.y += font.pixel_height() * scale;
				if (c == U'\n')
				{
					command_list_.back().element_count -= 6;
					continue;
				}
			}

			const auto& [glyph_rect, glyph_uv, glyph_advance_x] = [&]() -> const auto& {
				if (const auto it = glyphs.find(c);
					it != glyphs.end())
				{
					return it->second;
				}

				return fallback_glyph;
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

	auto DrawList::draw_image(
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

	auto DrawList::draw_image_rounded(
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
			flag = to_fixed_rect_corner_flag(flag);

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

	auto DrawList::path_stroke(const color_type& color, const DrawFlag flag, const float thickness) noexcept -> void
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

	auto DrawList::path_stroke(const color_type& color) noexcept -> void
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

	auto DrawList::path_arc_fast(const circle_type& circle, const int from, const int to) noexcept -> void
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

				const auto& sample_point = DrawListSharedData::vertex_sample_points[sample_index];

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

				const auto& sample_point = DrawListSharedData::vertex_sample_points[sample_index];

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

			const auto& sample_point = DrawListSharedData::vertex_sample_points[normalized_max_sample_index];

			path_pin({center + sample_point * radius});
		}
	}

	auto DrawList::path_arc_fast(const circle_type& circle, const ArcQuadrant quadrant) noexcept -> void
	{
		const auto [from, to] = range_of_arc_quadrant(quadrant);

		return path_arc_fast(circle, from, to);
	}

	auto DrawList::path_arc_n(const circle_type& circle, const float from, const float to, const std::uint32_t segments) noexcept -> void
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

	auto DrawList::path_arc(const circle_type& circle, const float from, const float to) noexcept -> void
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

	auto DrawList::path_arc_elliptical_n(const ellipse_type& ellipse, const float from, const float to, const std::uint32_t segments) noexcept -> void
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

	auto DrawList::path_quadrilateral(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4) noexcept -> void
	{
		path_pin(p1);
		path_pin(p2);
		path_pin(p3);
		path_pin(p4);
	}

	auto DrawList::path_rect(const rect_type& rect, float rounding, DrawFlag flag) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(rect.valid() and not rect.empty());

		if (rounding >= .5f)
		{
			flag = to_fixed_rect_corner_flag(flag);

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

	auto DrawList::path_bezier_cubic_curve_casteljau(
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

	auto DrawList::path_bezier_quadratic_curve_casteljau(const point_type& p1, const point_type& p2, const point_type& p3, const float tessellation_tolerance, const std::size_t level) noexcept -> void
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

	auto DrawList::path_bezier_curve(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4, const std::uint32_t segments) noexcept -> void
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
				path_pin(bezier_cubic_calc(p1, p2, p3, p4, step * static_cast<float>(i)));
			}
		}
	}

	auto DrawList::path_bezier_quadratic_curve(const point_type& p1, const point_type& p2, const point_type& p3, const std::uint32_t segments) noexcept -> void
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
				path_pin(bezier_quadratic_calc(p1, p2, p3, step * static_cast<float>(i)));
			}
		}
	}
}
