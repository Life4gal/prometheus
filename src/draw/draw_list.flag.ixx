// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.draw_list.flag;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <cstdint>

#include <prometheus/macro.hpp>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
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
}
