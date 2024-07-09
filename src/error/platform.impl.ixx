// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>
#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else
#error "fixme"
#endif

export module gal.prometheus.error:platform.impl;

import std;
import :platform;

#else
#include <system_error>

#include <prometheus/macro.hpp>
#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else
#include <cerrno>
#endif

#endif

namespace gal::prometheus::error
{
	auto get_error_message() -> std::string
	{
		return std::system_category().message(
				#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
				static_cast<int>(GetLastError())
				#else
				errno
				#endif
				);
	}
} // namespace gal::prometheus::error
