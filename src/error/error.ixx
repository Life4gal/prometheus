// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.error;

export import :exception;
export import :debug;
export import :platform;
export import :command_line;

#else
#pragma once

#include <error/exception.ixx>
#include <error/debug.ixx>
#include <error/platform.ixx>
#include <error/command_line.ixx>

#endif
