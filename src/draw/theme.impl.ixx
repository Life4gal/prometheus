// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.theme.impl;

import std;

import :primitive;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#include <prometheus/macro.hpp>
#include <draw/theme.ixx>
#include <primitive/primitive.ixx>

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
		colors[static_cast<std::size_t>(ThemeCategory::BORDER)] = primitive::colors::red;

		colors[static_cast<std::size_t>(ThemeCategory::BACKGROUND_WINDOW)] = primitive::colors::blue;
		colors[static_cast<std::size_t>(ThemeCategory::BACKGROUND_WIDGET)] = primitive::colors::green;

		colors[static_cast<std::size_t>(ThemeCategory::TITLE_BAR)] = primitive::colors::red;
		colors[static_cast<std::size_t>(ThemeCategory::TITLE_BAR_COLLAPSED)] = primitive::colors::red;

		colors[static_cast<std::size_t>(ThemeCategory::SLIDER)] = primitive::colors::red;
		colors[static_cast<std::size_t>(ThemeCategory::SLIDER_ACTIVATED)] = primitive::colors::red;

		colors[static_cast<std::size_t>(ThemeCategory::BUTTON)] = primitive::colors::red;
		colors[static_cast<std::size_t>(ThemeCategory::BUTTON_HOVERED)] = primitive::colors::red;
		colors[static_cast<std::size_t>(ThemeCategory::BUTTON_ACTIVATED)] = primitive::colors::red;

		colors[static_cast<std::size_t>(ThemeCategory::RESIZE_GRIP)] = primitive::colors::gold;
		colors[static_cast<std::size_t>(ThemeCategory::RESIZE_GRIP_HOVERED)] = primitive::colors::gold;
		colors[static_cast<std::size_t>(ThemeCategory::RESIZE_GRIP_ACTIVATED)] = primitive::colors::gold;

		colors[static_cast<std::size_t>(ThemeCategory::TOOLTIP_BACKGROUND)] = primitive::colors::black;
		colors[static_cast<std::size_t>(ThemeCategory::TOOLTIP_TEXT)] = primitive::colors::yellow;

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
				#error "fixme
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
