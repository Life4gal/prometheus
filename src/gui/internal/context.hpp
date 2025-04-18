// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <vector>

#include <memory/unique_ptr.hpp>

#include <gui/internal/common.hpp>
#include <gui/internal/font.hpp>
#include <gui/internal/mouse.hpp>

namespace gal::prometheus
{
	namespace gui
	{
		namespace internal
		{
			class Font;
			class DrawList;
			class Window;
		}

		class Context
		{
		public:
			using value_type = extent_type::value_type;

			struct theme_color_mod
			{
				ThemeCategory category;
				Theme::color_type old_color;
			};

			using window_type = internal::Window;
			using widget_id_type = internal::widget_id_type;

			// < 0
			constexpr static value_type window_fill_alpha_not_set{-.99999f};

			bool initialized;

			// ----------------------------------------------------------------------
			// DrawListFlag + DrawListSharedData + Font + Theme

			DrawListFlag draw_list_flag;

			internal::DrawListSharedData draw_list_shared_data;

			internal::Font font;

			Theme theme;
			std::vector<theme_color_mod> theme_mod_stack;

			// ----------------------------------------------------------------------
			// IO

			IO io;
			// time elapsed since program launch, in seconds
			time_type time_total;
			std::uint32_t frame_count;
			std::uint32_t frame_count_rendered;

			internal::Mouse mouse;
			// todo: keyboard

			// ----------------------------------------------------------------------
			// WINDOW

			point_type window_default_spawn_position;

			// All created windows
			std::vector<memory::UniquePointer<window_type>> window_hive;

			// Root window only, child windows are managed by their parent window
			// The order of the windows in this list determines the drawing order, the windows at the end are drawn last
			std::vector<window_type*> window_root_list;

			// Current window stack, push the stack when begin_window is called, and pop the stack when end_window is called (Nested calls are also included in this list)
			std::vector<window_type*> window_current_stack;

			// catch mouse
			window_type* window_hovered;
			// catch mouse (for focus/move only)
			window_type* window_hovered_root;
			// catch keyboard
			window_type* window_focused;

			// ----------------------------------------------------------------------
			// WIDGET

			// widget for mouse hovering in this frame
			widget_id_type widget_hovered;
			// widget for mouse selecting in this frame
			widget_id_type widget_activated;
			// widget for mouse hovering in previous frame
			widget_id_type widget_activated_previous_frame;
			// widget for mouse selecting in this frame is still alive
			bool widget_activated_still_alive;

			// Currently open combo window
			widget_id_type widget_activated_combo_id;

			// ----------------------------------------------------------------------
			// DRAW LIST

			std::vector<std::reference_wrapper<const internal::DrawList>> draw_lists;
		};

		namespace internal
		{
			auto begin_window(
				Context& context,
				std::string_view name,
				const extent_type& size,
				Theme::value_type fill_alpha,
				WindowFlag flag
			) noexcept -> bool;

			auto begin_child_window(
				Context& context,
				std::string_view name,
				Theme::alpha_type background_fill_alpha,
				const extent_type& size,
				bool border,
				WindowFlag flag
			) noexcept -> void;

			// ----------------------------------------------------------------------
			// DrawListFlag

			auto current_draw_list_flag(const Context& context) noexcept -> DrawListFlag;
			// auto push_draw_list_flag(Context& context, DrawListFlag flag) noexcept -> void;
			// auto pop_draw_list_flag(Context& context) noexcept -> void;

			// ----------------------------------------------------------------------
			// DrawListSharedData

			auto current_draw_list_shared_data(const Context& context) noexcept -> const DrawListSharedData&;
			// auto push_draw_list_shared_data(Context& context, const DrawListSharedData& shared_data) noexcept -> void;
			// auto pop_draw_list_shared_data(Context& context) noexcept -> void;

			// ----------------------------------------------------------------------
			// Font

			auto current_font(const Context& context) noexcept -> const Font&;
			// auto push_font(Context& context, memory::UniquePointer<Font> font) noexcept -> void;
			// auto pop_font(Context& context) noexcept -> void;

			// ----------------------------------------------------------------------
			// Theme

			auto current_theme(const Context& context) noexcept -> const Theme&;
			auto push_theme(Context& context, ThemeCategory category, Theme::color_type new_color) noexcept -> void;
			auto pop_theme(Context& context) noexcept -> void;

			[[nodiscard]] auto color_of(const Theme& theme, ThemeCategory category, Theme::value_type factor = 1) noexcept -> Theme::color_type;
			[[nodiscard]] auto color_of(const Context& context, ThemeCategory category, Theme::value_type factor = 1) noexcept -> Theme::color_type;

			// ----------------------------------------------------------------------
			// IO

			enum class MouseState : std::uint8_t
			{
				NONE = 0,

				HOVERED = 1 << 0,
				PRESSED = 1 << 1,
				KEEPING = 1 << 2,
			};

			// Test the behavior of the mouse on the target widget
			[[nodiscard]] auto test_mouse(Context& context, widget_id_type id, const rect_type& area, bool repeat = false) noexcept -> std::underlying_type_t<MouseState>;
			// Similar to test_mouse, but does not activate any widget
			[[nodiscard]] auto queue_mouse(Context& context, widget_id_type id, const rect_type& area) noexcept -> std::underlying_type_t<MouseState>;

			// ----------------------------------------------------------------------
			// WINDOW

			// Test that the mouse is hovering over the target window
			[[nodiscard]] auto is_window_hovered(const Context& context, const Window& window) noexcept -> bool;

			// Focus on the target window (this determines the order in which the windows are drawn)
			auto focus_window(Context& context, Window& window) noexcept -> void;

			// ----------------------------------------------------------------------
			// WIDGET

			// Whether the mouse is hovering over the target widget
			[[nodiscard]] auto is_widget_hovered(const Context& context, widget_id_type id) noexcept -> bool;
			// Whether the mouse is hovering over the widget
			[[nodiscard]] auto is_any_widget_hovered(const Context& context) noexcept -> bool;
			// Whether the mouse is selecting over the target widget
			[[nodiscard]] auto is_widget_activated(const Context& context, widget_id_type id) noexcept -> bool;
			// Whether the mouse is selecting over the widget
			[[nodiscard]] auto is_any_widget_activated(const Context& context) noexcept -> bool;

			// Marks the current widget alive, returns whether it is currently selected (activated) or not
			auto mark_widget_alive(Context& context, widget_id_type id) noexcept -> bool;
			// Marks that no widget is currently selected (activated)
			auto mark_widget_dead(Context& context, widget_id_type id = invalid_widget_id) noexcept -> void;

			// Activate the specified combo widget (window)
			auto mark_combo_alive(Context& context, widget_id_type id) noexcept -> void;
			// Deactivate the specified combo widget (window)
			auto mark_combo_dead(Context& context, widget_id_type id = invalid_widget_id) noexcept -> void;
			// Whether the target combo widget is active or not
			[[nodiscard]] auto is_combo_activated(const Context& context, widget_id_type id) noexcept -> bool;
		}
	}

	namespace meta::user_defined
	{
		template<>
		struct enum_is_flag<gui::internal::MouseState> : std::true_type {};
	}
}
