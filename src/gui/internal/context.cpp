// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gui/internal/context.hpp>

#include <array>
#include <limits>
#include <vector>

#include <gui/internal/font.hpp>
#include <gui/internal/window.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace
{
	using namespace gal::prometheus;
	using namespace gui;

	Context* g_context = nullptr;
}

namespace gal::prometheus::gui
{
	Context::Context() noexcept
		:
		initialized_{false},
		draw_list_flag_{DrawListFlag::NONE},
		draw_list_shared_data_{},
		font_{},
		theme_{},
		theme_mod_stack_{},
		io_{},
		time_total_{0},
		frame_count_{0},
		frame_count_rendered_{0},
		mouse_{},
		window_default_spawn_position_{50, 50},
		window_hive_{},
		window_root_list_{},
		window_current_stack_{},
		window_hovered_{nullptr},
		window_hovered_root_{nullptr},
		window_focused_{nullptr},
		widget_hovered_{internal::invalid_widget_id},
		widget_activated_{internal::invalid_widget_id},
		widget_activated_previous_frame_{internal::invalid_widget_id},
		widget_activated_still_alive_{false},
		widget_activated_combo_id_{internal::invalid_widget_id},
		draw_lists_{} {}

	auto Context::create() noexcept -> Context*
	{
		auto* p = new Context{};

		// todo
		p->initialized_ = true;

		return p;
	}

	auto Context::destroy(Context& context) noexcept -> void
	{
		delete std::addressof(context);
	}

	auto Context::set_default_draw_list_flag(const DrawListFlag flag) noexcept -> void
	{
		draw_list_flag_ = flag;
	}

	auto Context::current_draw_list_flag() const noexcept -> DrawListFlag
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);

		// return draw_list_flag_stack[draw_list_flag_current];
		return draw_list_flag_;
	}

	// auto Context::push_draw_list_flag(const DrawListFlag flag) noexcept -> void
	// {
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized);
	//
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
	// 		draw_list_flag_current == Context::stack_pointer_default or
	// 		draw_list_flag_current < Context::draw_list_flag_stack_size,
	// 		"DrawListFlag stack overflow"
	// 	);
	//
	// 	static_assert(
	// 		static_cast<::Context::stack_pointer_type>(::Context::stack_pointer_default + 1) ==
	// 		static_cast<::Context::stack_pointer_type>(0)
	// 	);
	//
	// 	draw_list_flag_current += 1;
	// 	draw_list_flag_stack[draw_list_flag_current] = flag;
	// }
	//
	// auto Context::pop_draw_list_flag() noexcept -> void
	// {
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized);
	//
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
	// 		draw_list_flag_current != Context::stack_pointer_default,
	// 		"Unable to popup the default DrawListFlag!"
	// 	);
	//
	// 	static_assert(
	// 		static_cast<Context::stack_pointer_type>(static_cast<Context::stack_pointer_type>(0) - 1) ==
	// 		Context::stack_pointer_default
	// 	);
	//
	// 	draw_list_flag_stack[draw_list_flag_current] = DrawListFlag::NONE;
	// 	draw_list_flag_current -= 1;
	// }

	auto Context::current_draw_list_shared_data() const noexcept -> const internal::DrawListSharedData&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);

		// return draw_list_shared_data_stack[draw_list_shared_data_current];
		return draw_list_shared_data_;
	}

	// auto Context::push_draw_list_shared_data(const DrawListSharedData& shared_data) noexcept -> void
	// {
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized);
	//
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
	// 		draw_list_shared_data_current == Context::stack_pointer_default or
	// 		draw_list_shared_data_current < Context::draw_list_shared_data_stack_size,
	// 		"DrawListSharedData stack overflow"
	// 	);
	//
	// 	static_assert(
	// 		static_cast<::Context::stack_pointer_type>(::Context::stack_pointer_default + 1) ==
	// 		static_cast<::Context::stack_pointer_type>(0)
	// 	);
	//
	// draw_list_shared_data_current += 1;
	// 	draw_list_shared_data_stack[draw_list_shared_data_current] = shared_data;
	// }
	//
	// auto Context::pop_draw_list_shared_data() noexcept -> void
	// {
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized);
	//
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
	// 		draw_list_shared_data_current != Context::stack_pointer_default,
	// 		"Unable to popup the default DrawListSharedData!"
	// 	);
	//
	// 	static_assert(
	// 		static_cast<Context::stack_pointer_type>(static_cast<Context::stack_pointer_type>(0) - 1) ==
	// 		Context::stack_pointer_default
	// 	);
	//
	// 	draw_list_shared_data_stack[draw_list_shared_data_current] = {};
	// 	draw_list_shared_data_current -= 1;
	// }

	auto Context::set_default_font(const FontOption& option) noexcept -> Texture
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);

		// if (context.font_current == Context::stack_pointer_default)
		// {
		// 	return push_font(context, option);
		// }
		//
		// auto font = memory::make_unique<internal::Font>();
		// auto texture = do_load_font(option, *font);
		//
		// context.font_stack[0] = std::move(font);
		//
		// return texture;
		return font_.load(option);
	}

	auto Context::current_font() const noexcept -> const internal::Font&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);

		// const auto& font = *font_stack[font_current];
		// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(font.loaded(), "Invalid font!");
		//
		// return font;
		return font_;
	}

	auto Context::set_default_theme(const Theme& default_theme) noexcept -> void
	{
		theme_ = default_theme;
	}

	auto Context::current_theme() const noexcept -> const Theme&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);

		return theme_;
	}

	auto Context::push_theme(const ThemeCategory category, const Theme::color_type new_color) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);

		const auto old_color = color_of(category);
		theme_mod_stack_.emplace_back(category, old_color);

		theme_.colors[static_cast<std::size_t>(category)] = new_color;
	}

	auto Context::pop_theme() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not theme_mod_stack_.empty());

		const auto [category, old_color] = theme_mod_stack_.back();
		theme_mod_stack_.pop_back();
		theme_.colors[static_cast<std::size_t>(category)] = old_color;
	}

	auto Context::color_of(const ThemeCategory category, const Theme::value_type factor) const noexcept -> Theme::color_type
	{
		const auto& theme = current_theme();

		return color_of(theme, category, factor);
	}

	auto Context::color_of(const Theme& theme, const ThemeCategory category, const Theme::value_type factor) const noexcept -> Theme::color_type
	{
		std::ignore = this;

		const auto index = static_cast<std::size_t>(category);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < theme_category_count);
		auto color = theme.colors[static_cast<std::size_t>(category)];

		color.alpha = static_cast<Theme::color_type::value_type>(theme.alpha * 255 * factor);

		return color;
	}

	auto Context::io() noexcept -> IO&
	{
		return io_;
	}

	auto Context::io() const noexcept -> const IO&
	{
		return io_;
	}

	auto Context::mouse() const noexcept -> const internal::Mouse&
	{
		return mouse_;
	}

	auto Context::test_mouse(const widget_id_type id, const rect_type& area, const bool repeat) noexcept -> std::underlying_type_t<MouseState>
	{
		const auto& window = current_window();
		const auto hovered =
				window_hovered_root_ == std::addressof(window.root()) and
				widget_hovered_ == internal::invalid_widget_id and
				window.is_hovered(*this, area);

		auto state = std::to_underlying(MouseState::NONE);
		if (hovered)
		{
			state |= MouseState::HOVERED;

			// hovering widget
			widget_hovered_ = id;

			if (mouse_.is_clicked(*this, MouseKey::LEFT, false))
			{
				// select widget
				widget_activated_ = id;
			}
			else if (
				repeat and
				widget_activated_ != internal::invalid_widget_id and
				mouse_.is_clicked(*this, MouseKey::LEFT, true)
			)
			{
				state |= MouseState::PRESSED;
			}
		}

		if (widget_activated_ == id)
		{
			if (mouse_.is_down(*this, MouseKey::LEFT))
			{
				// select current widget, keep the left mouse button pressed
				state |= MouseState::KEEPING;
			}
			else
			{
				if (hovered)
				{
					// select current widget, release the left mouse button on the widget
					state |= MouseState::PRESSED;
				}
				else
				{
					// select current widget, did not release the left mouse button on the widget
					// do nothing
				}

				// the widget is no longer selected
				widget_activated_ = internal::invalid_widget_id;
			}
		}

		return state;
	}

	auto Context::queue_mouse(const widget_id_type id, const rect_type& area) noexcept -> std::underlying_type_t<MouseState>
	{
		const auto& window = current_window();
		const auto hovered =
				window_hovered_root_ == std::addressof(window.root()) and
				widget_hovered_ == internal::invalid_widget_id and
				window.is_hovered(*this, area);

		auto state = std::to_underlying(MouseState::NONE);
		if (hovered)
		{
			state |= MouseState::HOVERED;

			// hovering widget
			widget_hovered_ = id;

			if (mouse_.is_clicked(*this, MouseKey::LEFT, false))
			{
				state |= MouseState::PRESSED;
			}
		}

		return state;
	}

	auto Context::find_window(std::string_view name) noexcept -> window_type*
	{
		const auto it = std::ranges::find_if(
			window_hive_,
			[name](const auto& window) noexcept -> bool
			{
				return window->name() == name;
			}
		);

		if (it != window_hive_.end())
		{
			return it.operator*().get();
		}

		return nullptr;
	}

	auto Context::find_or_create_window(const std::string_view name, const extent_type& size, const internal::WindowFlag flag) noexcept -> window_type&
	{
		[[maybe_unused]] const auto is_child_window = flag.is<internal::WindowInternalFlag::CHILD_WINDOW>();
		if (is_child_window)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window_current_stack_.empty());
		}

		if (auto* window = find_window(name);
			window == nullptr)
		{
			auto* root = is_child_window ? current_root_window() : nullptr;
			if (is_child_window and root == nullptr)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(window_current_stack_.empty());
			}

			// fixme: load cached settings?
			auto temp = std::make_unique<internal::Window>(
				name,
				flag,
				window_default_spawn_position_,
				size,
				root
			);

			auto& ref = window_hive_.emplace_back(std::move(temp));
			if (root == nullptr)
			{
				// The current window is the root window
				window_root_list_.emplace_back(ref.get());
			}

			window_current_stack_.emplace_back(ref.get());
		}
		else
		{
			window->reset(flag, size);

			window_current_stack_.emplace_back(window);
		}

		return *window_current_stack_.back();
	}

	auto Context::current_window() const noexcept -> window_type&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window_current_stack_.empty());

		const auto index = window_current_stack_.size() - 1;
		return *window_current_stack_[index];
	}

	auto Context::current_parent_window() const noexcept -> window_type&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(window_current_stack_.size() > 1);

		const auto index = window_current_stack_.size() - 2;
		return *window_current_stack_[index];
	}

	auto Context::current_root_window() noexcept -> window_type*
	{
		const auto view = window_current_stack_ | std::views::reverse;

		const auto it = std::ranges::find_if(
			view,
			[](const auto& window) noexcept -> bool
			{
				const auto flag = window->flag();
				return not flag.template is<internal::WindowInternalFlag::CHILD_WINDOW>();
			}
		);

		if (it == std::ranges::end(view))
		{
			return nullptr;
		}

		return it.operator*();
	}

	auto Context::set_next_window_point(const point_type& point) noexcept -> void
	{
		// todo
		window_default_spawn_position_ = point;
	}

	auto Context::begin_window(
		const std::string_view name,
		const extent_type& size,
		Theme::value_type background_fill_alpha,
		const internal::WindowFlag flag
	) noexcept -> bool
	{
		const auto is_child_window = flag.is<internal::WindowInternalFlag::CHILD_WINDOW>();
		auto* parent = is_child_window ? std::addressof(current_window()) : nullptr;

		auto& window = find_or_create_window(name, size, flag);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(window_current_stack_.back() == std::addressof(window));

		// alpha
		static_assert(window_fill_alpha_not_set < 0);
		if (background_fill_alpha < 0)
		{
			background_fill_alpha = theme_.window_background_alpha;
		}

		return window.begin_window(*this, parent, background_fill_alpha);
	}

	auto Context::end_window() noexcept -> void
	{
		auto& window = current_window();

		window.end_window(*this);

		// Select window for move/focus when we're done with all our widgets (we only consider non-children windows here)
		if (const auto rect = window.rect();
			widget_activated_ == internal::invalid_widget_id and
			widget_hovered_ == internal::invalid_widget_id and
			window_hovered_root_ == std::addressof(window) and
			window.is_hovered(*this, rect) and
			mouse_.is_clicked(*this, MouseKey::LEFT)
		)
		{
			widget_activated_ = window.id_of_move(*this);
		}

		window_current_stack_.pop_back();
	}

	auto Context::begin_child_window(
		const std::string_view name,
		const extent_type& size,
		const Theme::alpha_type background_fill_alpha,
		const internal::WindowFlag flag,
		const bool border
	) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not window_current_stack_.empty());
		auto& parent = current_window();

		// todo
		parent.begin_child_window(*this, name, background_fill_alpha, size, border, flag);
	}

	auto Context::end_child_window() noexcept -> void
	{
		auto& parent = current_parent_window();
		auto& child = current_window();
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(child.flag().is<internal::WindowInternalFlag::CHILD_WINDOW>());

		parent.end_child_window(*this, child);
	}

	auto Context::is_window_hovered(const window_type& window) const noexcept -> bool
	{
		return window_hovered_ == std::addressof(window);
	}

	auto Context::find_hovered_window(const point_type& position, const bool excludes_children) noexcept -> window_type*
	{
		for (const auto view = window_root_list_ | std::views::reverse;
		     auto* root: view)
		{
			if (auto* window = root->find_hovered_window(position, excludes_children);
				window != nullptr)
			{
				return window;
			}
		}

		return nullptr;
	}

	auto Context::focus_window(window_type& window) noexcept -> void
	{
		if (window_focused_ == std::addressof(window))
		{
			return;
		}

		window_focused_ = std::addressof(window);

		auto& root = window.root();

		const auto it = std::ranges::find(window_root_list_, std::addressof(root));
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it != window_root_list_.end());

		if (it != window_root_list_.end() - 1)
		{
			// The focused window is drawn last
			const auto p = *it;
			window_root_list_.erase(it);
			window_root_list_.push_back(p);
		}

		// todo: reorder child window?
	}

	auto Context::is_widget_hovered(const widget_id_type id) const noexcept -> bool
	{
		return widget_hovered_ == id;
	}

	auto Context::is_any_widget_hovered() const noexcept -> bool
	{
		return widget_hovered_ != internal::invalid_widget_id;
	}

	auto Context::is_widget_activated(const widget_id_type id) const noexcept -> bool
	{
		return widget_activated_ == id;
	}

	auto Context::is_any_widget_activated() const noexcept -> bool
	{
		return widget_activated_ != internal::invalid_widget_id;
	}

	auto Context::mark_widget_alive(const widget_id_type id) noexcept -> bool
	{
		if (is_widget_activated(id))
		{
			widget_activated_still_alive_ = true;

			return true;
		}

		return false;
	}

	auto Context::mark_widget_dead(const widget_id_type id) noexcept -> void
	{
		widget_activated_ = id;
	}

	auto Context::mark_combo_alive(const widget_id_type id) noexcept -> void
	{
		widget_activated_combo_id_ = id;
	}

	auto Context::mark_combo_dead(const widget_id_type id) noexcept -> void
	{
		widget_activated_combo_id_ = id;
	}

	auto Context::is_combo_activated(const widget_id_type id) const noexcept -> bool
	{
		return widget_activated_combo_id_ == id;
	}

	auto Context::current_time() const noexcept -> time_type
	{
		return time_total_;
	}

	auto Context::current_frame() const noexcept -> internal::frame_count_type
	{
		return frame_count_;
	}

	auto Context::new_frame() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(io_.display_size.width > 0 and io_.display_size.height > 0);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(io_.delta_time > 0);

		time_total_ += io_.delta_time;
		frame_count_ += 1;

		// Update mouse
		{
			mouse_.tick(*this);
		}

		const auto mouse_position = mouse_.position_current;

		// Update widget
		{
			// Clear reference to active widget if the widget isn't alive anymore
			widget_hovered_ = internal::invalid_widget_id;
			if (
				not widget_activated_still_alive_ and
				widget_activated_previous_frame_ == widget_activated_ and
				widget_activated_ != internal::invalid_widget_id
			)
			{
				widget_activated_ = internal::invalid_widget_id;
			}
			widget_activated_previous_frame_ = widget_activated_;
			widget_activated_still_alive_ = false;
		}

		// Update window
		{
			window_hovered_ = find_hovered_window(mouse_position, false);
			window_hovered_root_ = find_hovered_window(mouse_position, true);

			if (window_hovered_ != nullptr)
			{
				window_hovered_->handle_inputs(*this);
			}

			// Mark all windows as not visible
			std::ranges::for_each(
				window_root_list_,
				[](auto* window) noexcept -> void
				{
					window->hide();
				}
			);

			// No window should be open at the beginning of the frame
			// But in order to allow the user to call `new_frame` multiple times without calling `render`, we are doing an explicit clear
			window_current_stack_.clear();
		}
	}

	auto Context::end_frame() noexcept -> void
	{
		std::ignore = this;
	}

	auto Context::render() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);

		const auto& theme = current_theme();

		const auto is_first_render_this_frame = frame_count_rendered_ != frame_count_;
		frame_count_rendered_ = frame_count_;

		if (is_first_render_this_frame)
		{
			// clear all data for new frame
			io_.delta_time = -1;
			// io.mouse_position = {0, 0};
			io_.mouse_wheel = 0;
			// io.mouse_button_state.state.fill(false);
		}

		draw_lists_.clear();

		if (theme.alpha > .0f)
		{
			// gather windows to render
			std::ranges::for_each(
				window_root_list_,
				[this](auto* window) noexcept -> void
				{
					window->render(*this, draw_lists_);
				}
			);
		}
	}

	auto Context::get_draw_data() noexcept -> std::vector<DrawData>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(initialized_);

		std::vector<DrawData> data;
		data.reserve(draw_lists_.size());

		std::ranges::transform(
			draw_lists_,
			std::back_inserter(data),
			[](const auto& draw_list_ref) noexcept -> DrawData
			{
				auto& draw_list = draw_list_ref.get();

				return
				{
						.vertex_list = draw_list.vertex_list(),
						.index_list = draw_list.index_list(),
						.command_list = draw_list.command_list()
				};
			}
		);

		return data;
	}

	auto Context::show_theme_editor() noexcept -> bool
	{
		auto& context = get_current_context();
		auto& theme = context.theme_;

		const auto window_closed = gui::begin_window(context, "ThemeEditor");

		draw_slider<"window_background_alpha">(context, theme, 0, 1);
		draw_slider<"window_corner_rounding">(context, theme, 0, 24);
		draw_slider<"window_min_size.width">(context, theme, 64, 640);
		draw_slider<"window_min_size.height">(context, theme, 48, 480);
		draw_slider<"window_resize_grip_size.width">(context, theme, 10, 50);
		draw_slider<"window_resize_grip_size.height">(context, theme, 10, 50);
		draw_slider<"window_padding.width">(context, theme, 2, 20);
		draw_slider<"window_padding.height">(context, theme, 2, 20);
		draw_slider<"window_auto_fit_padding.width">(context, theme, 2, 20);
		draw_slider<"window_auto_fit_padding.height">(context, theme, 2, 20);
		draw_slider<"window_vertical_scrollbar_width">(context, theme, 6, 25);
		draw_slider<"item_default_width_factor">(context, theme, .35f, .85f);
		draw_slider<"item_frame_padding.width">(context, theme, 1, 10);
		draw_slider<"item_frame_padding.height">(context, theme, 1, 10);
		draw_slider<"item_spacing.width">(context, theme, 1, 10);
		draw_slider<"item_spacing.height">(context, theme, 1, 10);
		draw_slider<"alpha">(context, theme, 0, 1);

		// todo: update DrawListSharedData
		draw_slider<"circle_segment_max_error">(context, theme, 0, 1);
		draw_slider<"draw_curve_tessellation_tolerance">(context, theme, 0, 5);

		gui::end_window(context);

		return window_closed;
	}

	auto create_context() noexcept -> Context*
	{
		return Context::create();
	}

	auto destroy_context(Context& context) noexcept -> void
	{
		Context::destroy(context);
	}

	auto destroy_context(Context* context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context != nullptr);
		destroy_context(*context);
	}

	auto set_default_font(Context& context, const FontOption& option) noexcept -> Texture
	{
		return context.set_default_font(option);
	}

	auto set_default_theme(Context& context, const Theme& theme) noexcept -> void
	{
		context.set_default_theme(theme);
	}

	auto push_theme(Context& context, const ThemeCategory category, const Theme::color_type new_color) noexcept -> void
	{
		context.push_theme(category, new_color);
	}

	auto pop_theme(Context& context) noexcept -> void
	{
		context.pop_theme();
	}

	auto get_io(Context& context) noexcept -> IO&
	{
		return context.io();
	}

	auto set_default_draw_list_flag(Context& context, const DrawListFlag flag) noexcept -> void
	{
		context.set_default_draw_list_flag(flag);
	}

	auto new_frame(Context& context) noexcept -> void
	{
		context.new_frame();
	}

	auto end_frame(Context& context) noexcept -> void
	{
		context.end_frame();
	}

	auto render(Context& context) noexcept -> void
	{
		context.render();
	}

	[[nodiscard]] auto get_draw_data(Context& context) noexcept -> std::vector<DrawData>
	{
		return context.get_draw_data();
	}

	auto set_next_window_point(Context& context, const point_type& point) noexcept -> void
	{
		context.set_next_window_point(point);
	}

	auto begin_window(
		Context& context,
		const std::string_view name,
		const extent_type& size,
		const Theme::alpha_type fill_alpha,
		const WindowFlag flag
	) noexcept -> bool
	{
		return context.begin_window(name, size, fill_alpha, flag);
	}

	auto end_window(Context& context) noexcept -> void
	{
		context.end_window();
	}

	auto begin_child_window(
		Context& context,
		const std::string_view name,
		const extent_type& size,
		const bool border,
		const WindowFlag flag
	) noexcept -> void
	{
		context.begin_child_window(name, size, 0, flag, border);
	}

	auto end_child_window(Context& context) noexcept -> void
	{
		context.end_child_window();
	}

	auto begin_tooltip_window(Context& context) noexcept -> void
	{
		// todo
		constexpr std::string_view tooltip_window_name{"@WINDOW::TOOLTIP@"};
		constexpr internal::WindowFlag tooltip_window_flag
		{
				WindowFlag::NO_TITLEBAR | WindowFlag::NO_CLOSE | WindowFlag::NO_RESIZE | WindowFlag::NO_MOVE,
				internal::WindowInternalFlag::CATEGORY_TOOLTIP
		};

		std::ignore = context.begin_window(tooltip_window_name, {}, .9f, tooltip_window_flag);
	}

	auto end_tooltip_window(Context& context) noexcept -> void
	{
		const auto& window = context.current_window();
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(window.flag().is<internal::WindowInternalFlag::CATEGORY_TOOLTIP>());

		context.end_window();
	}

	auto draw_text(Context& context, const std::string_view utf8_text) noexcept -> void
	{
		auto& window = context.current_window();
		window.draw_text(context, utf8_text);
	}

	auto draw_text_colored(Context& context, const std::string_view utf8_text, const Theme::color_type color) noexcept -> void
	{
		push_theme(context, ThemeCategory::TEXT, color);
		draw_text(context, utf8_text);
		pop_theme(context);
	}

	auto draw_button(Context& context, const std::string_view utf8_text, const extent_type& size, const bool repeat_when_held) noexcept -> bool
	{
		auto& window = context.current_window();
		return window.draw_button(context, utf8_text, size, repeat_when_held);
	}

	auto draw_small_button(Context& context, const std::string_view utf8_text, const bool repeat_when_held) noexcept -> bool
	{
		auto& window = context.current_window();
		return window.draw_small_button(context, utf8_text, repeat_when_held);
	}

	auto draw_radio_button(Context& context, const std::string_view utf8_text, const bool checked) noexcept -> bool
	{
		auto& window = context.current_window();
		return window.draw_radio_button(context, utf8_text, checked);
	}

	auto draw_checkbox(Context& context, const std::string_view utf8_text, const bool checked) noexcept -> bool
	{
		auto& window = context.current_window();
		return window.draw_checkbox(context, utf8_text, checked);
	}

	auto draw_slider(
		Context& context,
		const std::string_view utf8_text,
		float& reference,
		const float min,
		const float max,
		const std::uint32_t decimal_precision,
		const float power
	) noexcept -> bool
	{
		auto& window = context.current_window();
		return window.draw_slider(context, utf8_text, reference, min, max, decimal_precision, power);
	}

	auto draw_slider_n(
		Context& context,
		const std::string_view utf8_text,
		const std::span<float> references,
		const float min,
		const float max,
		const std::uint32_t decimal_precision,
		const float power
	) noexcept -> bool
	{
		auto& window = context.current_window();
		return window.draw_slider_n(context, utf8_text, references, min, max, decimal_precision, power);
	}

	auto draw_combo(
		Context& context,
		const std::string_view utf8_text,
		const std::span<const std::string> selections,
		std::size_t& selected,
		const std::size_t show_selection_count
	) noexcept -> bool
	{
		auto& window = context.current_window();
		return window.draw_combo(context, utf8_text, selections, selected, show_selection_count);
	}

	auto draw_combo(
		Context& context,
		const std::string_view utf8_text,
		const std::span<const std::string_view> selections,
		std::size_t& selected,
		const std::size_t show_selection_count
	) noexcept -> bool
	{
		auto& window = context.current_window();
		return window.draw_combo(context, utf8_text, selections, selected, show_selection_count);
	}

	auto draw_combo(
		Context& context,
		const std::string_view utf8_text,
		const std::span<const char*> selections,
		std::size_t& selected,
		const std::size_t show_selection_count
	) noexcept -> bool
	{
		auto& window = context.current_window();
		return window.draw_combo(context, utf8_text, selections, selected, show_selection_count);
	}

	auto layout_same_line(Context& context, const Theme::value_type column_width, const Theme::value_type spacing_width) noexcept -> void
	{
		auto& window = context.current_window();
		window.same_line(context, column_width, spacing_width);
	}

	auto push_item_width(Context& context, const Theme::value_type new_item_width) noexcept -> void
	{
		auto& window = context.current_window();
		window.push_item_width(context, new_item_width);
	}

	auto pop_item_width(Context& context) noexcept -> void
	{
		auto& window = context.current_window();
		window.pop_item_width(context);
	}

	auto push_text_wrap_width(Context& context, const Theme::value_type new_wrap_width) noexcept -> void
	{
		auto& window = context.current_window();
		window.push_text_wrap_width(context, new_wrap_width);
	}

	auto pop_text_wrap_width(Context& context) noexcept -> void
	{
		auto& window = context.current_window();
		window.pop_text_wrap_width(context);
	}

	auto get_content_region_max(const Context& context) noexcept -> extent_type
	{
		const auto& window = context.current_window();
		return window.content_region_max(context);
	}

	auto get_window_content_region_min(const Context& context) noexcept -> extent_type
	{
		const auto& window = context.current_window();
		return window.window_content_region_min(context);
	}

	auto get_window_content_region_max(const Context& context) noexcept -> extent_type
	{
		const auto& window = context.current_window();
		return window.window_content_region_max(context);
	}

	auto is_item_hovered(const Context& context) noexcept -> bool
	{
		const auto& window = context.current_window();
		return window.is_item_hovered(context);
	}

	auto is_item_focused(const Context& context) noexcept -> bool
	{
		const auto& window = context.current_window();
		return window.is_item_hovered(context);
	}

	auto set_current_context(Context& context) noexcept -> void
	{
		g_context = std::addressof(context);
	}

	auto get_current_context() noexcept -> Context&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(g_context != nullptr);

		return *g_context;
	}

	auto create_current_context() noexcept -> void
	{
		g_context = create_context();
	}

	auto destroy_current_context() noexcept -> void
	{
		auto& context = get_current_context();

		destroy_context(context);
	}

	auto set_default_font(const FontOption& option) noexcept -> Texture
	{
		auto& context = get_current_context();

		return set_default_font(context, option);
	}

	auto set_default_theme(const Theme& theme) noexcept -> void
	{
		auto& context = get_current_context();

		set_default_theme(context, theme);
	}

	auto push_theme(const ThemeCategory category, const Theme::color_type new_color) noexcept -> void
	{
		auto& context = get_current_context();

		push_theme(context, category, new_color);
	}

	auto pop_theme() noexcept -> void
	{
		auto& context = get_current_context();

		pop_theme(context);
	}

	auto get_io() noexcept -> IO&
	{
		auto& context = get_current_context();

		return get_io(context);
	}

	auto set_default_draw_list_flag(const DrawListFlag flag) noexcept -> void
	{
		auto& context = get_current_context();

		set_default_draw_list_flag(context, flag);
	}

	auto new_frame() noexcept -> void
	{
		auto& context = get_current_context();

		return new_frame(context);
	}

	auto end_frame() noexcept -> void
	{
		auto& context = get_current_context();

		return end_frame(context);
	}

	auto render() noexcept -> void
	{
		auto& context = get_current_context();

		return render(context);
	}

	auto get_draw_data() noexcept -> std::vector<DrawData>
	{
		auto& context = get_current_context();

		return get_draw_data(context);
	}

	auto set_next_window_point(const point_type& point) noexcept -> void
	{
		auto& context = get_current_context();

		set_next_window_point(context, point);
	}

	auto begin_window(
		const std::string_view name,
		const extent_type& size,
		const Theme::value_type fill_alpha,
		const WindowFlag flag
	) noexcept -> bool
	{
		auto& context = get_current_context();

		return begin_window(context, name, size, fill_alpha, flag);
	}

	auto end_window() noexcept -> void
	{
		auto& context = get_current_context();

		return end_window(context);
	}

	auto begin_child_window(
		const std::string_view name,
		const extent_type& size,
		const bool border,
		const WindowFlag flag
	) noexcept -> void
	{
		auto& context = get_current_context();

		begin_child_window(context, name, size, border, flag);
	}

	auto end_child_window() noexcept -> void
	{
		auto& context = get_current_context();

		end_child_window(context);
	}

	auto begin_tooltip_window() noexcept -> void
	{
		auto& context = get_current_context();

		begin_tooltip_window(context);
	}

	auto end_tooltip_window() noexcept -> void
	{
		auto& context = get_current_context();

		end_tooltip_window(context);
	}

	auto draw_text(const std::string_view utf8_text) noexcept -> void
	{
		auto& context = get_current_context();

		draw_text(context, utf8_text);
	}

	auto draw_text_colored(const std::string_view utf8_text, const Theme::color_type color) noexcept -> void
	{
		auto& context = get_current_context();

		draw_text_colored(context, utf8_text, color);
	}

	auto draw_button(const std::string_view utf8_text, const extent_type& size, const bool repeat_when_held) noexcept -> bool
	{
		auto& context = get_current_context();

		return draw_button(context, utf8_text, size, repeat_when_held);
	}

	auto draw_small_button(const std::string_view utf8_text, const bool repeat_when_held) noexcept -> bool
	{
		auto& context = get_current_context();

		return draw_small_button(context, utf8_text, repeat_when_held);
	}

	auto draw_radio_button(const std::string_view utf8_text, const bool checked) noexcept -> bool
	{
		auto& context = get_current_context();

		return draw_radio_button(context, utf8_text, checked);
	}

	auto draw_checkbox(const std::string_view utf8_text, const bool checked) noexcept -> bool
	{
		auto& context = get_current_context();

		return draw_checkbox(context, utf8_text, checked);
	}

	auto draw_slider(
		const std::string_view utf8_text,
		float& reference,
		const float min,
		const float max,
		const std::uint32_t decimal_precision,
		const float power
	) noexcept -> bool
	{
		auto& context = get_current_context();

		return draw_slider(context, utf8_text, reference, min, max, decimal_precision, power);
	}

	auto draw_slider_n(
		const std::string_view utf8_text,
		const std::span<float> references,
		const float min,
		const float max,
		const std::uint32_t decimal_precision,
		const float power
	) noexcept -> bool
	{
		auto& context = get_current_context();

		return draw_slider_n(context, utf8_text, references, min, max, decimal_precision, power);
	}

	auto draw_combo(
		const std::string_view utf8_text,
		const std::span<const std::string> selections,
		std::size_t& selected,
		const std::size_t show_selection_count
	) noexcept -> bool
	{
		auto& context = get_current_context();

		return draw_combo(context, utf8_text, selections, selected, show_selection_count);
	}

	auto draw_combo(
		const std::string_view utf8_text,
		const std::span<const std::string_view> selections,
		std::size_t& selected,
		const std::size_t show_selection_count
	) noexcept -> bool
	{
		auto& context = get_current_context();

		return draw_combo(context, utf8_text, selections, selected, show_selection_count);
	}

	auto draw_combo(
		const std::string_view utf8_text,
		const std::span<const char*> selections,
		std::size_t& selected,
		const std::size_t show_selection_count
	) noexcept -> bool
	{
		auto& context = get_current_context();

		return draw_combo(context, utf8_text, selections, selected, show_selection_count);
	}

	auto layout_same_line(const Theme::value_type column_width, const Theme::value_type spacing_width) noexcept -> void
	{
		auto& context = get_current_context();

		layout_same_line(context, column_width, spacing_width);
	}

	auto push_item_width(const Theme::value_type new_item_width) noexcept -> void
	{
		auto& context = get_current_context();

		push_item_width(context, new_item_width);
	}

	auto pop_item_width() noexcept -> void
	{
		auto& context = get_current_context();

		pop_item_width(context);
	}

	auto push_text_wrap_width(const Theme::value_type new_wrap_width) noexcept -> void
	{
		auto& context = get_current_context();

		push_text_wrap_width(context, new_wrap_width);
	}

	auto pop_text_wrap_width() noexcept -> void
	{
		auto& context = get_current_context();

		pop_text_wrap_width(context);
	}

	auto get_content_region_max() noexcept -> extent_type
	{
		const auto& context = get_current_context();

		return get_content_region_max(context);
	}

	auto get_window_content_region_min() noexcept -> extent_type
	{
		const auto& context = get_current_context();

		return get_window_content_region_min(context);
	}

	auto get_window_content_region_max() noexcept -> extent_type
	{
		const auto& context = get_current_context();

		return get_window_content_region_max(context);
	}

	auto is_item_hovered() noexcept -> bool
	{
		const auto& context = get_current_context();

		return is_item_hovered(context);
	}

	auto is_item_focused() noexcept -> bool
	{
		const auto& context = get_current_context();

		return is_item_focused(context);
	}

	auto test_theme() noexcept -> Theme
	{
		constexpr auto default_colors = []() noexcept -> Theme::colors_type
		{
			using enum ThemeCategory;

			Theme::colors_type colors{};

			colors[static_cast<std::size_t>(TEXT)] = primitive::colors::black;

			colors[static_cast<std::size_t>(BORDER)] = primitive::colors::magenta;
			colors[static_cast<std::size_t>(BORDER_SHADOW)] = primitive::colors::red;

			colors[static_cast<std::size_t>(WINDOW_BACKGROUND)] = primitive::colors::gains_boro;

			colors[static_cast<std::size_t>(TITLEBAR)] = primitive::colors::light_coral;
			colors[static_cast<std::size_t>(TITLEBAR_COLLAPSED)] = primitive::colors::dark_khaki;

			colors[static_cast<std::size_t>(RESIZE_GRIP)] = primitive::colors::gold;
			colors[static_cast<std::size_t>(RESIZE_GRIP_HOVERED)] = primitive::colors::peru;
			colors[static_cast<std::size_t>(RESIZE_GRIP_ACTIVATED)] = primitive::colors::powder_blue;

			colors[static_cast<std::size_t>(SCROLLBAR_BACKGROUND)] = primitive::colors::white;
			colors[static_cast<std::size_t>(SCROLLBAR_GRAB)] = primitive::colors::dark_salmon;
			colors[static_cast<std::size_t>(SCROLLBAR_GRAB_HOVERED)] = primitive::colors::dark_green;
			colors[static_cast<std::size_t>(SCROLLBAR_GRAB_ACTIVATED)] = primitive::colors::dark_goldenrod;

			colors[static_cast<std::size_t>(CLOSE_BUTTON)] = primitive::colors::red;
			colors[static_cast<std::size_t>(CLOSE_BUTTON_HOVERED)] = primitive::colors::violet_red;
			colors[static_cast<std::size_t>(CLOSE_BUTTON_ACTIVATED)] = primitive::colors::white;

			colors[static_cast<std::size_t>(TOOLTIP_BACKGROUND)] = primitive::colors::black;
			colors[static_cast<std::size_t>(TOOLTIP_TEXT)] = primitive::colors::red;

			colors[static_cast<std::size_t>(BUTTON)] = primitive::colors::sienna;
			colors[static_cast<std::size_t>(BUTTON_HOVERED)] = primitive::colors::slate_gray;
			colors[static_cast<std::size_t>(BUTTON_ACTIVATED)] = primitive::colors::steel_blue;

			colors[static_cast<std::size_t>(FRAME_BACKGROUND)] = primitive::colors::steel_blue;

			colors[static_cast<std::size_t>(RADIO_BUTTON_HOVERED)] = primitive::colors::powder_blue;
			colors[static_cast<std::size_t>(RADIO_BUTTON_ACTIVATED)] = primitive::colors::green_yellow;

			colors[static_cast<std::size_t>(CHECKBOX_HOVERED)] = primitive::colors::powder_blue;
			colors[static_cast<std::size_t>(CHECKBOX_ACTIVATED)] = primitive::colors::green_yellow;

			colors[static_cast<std::size_t>(SLIDER)] = primitive::colors::light_blue;
			colors[static_cast<std::size_t>(SLIDER_ACTIVATED)] = primitive::colors::light_pink;

			colors[static_cast<std::size_t>(COMBO_ITEM)] = primitive::colors::red;
			colors[static_cast<std::size_t>(COMBO_ITEM_HOVERED)] = primitive::colors::violet_red;
			colors[static_cast<std::size_t>(COMBO_ITEM_ACTIVATED)] = primitive::colors::white;

			return colors;
		};

		return
		{
				.window_background_alpha = .65f,
				.window_corner_rounding = 0,
				.window_min_size = {64, 48},
				.window_resize_grip_size = {20, 20},
				.window_padding = {8, 8},
				.window_auto_fit_padding = {8, 8},
				.window_vertical_scrollbar_width = 10,
				.item_default_width_factor = .65f,
				.item_frame_padding = {4, 4},
				.item_spacing = {10, 5},
				.item_inner_spacing = {5, 5},
				.alpha = 1,
				.colors = default_colors(),
				.circle_segment_max_error = .3f,
				.draw_curve_tessellation_tolerance = 1.25f,
		};
	}

	auto show_theme_editor() noexcept -> bool
	{
		return Context::show_theme_editor();
	}
}
