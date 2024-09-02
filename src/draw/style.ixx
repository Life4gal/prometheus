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
		using color_type = primitive::colors::color_type;

		[[nodiscard]] static auto instance() noexcept -> Style&;

		float font_pixel_size;
		float line_pixel_width;

		float flex_pixel_x;
		float flex_pixel_y;

		color_type border_default_color;
		color_type window_title_default_color;
	};
}
