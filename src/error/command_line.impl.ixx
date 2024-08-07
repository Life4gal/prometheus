// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>
#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <cstdlib>
#endif

export module gal.prometheus.error:command_line.impl;

import std;
import :command_line;

#else

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <cstdlib>
#endif

#include <prometheus/macro.hpp>
#include <error/command_line.ixx>

#endif

namespace
{
	#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
	const int g_argc = *__p___argc();
	const char* const* g_argv = *__p___argv();
	#else
	int g_argc = 0;
	const char** g_argv = nullptr;

	__attribute__((constructor)) auto do_read_command_line(const int argc, const char* argv[]) -> void
	{
		g_argc = argc;
		g_argv = argv;
	}
	#endif
}

namespace gal::prometheus::error
{
	auto command_line_args_count() noexcept -> int
	{
		return g_argc;
	}

	auto command_line_args() noexcept -> std::span<const char* const>
	{
		return {g_argv, static_cast<std::span<const char*>::size_type>(command_line_args_count())};
	}
}
