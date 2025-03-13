// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if not defined(GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG 0
#endif
#endif

#include <draw/def.hpp>
#include <draw/font.hpp>
#include <draw/shared_data.hpp>
#include <draw/theme.hpp>
#include <draw/mouse.hpp>
#include <draw/window.hpp>

namespace gal::prometheus::draw
{
	class Context final
	{
	public:
		template<typename T>
		using container_type = DrawListDef::container_type<T>;

		using rect_type = DrawListDef::rect_type;
		using point_type = DrawListDef::point_type;
		using extent_type = DrawListDef::extent_type;

		using font_type = std::shared_ptr<Font>;

		constexpr static std::size_t draw_list_shared_data_stack_size = 8;
		constexpr static std::size_t font_stack_size = 8;
		constexpr static std::size_t theme_stack_size = 8;

		using id_type = Window::id_type;
		constexpr static auto invalid_id = Window::invalid_id;

		using windows_type = container_type<Window>;
		using index_type = windows_type::difference_type;

	private:
		using stack_pointer_type = std::uint8_t;
		constexpr static auto stack_use_default = std::numeric_limits<stack_pointer_type>::max();

		// ----------------------------------------------------------------------
		// DrawListSharedData + Font + Theme

		DrawListSharedData draw_list_shared_data_default_;
		font_type font_default_;
		Theme theme_default_;

		DrawListSharedData* draw_list_shared_data_stack_[draw_list_shared_data_stack_size];
		font_type* font_stack_[font_stack_size];
		Theme* theme_stack_[theme_stack_size];

		stack_pointer_type draw_list_shared_data_current_;
		stack_pointer_type font_current_;
		stack_pointer_type theme_current_;
		stack_pointer_type pad_;

		// ----------------------------------------------------------------------
		// TOOLTIP

		std::string tooltip_;

		// ----------------------------------------------------------------------
		// MOUSE

		Mouse mouse_;

		// ----------------------------------------------------------------------
		// WINDOW

		windows_type windows_;
		Window* window_current_;
		Window* window_hovered_;

		id_type widget_id_hovered_;
		id_type widget_id_activated_;

		container_type<std::reference_wrapper<DrawList>> window_draw_lists_;

		[[nodiscard]] auto find_window(const Window& window) const noexcept -> index_type;

		Context() noexcept;

	public:
		// ---------------------------------------------
		// SINGLETON

		[[nodiscard]] static auto instance() noexcept -> Context&;

		// ---------------------------------------------
		// DRAW LIST SHARED DATA

		[[nodiscard]] auto draw_list_shared_data() const noexcept -> const DrawListSharedData&;

		auto push_draw_list_shared_data(DrawListSharedData& shared_data) noexcept -> void;

		auto pop_draw_list_shared_data() noexcept -> void;

		// ---------------------------------------------
		// FONT

		auto set_default_font(font_type font) noexcept -> void;

		[[nodiscard]] auto font() const noexcept -> const Font&;

		auto push_font(font_type& font) noexcept -> void;

		auto pop_font() noexcept -> void;

		// ---------------------------------------------
		// THEME

		[[nodiscard]] auto theme() const noexcept -> const Theme&;

		auto push_theme(Theme& theme) noexcept -> void;

		auto pop_theme() noexcept -> void;

		// ---------------------------------------------
		// TOOLTIP

		[[nodiscard]] auto tooltip() const noexcept -> std::string_view;

		// ---------------------------------------------
		// MOUSE

		[[nodiscard]] auto mouse() const noexcept -> const Mouse&;

		// ---------------------------------------------
		// WINDOW & WIDGET

		[[nodiscard]] auto is_widget_hovered(id_type id) const noexcept -> bool;

		[[nodiscard]] auto is_widget_activated(id_type id) const noexcept -> bool;

		auto invalidate_widget_hovered(
			#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
			const std::string& reason,
			const std::source_location& location = std::source_location::current()
			#endif
		) noexcept -> void;

		auto invalidate_widget_activated(
			#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
			const std::string& reason,
			const std::source_location& location = std::source_location::current()
			#endif
		) noexcept -> void;

		struct widget_status_type
		{
			bool hovered;
			bool pressed;
			bool keeping;
		};

		[[nodiscard]] auto test_widget_status(
			id_type id,
			const rect_type& widget_rect,
			bool repeat
			#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
			,
			const std::string& reason
			#endif
		) noexcept -> widget_status_type;

		[[nodiscard]] auto find_window(std::string_view name) const noexcept -> std::optional<std::reference_wrapper<const Window>>;

		// ---------------------------------------------
		// RENDER

		auto new_frame() noexcept -> void;

		auto render() noexcept -> void;

		// ---------------------------------------------
		// for test only

		auto test_set_window(Window& window) noexcept -> void
		{
			window_current_ = &window;
		}
	};
}
