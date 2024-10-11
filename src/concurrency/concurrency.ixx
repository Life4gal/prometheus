// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:concurrency;

export import :concurrency.thread;
export import :concurrency.unfair_mutex;
export import :concurrency.queue;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <concurrency/thread.ixx>
#include <concurrency/unfair_mutex.ixx>
#include <concurrency/queue.ixx>

#endif
