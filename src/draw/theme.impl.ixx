// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.theme.impl;

import std;

import :primitive;

import :draw.theme;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#include <utility>

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>

#include <draw/theme.ixx>

#endif

namespace
{
	using namespace gal::prometheus;
	using colors_type = draw::Theme::colors_type;

	[[nodiscard]] constexpr auto default_colors() noexcept -> colors_type
	{
		using draw::ThemeCategory;

		colors_type colors{};

		colors[static_cast<std::size_t>(ThemeCategory::TEXT)] = primitive::colors::black;
		colors[static_cast<std::size_t>(ThemeCategory::BORDER)] = primitive::colors::magenta;

		colors[static_cast<std::size_t>(ThemeCategory::WINDOW_BACKGROUND)] = primitive::colors::gains_boro;

		colors[static_cast<std::size_t>(ThemeCategory::WIDGET_BACKGROUND)] = primitive::colors::white;
		colors[static_cast<std::size_t>(ThemeCategory::WIDGET_ACTIVATED)] = primitive::colors::dark_salmon;

		colors[static_cast<std::size_t>(ThemeCategory::TITLE_BAR)] = primitive::colors::light_coral;
		colors[static_cast<std::size_t>(ThemeCategory::TITLE_BAR_COLLAPSED)] = primitive::colors::dark_khaki;

		colors[static_cast<std::size_t>(ThemeCategory::SLIDER)] = primitive::colors::light_blue;
		colors[static_cast<std::size_t>(ThemeCategory::SLIDER_ACTIVATED)] = primitive::colors::deep_sky_blue;

		colors[static_cast<std::size_t>(ThemeCategory::BUTTON)] = primitive::colors::sienna;
		colors[static_cast<std::size_t>(ThemeCategory::BUTTON_HOVERED)] = primitive::colors::slate_gray;
		colors[static_cast<std::size_t>(ThemeCategory::BUTTON_ACTIVATED)] = primitive::colors::steel_blue;

		colors[static_cast<std::size_t>(ThemeCategory::RESIZE_GRIP)] = primitive::colors::gold;
		colors[static_cast<std::size_t>(ThemeCategory::RESIZE_GRIP_HOVERED)] = primitive::colors::peru;
		colors[static_cast<std::size_t>(ThemeCategory::RESIZE_GRIP_ACTIVATED)] = primitive::colors::powder_blue;

		colors[static_cast<std::size_t>(ThemeCategory::TOOLTIP_BACKGROUND)] = primitive::colors::black;
		colors[static_cast<std::size_t>(ThemeCategory::TOOLTIP_TEXT)] = primitive::colors::red;

		return colors;
	}
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT_IMPL(draw)
{
	auto Theme::default_theme() noexcept -> Theme
	{
		return
		{
				#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
				.font_name = R"(C:\Windows\Fonts\msyh.ttc)",
				.font_size = 18,
				#else
				#error "fixme"
				#endif
				.title_bar_height = 20,
				.window_rounding = 0,
				.window_padding = {8, 8},
				.window_min_size = {600, 480},
				.resize_grip_size = {20, 20},
				.item_spacing = {10, 5},
				.item_inner_spacing = {5, 5},
				.colors = default_colors()
		};
	}
}
