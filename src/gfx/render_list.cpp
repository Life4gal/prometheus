// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/render_list.hpp>

#include <gfx/context.hpp>

#include <math/cmath.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace
{
	using namespace gal::prometheus;
	using namespace gfx;

	// @see https://stackoverflow.com/a/2244088/15194693
	// Number of segments (N) is calculated using equation:
	//	N = ceil ( pi / acos(1 - error / r) ) where r > 0 and error <= r
	[[nodiscard]] constexpr auto circle_segments_calc(const float radius, const float max_error) noexcept -> auto
	{
		constexpr auto circle_segments_roundup_to_even = [](const auto v) noexcept -> auto
		{
			return (v + 1) / 2 * 2;
		};

		return std::ranges::clamp(
				circle_segments_roundup_to_even(static_cast<std::uint32_t>(std::ceil(std::numbers::pi_v<float> / std::acos(1 - std::ranges::min(radius, max_error) / radius)))),
				RenderListSharedData::circle_segments_min,
				RenderListSharedData::circle_segments_max
		);
	}

	[[nodiscard]] constexpr auto circle_segments_calc_radius(const std::size_t n, const float max_error) noexcept -> auto
	{
		return max_error / (1 - math::cos(std::numbers::pi_v<float> / std::ranges::max(static_cast<float>(n), std::numbers::pi_v<float>)));
	}

	[[nodiscard]] constexpr auto circle_segments_calc_error(const std::size_t n, const float radius) noexcept -> auto
	{
		return (1 - math::cos(std::numbers::pi_v<float> / std::ranges::max(static_cast<float>(n), std::numbers::pi_v<float>))) / radius;
	}

	template<std::size_t N>
	[[nodiscard]] constexpr auto vertex_sample_points_calc() noexcept -> RenderListSharedData::vertex_sample_points_type
	{
		return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> RenderListSharedData::vertex_sample_points_type
		{
			constexpr auto make_point = []<std::size_t I>() noexcept -> point_type
			{
				const auto a = static_cast<float>(I) / static_cast<float>(N) * 2 * std::numbers::pi_v<float>;
				return {math::cos(a), -math::sin(a)};
			};

			return { {make_point.template operator()<Index>()...} };
		}(std::make_index_sequence<N>{});
	}

	[[nodiscard]] constexpr auto range_of_arc(const RenderArcFlag flag) noexcept -> std::pair<int, int>
	{
		static_assert(RenderListSharedData::vertex_sample_points_count % 12 == 0);
		constexpr auto factor = static_cast<int>(RenderListSharedData::vertex_sample_points_count / 12);

		switch (flag)
		{
			case RenderArcFlag::Q1:
			{
				return std::make_pair(0 * factor, 3 * factor);
			}
			case RenderArcFlag::Q2:
			{
				return std::make_pair(3 * factor, 6 * factor);
			}
			case RenderArcFlag::Q3:
			{
				return std::make_pair(6 * factor, 9 * factor);
			}
			case RenderArcFlag::Q4:
			{
				return std::make_pair(9 * factor, 12 * factor);
			}
			case RenderArcFlag::TOP:
			{
				return std::make_pair(0 * factor, 6 * factor);
			}
			case RenderArcFlag::BOTTOM:
			{
				return std::make_pair(6 * factor, 12 * factor);
			}
			case RenderArcFlag::LEFT:
			{
				return std::make_pair(3 * factor, 9 * factor);
			}
			case RenderArcFlag::RIGHT:
			{
				return std::make_pair(9 * factor, 15 * factor);
			}
			case RenderArcFlag::ALL:
			{
				return std::make_pair(0 * factor, 12 * factor);
			}
			case RenderArcFlag::Q1_CLOCK_WISH:
			{
				return std::make_pair(3 * factor, 0 * factor);
			}
			case RenderArcFlag::Q2_CLOCK_WISH:
			{
				return std::make_pair(6 * factor, 3 * factor);
			}
			case RenderArcFlag::Q3_CLOCK_WISH:
			{
				return std::make_pair(9 * factor, 6 * factor);
			}
			case RenderArcFlag::Q4_CLOCK_WISH:
			{
				return std::make_pair(12 * factor, 9 * factor);
			}
			case RenderArcFlag::TOP_CLOCK_WISH:
			{
				return std::make_pair(6 * factor, 0 * factor);
			}
			case RenderArcFlag::BOTTOM_CLOCK_WISH:
			{
				return std::make_pair(12 * factor, 6 * factor);
			}
			case RenderArcFlag::LEFT_CLOCK_WISH:
			{
				return std::make_pair(9 * factor, 3 * factor);
			}
			case RenderArcFlag::RIGHT_CLOCK_WISH:
			{
				return std::make_pair(15 * factor, 9 * factor);
			}
			case RenderArcFlag::ALL_CLOCK_WISH:
			{
				return std::make_pair(12 * factor, 0 * factor);
			}
			default: // NOLINT(clang-diagnostic-covered-switch-default)
			{
				GAL_PROMETHEUS_ERROR_UNREACHABLE();
			}
		}
	}

	[[nodiscard]] constexpr auto to_fixed_rect_corner_flag(const RenderFlag flag) noexcept -> RenderFlag
	{
		using enum RenderFlag;

		if ((flag & ROUND_CORNER_MASK) == NONE)
		{
			return ROUND_CORNER_ALL | flag;
		}

		return flag;
	}

	[[nodiscard]] constexpr auto to_fixed_normal(const float x, const float y) noexcept -> std::pair<float, float>
	{
		if (const auto d = math::pow(x, 2) + math::pow(y, 2); d > 1e-6f)
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

	constexpr auto bezier_cubic_calc = [](const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4, const float tolerance) noexcept -> point_type
	{
		const auto u = 1.f - tolerance;

		const auto w1 = math::pow(u, 3);
		const auto w2 = 3 * math::pow(u, 2) * tolerance;
		const auto w3 = 3 * u * math::pow(tolerance, 2);
		const auto w4 = math::pow(tolerance, 3);

		return {p1.x * w1 + p2.x * w2 + p3.x * w3 + p4.x * w4, p1.y * w1 + p2.y * w2 + p3.y * w3 + p4.y * w4};
	};

	constexpr auto bezier_quadratic_calc = [](const point_type& p1, const point_type& p2, const point_type& p3, const float tolerance) noexcept -> point_type
	{
		const auto u = 1.f - tolerance;

		const auto w1 = math::pow(u, 2);
		const auto w2 = 2 * u * tolerance;
		const auto w3 = math::pow(tolerance, 2);

		return {p1.x * w1 + p2.x * w2 + p3.x * w3, p1.y * w1 + p2.y * w2 + p3.y * w3};
	};

	class RenderDataAppender final
	{
	public:
		using size_type = RenderData::size_type;

		using command_type = RenderData::command_type;

		using vertex_list_type = RenderData::vertex_list_type;
		using index_list_type = RenderData::index_list_type;

	private:
		// command_type::element_count
		memory::RefWrapper<size_type> element_count_;
		// RenderList::vertex_list
		memory::RefWrapper<vertex_list_type> vertex_list_;
		// RenderList::index_list_;
		memory::RefWrapper<index_list_type> index_list_;

	public:
		RenderDataAppender(command_type& command, vertex_list_type& vertex_list, index_list_type& index_list) noexcept
			: element_count_{command.element_count},
			  vertex_list_{vertex_list},
			  index_list_{index_list}
		{
		}

		[[nodiscard]] auto vertex_count() const noexcept -> size_type
		{
			const auto& list = vertex_list_.get();
			const auto size = list.size();

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size < std::numeric_limits<index_type>::max(), "Too many vertex!");

			return static_cast<size_type>(size);
		}

		auto reserve(const size_type vertex_count, const size_type index_count) noexcept -> void
		{
			auto& element_count = element_count_.get();
			auto& vertex = vertex_list_.get();
			auto& index = index_list_.get();

			element_count += index_count;
			vertex.reserve(vertex.size() + vertex_count);
			index.reserve(index.size() + index_count);
		}

		auto reserve(const std::size_t vertex_count, const std::size_t index_count) noexcept -> void
		{
			reserve(static_cast<size_type>(vertex_count), static_cast<size_type>(index_count));
		}

		auto add_vertex(const point_type& point, const uv_type& uv, color_type color) noexcept -> void
		{
			auto& list = vertex_list_.get();

			list.emplace_back(point, uv, color);
		}

		auto add_index(const index_type a, const index_type b, const index_type c) noexcept -> void
		{
			auto& list = index_list_.get();

			list.push_back(a);
			list.push_back(b);
			list.push_back(c);
		}
	};
} // namespace

namespace gal::prometheus::gfx
{
	RenderListSharedData::RenderListSharedData() noexcept
		: circle_segment_counts{},
		  vertex_sample_points{vertex_sample_points_calc<vertex_sample_points_count>()},
		  circle_segment_max_error{0},
		  arc_fast_radius_cutoff{0},
		  curve_tessellation_tolerance{1.25f}
	{
		set_circle_tessellation_max_error(.3f);
	}

	auto RenderListSharedData::circle_auto_segment_count(const float radius) const noexcept -> circle_segment_count_type
	{
		// ceil to never reduce accuracy
		if (const auto radius_index = static_cast<circle_segment_counts_type::size_type>(radius + .999999f); radius_index < circle_segment_counts.size())
		{
			return circle_segment_counts[radius_index];
		}
		return static_cast<circle_segment_count_type>(circle_segments_calc(radius, circle_segment_max_error));
	}

	auto RenderListSharedData::vertex_sample_point(const std::size_t index) const noexcept -> const point_type&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < vertex_sample_points.size());

		return vertex_sample_points[index];
	}

	auto RenderListSharedData::set_circle_tessellation_max_error(const float max_error) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(max_error > .0f);

		if (circle_segment_max_error == max_error) // NOLINT(clang-diagnostic-float-equal)
		{
			return;
		}

		for (auto [index, count]: circle_segment_counts | std::views::enumerate)
		{
			const auto radius = static_cast<float>(index);
			count = static_cast<circle_segment_count_type>(circle_segments_calc(radius, max_error));
		}

		circle_segment_max_error = max_error;
		arc_fast_radius_cutoff = circle_segments_calc_radius(vertex_sample_points_count, max_error);
	}

	auto RenderListSharedData::set_curve_tessellation_tolerance(const float tolerance) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(tolerance > .0f);

		curve_tessellation_tolerance = tolerance;
	}

	class RenderList::Drawer
	{
	public:
		memory::RefWrapper<RenderList> self;

		using path_list_type = std::vector<point_type>;

		path_list_type path_list;

		[[nodiscard]] auto make_appender() noexcept -> RenderDataAppender
		{
			auto& render_list = self.get();

			return {render_list.command_list_.back(), render_list.vertex_list_, render_list.index_list_};
		}

		auto draw_polygon_line(const color_type color, const RenderFlag render_flag, const float thickness) noexcept -> void
		{
			const auto path_point_count = path_list.size();
			const auto& path_point = path_list;

			if (path_point_count < 2 or color.alpha == 0)
			{
				return;
			}

			const auto& render_list = self.get();
			const auto& shared_data = render_list.shared_data();
			auto appender = make_appender();

			const auto is_closed = (render_flag & RenderFlag::CLOSED) != RenderFlag::NONE;
			const auto segments_count = is_closed ? path_point_count : path_point_count - 1;

			const auto vertex_count = segments_count * 4;
			const auto index_count = segments_count * 6;
			appender.reserve(vertex_count, index_count);

			for (std::decay_t<decltype(segments_count)> i = 0; i < segments_count; ++i)
			{
				const auto n = (i + 1) % path_point_count;

				const auto& p1 = path_point[i];
				const auto& p2 = path_point[n];

				auto [normalized_x, normalized_y] = math::normalize(p2.x - p1.x, p2.y - p1.y);
				normalized_x *= (thickness * .5f);
				normalized_y *= (thickness * .5f);

				const auto current_vertex_index = static_cast<index_type>(appender.vertex_count());
				const auto& opaque_uv = shared_data.white_pixel_uv;

				appender.add_vertex(p1 + point_type{normalized_y, -normalized_x}, opaque_uv, color);
				appender.add_vertex(p2 + point_type{normalized_y, -normalized_x}, opaque_uv, color);
				appender.add_vertex(p2 + point_type{-normalized_y, normalized_x}, opaque_uv, color);
				appender.add_vertex(p1 + point_type{-normalized_y, normalized_x}, opaque_uv, color);

				appender.add_index(current_vertex_index + 0, current_vertex_index + 1, current_vertex_index + 2);
				appender.add_index(current_vertex_index + 0, current_vertex_index + 2, current_vertex_index + 3);
			}
		}

		auto draw_polygon_line_aa(const color_type color, const RenderFlag render_flag, float thickness) noexcept -> void
		{
			const auto path_point_count = path_list.size();
			const auto& path_point = path_list;

			if (path_point_count < 2 or color.alpha == 0)
			{
				return;
			}

			const auto& render_list = self.get();
			const auto render_list_flag = render_list.render_list_flag_;
			const auto& shared_data = render_list.shared_data();
			auto appender = make_appender();

			const auto& opaque_uv = shared_data.white_pixel_uv;
			const auto transparent_color = color.transparent();

			const auto is_closed = (render_flag & RenderFlag::CLOSED) != RenderFlag::NONE;
			const auto segments_count = is_closed ? path_point_count : path_point_count - 1;
			const auto is_thick_line = thickness > 1.f;

			thickness = std::ranges::max(thickness, 1.f);
			const auto thickness_integer = static_cast<std::uint32_t>(thickness);
			const auto thickness_fractional = thickness - static_cast<float>(thickness_integer);

			const auto is_use_texture =
					((render_list_flag & RenderListFlag::ANTI_ALIASED_LINE_USE_TEXTURE) == RenderListFlag::ANTI_ALIASED_LINE_USE_TEXTURE and
					 (thickness_integer < RenderListSharedData::baked_line_uv_count) and (thickness_fractional <= .00001f));

			const auto vertex_cont = is_use_texture ? (path_point_count * 2) : (is_thick_line ? path_point_count * 4 : path_point_count * 3);
			const auto index_count = is_use_texture ? (segments_count * 6) : (is_thick_line ? segments_count * 18 : segments_count * 12);
			appender.reserve(vertex_cont, index_count);

			// The first <path_point_count> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
			path_list_type temp_buffer{};
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

				const auto current_vertex_index = static_cast<index_type>(appender.vertex_count());

				// Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
				// This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
				auto vertex_index_for_start = current_vertex_index;
				for (std::decay_t<decltype(segments_count)> first_point_of_segment = 0; first_point_of_segment < segments_count; ++first_point_of_segment)
				{
					const auto second_point_of_segment = (first_point_of_segment + 1) % path_point_count;
					const auto vertex_index_for_end = static_cast<index_type>(
							// closed
							(first_point_of_segment + 1) == path_point_count ? current_vertex_index : (vertex_index_for_start + (is_use_texture ? 2 : 3))
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
						appender.add_index(vertex_index_for_end + 0, vertex_index_for_start + 0, vertex_index_for_start + 1);
						// left
						appender.add_index(vertex_index_for_end + 1, vertex_index_for_start + 1, vertex_index_for_end + 0);
					}
					else
					{
						// Add indexes for four triangles

						// right 1
						appender.add_index(vertex_index_for_end + 0, vertex_index_for_start + 0, vertex_index_for_start + 2);
						// right 2
						appender.add_index(vertex_index_for_start + 2, vertex_index_for_end + 2, vertex_index_for_end + 0);
						// left 1
						appender.add_index(vertex_index_for_end + 1, vertex_index_for_start + 1, vertex_index_for_start + 0);
						// left 2
						appender.add_index(vertex_index_for_start + 0, vertex_index_for_end + 0, vertex_index_for_end + 1);
					}

					vertex_index_for_start = vertex_index_for_end;
				}

				// Add vertexes for each point on the line
				if (is_use_texture)
				{
					const auto& uv = shared_data.baked_line_uvs[thickness_integer];

					const auto uv0 = uv.left_top();
					const auto uv1 = uv.right_bottom();
					for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
					{
						// left-side outer edge
						appender.add_vertex(temp_buffer_points[i * 2 + 0], uv0, color);
						// right-side outer edge
						appender.add_vertex(temp_buffer_points[i * 2 + 1], uv1, color);
					}
				}
				else
				{
					// If we're not using a texture, we need the center vertex as well
					for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
					{
						// center of line
						appender.add_vertex(path_point[i], opaque_uv, color);
						// left-side outer edge
						appender.add_vertex(temp_buffer_points[i * 2 + 0], opaque_uv, transparent_color);
						// right-side outer edge
						appender.add_vertex(temp_buffer_points[i * 2 + 1], opaque_uv, transparent_color);
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

				const auto current_vertex_index = static_cast<index_type>(appender.vertex_count());

				// Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
				// This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
				auto vertex_index_for_start = current_vertex_index;
				for (std::decay_t<decltype(segments_count)> first_point_of_segment = 0; first_point_of_segment < segments_count; ++first_point_of_segment)
				{
					const auto second_point_of_segment = (first_point_of_segment + 1) % path_point_count;
					const auto vertex_index_for_end = static_cast<index_type>((first_point_of_segment + 1) == path_point_count ? current_vertex_index : (vertex_index_for_start + 4));

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
					appender.add_index(vertex_index_for_end + 1, vertex_index_for_end + 1, vertex_index_for_start + 2);
					appender.add_index(vertex_index_for_start + 2, vertex_index_for_end + 2, vertex_index_for_end + 1);
					appender.add_index(vertex_index_for_end + 1, vertex_index_for_start + 1, vertex_index_for_start + 0);
					appender.add_index(vertex_index_for_start + 0, vertex_index_for_end + 0, vertex_index_for_end + 1);
					appender.add_index(vertex_index_for_end + 2, vertex_index_for_start + 2, vertex_index_for_start + 3);
					appender.add_index(vertex_index_for_start + 3, vertex_index_for_end + 3, vertex_index_for_end + 2);

					vertex_index_for_start = vertex_index_for_end;
				}

				// Add vertices
				for (std::decay_t<decltype(path_point_count)> i = 0; i < path_point_count; ++i)
				{
					appender.add_vertex(temp_buffer_points[i * 4 + 0], opaque_uv, transparent_color);
					appender.add_vertex(temp_buffer_points[i * 4 + 1], opaque_uv, color);
					appender.add_vertex(temp_buffer_points[i * 4 + 2], opaque_uv, color);
					appender.add_vertex(temp_buffer_points[i * 4 + 2], opaque_uv, transparent_color);
				}
			}
		}

		auto draw_convex_polygon_line_filled(const color_type color) noexcept -> void
		{
			const auto path_point_count = path_list.size();
			const auto& path_point = path_list;

			if (path_point_count < 3 or color.alpha == 0)
			{
				return;
			}

			const auto& render_list = self.get();
			const auto& shared_data = render_list.shared_data();
			auto appender = make_appender();

			const auto vertex_count = path_point_count;
			const auto index_count = (path_point_count - 2) * 3;
			appender.reserve(vertex_count, index_count);

			const auto current_vertex_index = static_cast<index_type>(appender.vertex_count());
			const auto& opaque_uv = shared_data.white_pixel_uv;

			std::ranges::for_each(
					path_point,
					[&](const point_type& point) noexcept -> void
					{
						appender.add_vertex(point, opaque_uv, color);
					}
			);
			for (index_type i = 2; std::cmp_less(i, path_point_count); ++i)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_index + i - 1 >= current_vertex_index);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_index + i >= current_vertex_index);

				appender.add_index(current_vertex_index + 0, current_vertex_index + i - 1, current_vertex_index + i);
			}
		}

		auto draw_convex_polygon_line_filled_aa(const color_type color) noexcept -> void
		{
			const auto path_point_count = path_list.size();
			const auto& path_point = path_list;

			if (path_point_count < 3 or color.alpha == 0)
			{
				return;
			}

			const auto& render_list = self.get();
			const auto& shared_data = render_list.shared_data();
			auto appender = make_appender();

			const auto& opaque_uv = shared_data.white_pixel_uv;
			const auto transparent_color = color.transparent();

			const auto vertex_count = path_point_count * 2;
			const auto index_count = (path_point_count - 2) * 3 + path_point_count * 6;
			appender.reserve(vertex_count, index_count);

			const auto current_vertex_inner_index = static_cast<index_type>(appender.vertex_count());
			const auto current_vertex_outer_index = static_cast<index_type>(appender.vertex_count() + 1);

			// Add indexes for fill
			for (index_type i = 2; std::cmp_less(i, path_point_count); ++i)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>((i - 1) << 1) >= current_vertex_inner_index);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>(i << 1) >= current_vertex_inner_index);

				appender.add_index(current_vertex_inner_index + 0, current_vertex_inner_index + static_cast<index_type>((i - 1) << 1), current_vertex_inner_index + static_cast<index_type>(i << 1));
			}

			path_list_type temp_buffer{};
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
				appender.add_vertex(path_point[n] - point_type{dm_x, dm_y}, opaque_uv, color);
				// outer
				appender.add_vertex(path_point[n] + point_type{dm_x, dm_y}, opaque_uv, transparent_color);

				// Add indexes for fringes
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>(n << 1) >= current_vertex_inner_index);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>(i << 1) >= current_vertex_inner_index);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + static_cast<index_type>(i << 1) >= current_vertex_outer_index);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + static_cast<index_type>(i << 1) >= current_vertex_outer_index);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_outer_index + static_cast<index_type>(n << 1) >= current_vertex_outer_index);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_vertex_inner_index + static_cast<index_type>(n << 1) >= current_vertex_inner_index);

				appender.add_index(
						current_vertex_inner_index + static_cast<index_type>(n << 1),
						current_vertex_inner_index + static_cast<index_type>(i << 1),
						current_vertex_outer_index + static_cast<index_type>(i << 1)
				);
				appender.add_index(
						current_vertex_outer_index + static_cast<index_type>(i << 1),
						current_vertex_outer_index + static_cast<index_type>(n << 1),
						current_vertex_inner_index + static_cast<index_type>(n << 1)
				);
			}
		}

		// clang-format off
		auto draw_rect_filled(
			const rect_type& rect,
			const uv_type& uv,
			const color_type color_left_top,
			const color_type color_right_top,
			const color_type color_left_bottom,
			const color_type color_right_bottom
		) noexcept -> void
		// clang-format on
		{
			auto appender = make_appender();

			// two triangle without path
			constexpr size_type vertex_count = 4;
			constexpr size_type index_count = 6;
			appender.reserve(vertex_count, index_count);

			const auto current_vertex_index = static_cast<index_type>(appender.vertex_count());

			appender.add_vertex(rect.left_top(), uv, color_left_top);
			appender.add_vertex(rect.right_top(), uv, color_right_top);
			appender.add_vertex(rect.right_bottom(), uv, color_right_bottom);
			appender.add_vertex(rect.left_bottom(), uv, color_left_bottom);

			appender.add_index(current_vertex_index + 0, current_vertex_index + 1, current_vertex_index + 2);
			appender.add_index(current_vertex_index + 0, current_vertex_index + 2, current_vertex_index + 3);
		}

		// clang-format off
		auto draw_rect_filled(
			const rect_type& rect,
			const color_type color_left_top,
			const color_type color_right_top,
			const color_type color_left_bottom,
			const color_type color_right_bottom
		) noexcept -> void
		// clang-format on
		{
			const auto& render_list = self.get();
			const auto& shared_data = render_list.shared_data();
			const auto& opaque_uv = shared_data.white_pixel_uv;

			draw_rect_filled(rect, opaque_uv, color_left_top, color_right_top, color_left_bottom, color_right_bottom);
		}

		// clang-format off
		auto draw_text(
			const std::string_view utf8_text,
			const std::uint32_t font_size,
			const point_type& p,
			const color_type color,
			const float wrap_width
		) noexcept -> void
		// clang-format on
		{
			std::ignore = wrap_width;

			auto& render_list = self.get();
			auto& texture_context = render_list.context_.get().texture_context();

			const auto& glyphs = texture_context.glyph_of(utf8_text, font_size);
			for (auto x = p.x; const auto& glyph: glyphs)
			{
				if (glyph == nullptr)
				{
					return;
				}

				if (glyph->visible)
				{
					const auto& atlas = texture_context.atlas_of(*glyph);

					const auto new_texture = render_list.this_command_texture_ != atlas.id();

					if (new_texture)
					{
						render_list.push_texture(atlas.id());
					}

					// todo
					{
						const auto color_may_colored = glyph->colored ? color.transparent() : color;
						const auto glyph_point = point_type{x, glyph->rect.point.y};
						const auto glyph_width = glyph->rect.width();
						const auto glyph_height = glyph->rect.height();

						auto appender = make_appender();

						// two triangle without path
						constexpr size_type vertex_count = 4;
						constexpr size_type index_count = 6;
						appender.reserve(vertex_count, index_count);

						const auto current_vertex_index = static_cast<index_type>(appender.vertex_count());

						appender.add_vertex(glyph_point, glyph->uv.left_top(), color_may_colored);
						appender.add_vertex(glyph_point + extent_type{glyph_width, 0}, glyph->uv.right_top(), color_may_colored);
						appender.add_vertex(glyph_point + extent_type{0, glyph_height}, glyph->uv.left_bottom(), color_may_colored);
						appender.add_vertex(glyph_point + extent_type{glyph_width, glyph_height}, glyph->uv.right_bottom(), color_may_colored);

						appender.add_index(current_vertex_index + 0, current_vertex_index + 1, current_vertex_index + 2);
						appender.add_index(current_vertex_index + 0, current_vertex_index + 2, current_vertex_index + 3);
					}

					if (new_texture)
					{
						render_list.pop_texture();
					}
				}

				x += glyph->advance_x;
			}
		}

		// clang-format off
		auto draw_image(
			const texture_id_type texture_id,
			const point_type& display_p1,
			const point_type& display_p2,
			const point_type& display_p3,
			const point_type& display_p4,
			const uv_type& uv_p1,
			const uv_type& uv_p2,
			const uv_type& uv_p3,
			const uv_type& uv_p4,
			const color_type color
		) noexcept -> void
		// clang-format on
		{
			auto& render_list = self.get();

			const auto new_texture = render_list.this_command_texture_ != texture_id;

			if (new_texture)
			{
				render_list.push_texture(texture_id);
			}

			auto appender = make_appender();

			// two triangle without path
			constexpr size_type vertex_count = 4;
			constexpr size_type index_count = 6;
			appender.reserve(vertex_count, index_count);

			const auto current_vertex_index = static_cast<index_type>(appender.vertex_count());

			appender.add_vertex(display_p1, uv_p1, color);
			appender.add_vertex(display_p2, uv_p2, color);
			appender.add_vertex(display_p3, uv_p3, color);
			appender.add_vertex(display_p4, uv_p4, color);

			appender.add_index(current_vertex_index + 0, current_vertex_index + 1, current_vertex_index + 2);
			appender.add_index(current_vertex_index + 0, current_vertex_index + 2, current_vertex_index + 3);

			if (new_texture)
			{
				render_list.pop_texture();
			}
		}

		// clang-format off
		auto draw_image_rounded(
			const texture_id_type texture_id,
			const rect_type& display_rect,
			const rect_type& uv_rect,
			const color_type color,
			float rounding,
			RenderFlag flag
		) noexcept -> void
		// clang-format on
		{
			// @see `path_rect`
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(display_rect.valid() and not display_rect.empty());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(uv_rect.valid() and not uv_rect.empty());

			if (rounding >= .5f)
			{
				flag = to_fixed_rect_corner_flag(flag);

				const auto v = (flag & RenderFlag::ROUND_CORNER_TOP) == RenderFlag::ROUND_CORNER_TOP or (flag & RenderFlag::ROUND_CORNER_BOTTOM) == RenderFlag::ROUND_CORNER_BOTTOM;
				const auto h = (flag & RenderFlag::ROUND_CORNER_LEFT) == RenderFlag::ROUND_CORNER_LEFT or (flag & RenderFlag::ROUND_CORNER_RIGHT) == RenderFlag::ROUND_CORNER_RIGHT;

				rounding = std::ranges::min(rounding, display_rect.width() * (v ? .5f : 1.f) - 1.f);
				rounding = std::ranges::min(rounding, display_rect.height() * (h ? .5f : 1.f) - 1.f);
			}

			if (rounding < .5f or (RenderFlag::ROUND_CORNER_MASK & flag) == RenderFlag::ROUND_CORNER_NONE)
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
				auto& render_list = self.get();

				const auto new_texture = render_list.this_command_texture_ != texture_id;

				if (new_texture)
				{
					render_list.push_texture(texture_id);
				}

				const auto rounding_left_top = (flag & RenderFlag::ROUND_CORNER_LEFT_TOP) != RenderFlag::NONE ? rounding : 0;
				const auto rounding_right_top = (flag & RenderFlag::ROUND_CORNER_RIGHT_TOP) != RenderFlag::NONE ? rounding : 0;
				const auto rounding_left_bottom = (flag & RenderFlag::ROUND_CORNER_LEFT_BOTTOM) != RenderFlag::NONE ? rounding : 0;
				const auto rounding_right_bottom = (flag & RenderFlag::ROUND_CORNER_RIGHT_BOTTOM) != RenderFlag::NONE ? rounding : 0;

				path_arc_fast({display_rect.left_top() + point_type{rounding_left_top, rounding_left_top}, rounding_left_top}, RenderArcFlag::Q2_CLOCK_WISH);
				path_arc_fast({display_rect.right_top() + point_type{-rounding_right_top, rounding_right_top}, rounding_right_top}, RenderArcFlag::Q1_CLOCK_WISH);
				path_arc_fast({display_rect.right_bottom() + point_type{-rounding_right_bottom, -rounding_right_bottom}, rounding_right_bottom}, RenderArcFlag::Q4_CLOCK_WISH);
				path_arc_fast({display_rect.left_bottom() + point_type{rounding_left_bottom, -rounding_left_bottom}, rounding_left_bottom}, RenderArcFlag::Q3_CLOCK_WISH);

				const auto before_vertex_count = render_list.vertex_list_.size();
				// draw
				path_stroke(color);
				const auto after_vertex_count = render_list.vertex_list_.size();

				// set uv manually

				const auto display_size = display_rect.size();
				const auto uv_size = uv_rect.size();
				const auto scale = uv_size / display_size;

				auto it = render_list.vertex_list_.begin() + static_cast<vertex_list_type::difference_type>(before_vertex_count);
				const auto end = render_list.vertex_list_.begin() + static_cast<vertex_list_type::difference_type>(after_vertex_count);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it < end);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(end == render_list.vertex_list_.end());

				// note: linear uv
				const auto uv_min = uv_rect.left_top();
				// const auto uv_max = uv_rect.right_bottom();
				while (it != end)
				{
					const auto v = uv_min + (it->position - display_rect.left_top()) * scale;

					it->uv = {
						// std::ranges::clamp(v.x, uv_min.x, uv_max.x),
						v.x,
						// std::ranges::clamp(v.y, uv_min.y, uv_max.y)
						v.y
					};
					it += 1;
				}

				if (new_texture)
				{
					render_list.pop_texture();
				}
			}
		}

		auto path_clear() noexcept -> void
		{
			path_list.clear();
		}

		auto path_reserve(const std::size_t size) noexcept -> void
		{
			path_list.reserve(size);
		}

		auto path_reserve_extra(const std::size_t size) noexcept -> void
		{
			path_reserve(path_list.size() + size);
		}

		auto path_pin(const point_type& point) noexcept -> void
		{
			path_list.push_back(point);
		}

		auto path_stroke(const color_type color, const RenderFlag flag, const float thickness) noexcept -> void
		{
			if (const auto render_list_flag = self.get().render_list_flag_; (render_list_flag & RenderListFlag::ANTI_ALIASED_LINE) != RenderListFlag::NONE)
			{
				draw_polygon_line_aa(color, flag, thickness);
			}
			else
			{
				draw_polygon_line(color, flag, thickness);
			}

			path_clear();
		}

		auto path_stroke(const color_type color) noexcept -> void
		{
			if (const auto render_list_flag = self.get().render_list_flag_; (render_list_flag & RenderListFlag::ANTI_ALIASED_FILL) != RenderListFlag::NONE)
			{
				draw_convex_polygon_line_filled_aa(color);
			}
			else
			{
				draw_convex_polygon_line_filled(color);
			}

			path_clear();
		}

		auto path_arc_fast(const circle_type& circle, const int from, const int to) noexcept -> void
		{
			const auto& [center, radius] = circle;

			if (radius < .5f)
			{
				path_pin(center);
				return;
			}

			const auto& render_list = self.get();
			const auto& shared_data = render_list.shared_data();

			// Calculate arc auto segment step size
			auto step = RenderListSharedData::vertex_sample_points_count / shared_data.circle_auto_segment_count(radius);
			// Make sure we never do steps larger than one quarter of the circle
			step = std::clamp(step, static_cast<decltype(step)>(1), RenderListSharedData::vertex_sample_points_count / 4);

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
			if (sample_index < 0 or std::cmp_greater_equal(sample_index, RenderListSharedData::vertex_sample_points_count))
			{
				sample_index = sample_index % static_cast<int>(RenderListSharedData::vertex_sample_points_count);
				if (sample_index < 0)
				{
					sample_index += static_cast<int>(RenderListSharedData::vertex_sample_points_count);
				}
			}

			if (to >= from)
			{
				for (int i = from; i <= to; i += static_cast<int>(step), sample_index += static_cast<int>(step), step = next_step)
				{
					// a_step is clamped to vertex_sample_points_count, so we have guaranteed that it will not wrap over range twice or more
					if (std::cmp_greater_equal(sample_index, RenderListSharedData::vertex_sample_points_count))
					{
						sample_index -= static_cast<int>(RenderListSharedData::vertex_sample_points_count);
					}

					const auto& sample_point = shared_data.vertex_sample_point(sample_index);

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
						sample_index += static_cast<int>(RenderListSharedData::vertex_sample_points_count);
					}

					const auto& sample_point = shared_data.vertex_sample_point(sample_index);

					path_pin({center + sample_point * radius});
				}
			}

			if (extra_max_sample)
			{
				auto normalized_max_sample_index = to % static_cast<int>(RenderListSharedData::vertex_sample_points_count);
				if (normalized_max_sample_index < 0)
				{
					normalized_max_sample_index += RenderListSharedData::vertex_sample_points_count;
				}

				const auto& sample_point = shared_data.vertex_sample_point(normalized_max_sample_index);

				path_pin({center + sample_point * radius});
			}
		}

		auto path_arc_fast(const circle_type& circle, const RenderArcFlag flag) noexcept -> void
		{
			const auto [from, to] = range_of_arc(flag);

			return path_arc_fast(circle, from, to);
		}

		auto path_arc_n(const circle_type& circle, const float from, const float to, const std::uint32_t segments) noexcept -> void
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

		auto path_arc(const circle_type& circle, const float from, const float to) noexcept -> void
		{
			const auto& [center, radius] = circle;

			if (radius < .5f)
			{
				path_pin(center);
				return;
			}

			const auto& render_list = self.get();
			const auto& shared_data = render_list.shared_data();

			// Automatic segment count
			if (radius <= shared_data.arc_fast_radius_cutoff)
			{
				const auto is_reversed = to < from;

				// We are going to use precomputed values for mid-samples.
				// Determine first and last sample in lookup table that belong to the arc
				const auto sample_from_f = RenderListSharedData::vertex_sample_points_count * from / (std::numbers::pi_v<float> * 2);
				const auto sample_to_f = RenderListSharedData::vertex_sample_points_count * to / (std::numbers::pi_v<float> * 2);

				const auto sample_from = is_reversed ? static_cast<int>(math::floor(sample_from_f)) : static_cast<int>(math::ceil(sample_from_f));
				const auto sample_to = is_reversed ? static_cast<int>(math::ceil(sample_to_f)) : static_cast<int>(math::floor(sample_to_f));
				const auto sample_mid = is_reversed ? static_cast<int>(std::ranges::max(sample_from - sample_to, 0)) : static_cast<int>(std::ranges::max(sample_to - sample_from, 0));

				const auto segment_from_angle = static_cast<float>(sample_from) * std::numbers::pi_v<float> * 2 / RenderListSharedData::vertex_sample_points_count;
				const auto segment_to_angle = static_cast<float>(sample_to) * std::numbers::pi_v<float> * 2 / RenderListSharedData::vertex_sample_points_count;

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
				const auto circle_segment_count = shared_data.circle_auto_segment_count(radius);
				const auto arc_segment_count = std::ranges::max(
						static_cast<unsigned>(math::ceil(static_cast<float>(circle_segment_count) * arc_length / (std::numbers::pi_v<float> * 2))),
						static_cast<unsigned>(std::numbers::pi_v<float> * 2 / arc_length)
				);
				path_arc_n(circle, from, to, arc_segment_count);
			}
		}

		auto path_arc_elliptical_n(const ellipse_type& ellipse, const float from, const float to, const std::uint32_t segments) noexcept -> void
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

		auto path_quadrilateral(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4) noexcept -> void
		{
			path_pin(p1);
			path_pin(p2);
			path_pin(p3);
			path_pin(p4);
		}

		auto path_rect(const rect_type& rect, float rounding, RenderFlag flag) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(rect.valid() and not rect.empty());

			if (rounding >= .5f)
			{
				flag = to_fixed_rect_corner_flag(flag);

				const auto v = (flag & RenderFlag::ROUND_CORNER_TOP) == RenderFlag::ROUND_CORNER_TOP or (flag & RenderFlag::ROUND_CORNER_BOTTOM) == RenderFlag::ROUND_CORNER_BOTTOM;
				const auto h = (flag & RenderFlag::ROUND_CORNER_LEFT) == RenderFlag::ROUND_CORNER_LEFT or (flag & RenderFlag::ROUND_CORNER_RIGHT) == RenderFlag::ROUND_CORNER_RIGHT;

				rounding = std::ranges::min(rounding, rect.width() * (v ? .5f : 1.f) - 1.f);
				rounding = std::ranges::min(rounding, rect.height() * (h ? .5f : 1.f) - 1.f);
			}

			if (rounding < .5f or (RenderFlag::ROUND_CORNER_MASK & flag) == RenderFlag::ROUND_CORNER_NONE)
			{
				path_quadrilateral(rect.left_top(), rect.right_top(), rect.right_bottom(), rect.left_bottom());
			}
			else
			{
				const auto rounding_left_top = (flag & RenderFlag::ROUND_CORNER_LEFT_TOP) != RenderFlag::NONE ? rounding : 0;
				const auto rounding_right_top = (flag & RenderFlag::ROUND_CORNER_RIGHT_TOP) != RenderFlag::NONE ? rounding : 0;
				const auto rounding_left_bottom = (flag & RenderFlag::ROUND_CORNER_LEFT_BOTTOM) != RenderFlag::NONE ? rounding : 0;
				const auto rounding_right_bottom = (flag & RenderFlag::ROUND_CORNER_RIGHT_BOTTOM) != RenderFlag::NONE ? rounding : 0;

				path_arc_fast({rect.left_top() + point_type{rounding_left_top, rounding_left_top}, rounding_left_top}, RenderArcFlag::Q2_CLOCK_WISH);
				path_arc_fast({rect.right_top() + point_type{-rounding_right_top, rounding_right_top}, rounding_right_top}, RenderArcFlag::Q1_CLOCK_WISH);
				path_arc_fast({rect.right_bottom() + point_type{-rounding_right_bottom, -rounding_right_bottom}, rounding_right_bottom}, RenderArcFlag::Q4_CLOCK_WISH);
				path_arc_fast({rect.left_bottom() + point_type{rounding_left_bottom, -rounding_left_bottom}, rounding_left_bottom}, RenderArcFlag::Q3_CLOCK_WISH);
			}
		}

		// clang-format off
		auto path_bezier_cubic_curve_casteljau(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			const float tessellation_tolerance,
			const std::size_t level
		) noexcept -> void
		// clang-format on
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

		// clang-format off
		auto path_bezier_quadratic_curve_casteljau(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const float tessellation_tolerance,
			const std::size_t level
		) noexcept -> void
		// clang-format on
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

		auto path_bezier_curve(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4, const std::uint32_t segments) noexcept -> void
		{
			const auto& render_list = self.get();
			const auto& shared_data = render_list.shared_data();

			path_pin(p1);
			if (segments == 0)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data.curve_tessellation_tolerance > 0);

				path_reserve_extra(bezier_curve_casteljau_max_level * 2);
				// auto-tessellated
				path_bezier_cubic_curve_casteljau(p1, p2, p3, p4, shared_data.curve_tessellation_tolerance, 0);
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

		auto path_bezier_quadratic_curve(const point_type& p1, const point_type& p2, const point_type& p3, const std::uint32_t segments) noexcept -> void
		{
			const auto& render_list = self.get();
			const auto& shared_data = render_list.shared_data();

			path_pin(p1);
			if (segments == 0)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(shared_data.curve_tessellation_tolerance > 0);

				path_reserve_extra(bezier_curve_casteljau_max_level * 2);
				// auto-tessellated
				path_bezier_quadratic_curve_casteljau(p1, p2, p3, shared_data.curve_tessellation_tolerance, 0);
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
	};

	auto RenderList::push_command() noexcept -> void
	{
		// Fixme: If the window boundary is smaller than the rect boundary, the rect will no longer be valid.
		// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not this_command_clip_rect_.empty() and this_command_clip_rect_.valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(this_command_texture_ != invalid_texture_id);

		command_list_.emplace_back(
				command_type{
					.scissor = this_command_scissor_,
					.texture = this_command_texture_,
					.index_offset = static_cast<size_type>(index_list_.size()),
					// set by draw_xxx
					.element_count = 0
				}
		);
	}

	auto RenderList::on_scissor_changed() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not command_list_.empty());

		const auto command_count = command_list_.size();
		const auto& [current_scissor, current_texture, current_index_offset, current_element_count] = command_list_.back();

		if (current_element_count != 0)
		{
			if (current_scissor != this_command_scissor_)
			{
				push_command();

				return;
			}
		}

		// try to merge with previous command if it matches, else use current command
		if (command_count > 1)
		{
			if (const auto& [previous_scissor, previous_texture, previous_index_offset, previous_element_count] = command_list_[command_count - 2];
				current_element_count == 0 and (this_command_scissor_ == previous_scissor and this_command_texture_ == previous_texture) and
				// sequential
				current_index_offset == previous_index_offset + previous_element_count)
			{
				command_list_.pop_back();
				return;
			}
		}

		command_list_.back().scissor = this_command_scissor_;
	}

	auto RenderList::on_texture_changed() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not command_list_.empty());

		const auto command_count = command_list_.size();
		const auto& [current_scissor, current_texture, current_index_offset, current_element_count] = command_list_.back();

		if (current_element_count != 0)
		{
			if (current_texture != this_command_texture_)
			{
				push_command();

				return;
			}
		}

		// try to merge with previous command if it matches, else use current command
		if (command_count > 1)
		{
			if (const auto& [previous_scissor, previous_texture, previous_index_offset, previous_element_count] = command_list_[command_count - 2];
				current_element_count == 0 and (this_command_scissor_ == previous_scissor and this_command_texture_ == previous_texture) and
				// sequential
				current_index_offset == previous_index_offset + previous_element_count)
			{
				command_list_.pop_back();
				return;
			}
		}

		command_list_.back().texture = this_command_texture_;
	}

	auto RenderList::shared_data() const noexcept -> const RenderListSharedData&
	{
		return context_.get().render_list_shared_data();
	}

	auto RenderList::default_texture() const noexcept -> texture_id_type
	{
		return context_.get().texture_context().root_texture();
	}

	RenderList::RenderList(const RenderListFlag flag, RendererContext& context) noexcept
		: render_list_flag_{flag},
		  context_{context},
		  this_command_scissor_{0, 0, 0, 0},
		  this_command_texture_{default_texture()}
	{
		// we always have a command ready in the buffer
		command_list_.emplace_back(
				command_type{
					.scissor = this_command_scissor_,
					.texture = this_command_texture_,
					.index_offset = static_cast<size_type>(index_list_.size()),
					// set by subsequent draw_xxx
					.element_count = 0
				}
		);
	}

	auto RenderList::render_data() const noexcept -> RenderData
	{
		return {.vertex_list = vertex_list_, .index_list = index_list_, .command_list = command_list_};
	}

	auto RenderList::push_scissor(const rect_type& rect, const bool intersect_with_current_scissor) noexcept -> rect_type&
	{
		// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not rect.empty() and rect.valid());

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not command_list_.empty());

		const auto& [current_scissor, current_texture, current_index_offset, current_element_count] = command_list_.back();

		this_command_scissor_ = intersect_with_current_scissor ? rect.combine_min(current_scissor) : rect;

		on_scissor_changed();
		return command_list_.back().scissor;
	}

	auto RenderList::pop_scissor() noexcept -> void
	{
		// at least one command
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(command_list_.size() > 1);
		this_command_scissor_ = command_list_[command_list_.size() - 2].scissor;

		on_scissor_changed();
	}

	auto RenderList::push_texture(const texture_id_type texture) noexcept -> void
	{
		this_command_texture_ = texture;

		on_texture_changed();
	}

	auto RenderList::pop_texture() noexcept -> void
	{
		// at least one command
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(command_list_.size() > 1);
		this_command_texture_ = command_list_[command_list_.size() - 2].texture;

		on_texture_changed();
	}

	// clang-format off
	auto RenderList::line(
		const point_type& from,
		const point_type& to,
		const color_type color,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_pin(from);
		drawer.path_pin(to);

		drawer.path_stroke(color, RenderFlag::NONE, thickness);
	}

	// clang-format off
	auto RenderList::triangle(
		const point_type& a,
		const point_type& b,
		const point_type& c,
		const color_type color,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_pin(a);
		drawer.path_pin(b);
		drawer.path_pin(c);

		drawer.path_stroke(color, RenderFlag::CLOSED, thickness);
	}

	// clang-format off
	auto RenderList::triangle_filled(
		const point_type& a,
		const point_type& b,
		const point_type& c,
		const color_type color
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_pin(a);
		drawer.path_pin(b);
		drawer.path_pin(c);

		drawer.path_stroke(color);
	}

	// clang-format off
	auto RenderList::rect(
		const rect_type& rect,
		const color_type color,
		const float rounding,
		const RenderFlag flag,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_rect(rect, rounding, flag);

		drawer.path_stroke(color, RenderFlag::CLOSED, thickness);
	}

	// clang-format off
	auto RenderList::rect(
		const point_type& left_top,
		const point_type& right_bottom,
		const color_type color,
		const float rounding,
		const RenderFlag flag,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		return rect({left_top, right_bottom}, color, rounding, flag, thickness);
	}

	// clang-format off
	auto RenderList::rect_filled(
		const rect_type& rect,
		const color_type color,
		const float rounding,
		const RenderFlag flag
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		if (rounding < .5f or (RenderFlag::ROUND_CORNER_MASK & flag) == RenderFlag::ROUND_CORNER_NONE)
		{
			drawer.draw_rect_filled(rect, color, color, color, color);
		}
		else
		{
			drawer.path_rect(rect, rounding, flag);
			drawer.path_stroke(color);
		}
	}

	// clang-format off
	auto RenderList::rect_filled(
		const point_type& left_top,
		const point_type& right_bottom,
		const color_type color,
		const float rounding,
		const RenderFlag flag
	) noexcept -> void
	// clang-format on
	{
		return rect_filled({left_top, right_bottom}, color, rounding, flag);
	}

	// clang-format off
	auto RenderList::rect_filled(
		const rect_type& rect,
		const color_type color_left_top,
		const color_type color_right_top,
		const color_type color_left_bottom,
		const color_type color_right_bottom
	) noexcept -> void
	// clang-format on
	{
		if (color_left_top.alpha == 0 or color_right_top.alpha == 0 or color_left_bottom.alpha == 0 or color_right_bottom.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.draw_rect_filled(rect, color_left_top, color_right_top, color_left_bottom, color_right_bottom);
	}

	// clang-format off
	auto RenderList::rect_filled(
		const point_type& left_top,
		const point_type& right_bottom,
		const color_type color_left_top,
		const color_type color_right_top,
		const color_type color_left_bottom,
		const color_type color_right_bottom
	) noexcept -> void
	// clang-format on
	{
		return rect_filled({left_top, right_bottom}, color_left_top, color_right_top, color_left_bottom, color_right_bottom);
	}

	// clang-format off
	auto RenderList::quadrilateral(
		const point_type& p1,
		const point_type& p2,
		const point_type& p3,
		const point_type& p4,
		const color_type color,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_quadrilateral(p1, p2, p3, p4);

		drawer.path_stroke(color, RenderFlag::CLOSED, thickness);
	}

	// clang-format off
	auto RenderList::quadrilateral_filled(
		const point_type& p1,
		const point_type& p2,
		const point_type& p3,
		const point_type& p4,
		const color_type color
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_quadrilateral(p1, p2, p3, p4);

		drawer.path_stroke(color);
	}

	// clang-format off
	auto RenderList::circle_n(
		const circle_type& circle,
		const color_type color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0 or circle.radius < .5f or segments < 3)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_arc_n(circle, 0, std::numbers::pi_v<float> * 2, segments);

		drawer.path_stroke(color, RenderFlag::CLOSED, thickness);
	}

	// clang-format off
	auto RenderList::circle_n(
		const point_type& center,
		const float radius,
		const color_type color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		return circle_n({center, radius}, color, segments, thickness);
	}

	// clang-format off
	auto RenderList::ellipse_n(
		const ellipse_type& ellipse,
		const color_type color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f or segments < 3)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_arc_elliptical_n(ellipse, 0, std::numbers::pi_v<float> * 2, segments);

		drawer.path_stroke(color, RenderFlag::CLOSED, thickness);
	}

	// clang-format off
	void RenderList::ellipse_n(
		const point_type& center,
		const extent_type& radius,
		const float rotation,
		const color_type color,
		const std::uint32_t segments,
		const float thickness
	) noexcept
	// clang-format on
	{
		return ellipse_n({center, radius, rotation}, color, segments, thickness);
	}

	// clang-format off
	auto RenderList::circle_n_filled(
		const circle_type& circle,
		const color_type color,
		const std::uint32_t segments
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0 or circle.radius < .5f or segments < 3)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_arc_n(circle, 0, std::numbers::pi_v<float> * 2, segments);

		drawer.path_stroke(color);
	}

	// clang-format off
	auto RenderList::circle_n_filled(
		const point_type& center,
		const float radius,
		const color_type color,
		const std::uint32_t segments
	) noexcept -> void
	// clang-format on
	{
		return circle_n_filled({center, radius}, color, segments);
	}

	// clang-format off
	auto RenderList::ellipse_n_filled(
		const ellipse_type& ellipse,
		const color_type color,
		const std::uint32_t segments
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f or segments < 3)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_arc_elliptical_n(ellipse, 0, std::numbers::pi_v<float> * 2, segments);

		drawer.path_stroke(color);
	}

	// clang-format off
	auto RenderList::ellipse_n_filled(
		const point_type& center,
		const extent_type& radius,
		const float rotation,
		const color_type color,
		const std::uint32_t segments
	) noexcept -> void
	// clang-format on
	{
		return ellipse_n_filled({center, radius, rotation}, color, segments);
	}

	// clang-format off
	void RenderList::circle(
		const circle_type& circle,
		const color_type color,
		const std::uint32_t segments,
		const float thickness
	) noexcept
	// clang-format on
	{
		if (color.alpha == 0 or circle.radius < .5f)
		{
			return;
		}

		if (segments == 0)
		{
			Drawer drawer{.self = *this, .path_list = {}};

			drawer.path_arc_fast(circle, 0, RenderListSharedData::vertex_sample_points_count - 1);

			drawer.path_stroke(color, RenderFlag::CLOSED, thickness);
		}
		else
		{
			const auto clamped_segments = std::ranges::clamp(segments, RenderListSharedData::circle_segments_min, RenderListSharedData::circle_segments_max);

			circle_n(circle, color, clamped_segments, thickness);
		}
	}

	// clang-format off
	auto RenderList::circle(
		const point_type& center,
		const float radius,
		const color_type color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		return circle({center, radius}, color, segments, thickness);
	}

	// clang-format off
	auto RenderList::circle_filled(
		const circle_type& circle,
		const color_type color,
		const std::uint32_t segments
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0 or circle.radius < .5f)
		{
			return;
		}

		if (segments == 0)
		{
			Drawer drawer{.self = *this, .path_list = {}};

			drawer.path_arc_fast(circle, 0, RenderListSharedData::vertex_sample_points_count - 1);

			drawer.path_stroke(color);
		}
		else
		{
			const auto clamped_segments = std::ranges::clamp(segments, RenderListSharedData::circle_segments_min, RenderListSharedData::circle_segments_max);

			circle_n_filled(circle, color, clamped_segments);
		}
	}

	// clang-format off
	auto RenderList::circle_filled(
		const point_type& center,
		const float radius,
		const color_type color,
		const std::uint32_t segments
	) noexcept -> void
	// clang-format on
	{
		circle_filled({center, radius}, color, segments);
	}

	// clang-format off
	auto RenderList::ellipse(
		const ellipse_type& ellipse,
		const color_type color,
		std::uint32_t segments,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f)
		{
			return;
		}

		if (segments == 0)
		{
			// fixme
			segments = shared_data().circle_auto_segment_count(std::ranges::max(ellipse.radius.width, ellipse.radius.height));
		}

		ellipse_n(ellipse, color, segments, thickness);
	}

	// clang-format off
	auto RenderList::ellipse(
		const point_type& center,
		const extent_type& radius,
		const float rotation,
		const color_type color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		return ellipse({center, radius, rotation}, color, segments, thickness);
	}

	// clang-format off
	auto RenderList::ellipse_filled(
		const ellipse_type& ellipse,
		const color_type color,
		std::uint32_t segments
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0 or ellipse.radius.width < .5f or ellipse.radius.height < .5f)
		{
			return;
		}

		if (segments == 0)
		{
			// fixme
			segments = shared_data().circle_auto_segment_count(std::ranges::max(ellipse.radius.width, ellipse.radius.height));
		}

		ellipse_n_filled(ellipse, color, segments);
	}

	// clang-format off
	auto RenderList::ellipse_filled(
		const point_type& center,
		const extent_type& radius,
		const float rotation,
		const color_type color,
		const std::uint32_t segments
	) noexcept -> void
	// clang-format on
	{
		return ellipse_filled({center, radius, rotation}, color, segments);
	}

	// clang-format off
	auto RenderList::bezier_cubic(
		const point_type& p1,
		const point_type& p2,
		const point_type& p3,
		const point_type& p4,
		const color_type color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_bezier_curve(p1, p2, p3, p4, segments);

		drawer.path_stroke(color, RenderFlag::NONE, thickness);
	}

	// clang-format off
	auto RenderList::bezier_quadratic(
		const point_type& p1,
		const point_type& p2,
		const point_type& p3,
		const color_type color,
		const std::uint32_t segments,
		const float thickness
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.path_bezier_quadratic_curve(p1, p2, p3, segments);

		drawer.path_stroke(color, RenderFlag::NONE, thickness);
	}

	// clang-format off
	auto RenderList::text(
		const std::string_view utf8_text,
		const std::uint32_t font_size,
		const point_type& point,
		const color_type color,
		const float wrap_width
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(wrap_width > 0);

		Drawer drawer{.self = *this, .path_list = {}};
		drawer.draw_text(utf8_text, font_size, point, color, wrap_width);
	}

	// clang-format off
	auto RenderList::image(
		const texture_id_type texture_id,
		const point_type& display_p1,
		const point_type& display_p2,
		const point_type& display_p3,
		const point_type& display_p4,
		const uv_type& uv_p1,
		const uv_type& uv_p2,
		const uv_type& uv_p3,
		const uv_type& uv_p4,
		const color_type color
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.draw_image(texture_id, display_p1, display_p2, display_p3, display_p4, uv_p1, uv_p2, uv_p3, uv_p4, color);
	}

	// clang-format off
	auto RenderList::image(
		const texture_id_type texture_id,
		const rect_type& display_rect,
		const rect_type& uv_rect,
		const color_type color
	) noexcept -> void
	// clang-format on
	{
		image(texture_id,
			  display_rect.left_top(),
			  display_rect.right_top(),
			  display_rect.right_bottom(),
			  display_rect.left_bottom(),
			  uv_rect.left_top(),
			  uv_rect.right_top(),
			  uv_rect.right_bottom(),
			  uv_rect.left_bottom(),
			  color);
	}

	// clang-format off
	auto RenderList::image(
		const texture_id_type texture_id,
		const point_type& display_left_top,
		const point_type& display_right_bottom,
		const uv_type& uv_left_top,
		const uv_type& uv_right_bottom,
		const color_type color
	) noexcept -> void
	// clang-format on
	{
		image(texture_id, {display_left_top, display_right_bottom}, {uv_left_top, uv_right_bottom}, color);
	}

	// clang-format off
	auto RenderList::image_rounded(
		const texture_id_type texture_id,
		const rect_type& display_rect,
		const float rounding,
		const RenderFlag flag,
		const rect_type& uv_rect,
		const color_type color
	) noexcept -> void
	// clang-format on
	{
		if (color.alpha == 0)
		{
			return;
		}

		Drawer drawer{.self = *this, .path_list = {}};

		drawer.draw_image_rounded(texture_id, display_rect, uv_rect, color, rounding, flag);
	}

	// clang-format off
	auto RenderList::image_rounded(
		const texture_id_type texture_id,
		const point_type& display_left_top,
		const point_type& display_right_bottom,
		const float rounding,
		const RenderFlag flag,
		const uv_type& uv_left_top,
		const uv_type& uv_right_bottom,
		const color_type color
	) noexcept -> void
	// clang-format on
	{
		image_rounded(texture_id, {display_left_top, display_right_bottom}, rounding, flag, {uv_left_top, uv_right_bottom}, color);
	}
} // namespace gal::prometheus::gfx
