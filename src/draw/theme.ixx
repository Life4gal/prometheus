// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.theme;

import std;

import :primitive;

export import :draw.def;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <string>
#include <vector>

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>

#include <draw/def.ixx>

#endif

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: draw
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
#endif
{
	class [[nodiscard]] Theme
	{
	public:
		using name_type = std::string;

		using rect_type = DrawListType::rect_type;
		using point_type = DrawListType::point_type;
		using extent_type = DrawListType::extent_type;
		using value_type = point_type::value_type;

		using color_type = DrawListType::color_type;
		using colors_type = std::array<color_type, theme_category_count>;

		name_type font_name;
		value_type font_size;
		value_type title_bar_height;
		value_type window_rounding;
		extent_type window_padding;
		extent_type window_min_size;
		extent_type resize_grip_size;
		extent_type item_spacing;
		extent_type item_inner_spacing;
		colors_type colors;

		template<ThemeCategory Category>
		[[nodiscard]] constexpr auto color() const noexcept -> color_type
		{
			return colors[static_cast<std::size_t>(Category)];
		}

		[[nodiscard]] static auto default_theme() noexcept -> Theme;
	};
}
