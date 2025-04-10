// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gui/gui.hpp>

#include <primitive/circle.hpp>
#include <primitive/ellipse.hpp>

#include <memory/reference_wrapper.hpp>
#include <functional/hash.hpp>

namespace gal::prometheus::gui::internal
{
	using circle_type = primitive::basic_circle_2d<float, float>;
	using ellipse_type = primitive::basic_ellipse_2d<float, float, float>;

	using frame_count_type = std::uint32_t;
	// The number of frames is always counted from 1(call new_frame first),
	// so we can set the number of uninitialized frames to the maximum value,
	// and its +1 overflows to 0, then we can compare it with current frame count normally
	constexpr static auto frame_count_uninitialized{std::numeric_limits<frame_count_type>::max()};

	using widget_id_type = functional::hash_result_type;
	constexpr auto invalid_widget_id = std::numeric_limits<widget_id_type>::max();

	class [[nodiscard]] DrawListSharedData final
	{
	public:
		using circle_segment_count_type = std::uint8_t;
		constexpr static std::size_t circle_segment_counts_count = 64;
		using circle_segment_counts_type = std::array<circle_segment_count_type, circle_segment_counts_count>;

		constexpr static std::uint32_t circle_segments_min = 4;
		constexpr static std::uint32_t circle_segments_max = 512;

		constexpr static std::size_t vertex_sample_points_count = 48;
		using vertex_sample_points_type = std::array<point_type, vertex_sample_points_count>;

		circle_segment_counts_type circle_segment_counts;
		vertex_sample_points_type vertex_sample_points;

		// Maximum error (in pixels) allowed when using `circle`/`circle_filled` or drawing rounded corner rectangles with no explicit segment count specified.
		// Decrease for higher quality but more geometry.
		float circle_segment_max_error;
		// Cutoff radius after which arc drawing will fall back to slower `path_arc`
		float arc_fast_radius_cutoff;
		// Tessellation tolerance when using `path_bezier_curve` without a specific number of segments.
		// Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
		float curve_tessellation_tolerance;

		DrawListSharedData() noexcept;

		// --------------------------------------------------

		[[nodiscard]] auto circle_auto_segment_count(float radius) const noexcept -> circle_segment_count_type;

		[[nodiscard]] auto vertex_sample_point(std::size_t index) const noexcept -> const point_type&;

		// --------------------------------------------------

		auto set_circle_tessellation_max_error(float max_error) noexcept -> void;

		auto set_curve_tessellation_tolerance(float tolerance) noexcept -> void;
	};

	class [[nodiscard]] PrimitiveAppender final
	{
	public:
		using size_type = DrawData::size_type;

		using command_type = DrawData::command_type;

		using vertex_list_type = DrawData::vertex_list_type;
		using index_list_type = DrawData::index_list_type;

	private:
		// command_type::element_count
		memory::RefWrapper<size_type> element_count_;
		// DrawList::vertex_list
		memory::RefWrapper<vertex_list_type> vertex_list_;
		// DrawList::index_list_;
		memory::RefWrapper<index_list_type> index_list_;

	public:
		PrimitiveAppender(
			command_type& command,
			vertex_list_type& vertex_list,
			index_list_type& index_list
		) noexcept;

		[[nodiscard]] auto vertex_count() const noexcept -> size_type;

		auto reserve(size_type vertex_count, size_type index_count) noexcept -> void;

		auto reserve(std::size_t vertex_count, std::size_t index_count) noexcept -> void;

		auto add_vertex(const point_type& point, const uv_type& uv, color_type color) noexcept -> void;

		auto add_index(index_type a, index_type b, index_type c) noexcept -> void;
	};
}
