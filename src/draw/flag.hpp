// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <limits>

#include <functional/enumeration.hpp>

namespace gal::prometheus::draw
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

		PROMETHEUS_MAGIC_ENUM_FLAG = std::numeric_limits<std::uint8_t>::max(),
	};

	enum class DrawListFlag : std::uint8_t
	{
		NONE = 0,
		ANTI_ALIASED_LINE = 1 << 0,
		ANTI_ALIASED_LINE_USE_TEXTURE = 1 << 1,
		ANTI_ALIASED_FILL = 1 << 2,

		PROMETHEUS_MAGIC_ENUM_FLAG = std::numeric_limits<std::uint8_t>::max(),
	};

	enum class DrawArcFlag : std::uint8_t
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

		PROMETHEUS_MAGIC_ENUM_FLAG = std::numeric_limits<std::uint8_t>::max(),
	};

	[[nodiscard]] auto range_of_arc_quadrant(DrawArcFlag quadrant) noexcept -> std::pair<int, int>;

	enum class ThemeCategory : std::uint8_t
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

		PROMETHEUS_MAGIC_ENUM_FLAG = std::numeric_limits<std::uint8_t>::max(),
	};
}
