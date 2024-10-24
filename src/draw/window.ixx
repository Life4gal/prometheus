// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.window;

import std;

import :functional;
import :primitive;

export import :draw.window.flag;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <string>

#include <prometheus/macro.hpp>
#include <functional/functional.ixx>
#include <primitive/primitive.ixx>

#include <draw/draw_list.shared_data.ixx>
#include <draw/window.flag.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
{
	class Window
	{
	public:
		using name_type = std::string;
		using id_type = functional::hash_result_type;

		using rect_type = DrawListSharedData::rect_type;
		using point_type = DrawListSharedData::point_type;
		using extent_type = DrawListSharedData::extent_type;

	private:
		name_type name_;
		id_type id_;

		WindowFlag window_flag_;
		rect_type rect_;

		bool visible_;
		bool collapse_;

		[[nodiscard]] auto get_id(std::string_view name) noexcept -> id_type;
	};
}
