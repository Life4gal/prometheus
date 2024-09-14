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
import :element.element;

#else
#include <prometheus/macro.hpp>

#include <draw/surface.ixx>
#include <draw/style.ixx>
#include <draw/detail/element.ixx>

#endif

namespace gal::prometheus::draw
{
	auto Surface::render(const Style&style, detail::Element& element) noexcept -> void
	{
		element.calculate_requirement(style, *this);
		element.set_rect(style, {rect_.point.x, rect_.point.y, rect_.point.x + element.requirement().min_width, rect_.point.y + element.requirement().min_height});
		element.render(style, *this);
	}

	namespace detail
	{
		Element::~Element() noexcept = default;
	}
}
