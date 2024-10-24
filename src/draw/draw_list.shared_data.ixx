// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.draw_list.shared_data;

import std;

import :primitive;
import :functional;
#if GAL_PROMETHEUS_COMPILER_DEBUG
import :error;
#endif

import :draw.font;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>
#include <functional/functional.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <draw/font.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
{
	class DrawListSharedData final
	{
	public:
		using rect_type = primitive::basic_rect_2d<float, float>;
		using point_type = rect_type::point_type;
		using extent_type = rect_type::extent_type;

		using circle_type = primitive::basic_circle_2d<float, float>;
		using ellipse_type = primitive::basic_ellipse_2d<float, float, float>;

		using uv_type = primitive::basic_point_2d<float>;
		using color_type = primitive::basic_color<std::uint8_t>;
		using vertex_type = primitive::basic_vertex<point_type, uv_type, color_type>;
		using index_type = std::uint16_t;

		constexpr static std::size_t circle_segment_counts_count = 64;
		using circle_segment_counts_type = std::array<std::uint8_t, circle_segment_counts_count>;

		constexpr static std::uint32_t circle_segments_min = 4;
		constexpr static std::uint32_t circle_segments_max = 512;

		constexpr static std::size_t vertex_sample_points_count = 48;
		using vertex_sample_points_type = std::array<point_type, vertex_sample_points_count>;

		constexpr static auto vertex_sample_points = []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> vertex_sample_points_type
		{
			constexpr auto make_point = []<std::size_t I>() noexcept -> point_type
			{
				const auto a = static_cast<float>(I) / static_cast<float>(vertex_sample_points_type{}.size()) * 2 * std::numbers::pi_v<float>;
				return {functional::cos(a), -functional::sin(a)};
			};

			return {{make_point.template operator()<Index>()...}};
		}(std::make_index_sequence<vertex_sample_points_count>{});

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

		Font default_font_;

		// @see https://stackoverflow.com/a/2244088/15194693
		// Number of segments (N) is calculated using equation:
		//	N = ceil ( pi / acos(1 - error / r) ) where r > 0 and error <= r
		[[nodiscard]] constexpr static auto circle_segments_calc(const float radius, const float max_error) noexcept -> auto
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

		[[nodiscard]] constexpr static auto circle_segments_calc_radius(const std::size_t n, const float max_error) noexcept -> auto
		{
			return max_error / (1 - functional::cos(std::numbers::pi_v<float> / std::ranges::max(static_cast<float>(n), std::numbers::pi_v<float>)));
		}

		[[nodiscard]] constexpr static auto circle_segments_calc_error(const std::size_t n, const float radius) noexcept -> auto
		{
			return (1 - functional::cos(std::numbers::pi_v<float> / std::ranges::max(static_cast<float>(n), std::numbers::pi_v<float>))) / radius;
		}

	public:
		DrawListSharedData() noexcept
			:
			circle_segment_counts_{},
			circle_segment_max_error_{},
			arc_fast_radius_cutoff_{},
			curve_tessellation_tolerance_{1.25f}
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
				circle_segment_counts_[i] = static_cast<std::uint8_t>(circle_segments_calc(radius, max_error));
			}
			circle_segment_max_error_ = max_error;
			arc_fast_radius_cutoff_ = circle_segments_calc_radius(vertex_sample_points_count, max_error);
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
			return static_cast<circle_segment_counts_type::value_type>(circle_segments_calc(radius, circle_segment_max_error_));
		}

		[[nodiscard]] auto load_default_font(const std::string_view font_path, const std::uint32_t pixel_height, const glyph_ranges_view_type glyph_ranges) noexcept -> Font::texture_type
		{
			return default_font_.load(font_path, pixel_height, glyph_ranges);
		}

		[[nodiscard]] auto load_default_font(const std::string_view font_path, const std::uint32_t pixel_height, const glyph_ranges_type& glyph_ranges) noexcept -> Font::texture_type
		{
			return load_default_font(font_path, pixel_height, {glyph_ranges.data(), glyph_ranges.size()});
		}

		[[nodiscard]] auto load_default_font(const std::string_view font_path, const std::uint32_t pixel_height, const glyph_range_type& glyph_range) noexcept -> Font::texture_type
		{
			return load_default_font(font_path, pixel_height, {&glyph_range, 1});
		}

		[[nodiscard]] auto load_default_font(const std::string_view font_path, const std::uint32_t pixel_height, const glyph_range_views_type glyph_ranges) noexcept -> Font::texture_type
		{
			return default_font_.load(font_path, pixel_height, glyph_ranges);
		}

		[[nodiscard]] auto load_default_font(const std::string_view font_path, const std::uint32_t pixel_height, const glyph_range_view_type glyph_range) noexcept -> Font::texture_type
		{
			return load_default_font(font_path, pixel_height, {&glyph_range, 1});
		}

		[[nodiscard]] constexpr auto get_default_font() noexcept -> Font&
		{
			return default_font_;
		}
		
		[[nodiscard]] constexpr auto get_default_font() const noexcept -> const Font&
		{
			return default_font_;
		}
	};

	enum class ArcQuadrant : std::uint8_t
	{
		// [0~3)
		Q1 = 1 << 0,
		// [3~6)
		Q2 = 1 << 1,
		// [6~9)
		Q3 = 1 << 2,
		// [9~12)
		Q4 = 1 << 3,

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
		Q1_CLOCK_WISH = 1 << 4,
		// [6, 3)
		Q2_CLOCK_WISH = 1 << 5,
		// [9, 6)
		Q3_CLOCK_WISH = 1 << 6,
		// [12, 9)
		Q4_CLOCK_WISH = 1 << 7,

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

	[[nodiscard]] constexpr auto range_of_arc_quadrant(const ArcQuadrant quadrant) noexcept -> std::pair<int, int>
	{
		constexpr auto factor = static_cast<int>(DrawListSharedData::vertex_sample_points.size() / 12);

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

		GAL_PROMETHEUS_ERROR_UNREACHABLE();
	}
}
