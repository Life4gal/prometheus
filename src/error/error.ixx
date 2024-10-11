// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:error;

export import :error.exception;
export import :error.debug;
export import :error.platform;
export import :error.command_line;
export import :error.instruction_set;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <error/exception.ixx>
#include <error/debug.ixx>
#include <error/platform.ixx>
#include <error/command_line.ixx>
#include <error/instruction_set.ixx>

#endif
