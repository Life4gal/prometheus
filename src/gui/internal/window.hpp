// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <vector>

#include <gui/internal/common.hpp>
#include <gui/internal/draw_list.hpp>

namespace gal::prometheus::gui::internal
{
	class Window final
	{
	public:
		using value_type = extent_type::value_type;
		using alpha_type = Theme::alpha_type;

		// <= 0
		constexpr static auto auto_size = value_type{-.999999f};

	private:
		class IdMaker;
		class Drawer;
		class Anonymous;

		struct canvas_type
		{
			// Cursor start position of the canvas
			point_type cursor_start_line;
			// Cursor current position of the canvas
			point_type cursor_current_line;
			// Cursor previous (line) position of the canvas
			point_type cursor_previous_line;

			// Current line height
			value_type height_current_line;
			// Previous line height
			value_type height_previous_line;

			// The drawing area of the previous widget
			rect_type last_item_rect;
			// Whether the drawing area of the previous widget is hovered by the mouse
			bool last_item_hovered;
			// Whether the drawing area of the previous widget is focused by the mouse
			bool last_item_focused;

			// next item width
			std::vector<value_type> item_width;
			// <0(DrawList::text_wrap_width_not_set): disable
			// =0: window.content_region_max().width
			// >0: width
			std::vector<value_type> text_wrap_width;
		};

		// ==================
		// CANVAS

		canvas_type canvas_;

		// ==================
		// DRAW LIST

		DrawList draw_list_;

		// ==================
		// WINDOW INFO

		std::string name_;
		WindowFlag flag_;

		// The last non-child window in the Context::window_current_stack (this is different from the parent window, which can be a child of another window), you can find it in Context::window_root_list
		Window* root_;
		// All child windows that have been presented in this frame
		std::vector<Window*> children_this_frame_;

		// Position of the window
		point_type point_;
		// Size of the full (expanded) window
		extent_type size_full_;
		// Current window size (depends on whether it is expanded or not, if it is not expanded then it is the size of the titlebar)
		extent_type size_;

		// Current size of contents, extents reach by the drawing cursor
		extent_type size_of_content_;

		// Default width when creating items
		value_type default_item_width_;

		// Current position of the scrollbar
		value_type scroll_y_;
		// Next position of the scrollbar
		value_type scroll_next_y_;
		// Whether the scrollbar is visible (present)
		bool scroll_y_visible_;

		// Whether the window is visible (not closed)
		bool visible_;
		// Whether the window is collapsed or not (titlebar only)
		bool collapsed_;
		// == visible and not collapsed (utility, avoid pad)
		bool skip_item_;

		// Whether any (window's) widget accesses the current window
		bool accessed_;

		// Does auto-fit apply only when the size gets bigger
		bool auto_fit_only_grows_;
		// Frames requiring automatic resizing
		// 2 boolean, avoid padding
		std::int16_t auto_fit_frames_;

		// The number of frames the current window was last drawn, if the window is not visible it will stop counting
		frame_count_type last_drawn_frame_;

		// Used as a seed when generating widget ids, the first element is the id of the window itself
		std::vector<widget_id_type> id_stack_;
		// Clip area of the window, there should be only two elements before/after drawing the widget, the window area and the canvas area
		std::vector<rect_type> clip_rect_stack_;

	public:
		// -----------------------------------
		// ctor & reset

		/**
		 * @brief Create a new window
		 * @param name window name
		 * @param flag window flag
		 * @param point window position
		 * @param size window size
		 * @param root the root of the window (topmost parent window), if the current window is not a child window,
		 * this parameter must be a null pointer (while the root of the window is itself)
		 */
		Window(
			std::string_view name,
			WindowFlag flag,
			const point_type& point,
			const extent_type& size,
			Window* root
		) noexcept;

		/**
		 * @brief If a window has already been created, each "re-creation" only resets its flag (if necessary)
		 * @param flag window flag
		 * @param size If the current window is a child window, set the window size to @c size
		 */
		auto reset(WindowFlag flag, const extent_type& size) noexcept -> void;

		/**
		 * @brief Handles all input devices (mouse and keyboard) at the beginning of each frame (if necessary)
		 */
		auto handle_inputs(const Context& context) noexcept -> void;

		// -----------------------------------
		// DRAW BEGIN

		/**
		 * @brief Creating a canvas for the window
		 * @param context
		 * @param parent If the current window is a child window, then @c parent is its parent, otherwise this parameter must be a null pointer
		 * @param background_fill_alpha Alpha for window background color fill
		 * @return Whether the window is visible (not closed)
		 * @note Widgets can be drawn in the window if and only if its canvas has been created
		 */
		[[nodiscard]] auto begin_window(
			Context& context,
			Window* parent,
			alpha_type background_fill_alpha
		) noexcept -> bool;
		auto end_window(Context& context) noexcept -> void;

		auto begin_child_window(
			Context& context,
			std::string_view name,
			alpha_type background_fill_alpha,
			extent_type size,
			bool border,
			WindowFlag flag
		) noexcept -> void;
		auto end_child_window(Context& context, Window& child) noexcept -> void;

		auto render(Context& context, draw_lists_type& draw_lists) const noexcept -> void;

		// -----------------------------------
		// WIDGETS

		auto draw_text(Context& context, std::string_view utf8_text) noexcept -> void;

		auto draw_button(Context& context, std::string_view utf8_text, extent_type size, bool repeat_when_held) noexcept -> bool;

		auto draw_small_button(Context& context, std::string_view utf8_text, bool repeat_when_held) noexcept -> bool;

		auto draw_radio_button(Context& context, std::string_view utf8_text, bool checked) noexcept -> bool;

		auto draw_checkbox(Context& context, std::string_view utf8_text, bool checked) noexcept -> bool;

		auto draw_slider(
			Context& context,
			std::string_view utf8_text,
			float& reference,
			float min,
			float max,
			std::uint32_t decimal_precision,
			float power
		) noexcept -> bool;

		auto draw_slider_n(
			Context& context,
			std::string_view utf8_text,
			std::span<float> references,
			float min,
			float max,
			std::uint32_t decimal_precision,
			float power
		) noexcept -> bool;

		auto draw_combo(
			Context& context,
			std::string_view utf8_text,
			std::span<const std::string> selections,
			std::size_t& selected,
			std::size_t show_selection_count
		) noexcept -> bool;

		auto draw_combo(
			Context& context,
			std::string_view utf8_text,
			std::span<const std::string_view> selections,
			std::size_t& selected,
			std::size_t show_selection_count
		) noexcept -> bool;

		auto draw_combo(
			Context& context,
			std::string_view utf8_text,
			std::span<const char*> selections,
			std::size_t& selected,
			std::size_t show_selection_count
		) noexcept -> bool;

		// -----------------------------------
		// WIDGET LAYOUT

		auto same_line(Context& context, value_type column_width = layout_auto_size, value_type spacing_width = layout_auto_size) noexcept -> void;

		// -----------------------------------
		// CANVAS LAYOUT

		auto push_item_width(Context& context, Theme::value_type new_item_width) noexcept -> void;
		auto pop_item_width(Context& context) noexcept -> void;

		auto push_text_wrap_width(Context& context, Theme::value_type new_wrap_width) noexcept -> void;
		auto pop_text_wrap_width(Context& context) noexcept -> void;

		// -----------------------------------
		// ID

		[[nodiscard]] auto id_of_move(Context& context) const noexcept -> widget_id_type;

		[[nodiscard]] auto id_of_close(Context& context) const noexcept -> widget_id_type;

		[[nodiscard]] auto id_of_resize(Context& context) const noexcept -> widget_id_type;

		[[nodiscard]] auto id_of_scrollbar(Context& context) const noexcept -> widget_id_type;

		auto push_id(Context& context, std::string_view string) noexcept -> void;

		auto push_id(Context& context, const void* pointer) noexcept -> void;

		auto push_id(Context& context, widget_id_type value) noexcept -> void;

		auto pop_id(Context& context) noexcept -> void;

		// -----------------------------------
		// CLIP RECT

		auto push_clip_rect(Context& context, const rect_type& rect, bool clipped = true) noexcept -> void;

		auto pop_clip_rect(Context& context) noexcept -> void;

		// -----------------------------------
		// STATES

		[[nodiscard]] auto name() const noexcept -> std::string_view;

		[[nodiscard]] auto flag() const noexcept -> WindowFlag;

		[[nodiscard]] auto position() const noexcept -> point_type;

		[[nodiscard]] auto width() const noexcept -> value_type;

		[[nodiscard]] auto height() const noexcept -> value_type;

		[[nodiscard]] auto size() const noexcept -> extent_type;

		[[nodiscard]] auto rect() const noexcept -> rect_type;

		[[nodiscard]] auto visible() const noexcept -> bool;

		[[nodiscard]] auto collapsed() const noexcept -> bool;

		[[nodiscard]] auto root() const noexcept -> const Window&;

		[[nodiscard]] auto content_region_max(const Context& context) const noexcept -> extent_type;
		[[nodiscard]] auto window_content_region_min(const Context& context) const noexcept -> extent_type;
		[[nodiscard]] auto window_content_region_max(const Context& context) const noexcept -> extent_type;

		/**
		 * @brief Test if mouse cursor is hovering given rect
		 * @note @c rect is clipped by current clip rect setting 
		 */
		[[nodiscard]] auto is_hovered(const Context& context, const rect_type& rect) const noexcept -> bool;

		[[nodiscard]] auto is_item_hovered(const Context& context) const noexcept -> bool;
		[[nodiscard]] auto is_item_focused(const Context& context) const noexcept -> bool;

		// -----------------------------------
		// CONTEXT

		// auto show() noexcept -> void;
		auto hide() noexcept -> void;

		/**
		 * @brief Find the first (more top-level) window that contains the location of the given point
		 */
		[[nodiscard]] auto find_hovered_window(const point_type& position, bool excludes_children) noexcept -> Window*;
	};
}
