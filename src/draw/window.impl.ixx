// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.window.impl;

import std;

import :functional;
#if GAL_PROMETHEUS_COMPILER_DEBUG
import :error;
#endif

import :draw.window;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#include <array>
#include <span>
#include <ranges>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <draw/window.ixx>
#include <functional/functional.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

namespace
{
	

}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT_IMPL(draw)
{
	auto Window::get_id(const std::string_view name) noexcept -> id_type
	{
		return functional::hash_combine_2(id_, functional::hash<std::string_view>{}(name));
	}

}
