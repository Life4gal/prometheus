// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:platform.debug;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <prometheus/macro.hpp>

#endif

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: platform
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(platform)
#endif
{
	auto debug_break(const char* message) noexcept -> void;
} // namespace gal::prometheus::platform
