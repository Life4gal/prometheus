// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.numeric;

export import :random_engine;
export import :random;

#else
#pragma once

#include <numeric/random_engine.ixx>
#include <numeric/random.ixx>

#endif
