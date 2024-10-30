// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:draw.context.impl;

import std;

import :functional;
#if GAL_PROMETHEUS_COMPILER_DEBUG
import :platform;
#endif

import :draw.def;
import :draw.font;
import :draw.theme;
import :draw.window;
import :draw.context;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#include <array>
#include <span>
#include <ranges>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <functional/functional.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <draw/def.ixx>
#include <draw/font.ixx>
#include <draw/theme.ixx>
#include <draw/window.ixx>
#include <draw/context.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT_IMPL(draw)
{
	auto Context::window_index(const Window& window) const noexcept -> list_type<Window>::size_type
	{
		const auto it = std::ranges::find(
			windows_ | std::views::reverse,
			std::addressof(window),
			[](const auto& w) noexcept -> const auto* {
				return std::addressof(w);
			}
		);

		const auto it_base = it.base();
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_base != std::ranges::end(windows_));
		return std::ranges::distance(std::ranges::begin(windows_), it_base);
	}

	Context::Context() noexcept
		:
		draw_list_shared_data_{},
		current_draw_list_shared_data_{std::addressof(draw_list_shared_data_)},
		default_font_{},
		current_font_{std::addressof(default_font_)},
		theme_{Theme::default_theme()},
		current_theme_{std::addressof(theme_)},
		tooltip_{},
		mouse_{.3f, 36},
		windows_{},
		window_hovered_{nullptr},
		window_current_{nullptr},
		widget_id_hovered_{Window::invalid_id},
		widget_id_activated_{Window::invalid_id}
	{
		tooltip_.resize(1024);
		windows_.reserve(8);
	}

	auto Context::instance() noexcept -> Context&
	{
		static Context context{};
		return context;
	}

	auto Context::draw_list_shared_data() const noexcept -> const DrawListSharedData&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_draw_list_shared_data_ != nullptr);

		return *current_draw_list_shared_data_;
	}

	auto Context::load_default_font(const FontOption& option) noexcept -> Font::texture_type
	{
		return default_font_.load(option);
	}

	auto Context::font() const noexcept -> const Font&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_font_ != nullptr);

		return *current_font_;
	}

	auto Context::theme() const noexcept -> const Theme&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_theme_ != nullptr);

		return *current_theme_;
	}

	auto Context::mouse() const noexcept -> const Mouse&
	{
		return mouse_;
	}

	auto Context::tooltip() const noexcept -> const tooltip_type&
	{
		return tooltip_;
	}

	auto Context::is_widget_hovered(const id_type id) const noexcept -> bool
	{
		return widget_id_hovered_ == id;
	}

	auto Context::is_widget_activated(id_type id) const noexcept -> bool
	{
		return widget_id_activated_ == id;
	}

	auto Context::invalidate_widget_hovered(
		#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
		const std::string& reason,
		const std::source_location& location
		#endif
	) noexcept -> void
	{
		(void)reason;
		(void)location;

		widget_id_hovered_ = Window::invalid_id;
	}

	auto Context::invalidate_widget_activated(
		#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
		const std::string& reason,
		const std::source_location& location
		#endif
	) noexcept -> void
	{
		(void)reason;
		(void)location;

		widget_id_activated_ = Window::invalid_id;
	}

	auto Context::test_widget_status(
		const id_type id,
		const rect_type& widget_rect,
		const bool repeat
		#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
		,
		const std::string& reason
		#endif
	) noexcept -> widget_status_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(window_current_ != nullptr);

		auto& window = *window_current_;
		const rect_type rect{widget_rect.point + window.rect().left_top(), widget_rect.extent};
		const auto hovered = window_hovered_ == std::addressof(window) and rect.includes(mouse_.position());

		widget_status_type widget_status{.hovered = hovered, .pressed = false, .keeping = false};
		if (hovered)
		{
			widget_id_hovered_ = id;
			if (mouse_.clicked())
			{
				widget_id_activated_ = id;
			}
			else if (repeat and widget_id_activated_ != Window::invalid_id)
			{
				widget_status.pressed = true;
			}
		}

		if (widget_id_activated_ == id)
		{
			if (mouse_.down())
			{
				widget_status.keeping = true;
			}
			else
			{
				if (hovered)
				{
					widget_status.pressed = true;
				}
				invalidate_widget_activated(
					#if GAL_PROMETHEUS_DRAW_CONTEXT_DEBUG
					std::format("Release mouse on widget #{}.({})", id, reason)
					#endif
				);
			}
		}

		return widget_status;
	}

	auto Context::window(const std::string_view name) const noexcept -> std::optional<std::reference_wrapper<const Window>>
	{
		const auto it = std::ranges::find_if(
			windows_,
			[name](const auto& window) noexcept -> bool
			{
				return name == window.name();
			}
		);

		if (it == std::ranges::end(windows_))
		{
			return std::nullopt;
		}

		return std::make_optional(std::cref(*it));
	}

	auto Context::new_frame() noexcept -> void
	{
		tooltip_[0] = '\0';

		// todo
		mouse_.tick(1 / 60.f);

		std::ranges::for_each(
			windows_ | std::views::reverse,
			[this](auto& window) noexcept -> void
			{
				if (window.hovered(mouse_.position()))
				{
					window_hovered_ = std::addressof(window);
				}
			}
		);

		if (mouse_.clicked_)
		{
			if (window_hovered_ != nullptr)
			{
				const auto index = window_index(*window_hovered_);
				const auto it = windows_.begin() + index;

				auto&& window = std::move(windows_[index]);
				windows_.erase(it);
				windows_.emplace_back(std::move(window));
			}
		}
	}

	auto Context::render() noexcept -> void
	{
		draw_lists_.clear();
		std::ranges::transform(
			windows_,
			std::back_inserter(draw_lists_),
			[](auto& window) noexcept -> auto
			{
				return std::ref(window.render());
			}
		);

		if (window_current_ != nullptr and tooltip_[0] != '\0')
		{
			const auto index = window_index(*window_current_);

			auto& draw_list = draw_lists_[index].get();

			// todo
			rect_type tooltip_rect{mouse_.position(), extent_type{100, 100}};
			draw_list.rect_filled(tooltip_rect, theme_.color<ThemeCategory::TOOLTIP_BACKGROUND>());
			draw_list.text(theme_.font_size, mouse_.position(), theme_.color<ThemeCategory::TOOLTIP_TEXT>(), tooltip_);
		}
	}
}
