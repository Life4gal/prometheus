// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <draw/shared_data.hpp>

#include <algorithm>
#include <ranges>

#include <math/cmath.hpp>

#include <draw/flag.hpp>

namespace
{
	using namespace gal::prometheus;
	using namespace draw;

	constexpr auto circle_segments_min = DrawListSharedData::circle_segments_min;
	constexpr auto circle_segments_max = DrawListSharedData::circle_segments_max;

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
			circle_segments_min,
			circle_segments_max
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
	[[nodiscard]] constexpr auto vertex_sample_points_calc() noexcept -> DrawListSharedData::vertex_sample_points_type
	{
		return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> DrawListSharedData::vertex_sample_points_type
		{
			constexpr auto make_point = []<std::size_t I>() noexcept -> DrawListSharedData::point_type
			{
				const auto a = static_cast<float>(I) / static_cast<float>(N) * 2 * std::numbers::pi_v<float>;
				return {math::cos(a), -math::sin(a)};
			};

			return {{make_point.template operator()<Index>()...}};
		}(std::make_index_sequence<N>{});
	}
}

namespace gal::prometheus::draw
{
	[[nodiscard]] auto range_of_arc_quadrant(const DrawArcFlag quadrant) noexcept -> std::pair<int, int>
	{
		static_assert(DrawListSharedData::vertex_sample_points_count % 12 == 0);
		constexpr auto factor = static_cast<int>(DrawListSharedData::vertex_sample_points_count / 12);

		switch (quadrant)
		{
			case DrawArcFlag::Q1: { return std::make_pair(0 * factor, 3 * factor); }
			case DrawArcFlag::Q2: { return std::make_pair(3 * factor, 6 * factor); }
			case DrawArcFlag::Q3: { return std::make_pair(6 * factor, 9 * factor); }
			case DrawArcFlag::Q4: { return std::make_pair(9 * factor, 12 * factor); }
			case DrawArcFlag::TOP: { return std::make_pair(0 * factor, 6 * factor); }
			case DrawArcFlag::BOTTOM: { return std::make_pair(6 * factor, 12 * factor); }
			case DrawArcFlag::LEFT: { return std::make_pair(3 * factor, 9 * factor); }
			case DrawArcFlag::RIGHT: { return std::make_pair(9 * factor, 15 * factor); }
			case DrawArcFlag::ALL: { return std::make_pair(0 * factor, 12 * factor); }
			case DrawArcFlag::Q1_CLOCK_WISH: { return std::make_pair(3 * factor, 0 * factor); }
			case DrawArcFlag::Q2_CLOCK_WISH: { return std::make_pair(6 * factor, 3 * factor); }
			case DrawArcFlag::Q3_CLOCK_WISH: { return std::make_pair(9 * factor, 6 * factor); }
			case DrawArcFlag::Q4_CLOCK_WISH: { return std::make_pair(12 * factor, 9 * factor); }
			case DrawArcFlag::TOP_CLOCK_WISH: { return std::make_pair(6 * factor, 0 * factor); }
			case DrawArcFlag::BOTTOM_CLOCK_WISH: { return std::make_pair(12 * factor, 6 * factor); }
			case DrawArcFlag::LEFT_CLOCK_WISH: { return std::make_pair(9 * factor, 3 * factor); }
			case DrawArcFlag::RIGHT_CLOCK_WISH: { return std::make_pair(15 * factor, 9 * factor); }
			case DrawArcFlag::ALL_CLOCK_WISH: { return std::make_pair(12 * factor, 0 * factor); }
			default: { GAL_PROMETHEUS_ERROR_UNREACHABLE(); }
		}
	}

	DrawListSharedData::DrawListSharedData() noexcept
		:
		circle_segment_counts_{},
		vertex_sample_points_{vertex_sample_points_calc<vertex_sample_points_count>()},
		circle_segment_max_error_{},
		arc_fast_radius_cutoff_{},
		curve_tessellation_tolerance_{1.25f}
	{
		set_circle_tessellation_max_error(.3f);
	}

	auto DrawListSharedData::get_circle_auto_segment_count(const float radius) const noexcept -> circle_segment_count_type
	{
		// ceil to never reduce accuracy
		if (const auto radius_index = static_cast<circle_segment_counts_type::size_type>(radius + .999999f);
			radius_index < circle_segment_counts_.size())
		{
			return circle_segment_counts_[radius_index];
		}
		return static_cast<circle_segment_count_type>(circle_segments_calc(radius, circle_segment_max_error_));
	}

	auto DrawListSharedData::get_vertex_sample_point(const std::size_t index) const noexcept -> const point_type&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < vertex_sample_points_.size());

		return vertex_sample_points_[index];
	}

	auto DrawListSharedData::get_circle_tessellation_max_error() const noexcept -> float
	{
		return circle_segment_max_error_;
	}

	auto DrawListSharedData::get_arc_fast_radius_cutoff() const noexcept -> float
	{
		return arc_fast_radius_cutoff_;
	}

	auto DrawListSharedData::get_curve_tessellation_tolerance() const noexcept -> float
	{
		return curve_tessellation_tolerance_;
	}

	auto DrawListSharedData::set_circle_tessellation_max_error(const float max_error) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(max_error > .0f);

		if (circle_segment_max_error_ == max_error) // NOLINT(clang-diagnostic-float-equal)
		{
			return;
		}

		for (decltype(circle_segment_counts_.size()) i = 0; i < circle_segment_counts_.size(); ++i)
		{
			const auto radius = static_cast<float>(i);
			circle_segment_counts_[i] = static_cast<circle_segment_count_type>(circle_segments_calc(radius, max_error));
		}
		circle_segment_max_error_ = max_error;
		arc_fast_radius_cutoff_ = circle_segments_calc_radius(vertex_sample_points_count, max_error);
	}

	auto DrawListSharedData::set_curve_tessellation_tolerance(const float tolerance) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(tolerance > .0f);

		curve_tessellation_tolerance_ = tolerance;
	}
}
