// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else
#include <cerrno>
#endif

#include <prometheus/macro.hpp>

export module gal.prometheus:platform.exception.impl;

import std;

import :platform.exception;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else
#include <cerrno>
#endif

#include <string>
#include <system_error>

#include <prometheus/macro.hpp>

#include <platform/exception.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT_IMPL(platform)
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
} // namespace gal::prometheus::platform
