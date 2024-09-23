// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.concurrency;

export import :thread;
export import :unfair_mutex;
export import :queue;

#else
#pragma once

#include <concurrency/thread.ixx>
#include <concurrency/unfair_mutex.ixx>
#include <concurrency/queue.ixx>

#endif
