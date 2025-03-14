// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
#include <draw/theme.hpp>

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

	[[nodiscard]] constexpr auto another_colors_for_test() noexcept -> colors_type
	{
		using draw::ThemeCategory;

		colors_type colors{};

		colors[static_cast<std::size_t>(ThemeCategory::TEXT)] = primitive::colors::black;
		colors[static_cast<std::size_t>(ThemeCategory::BORDER)] = primitive::colors::magenta;

		colors[static_cast<std::size_t>(ThemeCategory::WINDOW_BACKGROUND)] = primitive::colors::pink;

		colors[static_cast<std::size_t>(ThemeCategory::WIDGET_BACKGROUND)] = primitive::colors::white;
		colors[static_cast<std::size_t>(ThemeCategory::WIDGET_ACTIVATED)] = primitive::colors::dark_salmon;

		colors[static_cast<std::size_t>(ThemeCategory::TITLE_BAR)] = primitive::colors::light_coral;
		colors[static_cast<std::size_t>(ThemeCategory::TITLE_BAR_COLLAPSED)] = primitive::colors::dark_khaki;

		colors[static_cast<std::size_t>(ThemeCategory::SLIDER)] = primitive::colors::light_blue;
		colors[static_cast<std::size_t>(ThemeCategory::SLIDER_ACTIVATED)] = primitive::colors::deep_sky_blue;

		colors[static_cast<std::size_t>(ThemeCategory::BUTTON)] = primitive::colors::sienna;
		colors[static_cast<std::size_t>(ThemeCategory::BUTTON_HOVERED)] = primitive::colors::slate_gray;
		colors[static_cast<std::size_t>(ThemeCategory::BUTTON_ACTIVATED)] = primitive::colors::steel_blue;

		colors[static_cast<std::size_t>(ThemeCategory::RESIZE_GRIP)] = primitive::colors::red;
		colors[static_cast<std::size_t>(ThemeCategory::RESIZE_GRIP_HOVERED)] = primitive::colors::yellow;
		colors[static_cast<std::size_t>(ThemeCategory::RESIZE_GRIP_ACTIVATED)] = primitive::colors::blue;

		colors[static_cast<std::size_t>(ThemeCategory::TOOLTIP_BACKGROUND)] = primitive::colors::black;
		colors[static_cast<std::size_t>(ThemeCategory::TOOLTIP_TEXT)] = primitive::colors::blue;

		return colors;
	}
}

namespace gal::prometheus::draw
{
	auto Theme::default_theme() noexcept -> Theme
	{
		return
		{
				#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
				.font_path = R"(C:\Windows\Fonts\msyh.ttc)",
				.font_size = 18,
				#else
				#error "fixme"
				#endif
				.title_bar_height = 20,
				.window_rounding = 0,
				.window_padding = {8, 8},
				.window_min_size = {640, 480},
				.resize_grip_size = {20, 20},
				.frame_padding = {4, 4},
				.item_spacing = {10, 5},
				.item_inner_spacing = {5, 5},
				.colors = default_colors()
		};
	}

	auto Theme::another_theme_for_test() noexcept -> Theme
	{
		return
		{
				#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
				.font_path = R"(C:\Windows\Fonts\msyh.ttc)",
				.font_size = 24,
				#else
				#error "fixme"
				#endif
				.title_bar_height = 40,
				.window_rounding = 5,
				.window_padding = {15, 15},
				.window_min_size = {1280, 960},
				.resize_grip_size = {40, 40},
				.frame_padding = {8, 8},
				.item_spacing = {20, 10},
				.item_inner_spacing = {10, 10},
				.colors = another_colors_for_test()
		};
	}
}
