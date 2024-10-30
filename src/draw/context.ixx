// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

#if not defined(GAL_PROMETHEUS_DRAW_LIST_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_LIST_DEBUG 0
#endif
#endif

export module gal.prometheus:draw.context;

import std;

import :draw.def;
import :draw.font;
import :draw.theme;
import :draw.window;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <string>
#include <vector>
#include <optional>

#include <prometheus/macro.hpp>

#if not defined(GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG)
#if defined(DEBUG) or defined(_DEBUG)
#define GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG 1
#else
#define GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG 0
#endif
#endif

#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
#include <source_location>
#endif

#include <draw/def.ixx>
#include <draw/font.ixx>
#include <draw/theme.ixx>
#include <draw/window.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(draw)
{
	class Context;

	class Mouse
	{
		friend Context;

	public:
		using rect_type = Theme::rect_type;
		using point_type = Theme::point_type;
		using extent_type = Theme::extent_type;
		using value_type = Theme::value_type;

		using time_type = float;

	private:
		// ==================================
		// static
		// ==================================

		time_type double_click_interval_threshold_;
		value_type double_click_distance_threshold_;

		// ==================================
		// dynamic
		// ==================================

		point_type position_current_;
		point_type position_previous_;
		point_type position_clicked_;

		bool down_;
		bool clicked_;
		bool double_clicked_;
		time_type down_duration_;
		time_type click_duration_;

		Mouse(const time_type double_click_interval_threshold, const value_type double_click_distance_threshold) noexcept
			: double_click_interval_threshold_{double_click_interval_threshold},
			  double_click_distance_threshold_{double_click_distance_threshold},
			  position_current_{std::numeric_limits<value_type>::min(), std::numeric_limits<value_type>::min()},
			  position_previous_{std::numeric_limits<value_type>::min(), std::numeric_limits<value_type>::min()},
			  position_clicked_{std::numeric_limits<value_type>::min(), std::numeric_limits<value_type>::min()},
			  down_{false},
			  clicked_{false},
			  double_clicked_{false},
			  down_duration_{0},
			  click_duration_{0} {}

	public:
		// ---------------------------------------------

		[[nodiscard]] auto position() const noexcept -> point_type
		{
			return position_current_;
		}

		[[nodiscard]] auto position_delta() const noexcept -> extent_type
		{
			return extent_type{position_current_ - position_previous_};
		}

		[[nodiscard]] auto down() const noexcept -> bool
		{
			return down_;
		}

		[[nodiscard]] auto clicked() const noexcept -> bool
		{
			return clicked_;
		}

		[[nodiscard]] auto double_clicked() const noexcept -> bool
		{
			return double_clicked_;
		}

	private:
		// ---------------------------------------------

		auto move(const point_type position) noexcept -> void
		{
			position_current_ = position;
		}

		// ---------------------------------------------

		auto tick(const time_type tick_time) noexcept -> void
		{
			position_previous_ = position_current_;

			clicked_ = false;
			double_clicked_ = false;
			if (down_)
			{
				if (down_duration_ > 0)
				{
					down_duration_ += tick_time;
				}
				else
				{
					down_duration_ = 0;
					clicked_ = true;
				}
			}
			else
			{
				down_duration_ = std::numeric_limits<time_type>::min();
			}
			if (clicked_)
			{
				if (0 - click_duration_ < double_click_interval_threshold_)
				{
					if (position_current_.distance(position_clicked_) < double_click_distance_threshold_)
					{
						double_clicked_ = true;
					}
					click_duration_ = std::numeric_limits<time_type>::min();
				}
				else
				{
					click_duration_ = 0;
					position_clicked_ = position_current_;
				}
			}
		}
	};

	class Context
	{
	public:
		using tooltip_type = std::string;

		using rect_type = Theme::rect_type;
		using point_type = Theme::point_type;
		using extent_type = Theme::extent_type;
		using value_type = Theme::value_type;

		using id_type = Window::id_type;

		template<typename T>
		using list_type = std::vector<T>;

	private:
		DrawListSharedData draw_list_shared_data_;
		DrawListSharedData* current_draw_list_shared_data_;

		Font default_font_;
		Font* current_font_;

		Theme theme_;
		Theme* current_theme_;

		tooltip_type tooltip_;

		Mouse mouse_;

		list_type<Window> windows_;
		Window* window_hovered_;
		Window* window_current_;
		id_type widget_id_hovered_;
		id_type widget_id_activated_;

		list_type<std::reference_wrapper<DrawList>> draw_lists_;

		[[nodiscard]] auto window_index(const Window& window) const noexcept -> list_type<Window>::size_type;

		Context() noexcept;

	public:
		// ---------------------------------------------
		// SINGLETON

		[[nodiscard]] static auto instance() noexcept -> Context&;

		// ---------------------------------------------
		// DRAW LIST SHARED DATA

		[[nodiscard]] auto draw_list_shared_data() const noexcept -> const DrawListSharedData&;

		// ---------------------------------------------
		// FONT

		[[nodiscard]] auto load_default_font(const FontOption& option) noexcept -> Font::texture_type;

		[[nodiscard]] auto font() const noexcept -> const Font&;

		// ---------------------------------------------
		// 

		[[nodiscard]] auto theme() const noexcept -> const Theme&;

		[[nodiscard]] auto mouse() const noexcept -> const Mouse&;

		[[nodiscard]] auto tooltip() const noexcept -> const tooltip_type&;

		// ---------------------------------------------

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

		// ---------------------------------------------

		[[nodiscard]] auto window(std::string_view name) const noexcept -> std::optional<std::reference_wrapper<const Window>>;

		// ---------------------------------------------

		auto new_frame() noexcept -> void;

		auto render() noexcept -> void;

		// ---------------------------------------------

		auto test_set_window(Window& window) noexcept -> void
		{
			window_current_ = &window;
		}
	};
}
