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

		[[nodiscard]] static auto instance() noexcept -> Style&;

		// Default font size used when drawing text
		float font_size;
		// Default width when drawing line
		float line_width;

		color_type separator_color;

		float flex_x;
		float flex_y;

		// Padding of the first/last element from the container boundary
		extern_type container_padding;
		// Spacing between elements in the container
		extern_type container_spacing;

		// Corner rounding when drawing borders
		float border_rounding;
		// Padding of the elements within the boundary from the border
		extern_type border_padding;
		// Default color of the border when drawing the border
		color_type border_default_color;
		// Default color of the title when drawing the window
		color_type window_title_default_color;
	};
}
