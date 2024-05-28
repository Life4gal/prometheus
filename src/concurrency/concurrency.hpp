// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.concurrency;

export import :thread;
export import :unfair_mutex;

#else
#include <concurrency/thread.hpp>
#include <concurrency/unfair_mutex.hpp>
#endif
