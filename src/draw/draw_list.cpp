// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
#include <draw/draw_list.hpp>

#include <functional/enumeration.hpp>
#include <math/cmath.hpp>

#include <draw/font.hpp>
#include <draw/shared_data.hpp>
#include <draw/context.hpp>

namespace
{
	using namespace gal::prometheus;

	[[nodiscard]] constexpr auto to_fixed_rect_corner_flag(const draw::DrawFlag flag) noexcept -> draw::DrawFlag
	{
		using enum draw::DrawFlag;

		if ((flag & ROUND_CORNER_MASK) == NONE)
		{
			return ROUND_CORNER_ALL | flag;
		}

		return flag;
	}

	[[nodiscard]] constexpr auto to_fixed_normal(const float x, const float y) noexcept -> std::pair<float, float>
	{
		if (const auto d = math::pow(x, 2) + math::pow(y, 2);
			d > 1e-6f)
		{
			// fixme
			const auto inv_len = [d]
			{
				// #if defined(__AVX512F__)
				// __m512 d_v = _mm512_set1_ps(d);
				// __m512 inv_len_v = _mm512_rcp14_ps(d_v);
				// return _mm512_cvtss_f32(inv_len_v);
				// #elif defined(__AVX__)
				// __m256 d_v = _mm256_set1_ps(d);
				// __m256 inv_len_v = _mm256_rcp_ps(d_v);
				// return _mm256_cvtss_f32(inv_len_v);
				// #elif defined(__SSE4_1__) or defined(__SSE3__) or defined(__SSE__)
				// __m128 d_v = _mm_set_ss(d);
				// __m128 inv_len_v = _mm_rcp_ss(d_v);
				// return _mm_cvtss_f32(inv_len_v);
				// #else
				return 1.0f / d;
				// #endif
			}();

			return {x * inv_len, y * inv_len};
		}

		return {x, y};
	}

	// fixme
	constexpr std::size_t bezier_curve_casteljau_max_level = 10;

	using point_type = draw::DrawList::point_type;

	constexpr auto bezier_cubic_calc = [](const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4, const float tolerance) noexcept -> point_type
	{
		const auto u = 1.f - tolerance;

		const auto w1 = math::pow(u, 3);
		const auto w2 = 3 * math::pow(u, 2) * tolerance;
		const auto w3 = 3 * u * math::pow(tolerance, 2);
		const auto w4 = math::pow(tolerance, 3);

		return
		{
				p1.x * w1 + p2.x * w2 + p3.x * w3 + p4.x * w4,
				p1.y * w1 + p2.y * w2 + p3.y * w3 + p4.y * w4
		};
	};

	constexpr auto bezier_quadratic_calc = [](const point_type& p1, const point_type& p2, const point_type& p3, const float tolerance) noexcept -> point_type
	{
		const auto u = 1.f - tolerance;

		const auto w1 = math::pow(u, 2);
		const auto w2 = 2 * u * tolerance;
		const auto w3 = math::pow(tolerance, 2);

		return
		{
				p1.x * w1 + p2.x * w2 + p3.x * w3,
				p1.y * w1 + p2.y * w2 + p3.y * w3
		};
	};
}

namespace gal::prometheus::draw
{
	auto DrawList::make_accessor() noexcept -> DrawListDef::Accessor
	{
		auto& [command_list, vertex_list, index_list] = private_data_;

		return
		{
				command_list.back(),
				vertex_list,
				index_list
		};
	}

	auto DrawList::push_command() noexcept -> void
	{
		// fixme: If the window boundary is smaller than the rect boundary, the rect will no longer be valid.
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not this_command_clip_rect_.empty() and this_command_clip_rect_.valid());

		auto& [command_list, vertex_list, index_list] = private_data_;

		command_list.emplace_back(
			command_type
			{
					.clip_rect = this_command_clip_rect_,
					.texture_id = this_command_texture_id_,
					.index_offset = index_list.size(),
					// set by subsequent draw_xxx
					.element_count = 0
			}
		);
	}

	auto DrawList::on_element_changed(const ChangedElement element) noexcept -> void
	{
		auto& [command_list, vertex_list, index_list] = private_data_;

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not command_list.empty());

		const auto command_count = command_list.size();
		const auto& [current_clip_rect, current_texture_id, current_index_offset, current_element_count] = command_list.back();

		if (current_element_count != 0)
		{
			if (element == ChangedElement::CLIP_RECT)
			{
				if (current_clip_rect != this_command_clip_rect_)
				{
					push_command();

					return;
				}
			}
			else if (element == ChangedElement::TEXTURE_ID)
			{
				if (current_texture_id != this_command_texture_id_)
				{
					push_command();

					return;
				}
			}
			else
			{
				GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();
			}
		}

		// try to merge with previous command if it matches, else use current command
		if (command_count > 1)
		{
			if (
				const auto& [previous_clip_rect, previous_texture, previous_index_offset, previous_element_count] = command_list[command_count - 2];
				current_element_count == 0 and
				(
					this_command_clip_rect_ == previous_clip_rect and
					this_command_texture_id_ == previous_texture
				) and
				// sequential
				current_index_offset == previous_index_offset + previous_element_count
			)
			{
				command_list.pop_back();
				return;
			}
		}

		if (element == ChangedElement::CLIP_RECT)
		{
			command_list.back().clip_rect = this_command_clip_rect_;
		}
		else if (element == ChangedElement::TEXTURE_ID)
		{
			command_list.back().texture_id = this_command_texture_id_;
		}
		else
		{
			GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();
		}
	}

	auto DrawList::draw_polygon_line(const color_type& color, const DrawFlag draw_flag, const float thickness) noexcept -> void
	{
		const auto& font = Context::instance().font();

		const auto accessor = make_accessor();

		const auto path_point_count = path_list_.size();
		const auto& path_point = path_list_;

		if (path_point_count < 2 or color.alpha == 0)
		{
			return;
		}

		const auto is_closed = (draw_flag & DrawFlag::CLOSED) != DrawFlag::NONE;
		const auto segments_count = is_closed ? path_point_count : path_point_count - 1;

		const auto vertex_count = segments_count * 4;
		const auto index_count = segments_count * 6;
		accessor.reserve(vertex_count, index_count);

		for (std::decay_t<decltype(segments_count)> i = 0; i < segments_count; ++i)
		{
			const auto n = (i + 1) % path_point_count;

			const auto& p1 = path_point[i];
			const auto& p2 = path_point[n];

			auto [normalized_x, normalized_y] = math::normalize(p2.x - p1.x, p2.y - p1.y);
			normalized_x *= (thickness * .5f);
			normalized_y *= (thickness * .5f);

			const auto current_vertex_index = static_cast<index_type>(accessor.size());
			const auto& opaque_uv = font.white_pixel_uv();

			accessor.add_vertex(p1 + point_type{normalized_y, -normalized_x}, opaque_uv, color);
			accessor.add_vertex(p2 + point_type{normalized_y, -normalized_x}, opaque_uv, color);
			accessor.add_vertex(p2 + point_type{-normalized_y, normalized_x}, opaque_uv, color);
			accessor.add_vertex(p1 + point_type{-normalized_y, normalized_x}, opaque_uv, color);

			accessor.add_index(current_vertex_index + 0, current_vertex_index + 1, current_vertex_index + 2);
			accessor.add_index(current_vertex_index + 0, current_vertex_index + 2, current_vertex_index + 3);
		}
	}

	auto DrawList::draw_polygon_line_aa(const color_type& color, const DrawFlag draw_flag, float thickness) noexcept -> void
	{
		const auto& font = Context::instance().font();

		const auto accessor = make_accessor();

		const auto path_point_count = path_list_.size();
		const auto& path_point = path_list_;

		if (path_point_count < 2 or color.alpha == 0)
		{
			return;
		}

		const auto& opaque_uv = font.white_pixel_uv();
		const auto transparent_color = color.transparent();

		const auto is_closed = (draw_flag & DrawFlag::CLOSED) != DrawFlag::NONE;
		const auto segments_count = is_closed ? path_point_count : path_point_count - 1;
		const auto is_thick_line = thickness > 1.f;

		thickness = std::ranges::max(thickness, 1.f);
		const auto thickness_integer = static_cast<std::uint32_t>(thickness);
		const auto thickness_fractional = thickness - static_cast<float>(thickness_integer);

		const auto is_use_texture =
		(
			(draw_list_flag_ & DrawListFlag::ANTI_ALIASED_LINE_USE_TEXTURE) == DrawListFlag::ANTI_ALIASED_LINE_USE_TEXTURE and
			(thickness_integer < font.baked_line_max_width()) and
			(thickness_fractional <= .00001f));

		const auto vertex_cont = is_use_texture ? (path_point_count * 2) : (is_thick_line ? path_point_count * 4 : path_point_count * 3);
		const auto index_count = is_use_texture ? (segments_count * 6) : (is_thick_line ? segments_count * 18 : segments_count * 12);
		accessor.reserve(vertex_cont, index_count);

		// The first <path_point_count> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
		container_type<point_type> temp_buffer{};
		temp_buffer.resize(path_point_count * ((is_use_texture or not is_thick_line) ? 3 : 5));
		auto temp_buffer_normals = std::span{temp_buffer.begin(), path_point_count};
		auto temp_buffer_points = std::span{temp_buffer.begin() + static_cast<std::ptrdiff_t>(path_point_count), temp_buffer.end()};

		// Calculate normals (tangents) for each line segment
		for (std::decay_t<decltype(segments_count)> i = 0; i < segments_count; ++i)
		{
			const auto n = (i + 1) % path_point_count;
			const auto d = path_point[n] - path_point[i];

			const auto [normalized_x, normalized_y] = math::normalize(d.x, d.y);
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

			const auto current_vertex_index = static_cast<index_type>(accessor.size());

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
					accessor.add_index(vertex_index_for_end + 0, vertex_index_for_start + 0, vertex_index_for_start + 1);
					// left
					accessor.add_index(vertex_index_for_end + 1, vertex_index_for_start + 1, vertex_index_for_end + 0);
				}
				else
				{
					// Add indexes for four triangles

					// right 1
					accessor.add_index(vertex_index_for_end + 0, vertex_index_for_start + 0, vertex_index_for_start + 2);
					// right 2
					accessor.add_index(vertex_index_for_start + 2, vertex_index_for_end + 2, vertex_index_for_end + 0);
					// left 1
					accessor.add_index(vertex_index_for_end + 1, vertex_index_for_start + 1, vertex_index_for_start + 0);
					// left 2
					accessor.add_index(vertex_index_for_start + 0, vertex_index_for_end + 0, vertex_index_for_end + 1);
				}

				vertex_index_for_start = vertex_index_for_end;
			}

			// Add vertexes for each point on the line
			if (is_use_texture)
			{
				GAL_PROMETHEUS_ERROR_ASSUME(not font.baked_line_uv().empty(), "draw::FontAtlasFlag::NO_BAKED_LINE");

				const auto& uv = font.baked_line_uv()[thickness_integer];

				const auto uv0 = uv.left_top();
				const auto uv1 = uv.right_bottom();
				for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
				{
					// left-side outer edge
					accessor.add_vertex(temp_buffer_points[i * 2 + 0], uv0, color);
					// right-side outer edge
					accessor.add_vertex(temp_buffer_points[i * 2 + 1], uv1, color);
				}
			}
			else
			{
				// If we're not using a texture, we need the center vertex as well
				for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
				{
					// center of line
					accessor.add_vertex(path_point[i], opaque_uv, color);
					// left-side outer edge
					accessor.add_vertex(temp_buffer_points[i * 2 + 0], opaque_uv, transparent_color);
					// right-side outer edge
					accessor.add_vertex(temp_buffer_points[i * 2 + 1], opaque_uv, transparent_color);
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

			const auto current_vertex_index = static_cast<index_type>(accessor.size());

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
				accessor.add_index(vertex_index_for_end + 1, vertex_index_for_end + 1, vertex_index_for_start + 2);
				accessor.add_index(vertex_index_for_start + 2, vertex_index_for_end + 2, vertex_index_for_end + 1);
				accessor.add_index(vertex_index_for_end + 1, vertex_index_for_start + 1, vertex_index_for_start + 0);
				accessor.add_index(vertex_index_for_start + 0, vertex_index_for_end + 0, vertex_index_for_end + 1);
				accessor.add_index(vertex_index_for_end + 2, vertex_index_for_start + 2, vertex_index_for_start + 3);
				accessor.add_index(vertex_index_for_start + 3, vertex_index_for_end + 3, vertex_index_for_end + 2);

				vertex_index_for_start = vertex_index_for_end;
			}

			// Add vertices
			for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
			{
				accessor.add_vertex(temp_buffer_points[i * 4 + 0], opaque_uv, transparent_color);
				accessor.add_vertex(temp_buffer_points[i * 4 + 1], opaque_uv, color);
				accessor.add_vertex(temp_buffer_points[i * 4 + 2], opaque_uv, color);
				accessor.add_vertex(temp_buffer_points[i * 4 + 2], opaque_uv, transparent_color);
			}
		}
	}

	auto DrawList::draw_convex_polygon_line_filled(const color_type& color) noexcept -> void
	{
		const auto& font = Context::instance().font();

		const auto accessor = make_accessor();

		const auto path_point_count = path_list_.size();
		const auto& path_point = path_list_;

		if (path_point_count < 3 or color.alpha == 0)
		{
			return;
		}

		const auto vertex_count = path_point_count;
		const auto index_count = (path_point_count - 2) * 3;
		accessor.reserve(vertex_count, index_count);

		const auto current_vertex_index = static_cast<index_type>(accessor.size());
		const auto& opaque_uv = font.white_pixel_uv();

		std::ranges::for_each(
			path_point,
			[&](const point_type& point) noexcept -> void
			{
				accessor.add_vertex(point, opaque_uv, color);
			}
		);
		for (index_type i = 2; std::cmp_less(i, path_point_count); ++i)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_index + i - 1 >= current_vertex_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_index + i >= current_vertex_index);

			accessor.add_index(current_vertex_index + 0, current_vertex_index + i - 1, current_vertex_index + i);
		}
	}

	auto DrawList::draw_convex_polygon_line_filled_aa(const color_type& color) noexcept -> void
	{
		const auto& font = Context::instance().font();

		const auto accessor = make_accessor();

		const auto path_point_count = path_list_.size();
		const auto& path_point = path_list_;

		if (path_point_count < 3 or color.alpha == 0)
		{
			return;
		}

		const auto& opaque_uv = font.white_pixel_uv();
		const auto transparent_color = color.transparent();

		const auto vertex_count = path_point_count * 2;
		const auto index_count = (path_point_count - 2) * 3 + path_point_count * 6;
		accessor.reserve(vertex_count, index_count);

		const auto current_vertex_inner_index = static_cast<index_type>(accessor.size());
		const auto current_vertex_outer_index = static_cast<index_type>(accessor.size() + 1);

		// Add indexes for fill
		for (index_type i = 2; std::cmp_less(i, path_point_count); ++i)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>((i - 1) << 1) >= current_vertex_inner_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>(i << 1) >= current_vertex_inner_index);

			accessor.add_index(
				current_vertex_inner_index + 0,
				current_vertex_inner_index + static_cast<index_type>((i - 1) << 1),
				current_vertex_inner_index + static_cast<index_type>(i << 1)
			);
		}

		container_type<point_type> temp_buffer{};
		temp_buffer.resize(path_point_count);
		auto temp_buffer_normals = std::span{temp_buffer.begin(), path_point_count};

		for (auto i = path_point_count - 1, n = static_cast<decltype(i)>(0); n < path_point_count; i = n++)
		{
			const auto d = path_point[n] - path_point[i];

			const auto [normalized_x, normalized_y] = math::normalize(d.x, d.y);
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
			accessor.add_vertex(path_point[n] - point_type{dm_x, dm_y}, opaque_uv, color);
			// outer
			accessor.add_vertex(path_point[n] + point_type{dm_x, dm_y}, opaque_uv, transparent_color);

			// Add indexes for fringes
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>(n << 1) >= current_vertex_inner_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>(i << 1) >= current_vertex_inner_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + static_cast<index_type>(i << 1) >= current_vertex_outer_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + static_cast<index_type>(i << 1) >= current_vertex_outer_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + static_cast<index_type>(n << 1) >= current_vertex_outer_index);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>(n << 1) >= current_vertex_inner_index);

			accessor.add_index(
				current_vertex_inner_index + static_cast<index_type>(n << 1),
				current_vertex_inner_index + static_cast<index_type>(i << 1),
				current_vertex_outer_index + static_cast<index_type>(i << 1)
			);
			accessor.add_index(
				current_vertex_outer_index + static_cast<index_type>(i << 1),
				current_vertex_outer_index + static_cast<index_type>(n << 1),
				current_vertex_inner_index + static_cast<index_type>(n << 1)
			);
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
		const auto& font = Context::instance().font();

		const auto accessor = make_accessor();

		// two triangle without path
		constexpr auto vertex_count = 4;
		constexpr auto index_count = 6;
		accessor.reserve(vertex_count, index_count);

		const auto& opaque_uv = font.white_pixel_uv();

		const auto current_vertex_index = static_cast<index_type>(accessor.size());

		accessor.add_vertex(rect.left_top(), opaque_uv, color_left_top);
		accessor.add_vertex(rect.right_top(), opaque_uv, color_right_top);
		accessor.add_vertex(rect.right_bottom(), opaque_uv, color_right_bottom);
		accessor.add_vertex(rect.left_bottom(), opaque_uv, color_left_bottom);

		accessor.add_index(current_vertex_index + 0, current_vertex_index + 1, current_vertex_index + 2);
		accessor.add_index(current_vertex_index + 0, current_vertex_index + 2, current_vertex_index + 3);
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

		font.text_draw(utf8_text, font_size, wrap_width, p, color, make_accessor());

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

		const auto accessor = make_accessor();

		// two triangle without path
		constexpr auto vertex_count = 4;
		constexpr auto index_count = 6;
		accessor.reserve(vertex_count, index_count);

		const auto current_vertex_index = static_cast<index_type>(accessor.size());

		accessor.add_vertex(display_p1, uv_p1, color);
		accessor.add_vertex(display_p2, uv_p2, color);
		accessor.add_vertex(display_p3, uv_p3, color);
		accessor.add_vertex(display_p4, uv_p4, color);

		accessor.add_index(current_vertex_index + 0, current_vertex_index + 1, current_vertex_index + 2);
		accessor.add_index(current_vertex_index + 0, current_vertex_index + 2, current_vertex_index + 3);

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

			const auto v =
					(flag & DrawFlag::ROUND_CORNER_TOP) == DrawFlag::ROUND_CORNER_TOP or
					(flag & DrawFlag::ROUND_CORNER_BOTTOM) == DrawFlag::ROUND_CORNER_BOTTOM;
			const auto h =
					(flag & DrawFlag::ROUND_CORNER_LEFT) == DrawFlag::ROUND_CORNER_LEFT or
					(flag & DrawFlag::ROUND_CORNER_RIGHT) == DrawFlag::ROUND_CORNER_RIGHT;

			rounding = std::ranges::min(rounding, display_rect.width() * (v ? .5f : 1.f) - 1.f);
			rounding = std::ranges::min(rounding, display_rect.height() * (h ? .5f : 1.f) - 1.f);
		}

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

			const auto rounding_left_top = (flag & DrawFlag::ROUND_CORNER_LEFT_TOP) != DrawFlag::NONE ? rounding : 0;
			const auto rounding_right_top = (flag & DrawFlag::ROUND_CORNER_RIGHT_TOP) != DrawFlag::NONE ? rounding : 0;
			const auto rounding_left_bottom = (flag & DrawFlag::ROUND_CORNER_LEFT_BOTTOM) != DrawFlag::NONE ? rounding : 0;
			const auto rounding_right_bottom = (flag & DrawFlag::ROUND_CORNER_RIGHT_BOTTOM) != DrawFlag::NONE ? rounding : 0;

			path_arc_fast({display_rect.left_top() + point_type{rounding_left_top, rounding_left_top}, rounding_left_top}, DrawArcFlag::Q2_CLOCK_WISH);
			path_arc_fast({display_rect.right_top() + point_type{-rounding_right_top, rounding_right_top}, rounding_right_top}, DrawArcFlag::Q1_CLOCK_WISH);
			path_arc_fast({display_rect.right_bottom() + point_type{-rounding_right_bottom, -rounding_right_bottom}, rounding_right_bottom}, DrawArcFlag::Q4_CLOCK_WISH);
			path_arc_fast({display_rect.left_bottom() + point_type{rounding_left_bottom, -rounding_left_bottom}, rounding_left_bottom}, DrawArcFlag::Q3_CLOCK_WISH);

			auto& vertex_list = private_data_.vertex_list;

			const auto before_vertex_count = vertex_list.size();
			// draw
			path_stroke(color);
			const auto after_vertex_count = vertex_list.size();

			// set uv manually

			const auto display_size = display_rect.size();
			const auto uv_size = uv_rect.size();
			const auto scale = uv_size / display_size;

			auto it = vertex_list.begin() + static_cast<vertex_list_type::difference_type>(before_vertex_count);
			const auto end = vertex_list.begin() + static_cast<vertex_list_type::difference_type>(after_vertex_count);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it < end);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(end == vertex_list.end());

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
		if ((draw_list_flag_ & DrawListFlag::ANTI_ALIASED_LINE) != DrawListFlag::NONE)
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
		if ((draw_list_flag_ & DrawListFlag::ANTI_ALIASED_FILL) != DrawListFlag::NONE)
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
		const auto& draw_list_shared_data = Context::instance().draw_list_shared_data();

		const auto& [center, radius] = circle;

		if (radius < .5f)
		{
			path_pin(center);
			return;
		}

		// Calculate arc auto segment step size
		auto step = DrawListSharedData::vertex_sample_points_count / draw_list_shared_data.get_circle_auto_segment_count(radius);
		// Make sure we never do steps larger than one quarter of the circle
		step = std::clamp(step, static_cast<decltype(step)>(1), DrawListSharedData::vertex_sample_points_count / 4);

		const auto sample_range = math::abs(to - from);
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

				const auto& sample_point = draw_list_shared_data.get_vertex_sample_point(sample_index);

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

				const auto& sample_point = draw_list_shared_data.get_vertex_sample_point(sample_index);

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

			const auto& sample_point = draw_list_shared_data.get_vertex_sample_point(normalized_max_sample_index);

			path_pin({center + sample_point * radius});
		}
	}

	auto DrawList::path_arc_fast(const circle_type& circle, const DrawArcFlag flag) noexcept -> void
	{
		const auto [from, to] = range_of_arc_quadrant(flag);

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
			path_pin({center + point_type{math::cos(a), math::sin(a)} * radius});
		}
	}

	auto DrawList::path_arc(const circle_type& circle, const float from, const float to) noexcept -> void
	{
		const auto& draw_list_shared_data = Context::instance().draw_list_shared_data();

		const auto& [center, radius] = circle;

		if (radius < .5f)
		{
			path_pin(center);
			return;
		}

		// Automatic segment count
		if (radius <= draw_list_shared_data.get_arc_fast_radius_cutoff())
		{
			const auto is_reversed = to < from;

			// We are going to use precomputed values for mid-samples.
			// Determine first and last sample in lookup table that belong to the arc
			const auto sample_from_f = DrawListSharedData::vertex_sample_points_count * from / (std::numbers::pi_v<float> * 2);
			const auto sample_to_f = DrawListSharedData::vertex_sample_points_count * to / (std::numbers::pi_v<float> * 2);

			const auto sample_from = is_reversed ? static_cast<int>(math::floor(sample_from_f)) : static_cast<int>(math::ceil(sample_from_f));
			const auto sample_to = is_reversed ? static_cast<int>(math::ceil(sample_to_f)) : static_cast<int>(math::floor(sample_to_f));
			const auto sample_mid = is_reversed ? static_cast<int>(std::ranges::max(sample_from - sample_to, 0)) : static_cast<int>(std::ranges::max(sample_to - sample_from, 0));

			const auto segment_from_angle = static_cast<float>(sample_from) * std::numbers::pi_v<float> * 2 / DrawListSharedData::vertex_sample_points_count;
			const auto segment_to_angle = static_cast<float>(sample_to) * std::numbers::pi_v<float> * 2 / DrawListSharedData::vertex_sample_points_count;

			const auto emit_start = math::abs(segment_from_angle - from) >= 1e-5f;
			const auto emit_end = math::abs(to - segment_to_angle) >= 1e-5f;

			if (emit_start)
			{
				// The quadrant must be the same, otherwise it is not continuous with the path drawn by `path_arc_fast`.
				path_pin({center + point_type{math::cos(from), -math::sin(from)} * radius});
			}
			if (sample_mid > 0)
			{
				path_arc_fast(circle, sample_from, sample_to);
			}
			if (emit_end)
			{
				// The quadrant must be the same, otherwise it is not continuous with the path drawn by `path_arc_fast`.
				path_pin({center + point_type{math::cos(to), -math::sin(to)} * radius});
			}
		}
		else
		{
			const auto arc_length = to - from;
			const auto circle_segment_count = draw_list_shared_data.get_circle_auto_segment_count(radius);
			const auto arc_segment_count = std::ranges::max(
				static_cast<unsigned>(math::ceil(static_cast<float>(circle_segment_count) * arc_length / (std::numbers::pi_v<float> * 2))),
				static_cast<unsigned>(std::numbers::pi_v<float> * 2 / arc_length)
			);
			path_arc_n(circle, from, to, arc_segment_count);
		}
	}

	auto DrawList::path_arc_elliptical_n(const ellipse_type& ellipse, const float from, const float to, const std::uint32_t segments) noexcept -> void
	{
		const auto& [center, radius, rotation] = ellipse;
		const auto cos_theta = math::cos(rotation);
		const auto sin_theta = math::sin(rotation);

		path_reserve_extra(segments);
		for (std::uint32_t i = 0; i < segments; ++i)
		{
			const auto a = from + static_cast<float>(i) / static_cast<float>(segments) * (to - from);
			const auto offset = point_type{math::cos(a), math::sin(a)} * radius;
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

			const auto v =
					(flag & DrawFlag::ROUND_CORNER_TOP) == DrawFlag::ROUND_CORNER_TOP or
					(flag & DrawFlag::ROUND_CORNER_BOTTOM) == DrawFlag::ROUND_CORNER_BOTTOM;
			const auto h =
					(flag & DrawFlag::ROUND_CORNER_LEFT) == DrawFlag::ROUND_CORNER_LEFT or
					(flag & DrawFlag::ROUND_CORNER_RIGHT) == DrawFlag::ROUND_CORNER_RIGHT;

			rounding = std::ranges::min(rounding, rect.width() * (v ? .5f : 1.f) - 1.f);
			rounding = std::ranges::min(rounding, rect.height() * (h ? .5f : 1.f) - 1.f);
		}

		if (rounding < .5f or (DrawFlag::ROUND_CORNER_MASK & flag) == DrawFlag::ROUND_CORNER_NONE)
		{
			path_quadrilateral(rect.left_top(), rect.right_top(), rect.right_bottom(), rect.left_bottom());
		}
		else
		{
			const auto rounding_left_top = (flag & DrawFlag::ROUND_CORNER_LEFT_TOP) != DrawFlag::NONE ? rounding : 0;
			const auto rounding_right_top = (flag & DrawFlag::ROUND_CORNER_RIGHT_TOP) != DrawFlag::NONE ? rounding : 0;
			const auto rounding_left_bottom = (flag & DrawFlag::ROUND_CORNER_LEFT_BOTTOM) != DrawFlag::NONE ? rounding : 0;
			const auto rounding_right_bottom = (flag & DrawFlag::ROUND_CORNER_RIGHT_BOTTOM) != DrawFlag::NONE ? rounding : 0;

			path_arc_fast({rect.left_top() + point_type{rounding_left_top, rounding_left_top}, rounding_left_top}, DrawArcFlag::Q2_CLOCK_WISH);
			path_arc_fast({rect.right_top() + point_type{-rounding_right_top, rounding_right_top}, rounding_right_top}, DrawArcFlag::Q1_CLOCK_WISH);
			path_arc_fast({rect.right_bottom() + point_type{-rounding_right_bottom, -rounding_right_bottom}, rounding_right_bottom}, DrawArcFlag::Q4_CLOCK_WISH);
			path_arc_fast({rect.left_bottom() + point_type{rounding_left_bottom, -rounding_left_bottom}, rounding_left_bottom}, DrawArcFlag::Q3_CLOCK_WISH);
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
		const auto d2 = math::abs((p2.x - p4.x) * dy - (p2.y - p4.y) * dx);
		const auto d3 = math::abs((p3.x - p4.x) * dy - (p3.y - p4.y) * dx);

		if (math::pow(d2 + d3, 2) < tessellation_tolerance * (math::pow(dx, 2) + math::pow(dy, 2)))
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

		if (math::pow(det, 2) * 4.f < tessellation_tolerance * (math::pow(dx, 2) + math::pow(dy, 2)))
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
		const auto& draw_list_shared_data = Context::instance().draw_list_shared_data();

		path_pin(p1);
		if (segments == 0)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(draw_list_shared_data.get_curve_tessellation_tolerance() > 0);

			path_reserve_extra(bezier_curve_casteljau_max_level * 2);
			// auto-tessellated
			path_bezier_cubic_curve_casteljau(p1, p2, p3, p4, draw_list_shared_data.get_curve_tessellation_tolerance(), 0);
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
		const auto& draw_list_shared_data = Context::instance().draw_list_shared_data();

		path_pin(p1);
		if (segments == 0)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(draw_list_shared_data.get_curve_tessellation_tolerance() > 0);

			path_reserve_extra(bezier_curve_casteljau_max_level * 2);
			// auto-tessellated
			path_bezier_quadratic_curve_casteljau(p1, p2, p3, draw_list_shared_data.get_curve_tessellation_tolerance(), 0);
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

	auto DrawList::reset() noexcept -> void
	{
		const auto& font = Context::instance().font();

		auto& [command_list, vertex_list, index_list] = private_data_;

		command_list.clear();
		vertex_list.resize(0);
		index_list.resize(0);

		// we don't know the size of the clip rect, so we need the user to set it
		this_command_clip_rect_ = {};
		// the first texture is always the (default) font texture
		this_command_texture_id_ = font.texture_id();

		path_list_.clear();

		// we always have a command ready in the buffer
		command_list.emplace_back(
			command_type
			{
					.clip_rect = this_command_clip_rect_,
					.texture_id = this_command_texture_id_,
					.index_offset = index_list.size(),
					// set by subsequent draw_xxx
					.element_count = 0
			}
		);
	}

	auto DrawList::push_clip_rect(const rect_type& rect, const bool intersect_with_current_clip_rect) noexcept -> rect_type&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not rect.empty() and rect.valid());

		auto& [command_list, _1, _2] = private_data_;

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not command_list.empty());

		const auto& [current_clip_rect, current_texture, current_index_offset, current_element_count] = command_list.back();

		this_command_clip_rect_ = intersect_with_current_clip_rect ? rect.combine_min(current_clip_rect) : rect;

		on_element_changed(ChangedElement::CLIP_RECT);
		return command_list.back().clip_rect;
	}

	auto DrawList::push_clip_rect(const point_type& left_top, const point_type& right_bottom, const bool intersect_with_current_clip_rect) noexcept -> rect_type&
	{
		return push_clip_rect({left_top, right_bottom}, intersect_with_current_clip_rect);
	}

	auto DrawList::pop_clip_rect() noexcept -> void
	{
		auto& [command_list, _1, _2] = private_data_;

		// at least one command
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(command_list.size() > 1);
		this_command_clip_rect_ = command_list[command_list.size() - 2].clip_rect;

		on_element_changed(ChangedElement::CLIP_RECT);
	}

	auto DrawList::push_texture_id(const texture_id_type texture) noexcept -> void
	{
		this_command_texture_id_ = texture;

		on_element_changed(ChangedElement::TEXTURE_ID);
	}

	auto DrawList::pop_texture_id() noexcept -> void
	{
		auto& [command_list, _1, _2] = private_data_;

		// at least one command
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(command_list.size() > 1);
		this_command_texture_id_ = command_list[command_list.size() - 2].texture_id;

		on_element_changed(ChangedElement::TEXTURE_ID);
	}

	auto DrawList::line(
		const point_type& from,
		const point_type& to,
		const color_type& color,
		const float thickness
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		// path_pin(from + point_type{.5f, .5f});
		// path_pin(to + point_type{.5f, .5f});
		path_pin(from);
		path_pin(to);

		path_stroke(color, DrawFlag::NONE, thickness);
	}


	auto DrawList::triangle(
		const point_type& a,
		const point_type& b,
		const point_type& c,
		const color_type& color,
		const float thickness
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		path_pin(a);
		path_pin(b);
		path_pin(c);

		path_stroke(color, DrawFlag::CLOSED, thickness);
	}

	auto DrawList::triangle_filled(
		const point_type& a,
		const point_type& b,
		const point_type& c,
		const color_type& color
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		path_pin(a);
		path_pin(b);
		path_pin(c);

		path_stroke(color);
	}

	auto DrawList::rect(
		const rect_type& rect,
		const color_type& color,
		const float rounding,
		const DrawFlag flag,
		const float thickness
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		// path_rect(rect_type{rect.left_top() + point_type{.5f, .5f}, rect.right_bottom() - point_type{.5f, .5f}}, rounding, flag);
		path_rect(rect, rounding, flag);

		path_stroke(color, DrawFlag::CLOSED, thickness);
	}

	auto DrawList::rect(
		const point_type& left_top,
		const point_type& right_bottom,
		const color_type& color,
		const float rounding,
		const DrawFlag flag,
		const float thickness
	) noexcept -> void
	{
		return rect({left_top, right_bottom}, color, rounding, flag, thickness);
	}

	auto DrawList::rect_filled(
		const rect_type& rect,
		const color_type& color,
		const float rounding,
		const DrawFlag flag
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		if (rounding < .5f or (DrawFlag::ROUND_CORNER_MASK & flag) == DrawFlag::ROUND_CORNER_NONE)
		{
			draw_rect_filled(rect, color, color, color, color);
		}
		else
		{
			path_rect(rect, rounding, flag);
			path_stroke(color);
		}
	}

	auto DrawList::rect_filled(
		const point_type& left_top,
		const point_type& right_bottom,
		const color_type& color,
		const float rounding,
		const DrawFlag flag
	) noexcept -> void
	{
		return rect_filled({left_top, right_bottom}, color, rounding, flag);
	}

	auto DrawList::rect_filled(
		const rect_type& rect,
		const color_type& color_left_top,
		const color_type& color_right_top,
		const color_type& color_left_bottom,
		const color_type& color_right_bottom
	) noexcept -> void
	{
		if (color_left_top.alpha == 0 or color_right_top.alpha == 0 or color_left_bottom.alpha == 0 or color_right_bottom.alpha == 0)
		{
			return;
		}

		draw_rect_filled(rect, color_left_top, color_right_top, color_left_bottom, color_right_bottom);
	}

	auto DrawList::rect_filled(
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

	auto DrawList::quadrilateral(
		const point_type& p1,
		const point_type& p2,
		const point_type& p3,
		const point_type& p4,
		const color_type& color,
		const float thickness
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		path_quadrilateral(p1, p2, p3, p4);

		path_stroke(color, DrawFlag::CLOSED, thickness);
	}

	auto DrawList::quadrilateral_filled(
		const point_type& p1,
		const point_type& p2,
		const point_type& p3,
		const point_type& p4,
		const color_type& color
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		path_quadrilateral(p1, p2, p3, p4);

		path_stroke(color);
	}

	auto DrawList::circle_n(
		const circle_type& circle,
		const color_type& color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		if (color.alpha == 0 or circle.radius < .5f or segments < 3)
		{
			return;
		}

		path_arc_n(circle, 0, std::numbers::pi_v<float> * 2, segments);

		path_stroke(color, DrawFlag::CLOSED, thickness);
	}

	auto DrawList::circle_n(
		const point_type& center,
		const float radius,
		const color_type& color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		return circle_n({center, radius}, color, segments, thickness);
	}

	auto DrawList::ellipse_n(
		const ellipse_type& ellipse,
		const color_type& color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f or segments < 3)
		{
			return;
		}

		path_arc_elliptical_n(ellipse, 0, std::numbers::pi_v<float> * 2, segments);

		path_stroke(color, DrawFlag::CLOSED, thickness);
	}

	auto DrawList::ellipse_n(
		const point_type& center,
		const extent_type& radius,
		const float rotation,
		const color_type& color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		return ellipse_n({center, radius, rotation}, color, segments, thickness);
	}

	auto DrawList::circle_n_filled(
		const circle_type& circle,
		const color_type& color,
		const std::uint32_t segments
	) noexcept -> void
	{
		if (color.alpha == 0 or circle.radius < .5f or segments < 3)
		{
			return;
		}

		path_arc_n(circle, 0, std::numbers::pi_v<float> * 2, segments);

		path_stroke(color);
	}

	auto DrawList::circle_n_filled(
		const point_type& center,
		const float radius,
		const color_type& color,
		const std::uint32_t segments
	) noexcept -> void
	{
		return circle_n_filled({center, radius}, color, segments);
	}

	auto DrawList::ellipse_n_filled(
		const ellipse_type& ellipse,
		const color_type& color,
		const std::uint32_t segments
	) noexcept -> void
	{
		if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f or segments < 3)
		{
			return;
		}

		path_arc_elliptical_n(ellipse, 0, std::numbers::pi_v<float> * 2, segments);

		path_stroke(color);
	}

	auto DrawList::ellipse_n_filled(
		const point_type& center,
		const extent_type& radius,
		const float rotation,
		const color_type& color,
		const std::uint32_t segments
	) noexcept -> void
	{
		return ellipse_n_filled({center, radius, rotation}, color, segments);
	}

	auto DrawList::circle(
		const circle_type& circle,
		const color_type& color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		if (color.alpha == 0 or circle.radius < .5f)
		{
			return;
		}

		if (segments == 0)
		{
			path_arc_fast(circle, 0, DrawListSharedData::vertex_sample_points_count - 1);

			path_stroke(color, DrawFlag::CLOSED, thickness);
		}
		else
		{
			const auto clamped_segments = std::ranges::clamp(segments, DrawListSharedData::circle_segments_min, DrawListSharedData::circle_segments_max);

			circle_n(circle, color, clamped_segments, thickness);
		}
	}

	auto DrawList::circle(
		const point_type& center,
		const float radius,
		const color_type& color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		return circle({center, radius}, color, segments, thickness);
	}

	auto DrawList::circle_filled(
		const circle_type& circle,
		const color_type& color,
		const std::uint32_t segments
	) noexcept -> void
	{
		if (color.alpha == 0 or circle.radius < .5f)
		{
			return;
		}

		if (segments == 0)
		{
			path_arc_fast(circle, 0, DrawListSharedData::vertex_sample_points_count - 1);

			path_stroke(color);
		}
		else
		{
			const auto clamped_segments = std::ranges::clamp(segments, DrawListSharedData::circle_segments_min, DrawListSharedData::circle_segments_max);

			circle_n_filled(circle, color, clamped_segments);
		}
	}

	auto DrawList::circle_filled(
		const point_type& center,
		const float radius,
		const color_type& color,
		const std::uint32_t segments
	) noexcept -> void
	{
		return circle_filled({center, radius}, color, segments);
	}

	auto DrawList::ellipse(
		const ellipse_type& ellipse,
		const color_type& color,
		std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		const auto& draw_list_shared_data = Context::instance().draw_list_shared_data();

		if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f)
		{
			return;
		}

		if (segments == 0)
		{
			// fixme: maybe there's a better computation to do here
			segments = draw_list_shared_data.get_circle_auto_segment_count(std::ranges::max(ellipse.radius.width, ellipse.radius.height));
		}

		ellipse_n(ellipse, color, segments, thickness);
	}

	auto DrawList::ellipse(
		const point_type& center,
		const extent_type& radius,
		const float rotation,
		const color_type& color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		return ellipse({center, radius, rotation}, color, segments, thickness);
	}

	auto DrawList::ellipse_filled(
		const ellipse_type& ellipse,
		const color_type& color,
		std::uint32_t segments
	) noexcept -> void
	{
		const auto& draw_list_shared_data = Context::instance().draw_list_shared_data();

		if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f)
		{
			return;
		}

		if (segments == 0)
		{
			// fixme: maybe there's a better computation to do here
			segments = draw_list_shared_data.get_circle_auto_segment_count(std::ranges::max(ellipse.radius.width, ellipse.radius.height));
		}

		ellipse_n_filled(ellipse, color, segments);
	}

	auto DrawList::ellipse_filled(
		const point_type& center,
		const extent_type& radius,
		const float rotation,
		const color_type& color,
		const std::uint32_t segments
	) noexcept -> void
	{
		return ellipse_filled({center, radius, rotation}, color, segments);
	}

	auto DrawList::bezier_cubic(
		const point_type& p1,
		const point_type& p2,
		const point_type& p3,
		const point_type& p4,
		const color_type& color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		path_bezier_curve(p1, p2, p3, p4, segments);

		path_stroke(color, DrawFlag::NONE, thickness);
	}

	auto DrawList::bezier_quadratic(
		const point_type& p1,
		const point_type& p2,
		const point_type& p3,
		const color_type& color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		path_bezier_quadratic_curve(p1, p2, p3, segments);

		path_stroke(color, DrawFlag::NONE, thickness);
	}

	auto DrawList::text(
		const Font& font,
		const float font_size,
		const point_type& p,
		const color_type& color,
		const std::string_view utf8_text,
		const float wrap_width
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		draw_text(font, font_size, p, color, utf8_text, wrap_width);
	}

	auto DrawList::text(
		const float font_size,
		const point_type& p,
		const color_type& color,
		const std::string_view utf8_text,
		const float wrap_width
	) noexcept -> void
	{
		const auto& font = Context::instance().font();

		this->text(font, font_size, p, color, utf8_text, wrap_width);
	}

	auto DrawList::image(
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
		if (color.alpha == 0)
		{
			return;
		}

		draw_image(texture_id, display_p1, display_p2, display_p3, display_p4, uv_p1, uv_p2, uv_p3, uv_p4, color);
	}

	auto DrawList::image(
		const texture_id_type texture_id,
		const rect_type& display_rect,
		const rect_type& uv_rect,
		const color_type& color
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

	auto DrawList::image(
		const texture_id_type texture_id,
		const point_type& display_left_top,
		const point_type& display_right_bottom,
		const uv_type& uv_left_top,
		const uv_type& uv_right_bottom,
		const color_type& color
	) noexcept -> void
	{
		image(texture_id, {display_left_top, display_right_bottom}, {uv_left_top, uv_right_bottom}, color);
	}

	auto DrawList::image_rounded(
		const texture_id_type texture_id,
		const rect_type& display_rect,
		const float rounding,
		const DrawFlag flag,
		const rect_type& uv_rect,
		const color_type& color
	) noexcept -> void
	{
		if (color.alpha == 0)
		{
			return;
		}

		draw_image_rounded(texture_id, display_rect, uv_rect, color, rounding, flag);
	}

	auto DrawList::image_rounded(
		const texture_id_type texture_id,
		const point_type& display_left_top,
		const point_type& display_right_bottom,
		const float rounding,
		const DrawFlag flag,
		const uv_type& uv_left_top,
		const uv_type& uv_right_bottom,
		const color_type& color
	) noexcept -> void
	{
		image_rounded(texture_id, {display_left_top, display_right_bottom}, rounding, flag, {uv_left_top, uv_right_bottom}, color);
	}
}
