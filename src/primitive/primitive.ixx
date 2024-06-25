// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.primitive;

export import :multidimensional;
export import :point;
export import :extent;
export import :rect;
export import :circle;
export import :ellipse;
export import :color;
export import :vertex;
export import :vertex_list;

#else
#pragma once

#include <primitive/multidimensional.ixx>
#include <primitive/point.ixx>
#include <primitive/extent.ixx>
#include <primitive/rect.ixx>
#include <primitive/circle.ixx>
#include <primitive/ellipse.ixx>
#include <primitive/color.ixx>
#include <primitive/vertex.ixx>
#include <primitive/vertex_list.ixx>

#endif
