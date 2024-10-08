// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.string;

export import :charconv;
export import :string_pool;

#else
#pragma once

#include <string/charconv.ixx>
#include <string/string_pool.ixx>

#endif
