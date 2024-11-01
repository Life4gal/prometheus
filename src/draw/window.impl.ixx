// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

#if not defined(GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG 0
#endif
#endif

export module gal.prometheus:draw.window.impl;

import std;

import :functional;
#if GAL_PROMETHEUS_COMPILER_DEBUG
import :platform;
#endif

import :draw.def;
import :draw.window;
import :draw.context;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#include <array>
#include <span>
#include <ranges>
#include <algorithm>

#include <prometheus/macro.hpp>

#if not defined(GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG 0
#endif
#endif

#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
#include <format>
#endif

#include <functional/functional.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <draw/def.ixx>
#include <draw/window.ixx>
#include <draw/context.ixx>

#endif

namespace
{
	using namespace gal::prometheus;

	using name_view_type = draw::Window::name_view_type;

	constexpr name_view_type window_widget_name_move{"@WINDOW::MOVE@"};
	constexpr name_view_type window_widget_name_resize{"@WINDOW::RESIZE@"};
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT_IMPL(draw)
{
	auto Window::get_id(const name_view_type name) noexcept -> id_type
	{
		return functional::hash_combine_2(id_, functional::hash<std::string_view>(name));
	}

	auto Window::title_bar_rect() const noexcept -> rect_type
	{
		const auto& context = Context::instance();
		const auto& theme = context.theme();

		return {rect_.left_top(), extent_type{rect_.width(), theme.title_bar_height}};
	}

	auto Window::resize_grip_rect() const noexcept -> rect_type
	{
		const auto& context = Context::instance();
		const auto& theme = context.theme();

		// RIGHT-BOTTOM-CORNER
		return {rect_.right_bottom() - theme.resize_grip_size, theme.resize_grip_size};
	}

	auto Window::canvas_rect() const noexcept -> rect_type
	{
		// const auto& context = Context::instance();
		// const auto& theme = context.theme();

		return rect_;
	}

	auto Window::make_1_test_title_bar() noexcept -> void
	{
		auto& window = *this;

		// auto& canvas = window.canvas_;
		// auto& draw_list = window.draw_list_;
		const auto flag_value = std::to_underlying(window.flag_);

		auto& context = Context::instance();
		// const auto& theme = context.theme();
		const auto& mouse = context.mouse();

		// ------------------------------
		// TITLE BAR

		if (flag_value & std::to_underlying(WindowFlag::NO_TITLE_BAR))
		{
			window.collapse_ = false;
		}
		else if (mouse.double_clicked() and window.title_bar_rect().includes(mouse.position()))
		{
			window.collapse_ = not window.collapse_;
		}

		// ------------------------------
		// MOVE

		if (flag_value & std::to_underlying(WindowFlag::NO_MOVE))
		{
			return;
		}

		if (const auto id = get_id(window_widget_name_move);
			context.is_widget_activated(id))
		{
			if (mouse.down())
			{
				window.rect_.point += mouse.position_delta();
			}
			else
			{
				context.invalidate_widget_activated(
					#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
					std::format("{} is not a movable window.", window.name_)
					#endif
				);
			}
		}
	}

	auto Window::make_2_test_resize_grip() noexcept -> color_type
	{
		auto& window = *this;

		// auto& canvas = window.canvas_;
		// auto& draw_list = window.draw_list_;
		const auto flag_value = std::to_underlying(window.flag_);

		auto& context = Context::instance();
		const auto& theme = context.theme();
		const auto& mouse = context.mouse();

		if (flag_value & std::to_underlying(WindowFlag::NO_RESIZE))
		{
			return theme.color<ThemeCategory::RESIZE_GRIP>();
		}

		if (not window.collapse_)
		{
			const auto id = get_id(window_widget_name_resize);
			const auto resize_grip_rect = window.resize_grip_rect();

			const auto status = context.test_widget_status(
				id,
				resize_grip_rect,
				false
				#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
				,
				"Test Window resize grip."
				#endif
			);

			if (status.keeping)
			{
				const auto target_size = window.rect_.size() + mouse.position_delta();
				const auto min_size = theme.window_min_size;
				window.rect_.extent = {std::ranges::max(target_size.width, min_size.width), std::ranges::max(target_size.height, min_size.height)};
				return theme.color<ThemeCategory::RESIZE_GRIP_ACTIVATED>();
			}

			if (status.hovered)
			{
				return theme.color<ThemeCategory::RESIZE_GRIP_HOVERED>();
			}

			return theme.color<ThemeCategory::RESIZE_GRIP>();
		}

		return theme.color<ThemeCategory::RESIZE_GRIP>();
	}

	auto Window::make_3_init_canvas() noexcept -> void
	{
		auto& window = *this;

		auto& canvas = window.canvas_;
		// auto& draw_list = window.draw_list_;
		const auto flag_value = std::to_underlying(window.flag_);

		const auto& context = Context::instance();
		const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		// ------------------------------
		// CANVAS

		if (flag_value & std::to_underlying(WindowFlag::NO_TITLE_BAR))
		{
			canvas.cursor_start_line = theme.window_padding + extent_type{0, 0};
		}
		else
		{
			canvas.cursor_start_line = theme.window_padding + extent_type{0, theme.title_bar_height};
		}
		canvas.cursor_current_line = canvas.cursor_start_line;
		canvas.cursor_previous_line = canvas.cursor_current_line;
		canvas.height_current_line = 0;
		canvas.height_previous_line = 0;
		canvas.item_width.resize(0);
		canvas.item_width.push_back(window.default_item_width_);
	}

	auto Window::make_4_draw_canvas_background() noexcept -> void
	{
		auto& window = *this;

		// auto& canvas = window.canvas_;
		auto& draw_list = window.draw_list_;
		const auto flag_value = std::to_underlying(window.flag_);

		const auto& context = Context::instance();
		const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		const auto canvas_rect = window.canvas_rect();

		if (window.collapse_)
		{
			return;
		}

		draw_list.rect_filled(canvas_rect, theme.color<ThemeCategory::WINDOW_BACKGROUND>(), theme.window_rounding, DrawFlag::ROUND_CORNER_ALL);
		if (flag_value & std::to_underlying(WindowFlag::BORDERED))
		{
			draw_list.rect(canvas_rect, theme.color<ThemeCategory::BORDER>(), theme.window_rounding, DrawFlag::ROUND_CORNER_ALL);
		}
	}

	auto Window::make_5_draw_title_bar() noexcept -> void
	{
		auto& window = *this;

		// auto& canvas = window.canvas_;
		auto& draw_list = window.draw_list_;
		const auto flag_value = std::to_underlying(window.flag_);

		const auto& context = Context::instance();
		const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		const auto title_bar_rect = window.title_bar_rect();

		if (window.collapse_)
		{
			draw_list.rect_filled(title_bar_rect, theme.color<ThemeCategory::TITLE_BAR_COLLAPSED>(), theme.window_rounding, DrawFlag::ROUND_CORNER_ALL);

			if (flag_value & std::to_underlying(WindowFlag::BORDERED))
			{
				draw_list.rect(title_bar_rect, theme.color<ThemeCategory::BORDER>(), theme.window_rounding, DrawFlag::ROUND_CORNER_ALL);
			}
		}
		else
		{
			draw_list.rect_filled(title_bar_rect, theme.color<ThemeCategory::TITLE_BAR>(), theme.window_rounding, DrawFlag::ROUND_CORNER_TOP);
		}
		// todo: position
		draw_list.text(theme.font_size, title_bar_rect.left_top(), theme.color<ThemeCategory::TEXT>(), window.name_);

		// todo: close button
	}

	auto Window::make_6_draw_resize_grip(const color_type color) noexcept -> void
	{
		auto& window = *this;

		// auto& canvas = window.canvas_;
		auto& draw_list = window.draw_list_;
		const auto flag_value = std::to_underlying(window.flag_);

		// const auto& context = Context::instance();
		// const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		const auto resize_grip_rect = window.resize_grip_rect();

		if (window.collapse_ or (flag_value & std::to_underlying(WindowFlag::NO_RESIZE)))
		{
			return;
		}

		// todo: rounding?
		draw_list.triangle_filled(resize_grip_rect.left_bottom(), resize_grip_rect.right_bottom(), resize_grip_rect.right_top(), color);
	}

	auto Window::adjust_item_size(const extent_type size) noexcept -> void
	{
		auto& window = *this;

		auto& canvas = window.canvas_;
		// auto& draw_list = window.draw_list_;
		// const auto flag_value = std::to_underlying(window.flag_);

		const auto& context = Context::instance();
		const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		const auto line_height = std::ranges::max(canvas.height_current_line, size.height);

		if (window.collapse_)
		{
			return;
		}

		// Always align ourselves on pixel boundaries
		canvas.cursor_previous_line = canvas.cursor_current_line + point_type{size.width, 0};
		canvas.cursor_current_line = point_type{theme.window_padding.width, canvas.cursor_current_line.y + line_height + theme.item_spacing.height};
		canvas.height_previous_line = line_height;
		canvas.height_current_line = 0;
	}

	Window::Window(const name_view_type name, const WindowFlag flag, const rect_type rect) noexcept
		: name_{name},
		  id_{functional::hash<name_type>(name_)},
		  flag_{flag},
		  rect_{rect},
		  visible_{true},
		  collapse_{false} {}

	auto Window::init() noexcept -> void
	{
		auto& window = *this;

		window.make_1_test_title_bar();
		const auto resize_grip_color = window.make_2_test_resize_grip();
		window.make_3_init_canvas();
		window.make_4_draw_canvas_background();
		window.make_5_draw_title_bar();
		window.make_6_draw_resize_grip(resize_grip_color);
	}

	auto Window::make(name_view_type name, WindowFlag flag, rect_type rect) noexcept -> Window
	{
		auto window = Window{name, flag, rect};
		window.init();

		return window;
	}

	auto Window::hovered(const point_type mouse) const noexcept -> bool
	{
		// todo
		return rect_.includes(mouse);
	}

	auto Window::draw_text(const std::string_view text) noexcept -> void
	{
		const auto& context = Context::instance();
		const auto& theme = context.theme();

		draw_list_.text(theme.font_size, canvas_.cursor_current_line +rect_.left_top(), theme.color<ThemeCategory::TEXT>(), text);
	}

	auto Window::render() noexcept -> DrawList&
	{
		return draw_list_;
	}
}
