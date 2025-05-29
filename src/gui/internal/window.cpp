// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gui/internal/window.hpp>

#include <memory/reference_wrapper.hpp>

#include <gui/internal/font.hpp>
#include <gui/internal/context.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace
{
	using namespace gal::prometheus;
	using namespace gui;

	// ReSharper disable once CppInconsistentNaming
	using MouseState = Context::MouseState;
}

namespace gal::prometheus::gui::internal
{
	class Window::IdMaker final
	{
	public:
		memory::RefWrapper<Window> self;

	private:
		[[nodiscard]] static auto make(const std::string_view string) noexcept -> widget_id_type
		{
			return functional::hash<std::string_view>(string);
		}

		[[nodiscard]] static auto make(const widget_id_type seed, const std::string_view string) noexcept -> widget_id_type
		{
			return functional::hash_combine_2(seed, make(string));
		}

		[[nodiscard]] static auto make(const void* pointer) noexcept -> widget_id_type
		{
			return static_cast<widget_id_type>(reinterpret_cast<std::uintptr_t>(pointer));
		}

		[[nodiscard]] static auto make(const widget_id_type seed, const void* pointer) noexcept -> widget_id_type
		{
			return functional::hash_combine_2(seed, make(pointer));
		}

		[[nodiscard]] static auto make(const widget_id_type value) noexcept -> widget_id_type
		{
			const auto* pointer = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(value)); // NOLINT(performance-no-int-to-ptr)
			return make(pointer);
		}

		[[nodiscard]] static auto make(const widget_id_type seed, const widget_id_type value) noexcept -> widget_id_type
		{
			return functional::hash_combine_2(seed, make(value));
		}

	public:
		auto make_id() noexcept -> void
		{
			auto& window = self.get();

			const auto id = make(window.name_);
			window.id_stack_.push_back(id);
		}

		[[nodiscard]] auto make_id(Context& context, const std::string_view string) const noexcept -> widget_id_type
		{
			auto& window = self.get();

			// id_stack.front() -> window id
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window.id_stack_.empty());

			const auto seed = window.id_stack_.back();
			const auto id = make(seed, string);

			context.mark_widget_alive(id);

			return id;
		}

		[[nodiscard]] auto make_id(Context& context, const void* pointer) const noexcept -> widget_id_type
		{
			auto& window = self.get();

			// id_stack.front() -> window id
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window.id_stack_.empty());

			const auto seed = window.id_stack_.back();
			const auto id = make(seed, pointer);

			context.mark_widget_alive(id);

			return id;
		}

		[[nodiscard]] auto make_id(Context& context, const widget_id_type value) const noexcept -> widget_id_type
		{
			auto& window = self.get();

			// id_stack.front() -> window id
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window.id_stack_.empty());

			const auto seed = window.id_stack_.back();
			const auto id = make(seed, value);

			context.mark_widget_alive(id);

			return id;
		}
	};

	class Window::Drawer final
	{
	public:
		memory::RefWrapper<Window> self;

		// -----------------------------------
		// FONT

		[[nodiscard]] auto font_size(const Context& context) const noexcept -> value_type
		{
			std::ignore = this;

			const auto& font = context.current_font();

			// todo: scale?
			return static_cast<value_type>(font.pixel_height) * font.scale;
		}

		// -----------------------------------
		// PADDING

		[[nodiscard]] auto window_padding(const Context& context) const noexcept -> extent_type
		{
			const auto& window = self.get();

			const auto& theme = context.current_theme();

			if (window.flag_.is<WindowInternalFlag::CHILD_WINDOW>() and not window.flag_.is<gui::WindowFlag::BORDERED>())
			{
				return {1, 1};
			}

			return theme.window_padding;
		}

		// -----------------------------------
		// TITLEBAR

		[[nodiscard]] auto titlebar_height(const Context& context) const noexcept -> value_type
		{
			const auto& window = self.get();

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window.flag_.is<gui::WindowFlag::NO_TITLEBAR>());

			const auto& theme = context.current_theme();

			return font_size(context) + theme.item_frame_padding.height * 2;
		}

		[[nodiscard]] auto titlebar_rect(const Context& context) const noexcept -> rect_type
		{
			const auto& window = self.get();

			return {window.point_, window.size_full_.width, titlebar_height(context)};
		}

		[[nodiscard]] auto titlebar_rect(const Context& context, const value_type height) const noexcept -> rect_type
		{
			std::ignore = context;

			const auto& window = self.get();

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(window.flag_.is<gui::WindowFlag::NO_TITLEBAR>());

			return {window.point_, window.size_full_.width, height};
		}

		// -----------------------------------
		// RESIZE-GRIP

		[[nodiscard]] auto resize_grip_size(const Context& context) const noexcept -> extent_type
		{
			std::ignore = this;

			const auto& theme = context.current_theme();

			return theme.window_resize_grip_size;
		}

		[[nodiscard]] auto resize_grip_rect(const Context& context) const noexcept -> rect_type
		{
			const auto& window = self.get();

			const auto s = resize_grip_size(context);

			// RIGHT-BOTTOM-CORNER
			const point_type p{window.point_.x + window.size_.width - s.width, window.point_.y + window.size_.height - s.height};
			return {p, s};
		}

		// -----------------------------------
		// CLOSE BUTTON

		[[nodiscard]] auto close_button_size(const Context& context) const noexcept -> extent_type
		{
			const auto& window = self.get();

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window.flag_.is<gui::WindowFlag::NO_TITLEBAR>());

			const auto height = titlebar_height(context);

			return {height - 4, height - 4};
		}

		[[nodiscard]] auto close_button_rect(const Context& context) const noexcept -> rect_type
		{
			const auto& window = self.get();

			const auto s = close_button_size(context);

			// LEFT-TOP-CORNER
			const point_type p{window.point_.x + window.size_.width - s.width - 3, window.point_.y + 2};
			return {p, s};
		}

		// -----------------------------------
		// CANVAS

		auto adjust_item_size(const Context& context, const extent_type& size) noexcept -> void
		{
			const auto& theme = context.current_theme();

			auto& window = self.get();
			auto& canvas = window.canvas_;

			const auto line_height = std::ranges::max(canvas.height_current_line, size.height);

			// Always align ourselves on pixel boundaries
			canvas.cursor_previous_line =
					// previous point
					canvas.cursor_current_line +
					// X: item width
					// Y: 0
					extent_type{size.width, 0};
			canvas.cursor_current_line =
					// todo X: start point X + columns offset
					// Y: previous Y + item height + item spacing
					{window.point_.x + 8, canvas.cursor_current_line.y + line_height + theme.item_spacing.height};
			// window.point_ + extent_type{8, canvas.cursor_current_line.y + line_height + theme.item_spacing.height};

			canvas.height_previous_line = line_height;
			canvas.height_current_line = 0;

			window.size_of_content_ = window.size_of_content_.combine_max(extent_type{canvas.cursor_previous_line.x, canvas.cursor_current_line.y + window.scroll_y_} - window.point_);
		}

		// cursor position is relative to window position
		[[nodiscard]] auto cursor_position(const Context& context) const noexcept -> point_type
		{
			std::ignore = context;

			const auto& window = self.get();
			const auto& canvas = window.canvas_;

			return canvas.cursor_previous_line - window.point_;
		}

		// cursor position in screen space
		[[nodiscard]] auto cursor_screen_position(const Context& context) const noexcept -> point_type
		{
			std::ignore = context;

			const auto& window = self.get();
			const auto& canvas = window.canvas_;

			return canvas.cursor_current_line;
		}

		auto set_cursor_position_x(const Context& context, const value_type x) noexcept -> void
		{
			std::ignore = context;

			auto& window = self.get();
			auto& canvas = window.canvas_;

			canvas.cursor_current_line.x = window.point_.x + x;
		}

		auto set_cursor_position_y(const Context& context, const value_type y) noexcept -> void
		{
			std::ignore = context;

			auto& window = self.get();
			auto& canvas = window.canvas_;

			canvas.cursor_current_line.y = window.point_.y + y;
		}

		auto set_cursor_position(const Context& context, const point_type& point) noexcept -> void
		{
			set_cursor_position_x(context, point.x);
			set_cursor_position_y(context, point.y);
		}

		auto set_scroll_position(const Context& context, const value_type y) noexcept -> void
		{
			std::ignore = context;

			auto& window = self.get();

			window.scroll_next_y_ = y;
		}

		// adjust scrolling position to center into the current cursor position
		auto set_scroll_position_here(const Context& context) noexcept -> void
		{
			const auto& window = self.get();
			const auto& canvas = window.canvas_;

			const auto middle = window.point_.y + window.size_full_.height * .5f;

			auto y = canvas.cursor_current_line.y - window.scroll_y_ - window_padding(context).height;

			y -= middle;
			if (not window.flag_.is<gui::WindowFlag::NO_TITLEBAR>())
			{
				y -= titlebar_height(context);
			}

			set_scroll_position(context, y);
		}

		// auto test_last_item(const Context& context, const rect_type& rect) noexcept -> void
		// {
		// 	auto& window = self.get();
		// 	auto& canvas = window.canvas_;
		//
		// 	canvas.last_item_rect = rect;
		// 	canvas.last_item_focused = false;
		// 	canvas.last_item_hovered = window.is_hovered(context, rect);
		// }
		//
		// [[nodiscard]] auto is_visible_area(const Context& context, const rect_type& rect) const noexcept -> bool
		// {
		// 	std::ignore = context;
		//
		// 	auto& window = self.get();
		//
		// 	const auto& last = window.clip_rect_stack_.back();
		// 	return last.intersects(rect);
		// }

		[[nodiscard]] auto test_last_item_visible(const Context& context, const rect_type& rect) noexcept -> bool
		{
			auto& window = self.get();
			auto& canvas = window.canvas_;

			const auto& last = window.clip_rect_stack_.back();
			const auto visible = last.intersects(rect);

			canvas.last_item_rect = rect;
			canvas.last_item_focused = false;
			canvas.last_item_hovered = visible ? window.is_hovered(context, rect) : false;

			return visible;
		}

		// -----------------------------------
		// FRAME

		auto draw_widget_frame(const Context& context, const rect_type& rect, const color_type color) noexcept -> void
		{
			const auto& theme = context.current_theme();

			auto& window = self.get();

			window.draw_list_.rect_filled(rect, color);
			if (window.flag_.is<gui::WindowFlag::BORDERED>())
			{
				const point_type outer_point{rect.left_top() + extent_type{.5f, .5f}};
				const extent_type outer_size{rect.size() - extent_type{.5f, .5f}};

				const point_type inner_point{rect.left_top() + extent_type{1.5f, 1.5f}};
				const extent_type inner_size{rect.size() - extent_type{.5f, .5f}};

				window.draw_list_.rect({inner_point, inner_size}, context.color_of(theme, ThemeCategory::BORDER_SHADOW));
				window.draw_list_.rect({outer_point, outer_size}, context.color_of(theme, ThemeCategory::BORDER));
			}
		}

		auto draw_widget_frame(const Context& context, const circle_type& circle, const color_type color) noexcept -> void
		{
			const auto& theme = context.current_theme();

			auto& window = self.get();

			window.draw_list_.circle_filled(circle, color);
			if (window.flag_.is<gui::WindowFlag::BORDERED>())
			{
				const point_type outer_point{circle.center() + extent_type{.5f, .5f}};
				const auto outer_radius = circle.radius - .5f;

				const point_type inner_point{circle.center() + extent_type{1.5f, 1.5f}};
				const auto inner_radius = circle.radius - .5f;

				window.draw_list_.circle({inner_point, inner_radius}, context.color_of(theme, ThemeCategory::BORDER_SHADOW));
				window.draw_list_.circle({outer_point, outer_radius}, context.color_of(theme, ThemeCategory::BORDER));
			}
		}

		auto draw_widget_frame(const Context& context, const point_type& a, const point_type& b, const point_type& c) noexcept -> void
		{
			const auto& theme = context.current_theme();

			auto& window = self.get();

			if (window.flag_.is<gui::WindowFlag::BORDERED>())
			{
				const auto offset_a = a + extent_type{1.5f, 1.5f};
				const auto offset_b = b + extent_type{1.5f, 1.5f};
				const auto offset_c = c + extent_type{1.5f, 1.5f};

				window.draw_list_.triangle_filled(offset_a, offset_b, offset_c, context.color_of(theme, ThemeCategory::BORDER_SHADOW));
			}
			window.draw_list_.triangle_filled(a, b, c, context.color_of(theme, ThemeCategory::BORDER));
		}
	};

	class Window::Anonymous final
	{
	public:
		memory::RefWrapper<Window> self;

		template<typename T>
			requires (std::is_same_v<T, float> or std::is_same_v<T, std::span<float>>)
		auto draw_slider(
			Context& context,
			const std::string_view utf8_text,
			T& reference,
			const float min,
			const float max,
			const std::uint32_t decimal_precision,
			const float power
		) noexcept -> bool
		{
			constexpr auto is_list = std::is_same_v<T, std::span<float>>;
			if constexpr (is_list)
			{
				const auto list_size = reference.size();
				if (list_size == 1)
				{
					return this->draw_slider(context, utf8_text, reference[0], min, max, decimal_precision, power);
				}
			}

			auto& window = self.get();

			if (window.skip_item_)
			{
				return false;
			}
			window.accessed_ = true;

			const auto& theme = context.current_theme();
			const auto& font = context.current_font();
			Drawer drawer{.self = window};
			const IdMaker id_maker{.self = window};

			const auto font_size = drawer.font_size(context);

			const auto do_draw_slider = [&]<typename ValueType>(const widget_id_type id, ValueType& ref_value, const rect_type& slider_rect, const rect_type& frame_rect) noexcept -> bool
			{
				const auto slider_point = slider_rect.left_top();
				const auto slider_size = slider_rect.size();

				const auto frame_point = frame_rect.left_top();
				const auto frame_size = frame_rect.size();

				const auto state = context.test_mouse(id, slider_rect, false);

				// draw □ (frame + slider + text)
				bool value_changed = false;

				// frame
				drawer.draw_widget_frame(context, frame_rect, context.color_of(theme, ThemeCategory::FRAME_BACKGROUND));

				// slider

				// todo
				constexpr float grab_size_in_pixels = 10.f;

				const auto slider_effective_width = slider_size.width - grab_size_in_pixels;
				const auto slider_effective_x1 = slider_point.x + grab_size_in_pixels * .5f;
				const auto slider_effective_x2 = slider_point.x + slider_size.width - grab_size_in_pixels * .5f;

				const auto linear_zero_pos = [=]() noexcept -> float
				{
					if (min * max < 0)
					{
						// different sign
						const auto linear_dist_min_to_0 = std::powf(std::fabs(.0f - min), 1.f / power);
						const auto linear_dist_max_to_0 = std::powf(std::fabs(max - .0f), 1.f / power);
						return linear_dist_min_to_0 / (linear_dist_min_to_0 + linear_dist_max_to_0);
					}

					// same sign
					return min < 0 ? 1.f : .0f;
				}();

				if (state & MouseState::KEEPING)
				{
					const auto mouse_position = context.mouse().position_current;
					const auto normalized_x = std::ranges::clamp((mouse_position.x - slider_effective_x1) / slider_effective_width, .0f, 1.f);

					// account for logarithmic scale on both sides of the zero
					auto value = [=]() noexcept -> float
					{
						if (normalized_x < linear_zero_pos)
						{
							// rescale to the negative range before powering
							auto v = 1.f - (normalized_x / linear_zero_pos);
							v = std::powf(v, power);

							const auto negative_part_max = std::ranges::min(max, .0f);
							return std::lerp(negative_part_max, min, v);
						}

						// rescale to the positive range before powering
						auto v = normalized_x;
						if (std::fabs(linear_zero_pos - 1.f) > 1e-6)
						{
							v = (v - linear_zero_pos) / (1.f - linear_zero_pos);
						}
						v = std::powf(v, power);

						const auto positive_part_min = std::ranges::max(min, .0f);
						return std::lerp(positive_part_min, max, v);
					}();

					const auto min_step = 1.f / std::powf(10.f, static_cast<float>(decimal_precision));
					const auto remainder = std::fmodf(value, min_step);

					if (remainder <= min_step * .5f)
					{
						value -= remainder;
					}
					else
					{
						value += (min_step - remainder);
					}

					// if (ref_value != value) // NOLINT(clang-diagnostic-float-equal)
					if (std::fabs(ref_value - value) > 1e-6f)
					{
						ref_value = value;
						value_changed = true;
					}
				}

				// grab
				{
					const auto v = [=]() noexcept -> float
					{
						const auto clamped = std::ranges::clamp(ref_value, min, max);
						if (clamped < .0f)
						{
							const auto f = 1.f - (clamped - min) / (std::ranges::min(.0f, max) - min);
							return (1.f - std::powf(f, 1.f / power)) * linear_zero_pos;
						}

						const auto f = (clamped - std::ranges::max(0.f, min)) / (max - std::ranges::max(0.f, min));
						return linear_zero_pos + std::powf(f, 1.f / power) * (1.f - linear_zero_pos);
					}();

					const auto x = std::lerp(slider_effective_x1, slider_effective_x2, v);
					const point_type grab_point{x - grab_size_in_pixels * .5f, frame_point.y + 2.f};
					const extent_type grab_size{grab_size_in_pixels, frame_size.height - 4.f};
					const rect_type grab_rect{grab_point, grab_size};

					if (state & MouseState::PRESSED)
					{
						window.draw_list_.rect_filled(
							grab_rect,
							context.color_of(theme, ThemeCategory::SLIDER_ACTIVATED),
							theme.window_corner_rounding,
							DrawFlag::ROUND_CORNER_ALL
						);
					}
					else
					{
						window.draw_list_.rect_filled(
							grab_rect,
							context.color_of(theme, ThemeCategory::SLIDER),
							theme.window_corner_rounding,
							DrawFlag::ROUND_CORNER_ALL
						);
					}
				}

				// text
				const auto value_text = std::format("{:.{}f}", ref_value, decimal_precision);
				const auto value_text_size = internal::text_size(font, value_text, font_size, Font::no_auto_wrap);
				const point_type value_text_point{slider_point.x + slider_size.width / 2 - value_text_size.width / 2, frame_point.y + theme.item_frame_padding.height};
				window.draw_list_.text(
					font,
					font_size,
					value_text_point,
					context.color_of(theme, ThemeCategory::TEXT),
					value_text
				);

				return value_changed;
			};

			const auto text_size = internal::text_size(font, utf8_text, font_size, Font::no_auto_wrap);

			const auto last_item_width = window.canvas_.item_width.back();

			const auto total_frame_point = window.canvas_.cursor_current_line;
			const auto total_frame_size = extent_type{last_item_width, text_size.height} + theme.item_frame_padding * 2;
			const rect_type total_frame_rect{total_frame_point, total_frame_size};

			const auto total_slider_point = total_frame_point + theme.item_frame_padding;
			const auto total_slider_size = total_frame_size - theme.item_frame_padding * 2;
			const rect_type total_slider_rect{total_slider_point, total_slider_size};

			drawer.adjust_item_size(context, total_frame_size);

			// □ text / □ □ .. □ □ text
			window.same_line(context, auto_size, theme.item_inner_spacing.width);

			// text
			const auto text_point = window.canvas_.cursor_current_line + theme.item_frame_padding;
			const rect_type text_rect{text_point, text_size};
			drawer.adjust_item_size(context, text_size);

			const rect_type total_rect{total_frame_point, text_rect.right_bottom()};
			// if (not drawer.is_visible_area(context, total_rect))
			// {
			// 	// invisible
			// 	return false;
			// }
			// drawer.test_last_item(context, total_rect);
			if (not drawer.test_last_item_visible(context, total_rect))
			{
				// invisible
				return false;
			}

			bool value_changed = false;

			if constexpr (is_list)
			{
				const auto list_size = reference.size();

				// width: (total - spacing) / size
				// height: total
				const auto each_frame_size = extent_type
				{
						(total_frame_size.width - theme.item_inner_spacing.width * static_cast<value_type>(list_size - 1)) / static_cast<value_type>(list_size),
						total_frame_size.height
				};
				const auto each_slider_size = each_frame_size - theme.item_frame_padding * 2;

				const auto offset_x = each_frame_size.width + theme.item_inner_spacing.width;

				// draw n slider
				// note: when drawing multiple sliders, the widget id is based on the slider index, not the label text
				// note: to avoid different sliders getting the same id (since ids are based on index), the label text is pushed in here as the seed
				window.push_id(context, utf8_text);
				{
					for (const auto view = reference | std::views::enumerate;
					     auto [index, ref_value]: view)
					{
						const auto id = id_maker.make_id(context, index);

						// x: total + offset * n
						// y: total
						const point_type frame_point
						{
								total_frame_point.x + offset_x * static_cast<value_type>(index),
								total_frame_point.y
						};
						const rect_type frame_rect{frame_point, each_frame_size};

						// x: total + offset * n
						// y: total
						const point_type slider_point
						{
								total_slider_point.x + offset_x * static_cast<value_type>(index),
								total_slider_point.y
						};
						const rect_type slider_rect{slider_point, each_slider_size};

						value_changed |= do_draw_slider(id, ref_value, slider_rect, frame_rect);
					}
				}
				window.pop_id(context);
			}
			else
			{
				// draw one slider
				const auto id = id_maker.make_id(context, utf8_text);
				value_changed |= do_draw_slider(id, reference, total_slider_rect, total_frame_rect);
			}

			// draw text
			window.draw_list_.text(
				font,
				font_size,
				text_rect.left_top(),
				context.color_of(theme, ThemeCategory::TEXT),
				utf8_text,
				text_rect.width()
			);

			return value_changed;
		}

		template<typename T>
		auto draw_combo(
			Context& context,
			const std::string_view utf8_text,
			const std::span<const T> selections,
			std::size_t& selected,
			const std::size_t show_selection_count
		) noexcept -> bool
		{
			auto& window = self.get();

			if (window.skip_item_)
			{
				return false;
			}
			window.accessed_ = true;

			const auto& theme = context.current_theme();
			const auto& font = context.current_font();
			Drawer drawer{.self = window};
			const IdMaker id_maker{.self = window};

			const auto font_size = drawer.font_size(context);

			const auto text_size = internal::text_size(font, utf8_text, font_size, Font::no_auto_wrap);

			const auto last_item_width = window.canvas_.item_width.back();

			const auto frame_point = window.canvas_.cursor_current_line;
			const auto frame_size = extent_type{last_item_width, text_size.height} + theme.item_frame_padding * 2;
			const rect_type frame_rect{frame_point, frame_size};

			const auto drop_down_button_width = font_size + theme.item_frame_padding.width * 2;
			const auto dropdown_button_point = frame_rect.right_top() - extent_type{drop_down_button_width, 0};
			const auto dropdown_button_size = extent_type{drop_down_button_width, drop_down_button_width};
			const rect_type dropdown_button_rect{dropdown_button_point, dropdown_button_size};

			const auto selection_point = frame_point + theme.item_frame_padding;
			const auto selection_size = frame_size - theme.item_frame_padding * 2;
			// const rect_type selection_rect{selection_point, selection_size};

			drawer.adjust_item_size(context, frame_size);

			// □ text
			window.same_line(context, auto_size, theme.item_inner_spacing.width);

			// text
			const auto text_point = window.canvas_.cursor_current_line + theme.item_frame_padding;
			const rect_type text_rect{text_point, text_size};
			drawer.adjust_item_size(context, text_size);

			const rect_type total_rect{frame_point, text_rect.right_bottom()};
			// if (not drawer.is_visible_area(context, total_rect))
			// {
			// 	// invisible
			// 	return false;
			// }
			// drawer.test_last_item(context, total_rect);
			if (not drawer.test_last_item_visible(context, total_rect))
			{
				// invisible
				return false;
			}

			bool value_changed = false;

			const auto id = id_maker.make_id(context, utf8_text);
			// note: Leave the frame inactive so that clicking on it again does not accidentally re-open the dropdown window again
			const auto state = context.queue_mouse(id, frame_rect);

			// draw frame
			drawer.draw_widget_frame(context, frame_rect, context.color_of(theme, ThemeCategory::FRAME_BACKGROUND));

			// draw dropdown button frame
			{
				if (state & MouseState::HOVERED)
				{
					drawer.draw_widget_frame(context, dropdown_button_rect, context.color_of(theme, ThemeCategory::BUTTON_HOVERED));
				}
				else
				{
					drawer.draw_widget_frame(context, dropdown_button_rect, context.color_of(theme, ThemeCategory::BUTTON));
				}

				const auto r = font_size * .5f;
				const auto center = dropdown_button_rect.center() - extent_type{0, r * .25f};

				const auto a = center + extent_type{0, 1} * r;
				const auto b = center + extent_type{-.667f, -.5f} * r;
				const auto c = center + extent_type{.667f, -.5f} * r;

				drawer.draw_widget_frame(context, a, b, c);
			}

			// fixme: leave empty or default?
			if (selected >= selections.size())
			{
				window.draw_list_.text(
					font,
					font_size,
					selection_point,
					context.color_of(theme, ThemeCategory::TEXT),
					"<unselected>",
					selection_size.width
				);
			}
			else
			{
				const auto& string = selections[selected];

				window.draw_list_.text(
					font,
					font_size,
					selection_point,
					context.color_of(theme, ThemeCategory::TEXT),
					string,
					selection_size.width
				);
			}

			// draw text
			window.draw_list_.text(
				font,
				font_size,
				text_rect.left_top(),
				context.color_of(theme, ThemeCategory::TEXT),
				utf8_text,
				text_rect.width()
			);

			// draw dropdown menu
			bool menu_toggled = false;
			if (state & MouseState::PRESSED)
			{
				menu_toggled = true;

				// If the combo is already open, click again to close the combo, otherwise open the combo
				if (context.is_combo_activated(id))
				{
					context.mark_combo_dead();
				}
				else
				{
					context.mark_combo_alive(id);
				}
			}

			if (context.is_combo_activated(id))
			{
				// The contents of the dropdown window should not affect the layout of the parent window, so after drawing the dropdown window, we reset the parent window canvas cursor
				const auto backup_position = drawer.cursor_position(context);
				{
					// const auto dropdown_offset_x = theme.item_inner_spacing.width;
					constexpr auto dropdown_offset_x = value_type{0};
					const auto dropdown_height =
							(text_size.height + theme.item_spacing.height) * static_cast<value_type>(std::ranges::min(selections.size(), show_selection_count)) +
							theme.window_padding.height;

					constexpr std::string_view combo_window_name{"@WINDOW::COMBO@"};
					const WindowFlag combo_window_flag
					{
							window.flag_.is<gui::WindowFlag::BORDERED>() ? gui::WindowFlag::BORDERED : gui::WindowFlag::NONE,
							WindowInternalFlag::CATEGORY_COMBO
					};

					const auto dropdown_point = point_type{frame_point.x + dropdown_offset_x, frame_point.y + frame_size.height};
					const auto dropdown_size = extent_type{frame_size.width - dropdown_offset_x, dropdown_height};

					// begin dropdown window (with background)
					context.begin_child_window(combo_window_name, dropdown_size, 1, combo_window_flag, false);
					{
						auto& combo_window = *window.children_this_frame_.back();
						auto& combo_window_canvas = combo_window.canvas_;
						IdMaker combo_window_id_maker{.self = combo_window};
						Drawer combo_window_drawer{.self = combo_window};

						const auto item_height = font_size;

						// Close the dropdown if the user interacts with another widget (unless the widget is the dropdown window's scrollbar)
						bool combo_item_active = false;
						combo_item_active |= context.is_widget_activated(combo_window.id_of_scrollbar(context));

						for (const auto view = selections | std::views::enumerate;
						     const auto [index, selection]: view)
						{
							const auto item_selected = std::cmp_equal(index, selected);
							const auto item_id = combo_window_id_maker.make_id(context, selection);

							const auto item_point = point_type{dropdown_point.x, combo_window_canvas.cursor_current_line.y - theme.item_spacing.height / 2};
							const auto item_size = extent_type{dropdown_size.width, item_height + theme.item_spacing.height};
							const rect_type item_rect{item_point, item_size};

							auto item_state = context.test_mouse(item_id, item_rect);
							combo_item_active |= context.is_widget_activated(item_id);

							if (item_state & MouseState::HOVERED or item_selected)
							{
								const auto color = [&]noexcept -> Theme::color_type
								{
									if (item_state & MouseState::HOVERED)
									{
										if (item_state & MouseState::KEEPING)
										{
											return context.color_of(theme, ThemeCategory::COMBO_ITEM_ACTIVATED);
										}
										return context.color_of(theme, ThemeCategory::COMBO_ITEM_HOVERED);
									}

									return context.color_of(theme, ThemeCategory::COMBO_ITEM);
								}();

								combo_window_drawer.draw_widget_frame(context, item_rect, color);
							}

							const auto& selection_text = selections[index];
							combo_window.draw_text(context, selection_text);

							// Scroll to the selected item when opening the window
							if (item_selected and menu_toggled)
							{
								combo_window_drawer.set_scroll_position_here(context);
							}

							// Close the dropdown window after selecting any item
							if (item_state & MouseState::PRESSED)
							{
								context.mark_widget_dead();
								context.mark_combo_dead();

								value_changed = true;
								selected = index;

								break;
							}
						}

						if (not combo_item_active and context.is_any_widget_activated())
						{
							context.mark_combo_dead();
						}
					}
					gui::end_child_window(context);
				}
				window.canvas_.cursor_current_line = backup_position;
			}

			return value_changed;
		}
	};

	Window::Window(
		const std::string_view name,
		const WindowFlag flag,
		const point_type& point,
		const extent_type& size,
		Window* root
	) noexcept
		: canvas_
		  {
				  .cursor_start_line = {0, 0},
				  .cursor_current_line = {0, 0},
				  .cursor_previous_line = {0, 0},
				  .height_current_line = 0,
				  .height_previous_line = 0,
				  .last_item_rect = {0, 0, 0, 0},
				  .last_item_hovered = false,
				  .last_item_focused = false,
				  .item_width = {},
				  .text_wrap_width = {}
		  },
		  name_{name},
		  flag_{flag},
		  root_{root},
		  point_{point},
		  size_full_{size},
		  size_{size},
		  size_of_content_{0, 0},
		  default_item_width_{0},
		  scroll_y_{0},
		  scroll_next_y_{0},
		  scroll_y_visible_{false},
		  visible_{false},
		  collapsed_{false},
		  skip_item_{true},
		  accessed_{false},
		  auto_fit_only_grows_{false},
		  auto_fit_frames_{-1},
		  last_drawn_frame_{frame_count_uninitialized}
	{
		// window id
		IdMaker{.self = *this}.make_id();

		if (flag.is<WindowInternalFlag::CHILD_WINDOW>())
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(root != nullptr);
		}
		else
		{
			root_ = this;
		}

		// auto-fit
		// if (size.width < 1e-3f or size.height < 1e-3f)
		{
			auto_fit_only_grows_ = true;
			auto_fit_frames_ = 2;
		}
	}

	auto Window::reset(const WindowFlag flag, const extent_type& size) noexcept -> void
	{
		flag_ = flag;

		if (flag_.is<WindowInternalFlag::CHILD_WINDOW>())
		{
			size_full_ = size;
		}
	}

	auto Window::handle_inputs(const Context& context) noexcept -> void
	{
		const auto& mouse = context.mouse();

		// scroll
		if (not flag_.is<gui::WindowFlag::NO_SCROLLBAR_WITH_MOUSE>())
		{
			// todo
			constexpr auto scroll_weight = static_cast<value_type>(5);
			scroll_next_y_ -= mouse.wheel * Drawer{.self = *this}.font_size(context) * scroll_weight;
		}
	}

	auto Window::begin_window(
		Context& context,
		Window* parent,
		alpha_type background_fill_alpha
	) noexcept -> bool
	{
		// This function can be called multiple times per frame,
		// but is only initialized the first time it is called (after that we can just append the contents)

		const auto current_frame_count = context.current_frame();
		const auto is_first_draw_this_frame = current_frame_count != last_drawn_frame_;

		const auto display_size = context.io().display_size;

		// ---------------------------------
		// initialize the window, set the window's clip rect
		{
			if (is_first_draw_this_frame)
			{
				// bind context
				draw_list_.bind_context(context);
				// clear render data
				draw_list_.reset();
				// clear all child
				children_this_frame_.clear();
				// show
				visible_ = true;
				// clear clip rect
				clip_rect_stack_.clear();
				// clear all ids
				id_stack_.clear();
				// generate a new id
				IdMaker{.self = *this}.make_id();

				if (flag_.is<WindowInternalFlag::CHILD_WINDOW>())
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parent != nullptr);
					parent->children_this_frame_.push_back(this);

					// Moves the child window to the current cursor position of the parent window
					point_ = parent->canvas_.cursor_current_line;
				}
			}

			// Outer clipping rectangle (window area)
			if (flag_.is<WindowInternalFlag::CHILD_WINDOW>())
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parent != nullptr);
				const auto& last = parent->clip_rect_stack_.back();

				// the viewing area of the child window must not exceed that of the parent window
				push_clip_rect(context, last);
			}
			else
			{
				// entire display area
				push_clip_rect(context, {0, 0, display_size});
			}
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(clip_rect_stack_.size() == 1);

		// ---------------------------------
		// initialize the canvas, set the canvas's clip rect
		bool close_button_pressed = false;
		{
			auto drawer = Drawer{*this};

			const auto& mouse = context.mouse();
			const auto& font = context.current_font();
			const auto& theme = context.current_theme();

			const auto font_size = drawer.font_size(context);
			const auto has_titlebar = not flag_.is<gui::WindowFlag::NO_TITLEBAR>();

			const auto is_child_window = flag_.is<WindowInternalFlag::CHILD_WINDOW>();
			const auto is_tooltip_window = flag_.is<WindowInternalFlag::CATEGORY_TOOLTIP>();

			if (is_child_window)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not has_titlebar, "The child window is not allowed to contain a titlebar!");
			}
			if (is_tooltip_window)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not has_titlebar, "The tootip window is not allowed to contain a titlebar!");
			}

			if (is_first_draw_this_frame)
			{
				if (last_drawn_frame_ + 1 < current_frame_count)
				{
					// The current window was not drawn in the last frame (or even many frames before that), this is usually because the window was just created, or the window was not visible before
					// Focus on the current window
					context.focus_window(*this);

					if (is_tooltip_window)
					{
						// In order to ensure that all the contents of the tooltip are visible, we delay the display of its contents by one frame
						visible_ = false;
						auto_fit_only_grows_ = false;
						auto_fit_frames_ = 2;
					}
				}
				last_drawn_frame_ = current_frame_count;

				// Follows the mouse if it's a tooltip window
				if (is_tooltip_window)
				{
					// todo: offset
					point_ = mouse.position_current + extent_type{32, 16} - theme.item_frame_padding * 2;
				}
				else
				{
					// If the user drags the window, we determine the new position of the window before the frame is drawn to avoid lag
					if (const auto activated = context.mark_widget_alive(id_of_move(context));
						activated)
					{
						if (mouse.is_down(context, MouseKey::LEFT))
						{
							// select current window
							context.focus_window(*this);

							if (not flag_.is<gui::WindowFlag::NO_MOVE>())
							{
								const auto delta = mouse.position_delta;

								// dragging
								point_ += delta;
							}
						}
						else
						{
							// No widgets are active
							context.mark_widget_dead();
						}
					}
				}

				if (not is_child_window)
				{
					const auto pad = extent_type{font_size * 2.f, font_size * 2.f};

					// Limit the current window from moving outside the program's visual area
					point_ = point_.clamp(
						(pad - size_).to<point_type>(),
						(display_size - pad).to<point_type>()
					);

					// Limit the current window to be not too small
					size_full_ = size_full_.combine_max(pad);
				}

				// set default item width
				if (
					size_.width > 0 and
					not is_tooltip_window and
					not flag_.is<gui::WindowFlag::AUTO_RESIZE>()
				)
				{
					default_item_width_ = size_.width * theme.item_default_width_factor;
				}
				else
				{
					// If this value is too small, the width of the widgets that use item_width will also be small,
					// if the width of the newly created (showed) window depends on these widgets,
					// the window may not be able to display the all widgets in its entirety
					// default_item_width_ = theme.window_min_size.width * theme.item_default_width_factor;
					default_item_width_ = 300;
				}

				// apply and clamp scrolling
				{
					scroll_y_ = std::ranges::max(scroll_next_y_, .0f);

					if (not collapsed_ and not skip_item_)
					{
						const float max_scroll_y = std::ranges::max(.0f, size_of_content_.height - size_full_.height);
						scroll_y_ = std::ranges::min(scroll_y_, max_scroll_y);
					}

					scroll_next_y_ = scroll_y_;
				}

				if (has_titlebar)
				{
					// `double left-click` on the `titlebar` of the `current window` to collapse the current window
					if (context.is_window_hovered(*this))
					{
						if (const auto rect = drawer.titlebar_rect(context);
							is_hovered(context, rect) and
							mouse.is_double_clicked(context, MouseKey::LEFT)
						)
						{
							// select current window
							context.focus_window(*this);

							// collapse window by double-clicking on titlebar
							collapsed_ = not collapsed_;
						}
					}
				}
				else
				{
					// Windows without a titlebar are not allowed to collapse
					collapsed_ = false;
				}

				if (collapsed_)
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(has_titlebar);

					// titlebar only
					const auto rect = drawer.titlebar_rect(context);
					size_ = rect.size();

					draw_list_.rect_filled(
						rect,
						context.color_of(theme, ThemeCategory::TITLEBAR_COLLAPSED),
						theme.window_corner_rounding
					);

					if (flag_.is<gui::WindowFlag::BORDERED>())
					{
						constexpr auto offset = extent_type{1, 1};

						draw_list_.rect(
							{rect.point + offset, rect.extent},
							context.color_of(theme, ThemeCategory::BORDER),
							theme.window_corner_rounding
						);
						draw_list_.rect(
							rect,
							context.color_of(theme, ThemeCategory::BORDER),
							theme.window_corner_rounding
						);
					}
				}
				else
				{
					size_ = size_full_;

					auto resize_grip_color = context.color_of(theme, ThemeCategory::RESIZE_GRIP);

					// The tooltip window always adapts to the size of the content
					if (is_tooltip_window)
					{
						if (auto_fit_frames_ > 0)
						{
							size_full_ = size_of_content_ + theme.window_padding - extent_type{0, theme.item_spacing.height};
						}
					}
					else
					{
						const auto auto_fit_size =
								(size_of_content_ + theme.window_auto_fit_padding).clamp(
									theme.window_min_size,
									display_size - theme.window_auto_fit_padding
								);

						if (flag_.is<gui::WindowFlag::AUTO_RESIZE>())
						{
							size_full_ = auto_fit_size;
						}
						else if (auto_fit_frames_ > 0)
						{
							// Auto-fit only grows during the first few frames
							if (auto_fit_only_grows_)
							{
								size_full_ = size_full_.combine_max(auto_fit_size);
							}
							else
							{
								size_full_ = auto_fit_size;
							}
						}
						else if (not flag_.is<gui::WindowFlag::NO_RESIZE>())
						{
							// resize grip
							const auto rect = drawer.resize_grip_rect(context);
							const auto id = id_of_resize(context);

							const auto state = context.test_mouse(id, rect);

							if (state & MouseState::KEEPING)
							{
								resize_grip_color = context.color_of(theme, ThemeCategory::RESIZE_GRIP_ACTIVATED);
							}
							else if (state & MouseState::HOVERED)
							{
								resize_grip_color = context.color_of(theme, ThemeCategory::RESIZE_GRIP_HOVERED);
							}

							if (state & MouseState::KEEPING)
							{
								// `double left-click` on the `resize-grip` to auto-fit the current window
								// allows the mouse to be offset from the current window area when resizing
								if (mouse.is_double_clicked(context, MouseKey::LEFT))
								{
									[[maybe_unused]] const auto old_size = size_;

									// auto-fit
									size_ = auto_fit_size;
									size_full_ = auto_fit_size;
								}
								else
								{
									const auto delta = mouse.position_delta;
									[[maybe_unused]] const auto old_size = size_;

									// resize
									size_ = theme.window_min_size.combine_max(size_full_ + delta);
									size_full_ = size_;
								}
							}
						}
					}

					// At this point, we can already determine the size of the window
					const auto current_titlebar_rect = [&]() noexcept -> rect_type
					{
						if (has_titlebar)
						{
							return drawer.titlebar_rect(context);
						}

						return drawer.titlebar_rect(context, 0);
					}();

					// background rect
					if (background_fill_alpha > 0)
					{
						draw_list_.rect_filled(
							{point_, size_},
							context.color_of(theme, ThemeCategory::WINDOW_BACKGROUND, background_fill_alpha),
							theme.window_corner_rounding
						);
					}

					// titlebar rect
					if (has_titlebar)
					{
						draw_list_.rect_filled(
							current_titlebar_rect,
							context.color_of(theme, ThemeCategory::TITLEBAR),
							theme.window_corner_rounding,
							DrawFlag::ROUND_CORNER_TOP
						);

						// titlebar border
						if (flag_.is<gui::WindowFlag::BORDERED>())
						{
							draw_list_.line(
								current_titlebar_rect.left_bottom(),
								current_titlebar_rect.right_bottom(),
								context.color_of(theme, ThemeCategory::BORDER)
							);
						}
					}

					// background border
					if (flag_.is<gui::WindowFlag::BORDERED>())
					{
						constexpr auto offset = extent_type{1, 1};

						draw_list_.rect(
							{point_ + offset, size_},
							context.color_of(theme, ThemeCategory::BORDER_SHADOW),
							theme.window_corner_rounding
						);
						draw_list_.rect(
							{point_, size_},
							context.color_of(theme, ThemeCategory::BORDER),
							theme.window_corner_rounding
						);
					}

					// scrollbar
					if (
						// no scrollbar
						flag_.is<gui::WindowFlag::NO_SCROLLBAR>() or
						// window space is greater than the space required for content
						size_.height > size_of_content_.height)
					{
						scroll_y_visible_ = false;
					}
					else
					{
						scroll_y_visible_ = true;

						const auto window_rect = rect();

						const auto right_top = window_rect.right_top();
						const auto right_bottom = window_rect.right_bottom();

						const auto background_y1 = right_top.y + current_titlebar_rect.height() + 1;
						const auto background_y2 = right_bottom.y - 1;
						const auto background_height = background_y2 - background_y1;

						// scrollbar background
						const point_type background_point{right_bottom.x - theme.window_vertical_scrollbar_width, background_y1};
						const extent_type background_size{theme.window_vertical_scrollbar_width, background_height};
						const rect_type background_rect{background_point, background_size};
						draw_list_.rect_filled(
							background_rect,
							context.color_of(theme, ThemeCategory::SCROLLBAR_BACKGROUND)
						);

						// scrollbar area  
						constexpr extent_type scrollbar_area_offset{3, 3};
						const auto scrollbar_area_point = background_point + scrollbar_area_offset;
						const auto scrollbar_area_size = background_size - scrollbar_area_offset * 2;
						const rect_type scrollbar_area_rect{scrollbar_area_point, scrollbar_area_size};

						const auto grab_size_y_normalized = std::ranges::clamp(
							size_.height / std::ranges::max(size_of_content_.height, size_.height),
							.0f,
							1.f
						);
						const auto grab_size_y = scrollbar_area_size.height * grab_size_y_normalized;

						auto grab_color = context.color_of(theme, ThemeCategory::SCROLLBAR_GRAB);
						if (grab_size_y_normalized < 1.f)
						{
							const auto id = id_of_scrollbar(context);

							const auto state = context.test_mouse(id, scrollbar_area_rect);

							if (state & MouseState::KEEPING)
							{
								grab_color = context.color_of(theme, ThemeCategory::SCROLLBAR_GRAB_ACTIVATED);

								const auto y_normalized = std::ranges::clamp(
									                          (mouse.position_current.y - (scrollbar_area_point.y + grab_size_y * .5f)) / (scrollbar_area_size.height - grab_size_y),
									                          .0f,
									                          1.f
								                          ) * (1.f - grab_size_y_normalized);

								scroll_y_ = size_of_content_.height * y_normalized;
								scroll_next_y_ = scroll_y_;
							}
							else if (state & MouseState::HOVERED)
							{
								grab_color = context.color_of(theme, ThemeCategory::SCROLLBAR_GRAB_HOVERED);
							}
						}

						// Normalized height of the grab
						const auto y_normalized = std::ranges::clamp(
							scroll_y_ / std::ranges::max(.00001f, size_of_content_.height),
							.0f,
							1.f
						);

						const auto grab_point = scrollbar_area_point + extent_type{0, scrollbar_area_size.height * y_normalized};
						const auto grab_size = extent_type{scrollbar_area_size.width, scrollbar_area_size.height * grab_size_y_normalized};
						const rect_type grab_rect{grab_point, grab_size};

						draw_list_.rect_filled(grab_rect, grab_color);
					}

					// resize-grip
					if (not flag_.is<gui::WindowFlag::NO_RESIZE>())
					{
						const auto rect = drawer.resize_grip_rect(context);
						// if (const auto rounding = theme.window_corner_rounding;
						// 	rounding == 0) // NOLINT(clang-diagnostic-float-equal)
						// {
						// 	draw_list_.triangle_filled(
						// 		rect.right_bottom(),
						// 		rect.left_bottom(),
						// 		rect.right_top(),
						// 		resize_grip_color
						// 	);
						// }
						// else
						// {
						// todo
						draw_list_.triangle_filled(
							rect.right_bottom(),
							rect.left_bottom(),
							rect.right_top(),
							resize_grip_color
						);
						// }
					}
				}

				// reset contents size for auto-fitting
				size_of_content_ = {0, 0};
				if (auto_fit_frames_ > 0)
				{
					auto_fit_frames_ -= 1;
				}

				// titlebar context
				if (has_titlebar)
				{
					if (not flag_.is<gui::WindowFlag::NO_CLOSE>())
					{
						const auto rect = drawer.close_button_rect(context);
						const auto id = id_of_close(context);

						const auto state = context.test_mouse(id, rect);

						auto close_button_color = context.color_of(theme, ThemeCategory::CLOSE_BUTTON);

						if (state & MouseState::HOVERED)
						{
							if (state & MouseState::KEEPING)
							{
								close_button_color = context.color_of(theme, ThemeCategory::CLOSE_BUTTON_ACTIVATED);
							}
							else
							{
								close_button_color = context.color_of(theme, ThemeCategory::CLOSE_BUTTON_HOVERED);
							}
						}

						const auto center = rect.center();

						// circle
						draw_list_.circle_filled(
							center,
							std::ranges::max(2.f, rect.height() * .5f),
							close_button_color,
							16
						);

						// ×
						if (state & MouseState::HOVERED)
						{
							const auto x = rect.extent.width * .5f * .667f - 1.f;
							const auto y = rect.extent.height * .5f * .667f - 1.f;

							draw_list_.line(
								center + extent_type{x, y},
								center + extent_type{-x, -y},
								context.color_of(theme, ThemeCategory::TEXT)
							);
							draw_list_.line(
								center + extent_type{x, -y},
								center + extent_type{-x, y},
								context.color_of(theme, ThemeCategory::TEXT)
							);
						}

						close_button_pressed = state & MouseState::PRESSED;
					}

					// title text
					const auto text_point = point_ + theme.item_frame_padding;
					const auto text_size = internal::text_size(font, name_, font_size, Font::no_auto_wrap);

					const auto max_width = size_.width - theme.item_frame_padding.width * 2;
					const auto max_height = size_.height;

					if (const auto do_clip = text_size.width > max_width or text_size.height > max_height;
						do_clip)
					{
						const auto width = std::ranges::min(text_size.width, max_width);
						const auto height = std::ranges::min(text_size.height, max_height);

						// If the titlebar is not large enough to accommodate the title content,
						// simply discard the content that exceeds the space,
						// rather than displaying the content on a new line
						push_clip_rect(context, {text_point, width, height});
						draw_list_.text(
							font,
							font_size,
							text_point,
							context.color_of(theme, ThemeCategory::TEXT),
							name_,
							width
						);
						pop_clip_rect(context);
					}
					else
					{
						draw_list_.text(
							font,
							font_size,
							text_point,
							context.color_of(theme, ThemeCategory::TEXT),
							name_,
							text_size.width
						);
					}
				}

				// init canvas
				{
					const auto current_titlebar_rect = [&]() noexcept -> rect_type
					{
						if (has_titlebar)
						{
							return drawer.titlebar_rect(context);
						}

						return drawer.titlebar_rect(context, 0);
					}();

					canvas_.cursor_start_line =
							// window start point
							point_ +
							// todo X: columns offset
							// Y: titlebar + padding
							extent_type{8, current_titlebar_rect.height() + drawer.window_padding(context).height} +
							// Y: scrollbar
							extent_type{0, -scroll_y_};
					canvas_.cursor_current_line = canvas_.cursor_start_line;
					canvas_.cursor_previous_line = canvas_.cursor_current_line;

					canvas_.height_current_line = 0;
					canvas_.height_previous_line = 0;

					canvas_.item_width.clear();
					canvas_.item_width.push_back(default_item_width_);

					canvas_.text_wrap_width.clear();
					static_assert(DrawList::text_wrap_width_not_set < 0);
					canvas_.text_wrap_width.push_back(DrawList::text_wrap_width_not_set);
				}
			}

			// Inner clipping rectangle (canvas area)
			{
				const auto window_rect = rect();
				const auto current_window_padding = drawer.window_padding(context);
				const auto current_titlebar_rect = [&]() noexcept -> rect_type
				{
					if (has_titlebar)
					{
						return drawer.titlebar_rect(context);
					}

					return drawer.titlebar_rect(context, 0);
				}();

				const point_type clip_rect_point
				{
						current_titlebar_rect.left_bottom().x + current_window_padding.width * .5f + .5f,
						current_titlebar_rect.left_bottom().y + .5f
				};
				const extent_type clip_rect_size
				{
						current_titlebar_rect.width() - current_window_padding.width - (scroll_y_visible_ ? theme.window_vertical_scrollbar_width : 0),
						window_rect.height() - current_titlebar_rect.height() - 2
				};
				const rect_type clip_rect{clip_rect_point, clip_rect_size};

				push_clip_rect(context, clip_rect);
			}

			// Child windows may not be visible (located in areas not visible to the parent window), we collapse it manually (so that we can skip the widgets drawn on it earlier)
			if (is_child_window)
			{
				const auto last_clip_rect = clip_rect_stack_.back();
				collapsed_ = not last_clip_rect.valid();

				// We also hide the window from rendering because we've already added its border to the command list
				// (we could perform the check earlier in the function, but it is simpler at this point)
				if (collapsed_)
				{
					visible_ = false;
				}
			}

			if (theme.alpha <= 0)
			{
				visible_ = false;
			}

			skip_item_ = collapsed_ or (not visible_ and auto_fit_frames_ == 0);

			if (is_first_draw_this_frame)
			{
				accessed_ = false;
			}
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(clip_rect_stack_.size() == 2);

		return close_button_pressed;
	}

	auto Window::end_window(Context& context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(clip_rect_stack_.size() == 2);

		// canvas rect
		pop_clip_rect(context);

		// window rect
		pop_clip_rect(context);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(clip_rect_stack_.empty());

		// window_data.root = nullptr;
	}

	auto Window::begin_child_window(
		Context& context,
		const std::string_view name,
		const alpha_type background_fill_alpha,
		extent_type size,
		const bool border,
		WindowFlag flag
	) noexcept -> void
	{
		const auto& theme = context.current_theme();

		flag |= gui::WindowFlag::NO_TITLEBAR | gui::WindowFlag::NO_CLOSE | gui::WindowFlag::NO_RESIZE | gui::WindowFlag::NO_MOVE;
		flag |= WindowInternalFlag::CHILD_WINDOW;

		if (border)
		{
			flag |= gui::WindowFlag::BORDERED;
		}

		const auto content_region = content_region_max(context);
		const auto remaining_size = content_region - (canvas_.cursor_current_line - point_);

		if (size.width <= 0)
		{
			flag |= WindowInternalFlag::CHILD_WINDOW_AUTO_FIT_X;

			size.width = std::ranges::max(remaining_size.width, theme.window_min_size.width);
		}
		if (size.height <= 0)
		{
			flag |= WindowInternalFlag::CHILD_WINDOW_AUTO_FIT_Y;

			size.height = std::ranges::max(remaining_size.height, theme.window_min_size.height);
		}

		const auto child_window_name = std::format("{}.{}", name_, name);
		context.begin_window(child_window_name, size, background_fill_alpha, flag);
		auto& window = context.current_window();

		if (border and not flag_.is<gui::WindowFlag::BORDERED>())
		{
			auto& child = *children_this_frame_.back();
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(std::addressof(window) == std::addressof(child));
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(child.name_ == child_window_name);

			// border only indicates whether the child window has a border, not the widgets of the child window
			child.flag_ &= ~gui::WindowFlag::BORDERED;
		}
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Window::end_child_window(Context& context, Window& child) noexcept -> void
	{
		context.end_window();

		// When using autofill child window, we don't provide the width/height to `adjust_item_size` so that it doesn't feed back into automatic size-fitting
		auto size = child.size_;
		if (child.flag_.is<WindowInternalFlag::CHILD_WINDOW_AUTO_FIT_X>())
		{
			size.width = 0;
		}
		if (child.flag_.is<WindowInternalFlag::CHILD_WINDOW_AUTO_FIT_Y>())
		{
			size.height = 0;
		}

		Drawer drawer{.self = *this};
		drawer.adjust_item_size(context, size);
	}

	auto Window::render(Context& context, draw_lists_type& draw_lists) const noexcept -> void
	{
		if (not visible_)
		{
			return;
		}

		draw_lists.emplace_back(draw_list_);

		std::ranges::for_each(
			children_this_frame_,
			[&](auto& child) noexcept -> void
			{
				child->render(context, draw_lists);
			}
		);
	}

	auto Window::draw_text(Context& context, const std::string_view utf8_text) noexcept -> void
	{
		if (skip_item_)
		{
			return;
		}
		accessed_ = true;

		const auto& theme = context.current_theme();
		const auto& font = context.current_font();
		Drawer drawer{.self = const_cast<Window&>(*this)};

		const auto font_size = drawer.font_size(context);

		static_assert(DrawList::text_wrap_width_not_set < 0);
		const auto wrap_width = canvas_.text_wrap_width.back();
		const auto wrap_enabled = wrap_width >= .0f;

		if (constexpr std::size_t long_text_threshold = 1500;
			utf8_text.size() > long_text_threshold and not wrap_enabled)
		{
			const auto clip_rect = clip_rect_stack_.back();
			const auto clip_rect_left_top = clip_rect.left_top();
			const auto clip_rect_left_bottom = clip_rect.left_bottom();

			extent_type text_size{0, 0};

			// Only text that is in the visible area is rendered
			if (const auto visible_area_height = clip_rect_left_bottom.y - canvas_.cursor_current_line.y;
				visible_area_height > 0)
			{
				const auto invisible_area_height = clip_rect_left_top.y - canvas_.cursor_current_line.y;

				const auto line_height = drawer.font_size(context);
				const auto invisible_lines = static_cast<int>(invisible_area_height / line_height - 1);
				const auto visible_lines = static_cast<int>(visible_area_height / line_height - 1);

				const auto has_invisible_lines = invisible_lines > 0;

				auto view = utf8_text | std::views::split('\n');
				auto visible_view = view | std::views::drop(has_invisible_lines ? invisible_lines : 0) | std::views::take(visible_lines);

				auto text_point = canvas_.cursor_current_line;

				if (has_invisible_lines)
				{
					text_point.y += static_cast<value_type>(invisible_lines) * line_height;
				}

				for (const auto sub: visible_view)
				{
					const std::string_view sub_string{sub};
					const auto this_line_text_size = internal::text_size(
						font,
						sub_string,
						font_size,
						Font::no_auto_wrap
					);

					// draw one line
					draw_list_.text(
						font,
						font_size,
						text_point,
						context.color_of(theme, ThemeCategory::TEXT),
						sub_string,
						Font::no_auto_wrap
					);

					text_point.y += line_height;
					text_size.width = std::ranges::max(text_size.width, this_line_text_size.width);
				}

				text_size.height = static_cast<value_type>(std::ranges::distance(view)) * line_height;
			}

			const rect_type rect{canvas_.cursor_current_line, text_size};

			drawer.adjust_item_size(context, text_size);
			// todo: test visible?
			// drawer.test_last_item(context, rect);
			std::ignore = drawer.test_last_item_visible(context, rect);
		}
		else
		{
			const auto this_wrap_width = wrap_enabled ? wrap_width : Font::no_auto_wrap;

			const auto text_point = canvas_.cursor_current_line;
			const auto text_size = internal::text_size(
				font,
				utf8_text,
				font_size,
				this_wrap_width
			);
			const rect_type rect{text_point, text_size};

			drawer.adjust_item_size(context, text_size);
			// if (not drawer.is_visible_area(context, rect))
			// {
			// 	return;
			// }
			// drawer.test_last_item(context, rect);
			if (not drawer.test_last_item_visible(context, rect))
			{
				// invisible
				return;
			}

			draw_list_.text(
				font,
				font_size,
				text_point,
				context.color_of(theme, ThemeCategory::TEXT),
				utf8_text,
				this_wrap_width
			);
		}
	}

	auto Window::draw_button(Context& context, const std::string_view utf8_text, extent_type size, const bool repeat_when_held) noexcept -> bool
	{
		if (skip_item_)
		{
			return false;
		}
		accessed_ = true;

		const auto& theme = context.current_theme();
		const auto& font = context.current_font();
		Drawer drawer{.self = const_cast<Window&>(*this)};
		const IdMaker id_maker{.self = *this};

		const auto font_size = drawer.font_size(context);

		const auto text_size = internal::text_size(font, utf8_text, font_size, Font::no_auto_wrap);

		if (size.width <= 0)
		{
			size.width = text_size.width;
		}
		if (size.height <= 0)
		{
			size.height = text_size.height;
		}

		const auto button_point = canvas_.cursor_current_line;
		const auto button_size = size + theme.item_frame_padding * 2;
		const rect_type button_rect{button_point, button_size};

		drawer.adjust_item_size(context, button_size);
		// if (not drawer.is_visible_area(context, button_rect))
		// {
		// 	// invisible
		// 	return false;
		// }
		// drawer.test_last_item(context, button_rect);
		if (not drawer.test_last_item_visible(context, button_rect))
		{
			// invisible
			return false;
		}

		const auto id = id_maker.make_id(context, utf8_text);
		const auto state = context.test_mouse(id, button_rect, repeat_when_held);

		color_type button_color = context.color_of(theme, ThemeCategory::BUTTON);
		{
			if (state & MouseState::KEEPING or state & MouseState::PRESSED)
			{
				button_color = context.color_of(theme, ThemeCategory::BUTTON_ACTIVATED);
			}
			else if (state & MouseState::HOVERED)
			{
				button_color = context.color_of(theme, ThemeCategory::BUTTON_HOVERED);
			}
		}

		// draw □
		drawer.draw_widget_frame(context, button_rect, button_color);

		const auto text_area_point = button_rect.left_top() + theme.item_frame_padding;

		// the given size may be smaller than the required size of the text, in which case the button text needs to be truncated
		const auto clip = size.width < text_size.width or size.height < text_size.height;
		if (clip)
		{
			const rect_type rect
			{
					text_area_point,
					// Allow extra to draw over the horizontal padding to make it visible that text doesn't fit
					button_rect.right_bottom() - extent_type{0, theme.item_frame_padding.height}
			};

			push_clip_rect(context, rect);
		}

		const auto text_offset = (size - text_size).combine_max({0, 0}) * .5f;
		const auto text_point =
				text_area_point +
				text_offset;

		// draw text
		draw_list_.text(
			font,
			font_size,
			text_point,
			context.color_of(theme, ThemeCategory::TEXT),
			utf8_text,
			button_rect.width()
		);

		if (clip)
		{
			pop_clip_rect(context);
		}

		return state & MouseState::PRESSED;
	}

	auto Window::draw_small_button(Context& context, const std::string_view utf8_text, const bool repeat_when_held) noexcept -> bool
	{
		if (skip_item_)
		{
			return false;
		}
		accessed_ = true;

		const auto& theme = context.current_theme();
		const auto& font = context.current_font();
		Drawer drawer{.self = const_cast<Window&>(*this)};
		const IdMaker id_maker{.self = *this};

		const auto font_size = drawer.font_size(context);

		const auto text_size = internal::text_size(font, utf8_text, font_size, Font::no_auto_wrap);

		const auto button_point = canvas_.cursor_current_line;
		const auto button_size = text_size + theme.item_frame_padding * 2;
		const rect_type button_rect{button_point, button_size};

		drawer.adjust_item_size(context, button_size);
		// if (not drawer.is_visible_area(context, button_rect))
		// {
		// 	// invisible
		// 	return false;
		// }
		// drawer.test_last_item(context, button_rect);
		if (not drawer.test_last_item_visible(context, button_rect))
		{
			// invisible
			return false;
		}

		const auto id = id_maker.make_id(context, utf8_text);
		const auto state = context.test_mouse(id, button_rect, repeat_when_held);

		color_type button_color = context.color_of(theme, ThemeCategory::BUTTON);
		{
			if (state & MouseState::KEEPING or state & MouseState::PRESSED)
			{
				button_color = context.color_of(theme, ThemeCategory::BUTTON_ACTIVATED);
			}
			else if (state & MouseState::HOVERED)
			{
				button_color = context.color_of(theme, ThemeCategory::BUTTON_HOVERED);
			}
		}

		// draw □
		drawer.draw_widget_frame(context, button_rect, button_color);

		const auto text_area_point = button_rect.left_top() + theme.item_frame_padding;

		// draw text
		draw_list_.text(
			font,
			font_size,
			text_area_point,
			context.color_of(theme, ThemeCategory::TEXT),
			utf8_text,
			button_rect.width()
		);

		return state & MouseState::PRESSED;
	}

	auto Window::draw_radio_button(Context& context, const std::string_view utf8_text, const bool checked) noexcept -> bool
	{
		if (skip_item_)
		{
			return false;
		}
		accessed_ = true;

		const auto& theme = context.current_theme();
		const auto& font = context.current_font();
		Drawer drawer{.self = const_cast<Window&>(*this)};
		const IdMaker id_maker{.self = *this};

		const auto font_size = drawer.font_size(context);

		const auto text_size = internal::text_size(font, utf8_text, font_size, Font::no_auto_wrap);

		// ○ + text

		// ○, diameter equals string rect height
		const auto check_point = canvas_.cursor_current_line;
		const auto check_size = extent_type{text_size.height + theme.item_frame_padding.height * 2 - 1, text_size.height + theme.item_frame_padding.height * 2 - 1};
		const rect_type check_rect{check_point, check_size};
		const circle_type check_circle{check_point + check_size / 2, check_size.width / 2};
		drawer.adjust_item_size(context, check_size);

		// ○ text
		same_line(context, auto_size, theme.item_inner_spacing.width);

		// text
		const auto text_point = canvas_.cursor_current_line + extent_type{0, theme.item_frame_padding.height};
		const rect_type text_rect{text_point, text_size};
		drawer.adjust_item_size(context, text_size);

		const rect_type total_rect{check_rect.left_top(), text_rect.right_bottom()};
		// if (not drawer.is_visible_area(context, total_rect))
		// {
		// 	// invisible
		// 	return false;
		// }
		// drawer.test_last_item(context, total_rect);
		if (not drawer.test_last_item_visible(context, total_rect))
		{
			// invisible
			return false;
		}

		const auto id = id_maker.make_id(context, utf8_text);
		// fixme: test check_rect or total_rect?
		const auto state = context.test_mouse(id, check_rect, false);

		// draw ○
		if (state & MouseState::HOVERED)
		{
			drawer.draw_widget_frame(context, check_circle, context.color_of(theme, ThemeCategory::RADIO_BUTTON_HOVERED));
		}
		else
		{
			drawer.draw_widget_frame(context, check_circle, context.color_of(theme, ThemeCategory::FRAME_BACKGROUND));
		}

		if (checked)
		{
			const auto check_fill_point = check_point + theme.item_inner_spacing;
			const auto check_fill_size = check_size - theme.item_inner_spacing * 2;
			const circle_type check_fill_circle{check_fill_point + check_fill_size / 2, check_fill_size.width / 2};
			draw_list_.circle_filled(check_fill_circle, context.color_of(theme, ThemeCategory::RADIO_BUTTON_ACTIVATED));
		}

		// draw text
		draw_list_.text(
			font,
			font_size,
			text_rect.left_top(),
			context.color_of(theme, ThemeCategory::TEXT),
			utf8_text,
			text_rect.width()
		);

		return state & MouseState::PRESSED;
	}

	auto Window::draw_checkbox(Context& context, const std::string_view utf8_text, bool checked) noexcept -> bool
	{
		if (skip_item_)
		{
			return false;
		}
		accessed_ = true;

		const auto& theme = context.current_theme();
		const auto& font = context.current_font();
		Drawer drawer{.self = const_cast<Window&>(*this)};
		const IdMaker id_maker{.self = *this};

		const auto font_size = drawer.font_size(context);

		const auto text_size = internal::text_size(font, utf8_text, font_size, Font::no_auto_wrap);

		// □ + text
		const auto width = text_size.height;

		// □, side length equals string rect height
		const auto check_point = canvas_.cursor_current_line;
		const auto check_size = extent_type{width + theme.item_frame_padding.height * 2, text_size.height + theme.item_frame_padding.height * 2};
		const rect_type check_rect{check_point, check_size};
		drawer.adjust_item_size(context, check_size);

		// □ text
		same_line(context, auto_size, theme.item_inner_spacing.width);

		// text
		const auto text_point = canvas_.cursor_current_line + extent_type{0, theme.item_frame_padding.height};
		const rect_type text_rect{text_point, text_size};
		drawer.adjust_item_size(context, text_size);

		const rect_type total_rect{check_rect.left_top(), text_rect.right_bottom()};
		// if (not drawer.is_visible_area(context, total_rect))
		// {
		// 	// invisible
		// 	return false;
		// }
		// drawer.test_last_item(context, total_rect);
		if (not drawer.test_last_item_visible(context, total_rect))
		{
			// invisible
			return false;
		}

		const auto id = id_maker.make_id(context, utf8_text);
		// fixme: test check_rect or total_rect?
		const auto state = context.test_mouse(id, check_rect, false);

		// draw □
		if (state & MouseState::HOVERED)
		{
			drawer.draw_widget_frame(context, check_rect, context.color_of(theme, ThemeCategory::CHECKBOX_HOVERED));
		}
		else
		{
			drawer.draw_widget_frame(context, check_rect, context.color_of(theme, ThemeCategory::FRAME_BACKGROUND));
		}

		if (state & MouseState::PRESSED)
		{
			checked = not checked;
		}

		if (checked)
		{
			const auto check_fill_point = check_point + theme.item_inner_spacing;
			const auto check_fill_size = check_size - theme.item_inner_spacing * 2;
			const rect_type check_fill_rect{check_fill_point, check_fill_size};
			draw_list_.rect_filled(check_fill_rect, context.color_of(theme, ThemeCategory::CHECKBOX_ACTIVATED));
		}

		// draw text
		draw_list_.text(
			font,
			font_size,
			text_rect.left_top(),
			context.color_of(theme, ThemeCategory::TEXT),
			utf8_text,
			text_rect.width()
		);

		return checked;
	}

	auto Window::draw_slider(
		Context& context,
		const std::string_view utf8_text,
		float& reference,
		const float min,
		const float max,
		const std::uint32_t decimal_precision,
		const float power
	) noexcept -> bool
	{
		Anonymous anonymous{.self = *this};

		return anonymous.draw_slider(context, utf8_text, reference, min, max, decimal_precision, power);
	}

	auto Window::draw_slider_n(
		Context& context,
		const std::string_view utf8_text,
		std::span<float> references,
		const float min,
		const float max,
		const std::uint32_t decimal_precision,
		const float power
	) noexcept -> bool
	{
		Anonymous anonymous{.self = *this};

		return anonymous.draw_slider(context, utf8_text, references, min, max, decimal_precision, power);
	}

	auto Window::draw_combo(
		Context& context,
		const std::string_view utf8_text,
		const std::span<const std::string> selections,
		std::size_t& selected,
		const std::size_t show_selection_count
	) noexcept -> bool
	{
		Anonymous anonymous{.self = *this};

		return anonymous.draw_combo<const std::string>(context, utf8_text, selections, selected, show_selection_count);
	}

	auto Window::draw_combo(
		Context& context,
		const std::string_view utf8_text,
		const std::span<const std::string_view> selections,
		std::size_t& selected,
		const std::size_t show_selection_count
	) noexcept -> bool
	{
		Anonymous anonymous{.self = *this};

		return anonymous.draw_combo<const std::string_view>(context, utf8_text, selections, selected, show_selection_count);
	}

	auto Window::draw_combo(
		Context& context,
		const std::string_view utf8_text,
		const std::span<const char*> selections,
		std::size_t& selected,
		const std::size_t show_selection_count
	) noexcept -> bool
	{
		Anonymous anonymous{.self = *this};

		return anonymous.draw_combo<const char*>(context, utf8_text, selections, selected, show_selection_count);
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Window::same_line(Context& context, const value_type column_width, value_type spacing_width) noexcept -> void
	{
		if (collapsed_)
		{
			return;
		}

		const auto& theme = context.current_theme();

		canvas_.height_current_line = canvas_.height_previous_line;
		canvas_.cursor_current_line = canvas_.cursor_previous_line;

		if (column_width < 0)
		{
			if (spacing_width < 0)
			{
				spacing_width = theme.item_spacing.width;
			}

			canvas_.cursor_current_line.x += spacing_width;
		}
		else
		{
			spacing_width = std::ranges::max(spacing_width, static_cast<value_type>(0));

			canvas_.cursor_current_line.x = column_width + spacing_width;
		}
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Window::push_item_width(Context& context, const Theme::value_type new_item_width) noexcept -> void
	{
		std::ignore = context;

		canvas_.item_width.push_back(new_item_width > 0 ? new_item_width : default_item_width_);
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Window::pop_item_width(Context& context) noexcept -> void
	{
		std::ignore = context;

		canvas_.item_width.pop_back();
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Window::push_text_wrap_width(Context& context, const Theme::value_type new_wrap_width) noexcept -> void
	{
		std::ignore = context;

		canvas_.text_wrap_width.push_back(new_wrap_width);
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Window::pop_text_wrap_width(Context& context) noexcept -> void
	{
		std::ignore = context;

		canvas_.text_wrap_width.pop_back();
	}

	auto Window::id_of_move(Context& context) const noexcept -> widget_id_type
	{
		constexpr std::string_view name{"@WINDOW::MOVE@"};
		return IdMaker{.self = const_cast<Window&>(*this)}.make_id(context, name);
	}

	auto Window::id_of_close(Context& context) const noexcept -> widget_id_type
	{
		constexpr std::string_view name{"@WINDOW::CLOSE@"};
		return IdMaker{.self = const_cast<Window&>(*this)}.make_id(context, name);
	}

	auto Window::id_of_resize(Context& context) const noexcept -> widget_id_type
	{
		constexpr std::string_view name{"@WINDOW::RESIZE@"};
		return IdMaker{.self = const_cast<Window&>(*this)}.make_id(context, name);
	}

	auto Window::id_of_scrollbar(Context& context) const noexcept -> widget_id_type
	{
		constexpr std::string_view name{"@WINDOW::SCROLLBAR@"};
		return IdMaker{.self = const_cast<Window&>(*this)}.make_id(context, name);
	}

	auto Window::push_id(Context& context, const std::string_view string) noexcept -> void
	{
		const auto id = IdMaker{.self = *this}.make_id(context, string);
		id_stack_.push_back(id);
	}

	auto Window::push_id(Context& context, const void* pointer) noexcept -> void
	{
		const auto id = IdMaker{.self = *this}.make_id(context, pointer);
		id_stack_.push_back(id);
	}

	auto Window::push_id(Context& context, const widget_id_type value) noexcept -> void
	{
		const auto id = IdMaker{.self = *this}.make_id(context, value);
		id_stack_.push_back(id);
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Window::pop_id(Context& context) noexcept -> void
	{
		std::ignore = context;

		id_stack_.pop_back();
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Window::push_clip_rect(Context& context, const rect_type& rect, const bool clipped) noexcept -> void
	{
		std::ignore = context;

		const auto clip_rect = [&]() noexcept -> rect_type
		{
			if (clipped and not clip_rect_stack_.empty())
			{
				// clip to a new rect
				const auto last = clip_rect_stack_.back();
				return last.combine_min(rect);
			}

			return rect;
		}();

		clip_rect_stack_.push_back(clip_rect);
		draw_list_.push_clip_rect(clip_rect, false);
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto Window::pop_clip_rect(Context& context) noexcept -> void
	{
		clip_rect_stack_.pop_back();

		if (clip_rect_stack_.empty())
		{
			const auto size = context.io().display_size;
			draw_list_.push_clip_rect({0, 0, size}, false);
		}
		else
		{
			draw_list_.push_clip_rect(clip_rect_stack_.back(), false);
		}
	}

	auto Window::name() const noexcept -> std::string_view
	{
		return name_;
	}

	auto Window::flag() const noexcept -> WindowFlag
	{
		return flag_;
	}

	auto Window::position() const noexcept -> point_type
	{
		return point_;
	}

	auto Window::width() const noexcept -> value_type
	{
		return size_.width;
	}

	auto Window::height() const noexcept -> value_type
	{
		return size_.height;
	}

	auto Window::size() const noexcept -> extent_type
	{
		return size_;
	}

	auto Window::rect() const noexcept -> rect_type
	{
		return {point_, size_};
	}

	auto Window::visible() const noexcept -> bool
	{
		return visible_;
	}

	auto Window::collapsed() const noexcept -> bool
	{
		return collapsed_;
	}

	auto Window::root() const noexcept -> const Window&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(root_ != nullptr);

		return *root_;
	}

	auto Window::content_region_max(const Context& context) const noexcept -> extent_type
	{
		const auto& theme = context.current_theme();
		const Drawer drawer{.self = const_cast<Window&>(*this)};

		auto size = size_ - drawer.window_padding(context);

		// todo
		if (scroll_y_visible_)
		{
			size.width -= theme.window_vertical_scrollbar_width;
		}

		return size;
	}

	auto Window::window_content_region_min(const Context& context) const noexcept -> extent_type
	{
		const Drawer drawer{.self = const_cast<Window&>(*this)};

		// titlebar + padding
		const auto titlebar_height = drawer.titlebar_height(context);
		const auto padding = drawer.window_padding(context);

		return extent_type{0, titlebar_height} + padding;
	}

	auto Window::window_content_region_max(const Context& context) const noexcept -> extent_type
	{
		const auto& theme = context.current_theme();
		const Drawer drawer{.self = const_cast<Window&>(*this)};

		auto size = size_ - drawer.window_padding(context);

		if (scroll_y_visible_)
		{
			size.width -= theme.window_vertical_scrollbar_width;
		}

		return size;
	}

	auto Window::is_hovered(const Context& context, const rect_type& rect) const noexcept -> bool
	{
		const auto& mouse = context.mouse();
		const auto position = mouse.position_current;

		const auto clipped = [&]() noexcept -> rect_type
		{
			if (not clip_rect_stack_.empty())
			{
				const auto& last = clip_rect_stack_.back();

				return rect.combine_min(last);
			}

			return rect;
		}();

		return clipped.includes(position);
	}

	auto Window::is_item_hovered(const Context& context) const noexcept -> bool
	{
		std::ignore = context;

		return canvas_.last_item_hovered;
	}

	auto Window::is_item_focused(const Context& context) const noexcept -> bool
	{
		std::ignore = context;

		return canvas_.last_item_focused;
	}

	// auto Window::show() noexcept -> void
	// {
	// 	visible_ = true;
	//
	// 	std::ranges::for_each(
	// 		children_this_frame_,
	// 		&Window::show
	// 	);
	// }

	auto Window::hide() noexcept -> void
	{
		visible_ = false;
		accessed_ = false;

		std::ranges::for_each(
			children_this_frame_,
			&Window::hide
		);
	}

	auto Window::find_hovered_window(const point_type& position, const bool excludes_children) noexcept -> Window*
	{
		if (not visible_)
		{
			return nullptr;
		}

		if (not excludes_children)
		{
			for (auto* child: children_this_frame_)
			{
				if (auto* window = child->find_hovered_window(position, excludes_children);
					window != nullptr)
				{
					return window;
				}
			}
		}

		if (const auto rect = this->rect();
			rect.includes(position))
		{
			return this;
		}

		return nullptr;
	}
}
