// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <vector>

#include <gui/internal/common.hpp>
#include <gui/internal/draw_list.hpp>

namespace gal::prometheus
{
	namespace gui::internal
	{
		enum class WindowInternalFlag : std::uint16_t
		{
			NONE = 0,

			CHILD_WINDOW = 1 << 0,
			CHILD_WINDOW_AUTO_FIT_X = 1 << 1,
			CHILD_WINDOW_AUTO_FIT_Y = 1 << 2,

			CATEGORY_TOOLTIP = 1 << 3,
		};
	}

	namespace meta::user_defined
	{
		template<>
		struct enum_is_flag<gui::internal::WindowInternalFlag> : std::true_type {};
	}

	namespace gui::internal
	{
		class Window final
		{
		public:
			using value_type = extent_type::value_type;

			class [[nodiscard]] Flag final
			{
				WindowFlag flag_;
				WindowInternalFlag internal_flag_;

			public:
				constexpr explicit (false) Flag(const WindowFlag flag) noexcept
					: flag_{flag},
					  internal_flag_{WindowInternalFlag::NONE} {}

				constexpr explicit Flag(const WindowFlag flag, const WindowInternalFlag internal_flag) noexcept
					: flag_{flag},
					  internal_flag_{internal_flag} {}

				[[nodiscard]] constexpr auto flag() const noexcept -> WindowFlag
				{
					return flag_;
				}

				[[nodiscard]] constexpr auto internal_flag() const noexcept -> WindowInternalFlag
				{
					return internal_flag_;
				}

				#if defined(GAL_PROMETHEUS_COMPILER_MSVC)

				template<typename T>
				struct workaround
				{
					T flag;

					constexpr explicit (false) workaround(const T flag) noexcept
						: flag{flag} {}
				};

				template<workaround<WindowFlag> Flag>
				[[nodiscard]] constexpr auto is() const noexcept -> bool
				{
					return std::to_underlying(flag_) & Flag.flag;
				}

				template<workaround<WindowInternalFlag> Flag>
				[[nodiscard]] constexpr auto is() const noexcept -> bool
				{
					return std::to_underlying(internal_flag_) & Flag.flag;
				}

				#else

				template<WindowFlag Flag>
				[[nodiscard]] constexpr auto is() const noexcept -> bool
				{
					return std::to_underlying(flag_) & Flag;
				}

				template<WindowInternalFlag Flag>
				[[nodiscard]] constexpr auto is() const noexcept -> bool
				{
					return std::to_underlying(internal_flag_) & Flag;
				}

				#endif
			};

			static_assert(sizeof(Flag) == sizeof(std::uint32_t));

			// <= 0
			constexpr static auto auto_size = value_type{-.999999f};

		private:
			class IdMaker;
			class Drawer;

			struct canvas_type
			{
				point_type cursor_start_line;
				point_type cursor_current_line;
				point_type cursor_previous_line;

				value_type height_current_line;
				value_type height_previous_line;

				std::vector<value_type> item_width;
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
			Flag flag_;
			Window* root_;

			point_type point_;
			// size_full / collapsed titlebar
			extent_type size_;
			extent_type size_full_;
			// size of contents, extents reach by the drawing cursor
			extent_type size_of_content_;

			value_type default_item_width_;

			value_type scroll_y_;
			value_type scroll_next_y_;
			bool scroll_y_visible_;

			bool visible_;
			// Set when collapsing window to become only titlebar
			bool collapsed_;
			// visible and not collapsed
			bool skip_item_;

			// Set to true when any widget access the current window
			bool accessed_;

			bool auto_fit_only_grows_;
			// 2 boolean, avoid padding
			std::int16_t auto_fit_frames_;

			frame_count_type last_drawn_frame_;

			std::vector<Window*> child_stack_;
			std::vector<widget_id_type> id_stack_;
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
			 * @param root the root of the window (parent window), if the current window is not a child window,
			 * this parameter must be a null pointer (while the root of the window is itself)
			 */
			Window(
				std::string_view name,
				Flag flag,
				const point_type& point,
				const extent_type& size,
				Window* root
			) noexcept;

			/**
			 * @brief If a window has already been created, each "re-creation" only resets its flag (if necessary)
			 * @param flag window flag
			 */
			auto reset(Flag flag) noexcept -> void;


			// -----------------------------------
			// DRAW BEGIN

			/**
			 * @brief Creating a canvas for the window
			 * @param context
			 * @param fill_alpha 
			 * @param parent If the current window is a child window, then @c parent is its parent, otherwise this parameter must be a null pointer
			 * @return Whether the window is visible (not closed)
			 * @note Widgets can be drawn in the window if and only if its canvas has been created
			 */
			[[nodiscard]] auto begin_draw(
				Context& context,
				value_type fill_alpha,
				Window* parent
			) noexcept -> bool;

			// -----------------------------------
			// WIDGETS

			// todo

			// -----------------------------------
			// DRAW END

			auto end_draw(Context& context) noexcept -> void;

			// -----------------------------------
			// RENDER

			auto render(Context& context) const noexcept -> void;

			// -----------------------------------
			// STATES

			[[nodiscard]] auto name() const noexcept -> std::string_view;

			[[nodiscard]] auto flag() const noexcept -> Flag;

			[[nodiscard]] auto position() const noexcept -> point_type;

			[[nodiscard]] auto width() const noexcept -> value_type;

			[[nodiscard]] auto height() const noexcept -> value_type;

			[[nodiscard]] auto size() const noexcept -> extent_type;

			[[nodiscard]] auto rect() const noexcept -> rect_type;

			[[nodiscard]] auto visible() const noexcept -> bool;

			[[nodiscard]] auto collapsed() const noexcept -> bool;

			[[nodiscard]] auto root() const noexcept -> const Window&;

			/**
			 * @brief Test if mouse cursor is hovering given rect
			 * @note @c rect is clipped by current clip rect setting 
			 */
			[[nodiscard]] auto hovered(const Context& context, const rect_type& rect) const noexcept -> bool;

			// -----------------------------------
			// 

			auto show() noexcept -> void;
			auto hide() noexcept -> void;

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
		};
	}
}
