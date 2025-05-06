// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/draw_list_shared_data.hpp>

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
			DrawListSharedData::circle_segments_min,
			DrawListSharedData::circle_segments_max
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

	template<std::size_t N> [[nodiscard]] constexpr auto vertex_sample_points_calc() noexcept -> DrawListSharedData::vertex_sample_points_type
	{
		return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> DrawListSharedData::vertex_sample_points_type
		{
			constexpr auto make_point = []<std::size_t I>() noexcept -> point_type
			{
				const auto a = static_cast<float>(I) / static_cast<float>(N) * 2 * std::numbers::pi_v<float>;
				return {math::cos(a), -math::sin(a)};
			};

			return {{make_point.template operator()<Index>()...}};
		}(std::make_index_sequence<N>{});
	}
}

namespace gal::prometheus::gfx
{
	DrawListSharedData::DrawListSharedData() noexcept
		: circle_segment_counts{},
		  vertex_sample_points{vertex_sample_points_calc<vertex_sample_points_count>()},
		  circle_segment_max_error{0},
		  arc_fast_radius_cutoff{0},
		  curve_tessellation_tolerance{1.25f}
	{
		set_circle_tessellation_max_error(.3f);
	}

	auto DrawListSharedData::circle_auto_segment_count(const float radius) const noexcept -> circle_segment_count_type
	{
		// ceil to never reduce accuracy
		if (const auto radius_index = static_cast<circle_segment_counts_type::size_type>(radius + .999999f);
			radius_index < circle_segment_counts.size())
		{
			return circle_segment_counts[radius_index];
		}
		return static_cast<circle_segment_count_type>(circle_segments_calc(radius, circle_segment_max_error));
	}

	auto DrawListSharedData::vertex_sample_point(const std::size_t index) const noexcept -> const point_type&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < vertex_sample_points.size());

		return vertex_sample_points[index];
	}

	auto DrawListSharedData::set_circle_tessellation_max_error(const float max_error) noexcept -> void
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

	auto DrawListSharedData::set_curve_tessellation_tolerance(const float tolerance) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(tolerance > .0f);

		curve_tessellation_tolerance = tolerance;
	}
}
