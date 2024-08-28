// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:dynamic_node.impl;

import std;

import :dynamic_node;

#else
#include <prometheus/macro.hpp>
#include <draw/dynamic_node.ixx>

#endif

namespace
{
	using namespace gal::prometheus::draw;

	class TrivialMouse final : public impl::Mouse {};
}

namespace gal::prometheus::draw::impl
{
	Mouse::~Mouse() noexcept = default;

	auto DynamicNode::try_capture_mouse(const event_type event) noexcept -> mouse_type
	{
		// todo
		(void)event;
		return std::make_unique<TrivialMouse>();
	}
}
