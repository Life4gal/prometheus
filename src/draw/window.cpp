// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
#include <draw/window.hpp>

#include <draw/context.hpp>

namespace
{
	using namespace gal::prometheus;

	constexpr std::string_view window_widget_name_move{"@WINDOW::MOVE@"};
	constexpr std::string_view window_widget_name_resize{"@WINDOW::RESIZE@"};
}

namespace gal::prometheus::draw
{
	auto Window::get_id(const std::string_view name) const noexcept -> id_type
	{
		return functional::hash_combine_2(id_, functional::hash<std::string_view>(name));
	}

	auto Window::rect_of_title_bar() const noexcept -> rect_type
	{
		const auto& context = Context::instance();
		const auto& theme = context.theme();

		return {rect_.left_top(), extent_type{rect_.width(), theme.title_bar_height}};
	}

	auto Window::rect_of_resize_grip() const noexcept -> rect_type
	{
		const auto& context = Context::instance();
		const auto& theme = context.theme();

		// RIGHT-BOTTOM-CORNER
		return {rect_.right_bottom() - theme.resize_grip_size, theme.resize_grip_size};
	}

	auto Window::rect_of_canvas() const noexcept -> rect_type
	{
		// const auto& context = Context::instance();
		// const auto& theme = context.theme();

		return rect_;
	}

	auto Window::make_canvas() noexcept -> void
	{
		auto& window = *this;

		auto& canvas = window.canvas_;
		auto& draw_list = window.draw_list_;
		const auto flag_value = std::to_underlying(window.flag_);

		auto& context = Context::instance();
		const auto& theme = context.theme();
		const auto& mouse = context.mouse();

		// ------------------------------
		// TEST TITLE BAR
		{
			if (flag_value & std::to_underlying(WindowFlag::NO_TITLE_BAR))
			{
				window.collapse_ = false;
			}
			else if (mouse.double_clicked() and window.rect_of_title_bar().includes(mouse.position()))
			{
				window.collapse_ = not window.collapse_;
			}

			if (flag_value & std::to_underlying(WindowFlag::NO_MOVE))
			{
				//
			}
			else
			{
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
		}

		// ------------------------------
		// TEST RESIZE GRIP
		auto resize_grip_color = theme.color<ThemeCategory::RESIZE_GRIP>();
		{
			if (flag_value & std::to_underlying(WindowFlag::NO_RESIZE))
			{
				//
			}
			else
			{
				if (not window.collapse_)
				{
					const auto id = get_id(window_widget_name_resize);
					const auto resize_grip_rect = window.rect_of_resize_grip();

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
						resize_grip_color = theme.color<ThemeCategory::RESIZE_GRIP_ACTIVATED>();
					}
					else if (status.hovered)
					{
						resize_grip_color = theme.color<ThemeCategory::RESIZE_GRIP_HOVERED>();
					}
				}
			}
		}

		// ------------------------------
		// INIT CANVAS
		{
			if (flag_value & std::to_underlying(WindowFlag::NO_TITLE_BAR))
			{
				canvas.cursor_start_line = (theme.window_padding + extent_type{0, 0}).to<point_type>();
			}
			else
			{
				canvas.cursor_start_line = (theme.window_padding + extent_type{0, theme.title_bar_height}).to<point_type>();
			}
			canvas.cursor_current_line = canvas.cursor_start_line;
			canvas.cursor_previous_line = canvas.cursor_current_line;
			canvas.height_current_line = 0;
			canvas.height_previous_line = 0;
			canvas.item_width.resize(0);
			canvas.item_width.push_back(window.default_item_width_);
		}

		// ------------------------------
		// DRAW CANVAS BACKGROUND
		{
			const auto canvas_rect = window.rect_of_canvas();

			if (window.collapse_)
			{
				//
			}
			else
			{
				draw_list.rect_filled(canvas_rect, theme.color<ThemeCategory::WINDOW_BACKGROUND>(), theme.window_rounding, DrawFlag::ROUND_CORNER_ALL);
				if (flag_value & std::to_underlying(WindowFlag::BORDERED))
				{
					draw_list.rect(canvas_rect, theme.color<ThemeCategory::BORDER>(), theme.window_rounding, DrawFlag::ROUND_CORNER_ALL);
				}
			}
		}

		// ------------------------------
		// DRAW TITTLE BAR
		{
			const auto title_bar_rect = window.rect_of_title_bar();

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

		// ------------------------------
		// DRAW RESIZE GRIP
		{
			const auto resize_grip_rect = window.rect_of_resize_grip();

			if (window.collapse_ or (flag_value & std::to_underlying(WindowFlag::NO_RESIZE)))
			{
				//
			}
			else
			{
				// todo: rounding?
				draw_list.triangle_filled(resize_grip_rect.left_bottom(), resize_grip_rect.right_bottom(), resize_grip_rect.right_top(), resize_grip_color);
			}
		}
	}

	auto Window::adjust_item_size(const extent_type& size) noexcept -> void
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

	Window::Window(const std::string_view name, const WindowFlag flag, const rect_type& rect) noexcept
		: name_{name},
		  id_{functional::hash<std::string_view>(name_)},
		  flag_{flag},
		  rect_{rect},
		  visible_{true},
		  collapse_{false} {}

	auto Window::make(const std::string_view name, const WindowFlag flag, const rect_type& rect) noexcept -> Window
	{
		Window window{name, flag, rect};
		window.make_canvas();

		return window;
	}

	auto Window::name() const noexcept -> std::string_view
	{
		return name_;
	}

	auto Window::rect() const noexcept -> const rect_type&
	{
		return rect_;
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

		draw_list_.text(theme.font_size, canvas_.cursor_current_line + rect_.left_top(), theme.color<ThemeCategory::TEXT>(), text);
	}

	auto Window::render() noexcept -> DrawList&
	{
		return draw_list_;
	}
}
