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

	/**
	 * @brief Finds the window with the given name
	 * @return Returns the corresponding window if it exists, otherwise it returns a null pointer
	 */
	[[nodiscard]] auto find_window(Context& context, const std::string_view name) noexcept -> internal::Window*
	{
		auto view = context.window_list | std::views::all;

		const auto it = std::ranges::find_if(
			view,
			[name](const auto& window) noexcept -> bool
			{
				return name == window->name();
			}
		);

		if (it == std::ranges::end(view))
		{
			return nullptr;
		}

		return it.operator*();
	}

	/**
	 * @brief Find the last possible parent (not child) window from the available windows in this frame
	 * @return If it is not found (if and only if there are no currently available windows, i.e. the window to be created is the first one), then the null pointer is returned
	 */
	[[nodiscard]] auto find_root_window(Context& context) noexcept -> internal::Window*
	{
		auto view = context.window_current_stack | std::views::all;

		const auto it = std::ranges::find_if(
			view,
			[](const auto& window) noexcept -> bool
			{
				return not window->flag().template is<internal::WindowInternalFlag::CHILD_WINDOW>();
			}
		);

		if (it == std::ranges::end(view))
		{
			return nullptr;
		}

		return it.operator*();
	}

	[[nodiscard]] auto find_or_create_window(Context& context, const std::string_view name, const extent_type& size, const internal::Window::Flag flag) noexcept -> internal::Window&
	{
		[[maybe_unused]] const auto is_child_window = flag.is<internal::WindowInternalFlag::CHILD_WINDOW>();

		if (auto* window = find_window(context, name);
			window == nullptr)
		{
			// find root
			auto* root = find_root_window(context);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(is_child_window == (root != nullptr));

			// fixme: load cached settings?
			auto temp = memory::make_unique<internal::Window>(name, flag, context.window_default_spawn_position, size, root);

			auto& ref = context.window_hive.emplace_back(std::move(temp));

			context.window_list.emplace_back(ref.get());
			context.window_current_stack.emplace_back(ref.get());
		}
		else
		{
			window->reset(flag);
			context.window_current_stack.emplace_back(window);
		}

		return *context.window_current_stack.back();
	}

	/**
	 * @brief Find the first (more top-level) window that contains the location of the given point
	 */
	[[nodiscard]] auto find_hovered_window(Context& context, const point_type& position, bool parent_only) noexcept -> internal::Window*
	{
		auto view = context.window_list | std::views::reverse;

		const auto it = std::ranges::find_if(
			view,
			[position, parent_only](const auto& window) noexcept -> bool
			{
				if (not window->visible())
				{
					return false;
				}

				if (parent_only and window->flag().template is<internal::WindowInternalFlag::CHILD_WINDOW>())
				{
					return false;
				}

				const auto rect = window->rect();
				return rect.includes(position);
			}
		);

		if (it == std::ranges::end(view))
		{
			return nullptr;
		}

		return it.operator*();
	}

	Context* g_context = nullptr;
}

namespace gal::prometheus::gui
{
	auto create_context() noexcept -> Context*
	{
		auto* p = new ::Context{
				.initialized = false,
				.draw_list_flag = DrawListFlag::NONE,
				.draw_list_shared_data = {},
				.font = {},
				.theme = {},
				.theme_mod_stack = {},
				.io = {},
				.time_total = 0,
				.frame_count = 0,
				.frame_count_rendered = 0,
				.mouse = {},
				.window_default_spawn_position = {50, 50},
				.window_hive = {},
				.window_list = {},
				.window_current_stack = {},
				.window_hovered = nullptr,
				.window_hovered_root = nullptr,
				.window_focused = nullptr,
				.widget_hovered = internal::invalid_widget_id,
				.widget_activated = internal::invalid_widget_id,
				.widget_activated_previous_frame = internal::invalid_widget_id,
				.widget_activated_still_alive = false,
				.draw_lists = {}
		};

		// todo
		p->initialized = true;

		return p;
	}

	auto destroy_context(Context& context) noexcept -> void
	{
		delete std::addressof(context);
	}

	auto destroy_context(Context* context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context != nullptr);
		destroy_context(*context);
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
			colors[static_cast<std::size_t>(SLIDER_ACTIVATED)] = primitive::colors::deep_sky_blue;

			return colors;
		};

		return
		{
				.window_background_alpha = .65f,
				.window_titlebar_height = 20,
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

	auto set_default_theme(Context& context, const Theme& theme) noexcept -> void
	{
		context.theme = theme;
	}

	auto push_theme(Context& context, const ThemeCategory category, const Theme::color_type new_color) noexcept -> void
	{
		internal::push_theme(context, category, new_color);
	}

	auto pop_theme(Context& context) noexcept -> void
	{
		internal::pop_theme(context);
	}

	auto get_io(Context& context) noexcept -> IO&
	{
		return context.io;
	}

	auto set_default_draw_list_flag(Context& context, const DrawListFlag flag) noexcept -> void
	{
		context.draw_list_flag = flag;
	}

	auto new_frame(Context& context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.io.display_size.width > 0 and context.io.display_size.height > 0);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.io.delta_time > 0);

		context.time_total += context.io.delta_time;
		context.frame_count += 1;

		// Update mouse
		{
			context.mouse.tick(context);
		}

		const auto mouse_position = context.mouse.position_current;

		// Update widget
		{
			// Clear reference to active widget if the widget isn't alive anymore
			context.widget_hovered = internal::invalid_widget_id;
			if (
				not context.widget_activated_still_alive and
				context.widget_activated_previous_frame == context.widget_activated and
				context.widget_activated != internal::invalid_widget_id
			)
			{
				context.widget_activated_previous_frame = context.widget_activated;
			}
			context.widget_activated_still_alive = false;
		}

		// Update window
		{
			context.window_hovered = find_hovered_window(context, mouse_position, false);
			context.window_hovered_root = find_hovered_window(context, mouse_position, true);

			// Mark all windows as not visible
			std::ranges::for_each(
				context.window_list,
				[](auto* window) noexcept -> void
				{
					window->hide();
				}
			);

			// No window should be open at the beginning of the frame
			// But in order to allow the user to call `new_frame` multiple times without calling `render`, we are doing an explicit clear
			context.window_current_stack.clear();

			if (context.window_hovered != nullptr)
			{
				context.window_hovered->handle_inputs(context);
			}
		}
	}

	// ReSharper disable once CppParameterMayBeConstPtrOrRef
	auto end_frame(Context& context) noexcept -> void
	{
		std::ignore = context;
	}

	auto render(Context& context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);

		const auto& theme = internal::current_theme(context);

		const auto is_first_render_this_frame = context.frame_count_rendered != context.frame_count;
		context.frame_count_rendered = context.frame_count;

		if (is_first_render_this_frame)
		{
			// Sort the window list so that all child windows are after their parent
			// We cannot do that on `focus` because children may not exist yet

			std::vector<Context::window_type*> sorted_windows{};
			sorted_windows.reserve(context.window_list.size());

			std::ranges::for_each(
				context.window_list,
				[&](auto* window) noexcept -> void
				{
					// todo: child window
					if (window->flag().template is<internal::WindowInternalFlag::CHILD_WINDOW>() and window->visible())
					{
						return;
					}

					sorted_windows.push_back(window);
				}
			);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(sorted_windows.size() == context.window_list.size());

			context.window_list.swap(sorted_windows);

			// clear all data for new frame
			context.io.delta_time = -1;
			// context.io.mouse_position = {0, 0};
			context.io.mouse_wheel = 0;
			// context.io.mouse_button_state.state.fill(false);
		}

		context.draw_lists.clear();

		if (theme.alpha > .0f)
		{
			// gather windows to render
			std::ranges::for_each(
				context.window_list,
				[&context](auto* window) noexcept -> void
				{
					if (window->visible())
					{
						window->render(context);
					}
				}
			);
		}
	}

	auto get_draw_data(Context& context) noexcept -> std::vector<DrawData>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);

		std::vector<DrawData> data;
		data.reserve(context.draw_lists.size());

		std::ranges::transform(
			context.draw_lists,
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

	auto begin_window(
		Context& context,
		const std::string_view name,
		const extent_type& size,
		Theme::value_type fill_alpha,
		const WindowFlag flag
	) noexcept -> bool
	{
		const auto& theme = internal::current_theme(context);

		auto& window = find_or_create_window(context, name, size, flag);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.window_current_stack.back() == std::addressof(window));

		// alpha
		static_assert(Context::window_fill_alpha_not_set < 0);
		if (fill_alpha < 0)
		{
			fill_alpha = theme.window_background_alpha;
		}

		const auto is_child_window = window.flag().is<internal::WindowInternalFlag::CHILD_WINDOW>();
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(is_child_window == (context.window_current_stack.size() > 1));

		auto* parent = is_child_window ? context.window_current_stack[context.window_current_stack.size() - 2] : nullptr;

		return window.begin_draw(context, fill_alpha, parent);
	}

	auto end_window(Context& context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		auto& window = *context.window_current_stack.back();

		window.end_draw(context);

		// Select window for move/focus when we're done with all our widgets (we only consider non-children windows here)
		if (const auto rect = window.rect();
			context.widget_activated == internal::invalid_widget_id and
			context.widget_hovered == internal::invalid_widget_id and
			context.window_hovered_root == std::addressof(window) and
			window.hovered(context, rect) and
			context.mouse.is_clicked(context, MouseKey::LEFT)
		)
		{
			context.widget_activated = window.id_of_move(context);
		}

		context.window_current_stack.pop_back();
	}

	auto draw_text(Context& context, const std::string_view utf8_text) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		auto& window = *context.window_current_stack.back();
		window.draw_text(context, utf8_text);
	}

	auto draw_button(Context& context, const std::string_view utf8_text, const extent_type& size, const bool repeat_when_held) noexcept -> bool
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		auto& window = *context.window_current_stack.back();
		return window.draw_button(context, utf8_text, size, repeat_when_held);
	}

	auto draw_small_button(Context& context, const std::string_view utf8_text, const bool repeat_when_held) noexcept -> bool
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		auto& window = *context.window_current_stack.back();
		return window.draw_small_button(context, utf8_text, repeat_when_held);
	}

	auto draw_radio_button(Context& context, const std::string_view utf8_text, const bool checked) noexcept -> bool
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		auto& window = *context.window_current_stack.back();
		return window.draw_radio_button(context, utf8_text, checked);
	}

	auto draw_checkbox(Context& context, const std::string_view utf8_text, const bool checked) noexcept -> bool
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		auto& window = *context.window_current_stack.back();
		return window.draw_checkbox(context, utf8_text, checked);
	}

	auto layout_same_line(const Context& context, const Theme::value_type column_width, const Theme::value_type spacing_width) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		auto& window = *context.window_current_stack.back();
		window.same_line(context, column_width, spacing_width);
	}

	auto get_content_region_max(const Context& context) noexcept -> extent_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		const auto& window = *context.window_current_stack.back();
		return window.content_region_max(context);
	}

	auto get_window_content_region_min(const Context& context) noexcept -> extent_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		const auto& window = *context.window_current_stack.back();
		return window.window_content_region_min(context);
	}

	auto get_window_content_region_max(const Context& context) noexcept -> extent_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

		const auto& window = *context.window_current_stack.back();
		return window.window_content_region_max(context);
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

	auto draw_text(const std::string_view utf8_text) noexcept -> void
	{
		auto& context = get_current_context();

		draw_text(context, utf8_text);
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

	auto layout_same_line(const Theme::value_type column_width, const Theme::value_type spacing_width) noexcept -> void
	{
		const auto& context = get_current_context();

		layout_same_line(context, column_width, spacing_width);
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

	namespace internal
	{
		auto current_draw_list_flag(const Context& context) noexcept -> DrawListFlag
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);

			// return context.draw_list_flag_stack[context.draw_list_flag_current];
			return context.draw_list_flag;
		}

		// auto push_draw_list_flag(Context& context, const DrawListFlag flag) noexcept -> void
		// {
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);
		//
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
		// 		context.draw_list_flag_current == Context::stack_pointer_default or
		// 		context.draw_list_flag_current < Context::draw_list_flag_stack_size,
		// 		"DrawListFlag stack overflow"
		// 	);
		//
		// 	static_assert(
		// 		static_cast<::Context::stack_pointer_type>(::Context::stack_pointer_default + 1) ==
		// 		static_cast<::Context::stack_pointer_type>(0)
		// 	);
		//
		// 	context.draw_list_flag_current += 1;
		// 	context.draw_list_flag_stack[context.draw_list_flag_current] = flag;
		// }
		//
		// auto pop_draw_list_flag(Context& context) noexcept -> void
		// {
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);
		//
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
		// 		context.draw_list_flag_current != Context::stack_pointer_default,
		// 		"Unable to popup the default DrawListFlag!"
		// 	);
		//
		// 	static_assert(
		// 		static_cast<Context::stack_pointer_type>(static_cast<Context::stack_pointer_type>(0) - 1) ==
		// 		Context::stack_pointer_default
		// 	);
		//
		// 	context.draw_list_flag_stack[context.draw_list_flag_current] = DrawListFlag::NONE;
		// 	context.draw_list_flag_current -= 1;
		// }

		auto current_draw_list_shared_data(const Context& context) noexcept -> const DrawListSharedData&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);

			// return context.draw_list_shared_data_stack[context.draw_list_shared_data_current];
			return context.draw_list_shared_data;
		}

		// auto push_draw_list_shared_data(Context& context, const DrawListSharedData& shared_data) noexcept -> void
		// {
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);
		//
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
		// 		context.draw_list_shared_data_current == Context::stack_pointer_default or
		// 		context.draw_list_shared_data_current < Context::draw_list_shared_data_stack_size,
		// 		"DrawListSharedData stack overflow"
		// 	);
		//
		// 	static_assert(
		// 		static_cast<::Context::stack_pointer_type>(::Context::stack_pointer_default + 1) ==
		// 		static_cast<::Context::stack_pointer_type>(0)
		// 	);
		//
		// 	context.draw_list_shared_data_current += 1;
		// 	context.draw_list_shared_data_stack[context.draw_list_shared_data_current] = shared_data;
		// }
		//
		// auto pop_draw_list_shared_data(Context& context) noexcept -> void
		// {
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);
		//
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
		// 		context.draw_list_shared_data_current != Context::stack_pointer_default,
		// 		"Unable to popup the default DrawListSharedData!"
		// 	);
		//
		// 	static_assert(
		// 		static_cast<Context::stack_pointer_type>(static_cast<Context::stack_pointer_type>(0) - 1) ==
		// 		Context::stack_pointer_default
		// 	);
		//
		// 	context.draw_list_shared_data_stack[context.draw_list_shared_data_current] = {};
		// 	context.draw_list_shared_data_current -= 1;
		// }

		auto current_font(const Context& context) noexcept -> const Font&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);

			// const auto& font = *context.font_stack[context.font_current];
			// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(font.loaded(), "Invalid font!");
			//
			// return font;
			return context.font;
		}

		// auto push_font(Context& context, memory::UniquePointer<Font> font) noexcept -> void
		// {
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);
		//
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
		// 		context.font_current == Context::stack_pointer_default or
		// 		context.font_current < Context::font_stack_size,
		// 		"Font stack overflow"
		// 	);
		//
		// 	static_assert(
		// 		static_cast<Context::stack_pointer_type>(Context::stack_pointer_default + 1) ==
		// 		static_cast<Context::stack_pointer_type>(0)
		// 	);
		//
		// 	context.font_current += 1;
		// 	context.font_stack[context.font_current] = std::move(font);
		// }
		//
		// auto pop_font(Context& context) noexcept -> void
		// {
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);
		//
		// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(
		// 		context.font_current != Context::stack_pointer_default,
		// 		"Unable to popup the default Font!"
		// 	);
		//
		// 	static_assert(
		// 		static_cast<Context::stack_pointer_type>(static_cast<Context::stack_pointer_type>(0) - 1) ==
		// 		Context::stack_pointer_default
		// 	);
		//
		// 	context.font_stack[context.font_current].reset();
		// 	context.font_current -= 1;
		// }

		auto current_theme(const Context& context) noexcept -> const Theme&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);

			return context.theme;
		}

		auto push_theme(Context& context, const ThemeCategory category, const Theme::color_type new_color) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);

			const auto old_color = color_of(context, category);
			context.theme_mod_stack.emplace_back(category, old_color);

			context.theme.colors[static_cast<std::size_t>(category)] = new_color;
		}

		auto pop_theme(Context& context) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context.initialized);

			const auto [category, old_color] = context.theme_mod_stack.back();
			context.theme_mod_stack.pop_back();
			context.theme.colors[static_cast<std::size_t>(category)] = old_color;
		}

		auto color_of(const Theme& theme, const ThemeCategory category, const Theme::value_type factor) noexcept -> Theme::color_type
		{
			const auto index = static_cast<std::size_t>(category);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < theme_category_count);
			auto color = theme.colors[static_cast<std::size_t>(category)];

			color.alpha = static_cast<Theme::color_type::value_type>(theme.alpha * 255 * factor);

			return color;
		}

		auto color_of(const Context& context, const ThemeCategory category, const Theme::value_type factor) noexcept -> Theme::color_type
		{
			const auto& theme = current_theme(context);

			return color_of(theme, category, factor);
		}

		auto test_mouse(Context& context, const widget_id_type id, const rect_type& area, const bool repeat) noexcept -> std::underlying_type_t<MouseState>
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not context.window_current_stack.empty());

			const auto& window = *context.window_current_stack.back();
			auto state = std::to_underlying(MouseState::NONE);

			const auto hovered =
					// window
					context.window_hovered_root == std::addressof(window) and
					// new
					context.widget_hovered == invalid_widget_id and
					// mouse
					window.hovered(context, area);

			if (hovered)
			{
				state |= MouseState::HOVERED;

				// hovering widget
				context.widget_hovered = id;

				if (context.mouse.is_clicked(context, MouseKey::LEFT, false))
				{
					// select widget
					context.widget_activated = id;
				}
				else if (
					repeat and
					context.widget_activated != invalid_widget_id and
					context.mouse.is_clicked(context, MouseKey::LEFT, true)
				)
				{
					state |= MouseState::PRESSED;
				}
			}

			if (context.widget_activated == id)
			{
				if (context.mouse.is_down(context, MouseKey::LEFT))
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
					context.widget_activated = invalid_widget_id;
				}
			}

			return state;
		}

		auto is_window_hovered(const Context& context, const Window& window) noexcept -> bool
		{
			return context.window_hovered == std::addressof(window);
		}

		auto focus_window(Context& context, Window& window) noexcept -> void
		{
			context.window_focused = std::addressof(window);

			const auto it = std::ranges::find(context.window_list, std::addressof(window));

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it != context.window_list.end());

			const auto p = *it;
			context.window_list.erase(it);
			context.window_list.push_back(p);
		}

		auto is_widget_hovered(const Context& context, const widget_id_type id) noexcept -> bool
		{
			return context.widget_hovered == id;
		}

		auto is_widget_activated(const Context& context, const widget_id_type id) noexcept -> bool
		{
			return context.widget_activated == id;
		}

		auto mark_widget_alive(Context& context, const widget_id_type id) noexcept -> bool
		{
			if (is_widget_activated(context, id))
			{
				context.widget_activated_still_alive = true;

				return true;
			}

			return false;
		}

		auto mark_widget_dead(Context& context, const widget_id_type id) noexcept -> void
		{
			context.widget_activated = id;
		}
	}
}
