// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <draw/context.hpp>

namespace {}

namespace gal::prometheus::draw
{
	auto Context::find_window(const Window& window) const noexcept -> index_type
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
		// default_draw_list_shared_data_{},
		// font_default_{},
		theme_default_{Theme::default_theme()},
		draw_list_shared_data_stack_{},
		font_stack_{},
		theme_stack_{},
		draw_list_shared_data_current_{stack_use_default},
		font_current_{stack_use_default},
		theme_current_{stack_use_default},
		pad_{0},
		mouse_{.3f, 36},
		window_current_{nullptr},
		window_hovered_{nullptr},
		widget_id_hovered_{invalid_id},
		widget_id_activated_{invalid_id}
	{
		std::ignore = pad_;
	}

	auto Context::instance() noexcept -> Context&
	{
		static Context context{};
		return context;
	}

	auto Context::draw_list_shared_data() const noexcept -> const DrawListSharedData&
	{
		if (draw_list_shared_data_current_ == stack_use_default)
		{
			return draw_list_shared_data_default_;
		}

		const auto* p = draw_list_shared_data_stack_[draw_list_shared_data_current_];
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(p != nullptr);

		return *p;
	}

	auto Context::push_draw_list_shared_data(DrawListSharedData& shared_data) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(draw_list_shared_data_current_ == stack_use_default or draw_list_shared_data_current_ < draw_list_shared_data_stack_size);

		draw_list_shared_data_current_ += 1;
		draw_list_shared_data_stack_[draw_list_shared_data_current_] = std::addressof(shared_data);
	}

	auto Context::pop_draw_list_shared_data() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(draw_list_shared_data_current_ != stack_use_default, "Unable to popup the default DrawListSharedData!");

		if (draw_list_shared_data_current_ == 0)
		{
			draw_list_shared_data_current_ = stack_use_default;
		}
		else
		{
			draw_list_shared_data_current_ -= 1;
		}
	}

	auto Context::set_default_font(font_type font) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(font != nullptr);

		font_default_ = std::move(font);
	}

	auto Context::font() const noexcept -> const Font&
	{
		if (font_current_ == stack_use_default)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(font_default_ != nullptr);

			return *font_default_;
		}

		const auto* p = font_stack_[font_current_];
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(p != nullptr);

		return **p;
	}

	auto Context::push_font(font_type& font) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(font != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(font_current_ == stack_use_default or font_current_ < font_stack_size);

		font_current_ += 1;
		font_stack_[font_current_] = std::addressof(font);
	}

	auto Context::pop_font() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(font_current_ != stack_use_default, "Unable to popup the default Font!");

		if (font_current_ == 0)
		{
			font_current_ = stack_use_default;
		}
		else
		{
			font_current_ -= 1;
		}
	}

	auto Context::theme() const noexcept -> const Theme&
	{
		if (theme_current_ == stack_use_default)
		{
			return theme_default_;
		}

		const auto* p = theme_stack_[theme_current_];
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(p != nullptr);

		return *p;
	}

	auto Context::push_theme(Theme& theme) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(theme_current_== stack_use_default or theme_current_ < theme_stack_size);

		theme_current_ += 1;
		theme_stack_[theme_current_] = std::addressof(theme);
	}

	auto Context::pop_theme() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(theme_current_ != stack_use_default, "Unable to popup the default Theme!");

		if (theme_current_ == 0)
		{
			theme_current_ = stack_use_default;
		}
		else
		{
			theme_current_ -= 1;
		}
	}

	auto Context::tooltip() const noexcept -> std::string_view
	{
		return tooltip_;
	}

	auto Context::mouse() const noexcept -> const Mouse&
	{
		return mouse_;
	}

	auto Context::is_widget_hovered(const id_type id) const noexcept -> bool
	{
		return widget_id_hovered_ == id;
	}

	auto Context::is_widget_activated(const id_type id) const noexcept -> bool
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

	auto Context::find_window(const std::string_view name) const noexcept -> std::optional<std::reference_wrapper<const Window>>
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
				const auto index = find_window(*window_hovered_);
				const auto it = windows_.begin() + index;

				auto&& window = std::move(windows_[index]);
				windows_.erase(it);
				windows_.emplace_back(std::move(window));
			}
		}
	}

	auto Context::render() noexcept -> void
	{
		window_draw_lists_.clear();
		std::ranges::transform(
			windows_,
			std::back_inserter(window_draw_lists_),
			[](auto& window) noexcept -> auto
			{
				return std::ref(window.render());
			}
		);

		if (window_current_ != nullptr and tooltip_[0] != '\0')
		{
			const auto index = find_window(*window_current_);

			auto& draw_list = window_draw_lists_[index].get();
			const auto& theme = this->theme();

			// todo
			const rect_type tooltip_rect{mouse_.position(), extent_type{100, 100}};
			draw_list.rect_filled(tooltip_rect, theme.color<ThemeCategory::TOOLTIP_BACKGROUND>());
			draw_list.text(theme.font_size, mouse_.position(), theme.color<ThemeCategory::TOOLTIP_TEXT>(), tooltip_);
		}
	}
}
