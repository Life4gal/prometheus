// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.primitive;

export import :multi_dimension;
export import :point;
export import :extent;

#else
#include <primitive/multi_dimension.hpp>
#include <primitive/point.hpp>
#include <primitive/extent.hpp>
#endif
