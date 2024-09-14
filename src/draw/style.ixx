// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:style;

import std;
import gal.prometheus.primitive;

#else
#pragma once

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw)
{
	class Style final
	{
	public:
		using extern_type = primitive::basic_extent_2d<float>;
		using color_type = primitive::colors::color_type;

		[[nodiscard]] static auto fallback() noexcept -> const Style&;

		// Font size used when drawing text
		float font_size;
		// Color used when drawing text
		color_type text_color;

		// Width when drawing line
		float line_width;

		color_type separator_color;

		// Percentage of space remaining in the flexible container
		// 
		// | <--- 100 ---> |
		// | e1          e2     |
		// 
		// e1.flex_x = 1
		// e2.flex_x = 2
		//
		// e1.width = 100 * (1 / (1 + 2))
		// e2.width = 100 * (2 / (1 + 2))
		float flex_x;
		float flex_y;

		// horizontal => height
		// vertical => width
		float gauge_size;
		color_type gauge_color;

		// Padding of the first/last element from the container boundary
		extern_type container_padding;
		// Spacing between elements in the container
		extern_type container_spacing;

		// Corner rounding when drawing borders
		float border_rounding;
		// Padding of the elements within the boundary from the border
		extern_type border_padding;
		// Color of the border when drawing the border
		color_type border_color;
		// Color of the title when drawing the window
		color_type window_title_color;
	};

	inline auto Style::fallback() noexcept -> const Style&
	{
		constexpr static Style style
		{
				.font_size = 18.f,
				.text_color = primitive::colors::black,
				.line_width = 1.f,
				.separator_color = primitive::colors::red,
				.flex_x = 1.f,
				.flex_y = 1.f,
				.gauge_size = 10.f,
				.gauge_color = primitive::colors::green,
				.container_padding = {1.f, 1.f},
				.container_spacing = {2.f, 2.f},
				.border_rounding = 2.f,
				.border_padding = {3.f, 3.f},
				.border_color = primitive::colors::black,
				.window_title_color = primitive::colors::blue_violet
		};

		return style;
	}
}
