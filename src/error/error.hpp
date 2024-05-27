// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.error;

export import :debug;
import :debug.impl;
export import :exception;
export import :platform;

#else
#include <error/debug.hpp>
#include <error/debug_win.hpp>
#include <error/exception.hpp>
#include <error/platform.hpp>
#endif
