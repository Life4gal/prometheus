// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.primitive;

export import :multidimensional;
export import :point;
export import :extent;
export import :rect;
export import :circle;
export import :color;
export import :vertex;

#else
#include <primitive/multidimensional.hpp>
#include <primitive/point.hpp>
#include <primitive/extent.hpp>
#include <primitive/rect.hpp>
#include <primitive/circle.hpp>
#include <primitive/color.hpp>
#include <primitive/vertex.hpp>
#endif
