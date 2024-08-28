// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:node.impl;

import std;

import :node;

#else
#include <prometheus/macro.hpp>
#include <draw/node.ixx>

#endif

namespace gal::prometheus::draw::impl
{
	Node::~Node() noexcept = default;
}
