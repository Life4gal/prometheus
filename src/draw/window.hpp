// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <functional/hash.hpp>

#include <draw/def.hpp>
#include <draw/flag.hpp>
#include <draw/theme.hpp>
#include <draw/draw_list.hpp>

namespace gal::prometheus::draw
{
	class [[nodiscard]] Window final
	{
	public:
		template<typename T>
		using container_type = DrawListDef::container_type<T>;

		using rect_type = DrawListDef::rect_type;
		using point_type = DrawListDef::point_type;
		using extent_type = DrawListDef::extent_type;

		using value_type = point_type::value_type;

		using color_type = DrawListDef::color_type;

		using id_type = functional::hash_result_type;
		constexpr static auto invalid_id = std::numeric_limits<id_type>::max();

	private:
		struct canvas_type
		{
			point_type cursor_start_line{0};
			point_type cursor_current_line{0};
			point_type cursor_previous_line{0};
			value_type height_current_line{0};
			value_type height_previous_line{0};
			container_type<value_type> item_width{};
		};

		std::string name_;
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

		[[nodiscard]] auto get_id(std::string_view name) const noexcept -> id_type;

		// -----------------------------------
		// CANVAS

		[[nodiscard]] auto rect_of_title_bar() const noexcept -> rect_type;

		[[nodiscard]] auto rect_of_resize_grip() const noexcept -> rect_type;

		[[nodiscard]] auto rect_of_canvas() const noexcept -> rect_type;

		auto make_canvas() noexcept -> void;

		// -----------------------------------
		// CANVAS CONTEXT

		auto adjust_item_size(const extent_type& size) noexcept -> void;

		// -----------------------------------
		// INITIALIZE

		Window(std::string_view name, WindowFlag flag, const rect_type& rect) noexcept;

	public:
		static auto make(std::string_view name, WindowFlag flag, const rect_type& rect) noexcept -> Window;

		// ---------------------------------------------

		[[nodiscard]] auto name() const noexcept -> std::string_view;

		[[nodiscard]] auto rect() const noexcept -> const rect_type&;

		// ---------------------------------------------

		[[nodiscard]] auto hovered(point_type mouse) const noexcept -> bool;

		// ---------------------------------------------

		auto draw_text(std::string_view text) noexcept -> void;

		// ---------------------------------------------

		auto render() noexcept -> DrawList&;

		// ---------------------------------------------
		// for test only

		Window(const std::string_view name, const rect_type& rect) noexcept
			: Window{name, WindowFlag::NONE, rect} {}

		auto test_init() noexcept -> void
		{
			make_canvas();
		}

		[[nodiscard]] auto test_draw_list() noexcept -> DrawList&
		{
			return draw_list_;
		}
	};
}
