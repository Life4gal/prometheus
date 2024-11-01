// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:platform.command_line;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <span>

#include <prometheus/macro.hpp>

#endif

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: platform
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(platform)
#endif
{
	auto command_line_args_count() noexcept -> int;

	auto command_line_args() noexcept -> std::span<const char* const>;
} // namespace gal::prometheus::platform
