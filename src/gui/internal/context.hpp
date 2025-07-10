// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <vector>
#include <memory>

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

		private:
			bool initialized_;

			// ----------------------------------------------------------------------
			// DrawListFlag + DrawListSharedData + Font + Theme

			DrawListFlag draw_list_flag_;

			internal::DrawListSharedData draw_list_shared_data_;

			internal::Font font_;

			Theme theme_;
			std::vector<theme_color_mod> theme_mod_stack_;

			// ----------------------------------------------------------------------
			// IO

			IO io_;
			// time elapsed since program launch, in seconds
			time_type time_total_;
			std::uint32_t frame_count_;
			std::uint32_t frame_count_rendered_;

			internal::Mouse mouse_;
			// todo: keyboard

			// ----------------------------------------------------------------------
			// WINDOW

			point_type window_default_spawn_position_;

			// All created windows
			std::vector<std::unique_ptr<window_type>> window_hive_;

			// Root window only, child windows are managed by their parent window
			// The order of the windows in this list determines the drawing order, the windows at the end are drawn last
			std::vector<window_type*> window_root_list_;

			// Current window stack, push the stack when begin_window is called, and pop the stack when end_window is called (Nested calls are also included in this list)
			std::vector<window_type*> window_current_stack_;

			// catch mouse
			window_type* window_hovered_;
			// catch mouse (for focus/move only)
			window_type* window_hovered_root_;
			// catch keyboard
			window_type* window_focused_;

			// ----------------------------------------------------------------------
			// WIDGET

			// widget for mouse hovering in this frame
			widget_id_type widget_hovered_;
			// widget for mouse selecting in this frame
			widget_id_type widget_activated_;
			// widget for mouse hovering in previous frame
			widget_id_type widget_activated_previous_frame_;
			// widget for mouse selecting in this frame is still alive
			bool widget_activated_still_alive_;

			// Currently open combo window
			widget_id_type widget_activated_combo_id_;

			// ----------------------------------------------------------------------
			// DRAW LIST

			internal::draw_lists_type draw_lists_;

			Context() noexcept;

		public:
			// ----------------------------------------------------------------------
			// Context

			[[nodiscard]] static auto create() noexcept -> Context*;
			static auto destroy(Context& context) noexcept -> void;

			// ----------------------------------------------------------------------
			// DrawListFlag

			auto set_default_draw_list_flag(DrawListFlag flag) noexcept -> void;

			[[nodiscard]] auto current_draw_list_flag() const noexcept -> DrawListFlag;
			// auto push_draw_list_flag(DrawListFlag flag) noexcept -> void;
			// auto pop_draw_list_flag() noexcept -> void;

			// ----------------------------------------------------------------------
			// DrawListSharedData

			[[nodiscard]] auto current_draw_list_shared_data() const noexcept -> const internal::DrawListSharedData&;
			// auto push_draw_list_shared_data(const DrawListSharedData& shared_data) noexcept -> void;
			// auto pop_draw_list_shared_data() noexcept -> void;

			// ----------------------------------------------------------------------
			// Font

			[[nodiscard]] auto set_default_font(const FontOption& option) noexcept -> Texture;

			[[nodiscard]] auto current_font() const noexcept -> const internal::Font&;

			// ----------------------------------------------------------------------
			// Theme

			auto set_default_theme(const Theme& default_theme) noexcept -> void;

			[[nodiscard]] auto current_theme() const noexcept -> const Theme&;
			auto push_theme(ThemeCategory category, Theme::color_type new_color) noexcept -> void;
			auto pop_theme() noexcept -> void;

			[[nodiscard]] auto color_of(ThemeCategory category, Theme::value_type factor = 1) const noexcept -> Theme::color_type;
			[[nodiscard]] auto color_of(const Theme& theme, ThemeCategory category, Theme::value_type factor = 1) const noexcept -> Theme::color_type;

			// ----------------------------------------------------------------------
			// IO

			[[nodiscard]] auto io() noexcept -> IO&;

			[[nodiscard]] auto io() const noexcept -> const IO&;

			[[nodiscard]] auto mouse() const noexcept -> const internal::Mouse&;

			enum class MouseState : std::uint8_t
			{
				NONE = 0,

				HOVERED = 1 << 0,
				PRESSED = 1 << 1,
				KEEPING = 1 << 2,
			};

			/**
			 * @brief Test the behavior of the mouse on the target widget
			 * @param id widget id
			 * @param area widget rect
			 * @param repeat 
			 * @note If the left mouse button is clicked, the target widget is selected
			 */
			[[nodiscard]] auto test_mouse(widget_id_type id, const rect_type& area, bool repeat = false) noexcept -> std::underlying_type_t<MouseState>;

			/**
			 * @brief Test the behavior of the mouse on the target widget
			 * @param id widget id
			 * @param area widget rect
			 * @note Even if the left mouse button is clicked, the target widget will not be selected,
			 * this applies to some widget that do not want to be selected
			 */
			[[nodiscard]] auto queue_mouse(widget_id_type id, const rect_type& area) noexcept -> std::underlying_type_t<MouseState>;

			// ----------------------------------------------------------------------
			// WINDOW

		private:
			/**
			 * @brief Finds the window with the given name
			 * @return Returns the corresponding window if it exists, otherwise it returns a null pointer
			 * @note The naming convention for child windows is parent_name.child_name
			 */
			[[nodiscard]] auto find_window(std::string_view name) noexcept -> window_type*;

			/**
			 * @brief If the target window does not exist, then create it, otherwise return the corresponding window
			 * @param name window name
			 * @param size window size (used only when creating window)
			 * @param flag window flag
			 */
			[[nodiscard]] auto find_or_create_window(std::string_view name, const extent_type& size, internal::WindowFlag flag) noexcept -> window_type&;

		public:
			/**
			 * @brief Find the last window from the available windows in this frame
			 */
			[[nodiscard]] auto current_window() const noexcept -> window_type&;

			/**
			 * @brief Find the last parent window from the available windows in this frame
			 */
			[[nodiscard]] auto current_parent_window() const noexcept -> window_type&;

			/**
			 * @brief Find the last possible root (not child) window from the available windows in this frame
			 * @return If it is not found (if and only if there are no currently available windows, i.e. the window to be created is the first one), then the null pointer is returned
			 */
			[[nodiscard]] auto current_root_window() noexcept -> window_type*;

			auto set_next_window_point(const point_type& point) noexcept -> void;

			auto begin_window(
				std::string_view name,
				const extent_type& size,
				Theme::value_type background_fill_alpha,
				internal::WindowFlag flag
			) noexcept -> bool;
			auto end_window() noexcept -> void;

			auto begin_child_window(
				std::string_view name,
				const extent_type& size,
				Theme::alpha_type background_fill_alpha,
				internal::WindowFlag flag,
				bool border
			) noexcept -> void;
			auto end_child_window() noexcept -> void;

			// ----------------------------------------------------------------------
			// WINDOW STATE

			/**
			 * @brief Test that the mouse is hovering over the target window
			 */
			[[nodiscard]] auto is_window_hovered(const window_type& window) const noexcept -> bool;

			/**
			 * @brief Find the first (more top-level) window that contains the location of the given point
			 */
			[[nodiscard]] auto find_hovered_window(const point_type& position, bool excludes_children) noexcept -> window_type*;

			/**
			 * @brief Focus on the target window (this determines the order in which the windows are drawn)
			 */
			auto focus_window(window_type& window) noexcept -> void;

			// ----------------------------------------------------------------------
			// WIDGET STATE

			// Whether the mouse is hovering over the target widget
			[[nodiscard]] auto is_widget_hovered(widget_id_type id) const noexcept -> bool;
			// Whether the mouse is hovering over the widget
			[[nodiscard]] auto is_any_widget_hovered() const noexcept -> bool;
			// Whether the mouse is selecting over the target widget
			[[nodiscard]] auto is_widget_activated(widget_id_type id) const noexcept -> bool;
			// Whether the mouse is selecting over the widget
			[[nodiscard]] auto is_any_widget_activated() const noexcept -> bool;

			// Marks the current widget alive, returns whether it is currently selected (activated) or not
			auto mark_widget_alive(widget_id_type id) noexcept -> bool;
			// Marks that no widget is currently selected (activated)
			auto mark_widget_dead(widget_id_type id = internal::invalid_widget_id) noexcept -> void;

			// Activate the specified combo widget (window)
			auto mark_combo_alive(widget_id_type id) noexcept -> void;
			// Deactivate the specified combo widget (window)
			auto mark_combo_dead(widget_id_type id = internal::invalid_widget_id) noexcept -> void;
			// Whether the target combo widget is active or not
			[[nodiscard]] auto is_combo_activated(widget_id_type id) const noexcept -> bool;

			// ----------------------------------------------------------------------
			// RENDER

			[[nodiscard]] auto current_time() const noexcept -> time_type;

			[[nodiscard]] auto current_frame() const noexcept -> internal::frame_count_type;

			auto new_frame() noexcept -> void;

			auto end_frame() noexcept -> void;

			auto render() noexcept -> void;

			[[nodiscard]] auto get_draw_data() noexcept -> std::vector<DrawData>;

			// ----------------------------------------------------------------------
			// TEST

			static auto show_theme_editor() noexcept -> bool;
		};
	}

	template<>
	struct meta::user_defined::enum_is_flag<gui::Context::MouseState> : std::true_type {};
}
