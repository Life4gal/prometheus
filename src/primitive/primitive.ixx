// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

export module gal.prometheus:primitive;

export import :primitive.multidimensional;
export import :primitive.point;
export import :primitive.extent;
export import :primitive.rect;
export import :primitive.circle;
export import :primitive.ellipse;
export import :primitive.color;
export import :primitive.vertex;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <primitive/multidimensional.ixx>
#include <primitive/point.ixx>
#include <primitive/extent.ixx>
#include <primitive/rect.ixx>
#include <primitive/circle.ixx>
#include <primitive/ellipse.ixx>
#include <primitive/color.ixx>
#include <primitive/vertex.ixx>

#endif
