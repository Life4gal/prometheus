// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.window;

import std;

import :functional;
import :primitive;

export import :draw.def;
export import :draw.draw_list;
export import :draw.theme;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <string>
#include <vector>
#include <list>

#include <prometheus/macro.hpp>
#include <functional/functional.ixx>
#include <primitive/primitive.ixx>

#include <draw/def.ixx>
#include <draw/draw_list.ixx>
#include <draw/theme.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
{
	class Window
	{
	public:
		using name_type = Theme::name_type;
		using name_view_type = std::basic_string_view<name_type::value_type>;

		using rect_type = Theme::rect_type;
		using point_type = Theme::point_type;
		using extent_type = Theme::extent_type;
		using value_type = Theme::value_type;

		using color_type = Theme::color_type;

		using id_type = functional::hash_result_type;
		constexpr static auto invalid_id = std::numeric_limits<id_type>::max();

		template<typename T>
		using contiguous_list_type = std::vector<T>;
		template<typename T>
		using list_type = std::list<T>;

	private:
		struct canvas_type
		{
			point_type cursor_start_line{0};
			point_type cursor_current_line{0};
			point_type cursor_previous_line{0};
			value_type height_current_line{0};
			value_type height_previous_line{0};
			contiguous_list_type<value_type> item_width{};
		};

		name_type name_;
		id_type id_;
		WindowFlag flag_;
		rect_type rect_;

		DrawList draw_list_;

		canvas_type canvas_;

		value_type default_item_width_;

		bool visible_;
		bool collapse_;

		// -----------------------------------
		// ID

		[[nodiscard]] auto get_id(name_view_type name) noexcept -> id_type;

		// -----------------------------------
		// CANVAS

		[[nodiscard]] auto title_bar_rect() const noexcept -> rect_type;

		[[nodiscard]] auto resize_grip_rect() const noexcept -> rect_type;

		[[nodiscard]] auto canvas_rect() const noexcept -> rect_type;

		auto make_1_test_title_bar() noexcept -> void;
		[[nodiscard]] auto make_2_test_resize_grip() noexcept -> color_type;
		auto make_3_init_canvas() noexcept -> void;
		auto make_4_draw_canvas_background() noexcept -> void;
		auto make_5_draw_title_bar() noexcept -> void;
		auto make_6_draw_resize_grip(color_type color) noexcept -> void;

		// -----------------------------------
		// CANVAS CONTEXT

		auto adjust_item_size(extent_type size) noexcept -> void;

		// -----------------------------------
		// INITIALIZE

		Window(name_view_type name, WindowFlag flag, rect_type rect) noexcept;

		auto init() noexcept -> void;

		// -----------------------------------

	public:
		// ---------------------------------------------

		static auto make(name_view_type name, WindowFlag flag, rect_type rect) noexcept -> Window;

		// ---------------------------------------------

		[[nodiscard]] auto name() const noexcept -> const name_type&
		{
			return name_;
		}

		[[nodiscard]] auto rect() const noexcept -> const rect_type&
		{
			return rect_;
		}

		// ---------------------------------------------
		// todo: TEST

		Window(name_view_type name, rect_type rect) noexcept
			: Window{name, WindowFlag::NONE, rect} {}

		auto test_init() noexcept -> void
		{
			init();
		}

		[[nodiscard]] auto draw_list() noexcept -> DrawList&
		{
			return draw_list_;
		}
		// ---------------------------------------------

		// ---------------------------------------------

		[[nodiscard]] auto hovered(point_type mouse) const noexcept -> bool;

		// ---------------------------------------------

		auto draw_text(std::string_view text);

		// ---------------------------------------------

		auto render() noexcept -> DrawList&;
	};
}
