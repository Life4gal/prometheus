// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.coroutine;

export import :task;
export import :generator;

#else
#pragma once

#include <coroutine/task.ixx>
#include <coroutine/generator.ixx>

#endif
