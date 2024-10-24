// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:platform;

export import :platform.exception;
export import :platform.debug;
export import :platform.command_line;
export import :platform.instruction_set;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <platform/exception.ixx>
#include <platform/debug.ixx>
#include <platform/command_line.ixx>
#include <platform/instruction_set.ixx>

#endif
