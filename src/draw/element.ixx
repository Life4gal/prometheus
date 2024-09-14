// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element;

import gal.prometheus.functional;

export import :element.element;
export import :element.text;
export import :element.separator;
export import :element.border;
export import :element.boundary;
export import :element.flex;
export import :element.box;
export import :element.flex_box;
export import :element.gauge;

#else
#pragma once

#include <functional/functional.ixx>

#include <draw/detail/element.ixx>
#include <draw/detail/element.text.ixx>
#include <draw/detail/element.separator.ixx>
#include <draw/detail/element.border.ixx>
#include <draw/detail/element.boundary.ixx>
#include <draw/detail/element.flex.ixx>
#include <draw/detail/element.box.ixx>
#include <draw/detail/element.flex_box.ixx>
#include <draw/detail/element.gauge.ixx>

#endif
