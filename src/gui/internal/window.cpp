// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gui/internal/window.hpp>

#include <memory/reference_wrapper.hpp>

#include <gui/internal/font.hpp>
#include <gui/internal/context.hpp>

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

			mark_widget_alive(context, id);

			return id;
		}

		[[nodiscard]] auto make_id(Context& context, const void* pointer) const noexcept -> widget_id_type
		{
			auto& window = self.get();

			// id_stack.front() -> window id
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window.id_stack_.empty());

			const auto seed = window.id_stack_.back();
			const auto id = make(seed, pointer);

			mark_widget_alive(context, id);

			return id;
		}

		[[nodiscard]] auto make_id(Context& context, const widget_id_type value) const noexcept -> widget_id_type
		{
			auto& window = self.get();

			// id_stack.front() -> window id
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window.id_stack_.empty());

			const auto seed = window.id_stack_.back();
			const auto id = make(seed, value);

			mark_widget_alive(context, id);

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

			const auto& font = current_font(context);

			// todo: scale?
			return static_cast<value_type>(font.pixel_height) * font.scale;
		}

		// -----------------------------------
		// PADDING

		[[nodiscard]] auto window_padding(const Context& context) const noexcept -> extent_type
		{
			const auto& window = self.get();

			const auto& theme = current_theme(context);

			if (window.flag_.is<WindowInternalFlag::CHILD_WINDOW>() and not window.flag_.is<WindowFlag::BORDERED>())
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

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window.flag_.is<WindowFlag::NO_TITLEBAR>());

			const auto& theme = current_theme(context);

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

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(window.flag_.is<WindowFlag::NO_TITLEBAR>());

			return {window.point_, window.size_full_.width, height};
		}

		// -----------------------------------
		// RESIZE-GRIP

		[[nodiscard]] auto resize_grip_size(const Context& context) const noexcept -> extent_type
		{
			std::ignore = this;

			const auto& theme = current_theme(context);

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

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window.flag_.is<WindowFlag::NO_TITLEBAR>());

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
			const auto& theme = current_theme(context);

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
	};

	Window::Window(
		const std::string_view name,
		const Flag flag,
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
				  .item_width = {},
				  .text_wrap_width = {}
		  },
		  name_{name},
		  flag_{flag},
		  root_{root},
		  point_{point},
		  size_{size},
		  size_full_{size},
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

		// auto fit
		if (size.width < .001f or size.height < .001f)
		{
			auto_fit_only_grows_ = true;
			auto_fit_frames_ = 2;
		}
	}

	auto Window::reset(const Flag flag) noexcept -> void
	{
		flag_ = flag;
	}

	auto Window::handle_inputs(const Context& context) noexcept -> void
	{
		// scroll
		if (not flag_.is<WindowFlag::NO_SCROLLBAR_WITH_MOUSE>())
		{
			// todo
			constexpr auto scroll_weight = static_cast<value_type>(5);
			scroll_next_y_ -= context.mouse.wheel * Drawer{.self = *this}.font_size(context) * scroll_weight;
		}
	}

	auto Window::begin_draw(
		Context& context,
		value_type fill_alpha,
		Window* parent
	) noexcept -> bool
	{
		// This function can be called multiple times per frame,
		// but is only initialized the first time it is called (after that we can just append the contents)

		const auto current_frame_count = context.frame_count;
		const auto is_first_draw_this_frame = current_frame_count != last_drawn_frame_;

		const auto display_size = context.io.display_size;

		// ---------------------------------
		// initialize the window, set the window's clip rect
		{
			if (is_first_draw_this_frame)
			{
				// bind context
				draw_list_.bind_context(context);
				// clear render data
				draw_list_.reset();
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

					parent->child_stack_.push_back(this);

					// moves the child window to the current cursor position of the parent window
					point_ = parent->canvas_.cursor_current_line;

					// the viewing area of the child window must not exceed that of the parent window
					push_clip_rect(context, parent->clip_rect_stack_.back());
				}
				else
				{
					// entire display area
					push_clip_rect(context, {0, 0, display_size});
				}
			}
			else
			{
				// the viewing area of the child window must not exceed that of the parent window
				if (flag_.is<WindowInternalFlag::CHILD_WINDOW>())
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parent != nullptr);

					// the viewing area of the child window must not exceed that of the parent window
					push_clip_rect(context, parent->clip_rect_stack_.back());
				}
				else
				{
					// entire display area
					push_clip_rect(context, {0, 0, display_size});
				}
			}
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(clip_rect_stack_.size() == 1);

		// ---------------------------------
		// initialize the canvas, set the canvas's clip rect
		bool close_button_pressed = false;
		{
			auto drawer = Drawer{*this};

			const auto& mouse = context.mouse;
			const auto& font = current_font(context);
			const auto& theme = current_theme(context);

			const auto has_titlebar = not flag_.is<WindowFlag::NO_TITLEBAR>();

			const auto is_child_window = flag_.is<WindowInternalFlag::CHILD_WINDOW>();
			const auto is_tooltip_window = flag_.is<WindowInternalFlag::CATEGORY_TOOLTIP>();

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(has_titlebar != is_child_window, "The child window is not allowed to contain a titlebar!");
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(has_titlebar != is_tooltip_window, "The tootip window is not allowed to contain a titlebar!");

			if (is_first_draw_this_frame)
			{
				if (last_drawn_frame_ + 1 < current_frame_count)
				{
					// The current window was not drawn in the last frame (or even many frames before that), this is usually because the window was just created, or the window was not visible before
					// Focus on the current window
					focus_window(context, *this);

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
					if (const auto activated = mark_widget_alive(context, id_of_move(context));
						activated)
					{
						if (mouse.is_down(context, MouseKey::LEFT))
						{
							// select current window
							focus_window(context, *this);

							if (not flag_.is<WindowFlag::NO_MOVE>())
							{
								const auto delta = mouse.position_delta;

								// dragging
								point_ += delta;
							}
						}
						else
						{
							// No widgets are active
							mark_widget_dead(context);
						}
					}
				}

				if (not is_child_window)
				{
					const auto s = drawer.font_size(context);
					const auto pad = extent_type{s * 2.f, s * 2.f};

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
					not flag_.is<WindowFlag::AUTO_RESIZE>()
				)
				{
					default_item_width_ = size_.width * theme.item_default_width_factor;
				}
				else
				{
					// todo: default item width
					default_item_width_ = theme.window_min_size.width * theme.item_default_width_factor;
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
					if (is_window_hovered(context, *this))
					{
						if (const auto rect = drawer.titlebar_rect(context);
							hovered(context, rect) and
							mouse.is_double_clicked(context, MouseKey::LEFT)
						)
						{
							// select current window
							focus_window(context, *this);

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

					draw_list_.rect_filled(
						rect,
						color_of(theme, ThemeCategory::TITLEBAR_COLLAPSED),
						theme.window_corner_rounding
					);

					if (flag_.is<WindowFlag::BORDERED>())
					{
						constexpr auto offset = extent_type{1, 1};

						draw_list_.rect(
							{rect.point + offset, rect.extent},
							color_of(theme, ThemeCategory::BORDER),
							theme.window_corner_rounding
						);
						draw_list_.rect(
							rect,
							color_of(theme, ThemeCategory::BORDER),
							theme.window_corner_rounding
						);
					}
				}
				else
				{
					size_ = size_full_;

					auto resize_grip_color = color_of(theme, ThemeCategory::RESIZE_GRIP);

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

						if (flag_.is<WindowFlag::AUTO_RESIZE>())
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
						else if (not flag_.is<WindowFlag::NO_RESIZE>())
						{
							// resize grip
							const auto rect = drawer.resize_grip_rect(context);
							const auto id = id_of_resize(context);

							const auto state = test_mouse(context, id, rect);
							if (state & MouseState::KEEPING)
							{
								resize_grip_color = color_of(theme, ThemeCategory::RESIZE_GRIP_ACTIVATED);
							}
							else if (state & MouseState::HOVERED)
							{
								resize_grip_color = color_of(theme, ThemeCategory::RESIZE_GRIP_HOVERED);
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
					if (fill_alpha > 0)
					{
						draw_list_.rect_filled(
							{point_, size_},
							color_of(theme, ThemeCategory::WINDOW_BACKGROUND, fill_alpha),
							theme.window_corner_rounding
						);

						// border
						if (flag_.is<WindowFlag::BORDERED>())
						{
							constexpr auto offset = extent_type{1, 1};

							draw_list_.rect(
								{point_ + offset, size_},
								color_of(theme, ThemeCategory::BORDER_SHADOW),
								theme.window_corner_rounding
							);
							draw_list_.rect(
								{point_, size_},
								color_of(theme, ThemeCategory::BORDER),
								theme.window_corner_rounding
							);
						}
					}

					// titlebar rect
					if (has_titlebar)
					{
						draw_list_.rect_filled(
							current_titlebar_rect,
							color_of(theme, ThemeCategory::TITLEBAR),
							theme.window_corner_rounding,
							DrawFlag::ROUND_CORNER_TOP
						);

						// border
						if (flag_.is<WindowFlag::BORDERED>())
						{
							draw_list_.line(
								current_titlebar_rect.left_bottom(),
								current_titlebar_rect.right_bottom(),
								color_of(theme, ThemeCategory::BORDER)
							);
						}
					}

					// scrollbar
					if (
						// no scrollbar
						flag_.is<WindowFlag::NO_SCROLLBAR>() or
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
							color_of(theme, ThemeCategory::SCROLLBAR_BACKGROUND)
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

						auto grab_color = color_of(theme, ThemeCategory::SCROLLBAR_GRAB);
						if (grab_size_y_normalized < 1.f)
						{
							const auto id = id_of_scrollbar(context);

							if (const auto state = test_mouse(context, id, scrollbar_area_rect);
								state & MouseState::KEEPING)
							{
								grab_color = color_of(theme, ThemeCategory::SCROLLBAR_GRAB_ACTIVATED);

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
								grab_color = color_of(theme, ThemeCategory::SCROLLBAR_GRAB_HOVERED);
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
					if (not flag_.is<WindowFlag::NO_RESIZE>())
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
					if (not flag_.is<WindowFlag::NO_CLOSE>())
					{
						const auto rect = drawer.close_button_rect(context);
						const auto id = id_of_close(context);

						const auto state = test_mouse(context, id, rect);

						auto close_button_color = color_of(theme, ThemeCategory::CLOSE_BUTTON);
						if (state & MouseState::HOVERED)
						{
							if (state & MouseState::KEEPING)
							{
								close_button_color = color_of(theme, ThemeCategory::CLOSE_BUTTON_ACTIVATED);
							}
							else
							{
								close_button_color = color_of(theme, ThemeCategory::CLOSE_BUTTON_HOVERED);
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
								color_of(theme, ThemeCategory::TEXT)
							);
							draw_list_.line(
								center + extent_type{x, -y},
								center + extent_type{-x, y},
								color_of(theme, ThemeCategory::TEXT)
							);
						}

						close_button_pressed = state & MouseState::PRESSED;
					}

					// title text
					const auto font_size = drawer.font_size(context);
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
							color_of(theme, ThemeCategory::TEXT),
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
							color_of(theme, ThemeCategory::TEXT),
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
			else
			{
				//
			}

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

			// Inner clipping rectangle (canvas area)
			push_clip_rect(context, clip_rect);

			if (is_first_draw_this_frame)
			{
				accessed_ = false;
			}

			// > Limit the current window from moving outside the program's visual area (if it is not a child window) (see code above)
			// If it is a child window, it may move out of the visual area because the parent window moves, and we collapse it (so that we can skip the widgets drawn on it earlier)
			if (is_child_window)
			{
				// todo
				collapsed_ = not clip_rect.valid();

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
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(clip_rect_stack_.size() == 2);

		return close_button_pressed;
	}

	auto Window::draw_text(Context& context, const std::string_view utf8_text) noexcept -> void
	{
		if (skip_item_)
		{
			return;
		}
		accessed_ = true;

		const auto& theme = current_theme(context);
		const auto& font = current_font(context);
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
						color_of(theme, ThemeCategory::TEXT),
						sub_string,
						Font::no_auto_wrap
					);

					text_point.y += line_height;
					text_size.width = std::ranges::max(text_size.width, this_line_text_size.width);
				}

				text_size.height = static_cast<value_type>(std::ranges::distance(view)) * line_height;
			}

			drawer.adjust_item_size(context, text_size);

			// fixme: hovering text?
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
			drawer.adjust_item_size(context, text_size);

			// fixme: hovering text?

			draw_list_.text(
				font,
				font_size,
				text_point,
				color_of(theme, ThemeCategory::TEXT),
				utf8_text,
				this_wrap_width
			);
		}
	}

	auto Window::same_line(const Context& context, const value_type column_width, value_type spacing_width) noexcept -> void
	{
		if (collapsed_)
		{
			return;
		}

		const auto& theme = current_theme(context);

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

	auto Window::end_draw(Context& context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(clip_rect_stack_.size() == 2);

		// canvas rect
		pop_clip_rect(context);

		// window rect
		pop_clip_rect(context);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(clip_rect_stack_.empty());

		// window_data.root = nullptr;
	}

	auto Window::render(Context& context) const noexcept -> void
	{
		context.draw_lists.emplace_back(draw_list_);
	}

	auto Window::name() const noexcept -> std::string_view
	{
		return name_;
	}

	auto Window::flag() const noexcept -> Flag
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
		const auto& theme = current_theme(context);
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
		const auto& theme = current_theme(context);
		const Drawer drawer{.self = const_cast<Window&>(*this)};

		auto size = size_ - drawer.window_padding(context);

		if (scroll_y_visible_)
		{
			size.width -= theme.window_vertical_scrollbar_width;
		}

		return size;
	}

	auto Window::hovered(const Context& context, const rect_type& rect) const noexcept -> bool
	{
		const auto clipped = [&]() noexcept -> rect_type
		{
			if (not clip_rect_stack_.empty())
			{
				const auto& last = clip_rect_stack_.back();
				// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(last.includes(rect));

				return rect.combine_min(last);
			}

			return rect;
		}();

		return clipped.includes(context.mouse.position_current);
	}

	auto Window::show() noexcept -> void
	{
		visible_ = true;
	}

	auto Window::hide() noexcept -> void
	{
		visible_ = false;
		accessed_ = false;
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
			const auto size = context.io.display_size;
			draw_list_.push_clip_rect({0, 0, size}, false);
		}
		else
		{
			draw_list_.push_clip_rect(clip_rect_stack_.back(), false);
		}
	}
}
