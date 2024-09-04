// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.impl;

import std;

import :surface;
import :style;
import :element;

#else
#include <prometheus/macro.hpp>
#include <draw/surface.ixx>
#include <draw/style.ixx>
#include <draw/element.ixx>

#endif

namespace gal::prometheus::draw
{
	auto Surface::render(impl::Element& element) noexcept -> void
	{
		element.calculate_requirement(*this);
		element.set_rect({rect_.point.x, rect_.point.y, rect_.point.x + element.requirement().min_width, rect_.point.y + element.requirement().min_height});
		element.render(*this);
	}

	auto Style::instance() noexcept -> Style&
	{
		static Style style{
				.font_size = 18.f,
				.line_width = 1.f,
				.separator_color = primitive::colors::red,
				.flex_x = 3.f,
				.flex_y = 3.f,
				.container_padding = {1.f, 1.f},
				.container_spacing = {2.f, 2.f},
				.border_rounding = 2.f,
				.border_padding = {3.f, 3.f},
				.border_default_color = primitive::colors::black,
				.window_title_default_color = primitive::colors::blue_violet
		};

		return style;
	}

	namespace impl
	{
		Element::~Element() noexcept = default;
	}
}
