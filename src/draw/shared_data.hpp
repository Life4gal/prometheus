// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <draw/def.hpp>

namespace gal::prometheus::draw
{
	class DrawListSharedData final
	{
	public:
		using rect_type = DrawListDef::rect_type;
		using point_type = DrawListDef::point_type;
		using extent_type = DrawListDef::extent_type;

		using circle_segment_count_type = std::uint8_t;
		constexpr static std::size_t circle_segment_counts_count = 64;
		using circle_segment_counts_type = std::array<circle_segment_count_type, circle_segment_counts_count>;

		constexpr static std::uint32_t circle_segments_min = 4;
		constexpr static std::uint32_t circle_segments_max = 512;

		constexpr static std::size_t vertex_sample_points_count = 48;
		using vertex_sample_points_type = std::array<point_type, vertex_sample_points_count>;

	private:
		circle_segment_counts_type circle_segment_counts_;
		vertex_sample_points_type vertex_sample_points_;

		// Maximum error (in pixels) allowed when using `circle`/`circle_filled` or drawing rounded corner rectangles with no explicit segment count specified.
		// Decrease for higher quality but more geometry.
		float circle_segment_max_error_;
		// Cutoff radius after which arc drawing will fall back to slower `path_arc`
		float arc_fast_radius_cutoff_;
		// Tessellation tolerance when using `path_bezier_curve` without a specific number of segments.
		// Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
		float curve_tessellation_tolerance_;

	public:
		DrawListSharedData() noexcept;

		// --------------------------------------------------

		[[nodiscard]] auto get_circle_auto_segment_count(float radius) const noexcept -> circle_segment_count_type;

		[[nodiscard]] auto get_vertex_sample_point(std::size_t index) const noexcept -> const point_type&;

		[[nodiscard]] auto get_circle_tessellation_max_error() const noexcept -> float;

		[[nodiscard]] auto get_arc_fast_radius_cutoff() const noexcept -> float;

		[[nodiscard]] auto get_curve_tessellation_tolerance() const noexcept -> float;

		// --------------------------------------------------

		auto set_circle_tessellation_max_error(float max_error) noexcept -> void;

		auto set_curve_tessellation_tolerance(float tolerance) noexcept -> void;
	};
}
