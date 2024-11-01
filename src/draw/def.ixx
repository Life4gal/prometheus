// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.def;

import std;

import :primitive;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <cstdint>
#include <utility>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <memory>

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>

#endif

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: draw
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
#endif
{
	enum class DrawFlag : std::uint8_t
	{
		NONE = 0,
		// specify that shape should be closed
		// @see DrawList::draw_polygon_line
		// @see DrawList::draw_polygon_line_aa
		// @see DrawList::path_stroke
		CLOSED = 1 << 0,
		// enable rounding left-top corner only (when rounding > 0.0f, we default to all corners)
		// @see DrawList::path_rect
		// @see DrawList::rect
		// @see DrawList::rect_filled
		ROUND_CORNER_LEFT_TOP = 1 << 1,
		// enable rounding right_top corner only (when rounding > 0.0f, we default to all corners)
		// @see DrawList::path_rect
		// @see DrawList::rect
		// @see DrawList::rect_filled
		ROUND_CORNER_RIGHT_TOP = 1 << 2,
		// enable rounding left-bottom corner only (when rounding > 0.0f, we default to all corners)
		// @see DrawList::path_rect
		// @see DrawList::rect
		// @see DrawList::rect_filled
		ROUND_CORNER_LEFT_BOTTOM = 1 << 3,
		// enable rounding right-bottom corner only (when rounding > 0.0f, we default to all corners)
		// @see DrawList::path_rect
		// @see DrawList::rect
		// @see DrawList::rect_filled
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

	enum class DrawListFlag : std::uint8_t
	{
		NONE = 0,
		ANTI_ALIASED_LINE = 1 << 0,
		ANTI_ALIASED_LINE_USE_TEXTURE = 1 << 1,
		ANTI_ALIASED_FILL = 1 << 2,
	};

	class DrawListType final
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
	};

	class FontType
	{
	public:
		using rect_type = primitive::basic_rect_2d<std::int32_t, std::uint32_t>;
		using point_type = rect_type::point_type;
		using extent_type = rect_type::extent_type;

		using uv_rect_type = primitive::basic_rect_2d<float>;
		using uv_point_type = uv_rect_type::point_type;
		using uv_extent_type = uv_rect_type::extent_type;

		using char_type = char32_t;

		using texture_id_type = std::uintptr_t;

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH
		// Variable '%1$s' is uninitialized. Always initialize a member variable (type.6).
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING(26495)
		#endif

		struct glyph_type
		{
			rect_type rect;
			uv_rect_type uv;
			float advance_x;
		};

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_POP
		#endif

		using glyphs_type = std::unordered_map<char_type, glyph_type>;

		struct [[nodiscard]] texture_type
		{
			extent_type size;
			// size.width * size.height (RGBA)
			std::unique_ptr<std::uint32_t[]> data;

			// write external
			texture_id_type& id;
		};

		using baked_line_uv_type = std::vector<uv_rect_type>;
	};

	class FontGlyphRangeBuilder
	{
	public:
		using glyph_value_type = std::uint32_t;
		using glyph_pair_type = std::pair<glyph_value_type, glyph_value_type>;

		using glyph_range_type = std::vector<glyph_pair_type>;

	private:
		glyph_range_type glyph_range_;

	public:
		template<typename Self>
		[[nodiscard]] auto glyph_range(this Self&& self) noexcept -> decltype(auto)
		{
			return std::forward<Self>(self).glyph_range_;
		}

		// Latin
		auto latin() & noexcept -> FontGlyphRangeBuilder&;
		auto latin() && noexcept -> FontGlyphRangeBuilder&&;

		// Latin + Greek and Coptic
		auto greek() & noexcept -> FontGlyphRangeBuilder&;
		auto greek() && noexcept -> FontGlyphRangeBuilder&&;

		// Latin + Half-Width + Japanese Hiragana/Katakana + set of 2500 CJK Unified Ideographs for common simplified Chinese
		auto simplified_chinese_common() & noexcept -> FontGlyphRangeBuilder&;
		auto simplified_chinese_common() && noexcept -> FontGlyphRangeBuilder&&;

		// Latin + Half-Width + Japanese Hiragana/Katakana + full set of about 21000 CJK Unified Ideographs
		auto simplified_chinese_all() & noexcept -> FontGlyphRangeBuilder&;
		auto simplified_chinese_all() && noexcept -> FontGlyphRangeBuilder&&;
	};

	class FontOption
	{
	public:
		using glyph_range_type = FontGlyphRangeBuilder::glyph_range_type;

		constexpr static std::uint32_t default_baked_line_max_width = 63;

		std::string font_path;
		glyph_range_type glyph_range;
		std::uint32_t pixel_height;
		// 0 => default_baked_line_max_width
		std::uint32_t baked_line_max_width;
	};

	class DrawListSharedData final
	{
	public:
		using rect_type = DrawListType::rect_type;
		using point_type = DrawListType::point_type;
		using extent_type = DrawListType::extent_type;

		using circle_type = DrawListType::circle_type;
		using ellipse_type = DrawListType::ellipse_type;

		// ----------------------------------------------------

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
		static_assert(DrawListSharedData::vertex_sample_points_count % 12 == 0);
		constexpr auto factor = static_cast<int>(DrawListSharedData::vertex_sample_points_count / 12);

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

	enum class ThemeCategory
	{
		TEXT = 0,
		BORDER,

		WINDOW_BACKGROUND,

		WIDGET_BACKGROUND,
		WIDGET_ACTIVATED,

		TITLE_BAR,
		TITLE_BAR_COLLAPSED,

		SLIDER,
		SLIDER_ACTIVATED,

		BUTTON,
		BUTTON_HOVERED,
		BUTTON_ACTIVATED,

		RESIZE_GRIP,
		RESIZE_GRIP_HOVERED,
		RESIZE_GRIP_ACTIVATED,

		TOOLTIP_BACKGROUND,
		TOOLTIP_TEXT,

		// -------------------------------
		INTERNAL_COUNT
	};

	constexpr auto theme_category_count = static_cast<std::size_t>(ThemeCategory::INTERNAL_COUNT);

	enum class WindowFlag : std::uint8_t
	{
		NONE = 0,

		BORDERED = 1 << 0,
		NO_TITLE_BAR = 1 << 1,
		NO_RESIZE = 1 << 2,
		NO_MOVE = 1 << 3,
	};
}
