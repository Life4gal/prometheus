// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.error:command_line;

import std;

#else
#pragma once

#include <span>

#include <prometheus/macro.hpp>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::error)
{
	auto command_line_args_count() noexcept -> int;

	auto command_line_args() noexcept -> std::span<const char* const>;
} // namespace gal::prometheus::error
