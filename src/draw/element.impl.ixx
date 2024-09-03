// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.impl;

import std;

GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

import :surface;
import :style;
import :element;

#else
#include <prometheus/macro.hpp>
#include <draw/surface.ixx>
#include <draw/style.ixx>
#include <draw/element.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

namespace gal::prometheus::draw
{
	auto Surface::render(impl::Element& element) noexcept -> void
	{
		// todo
		element.calculate_requirement(*this);
		element.set_rect({rect_.point.x, rect_.point.y, rect_.point.x + element.requirement().min_width, rect_.point.y + element.requirement().min_height});
		element.render(*this);
	}

	auto Style::instance() noexcept -> Style&
	{
		static Style style{
				.font_pixel_size = 18.f,
				.line_pixel_width = 1.f,
				.flex_pixel_x = 3.f,
				.flex_pixel_y = 3.f,
				.border_default_color = primitive::colors::black,
				.window_title_default_color = primitive::colors::red
		};

		return style;
	}

	namespace impl
	{
		Element::~Element() noexcept = default;
	}
}
