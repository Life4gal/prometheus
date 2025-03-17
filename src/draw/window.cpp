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
	constexpr std::string_view window_widget_name_close{"@WINDOW::CLOSE@"};
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

		const auto point = rect_.left_top();
		const auto size = extent_type{rect_.width(), theme.title_bar_height};
		return {point, size};
	}

	auto Window::rect_of_close_button() const noexcept -> rect_type
	{
		const auto& context = Context::instance();
		const auto& theme = context.theme();

		// RIGHT-TOP-CORNER
		const auto point = rect_.right_top() - extent_type{theme.title_bar_height, 0};
		const auto size = extent_type{theme.title_bar_height, theme.title_bar_height};
		return {point, size};
	}

	auto Window::rect_of_resize_grip() const noexcept -> rect_type
	{
		const auto& context = Context::instance();
		const auto& theme = context.theme();

		// RIGHT-BOTTOM-CORNER
		const auto point = rect_.right_bottom() - theme.resize_grip_size;
		const auto size = theme.resize_grip_size;
		return {point, size};
	}

	auto Window::rect_of_canvas() const noexcept -> rect_type
	{
		// const auto& context = Context::instance();
		// const auto& theme = context.theme();

		return rect_;
	}

	auto Window::make_canvas() noexcept -> bool
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
		auto close_button_color = theme.color<ThemeCategory::BUTTON>();
		auto close_button_pressed = false;
		{
			if (flag_value & std::to_underlying(WindowFlag::NO_TITLE_BAR))
			{
				window.collapse_ = false;
			}
			else if (mouse.double_clicked() and window.rect_of_title_bar().includes(mouse.position()))
			{
				window.collapse_ = not window.collapse_;
			}

			if (flag_value & std::to_underlying(WindowFlag::NO_CLOSE))
			{
				//
			}
			else
			{
				const auto id = get_id(window_widget_name_close);
				const auto close_button_rect = window.rect_of_close_button();

				const auto status = context.test_widget_status(
					id,
					close_button_rect,
					false
					#if defined(GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG)
					,
					std::format("Test Window({})'s close-button({}).", window.name_, close_button_rect)
					#endif
				);

				if (status.hovered)
				{
					if (status.keeping)
					{
						close_button_color = theme.color<ThemeCategory::BUTTON_ACTIVATED>();
					}
					else
					{
						close_button_color = theme.color<ThemeCategory::BUTTON_HOVERED>();
					}
				}

				if (status.pressed)
				{
					close_button_pressed = true;
				}
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
							#if defined(GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG)
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
						#if defined(GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG)
						,
						std::format("Test Window({})'s resize-grip({}).", window.name_, resize_grip_rect)
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
			const auto text_point = title_bar_rect.left_top() + extent_type{theme.item_inner_spacing.width, 0};
			draw_list.text(
				theme.font_size,
				text_point,
				theme.color<ThemeCategory::TEXT>(),
				window.name_
			);

			const auto close_button_rect = window.rect_of_close_button();
			if (flag_value & std::to_underlying(WindowFlag::NO_CLOSE))
			{
				//
			}
			else
			{
				const auto center = close_button_rect.center();
				const auto radius = close_button_rect.width() / 2;

				const auto r = radius / std::numbers::sqrt2_v<float>;

				const auto line1_from = center + extent_type{-r, -r};
				const auto line1_to = center + extent_type{r, r};

				const auto line2_from = center + extent_type{-r, r};
				const auto line2_to = center + extent_type{r, -r};

				draw_list.circle_filled(close_button_rect.center(), close_button_rect.width() / 2, close_button_color);
				draw_list.line(line1_from, line1_to, theme.color<ThemeCategory::TEXT>());
				draw_list.line(line2_from, line2_to, theme.color<ThemeCategory::TEXT>());
			}
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

		return close_button_pressed;
	}

	auto Window::cursor_abs_position() const noexcept -> point_type
	{
		auto& window = *this;

		const auto& canvas = window.canvas_;
		// auto& draw_list = window.draw_list_;
		// const auto flag_value = std::to_underlying(window.flag_);

		// const auto& context = Context::instance();
		// const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		return window.rect_.left_top() + canvas.cursor_current_line;
	}

	auto Window::cursor_remaining_width() const noexcept -> value_type
	{
		auto& window = *this;

		const auto& canvas = window.canvas_;
		// auto& draw_list = window.draw_list_;
		// const auto flag_value = std::to_underlying(window.flag_);

		// const auto& context = Context::instance();
		// const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		return window.rect_.width() - canvas.cursor_current_line.x;
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

		if (window.collapse_)
		{
			return;
		}

		const auto line_height = std::ranges::max(canvas.height_current_line, size.height);

		// Always align ourselves on pixel boundaries
		canvas.cursor_previous_line = {canvas.cursor_current_line.x + size.width, canvas.cursor_current_line.y};
		canvas.cursor_current_line = {theme.window_padding.width, canvas.cursor_current_line.y + line_height + theme.item_spacing.height};

		canvas.height_previous_line = line_height;
		canvas.height_current_line = 0;
	}

	auto Window::draw_widget_frame(const rect_type& rect, const color_type& color) noexcept -> void
	{
		auto& window = *this;

		// auto& canvas = window.canvas_;
		auto& draw_list = window.draw_list_;
		const auto flag_value = std::to_underlying(window.flag_);

		const auto& context = Context::instance();
		// const auto& font = context.font();
		const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		draw_list.rect_filled(rect, color);
		if (flag_value & std::to_underlying(WindowFlag::BORDERED))
		{
			draw_list.rect(rect, theme.color<ThemeCategory::BORDER>());
		}
	}

	Window::Window(const std::string_view name, const WindowFlag flag, const rect_type& rect) noexcept
		: name_{name},
		  id_{functional::hash<std::string_view>(name_)},
		  flag_{flag},
		  rect_{rect},
		  default_item_width_{0},
		  visible_{true},
		  collapse_{false} {}

	auto Window::make(const std::string_view name, const WindowFlag flag, const rect_type& rect) noexcept -> Window
	{
		Window window{name, flag, rect};
		window.draw_list_.reset();
		// todo
		std::ignore = window.make_canvas();

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

	auto Window::layout_same_line(const value_type column_width, value_type spacing_width) noexcept -> void
	{
		auto& window = *this;

		auto& canvas = window.canvas_;
		// auto& draw_list = window.draw_list_;
		// const auto flag_value = std::to_underlying(window.flag_);

		const auto& context = Context::instance();
		// const auto& font = context.font();
		const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		if (window.collapse_)
		{
			return;
		}

		canvas.height_current_line = canvas.height_previous_line;
		canvas.cursor_current_line = canvas.cursor_previous_line;

		if (column_width <= 0)
		{
			if (spacing_width <= 0)
			{
				spacing_width = theme.item_spacing.width;
			}

			canvas.cursor_current_line.x += spacing_width;
		}
		else
		{
			spacing_width = std::ranges::max(spacing_width, static_cast<value_type>(0));

			canvas.cursor_current_line.x = column_width + spacing_width;
		}
	}

	auto Window::draw_text(const std::string_view utf8_text) noexcept -> void
	{
		auto& window = *this;

		// const auto& canvas = window.canvas_;
		auto& draw_list = window.draw_list_;
		// const auto flag_value = std::to_underlying(window.flag_);

		const auto& context = Context::instance();
		const auto& font = context.font();
		const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		if (window.collapse_)
		{
			return;
		}

		const auto text_context_size = font.text_size(utf8_text, theme.font_size, cursor_remaining_width());

		const auto text_point = cursor_abs_position();
		const auto text_size = text_context_size;
		const rect_type text_rect{text_point, text_size};
		adjust_item_size(text_size);

		draw_list.text(
			font,
			theme.font_size,
			text_rect.left_top(),
			theme.color<ThemeCategory::TEXT>(),
			utf8_text,
			text_rect.width()
		);
	}

	auto Window::draw_button(std::string_view utf8_text, extent_type size) noexcept -> bool
	{
		auto& window = *this;

		// const auto& canvas = window.canvas_;
		auto& draw_list = window.draw_list_;
		// const auto flag_value = std::to_underlying(window.flag_);

		auto& context = Context::instance();
		const auto& font = context.font();
		const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		if (window.collapse_)
		{
			return false;
		}

		const auto text_context_size = font.text_size(utf8_text, theme.font_size, cursor_remaining_width());

		if (size.width <= 0)
		{
			size.width = text_context_size.width;
		}
		if (size.height <= 0)
		{
			size.height = text_context_size.height;
		}

		// todo
		const auto text_point = cursor_abs_position() + extent_type{theme.item_inner_spacing.width, theme.frame_padding.height};

		const auto button_point = cursor_abs_position();
		const auto button_size = size + theme.frame_padding * 2;
		const rect_type button_rect{button_point, button_size};
		adjust_item_size(button_size);

		const auto id = get_id(utf8_text);
		const auto status = context.test_widget_status(
			id,
			button_rect,
			false
			#if defined(GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG)
			,
			std::format("Test Window({})'s button[{}]({}).", window.name_, utf8_text, button_rect)
			#endif
		);

		color_type button_color = theme.color<ThemeCategory::BUTTON>();
		{
			if (status.keeping or status.pressed)
			{
				button_color = theme.color<ThemeCategory::BUTTON_ACTIVATED>();
			}
			else if (status.hovered)
			{
				button_color = theme.color<ThemeCategory::BUTTON_HOVERED>();
			}
		}
		draw_widget_frame(button_rect, button_color);

		draw_list.text(
			font,
			theme.font_size,
			text_point,
			theme.color<ThemeCategory::TEXT>(),
			utf8_text,
			button_rect.width()
		);

		return status.pressed;
	}

	auto Window::draw_checkbox(std::string_view utf8_text, bool checked, extent_type size) noexcept -> bool
	{
		auto& window = *this;

		// const auto& canvas = window.canvas_;
		auto& draw_list = window.draw_list_;
		// const auto flag_value = std::to_underlying(window.flag_);

		auto& context = Context::instance();
		const auto& font = context.font();
		const auto& theme = context.theme();
		// const auto& mouse = context.mouse();

		if (window.collapse_)
		{
			return false;
		}

		const auto text_context_size = font.text_size(utf8_text, theme.font_size, cursor_remaining_width());

		if (size.width <= 0)
		{
			size.width = text_context_size.width;
		}
		if (size.height <= 0)
		{
			size.height = text_context_size.height;
		}

		// □ + text

		// □, side length equals string rect height
		const auto check_point = cursor_abs_position();
		const auto check_size = extent_type{size.height + theme.frame_padding.height * 2, size.height + theme.frame_padding.height * 2};
		const rect_type check_rect{check_point, check_size};
		adjust_item_size(check_size);

		// □ text
		layout_same_line(0, theme.item_inner_spacing.width);

		// text
		const auto text_point = cursor_abs_position() + extent_type{0, theme.frame_padding.height};
		const auto text_size = size;
		const rect_type text_rect{text_point, text_size};
		adjust_item_size(text_size);

		draw_widget_frame(check_rect, theme.color<ThemeCategory::WIDGET_BACKGROUND>());

		const auto id = get_id(utf8_text);
		const auto status = context.test_widget_status(
			id,
			check_rect,
			false
			#if defined(GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG)
			,
			std::format("Test Window({})'s checkbox[{}]({}).", window.name_, utf8_text, check_rect)
			#endif
		);

		if (status.pressed)
		{
			checked = not checked;
		}

		if (checked)
		{
			const auto check_fill_point = check_point + theme.item_inner_spacing;
			const auto check_fill_size = check_size - theme.item_inner_spacing * 2;
			const rect_type check_fill_rect{check_fill_point, check_fill_size};
			draw_list.rect_filled(check_fill_rect, theme.color<ThemeCategory::WIDGET_ACTIVATED>());
		}

		draw_list.text(
			font,
			theme.font_size,
			text_rect.left_top(),
			theme.color<ThemeCategory::TEXT>(),
			utf8_text,
			text_rect.width()
		);

		return checked;
	}

	auto Window::render() noexcept -> DrawList&
	{
		return draw_list_;
	}
}
