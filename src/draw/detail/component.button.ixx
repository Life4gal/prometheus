// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:component.button;

import std;

import gal.prometheus.primitive;
import gal.prometheus.functional;

import :style;
import :element;
import :component.component;

#else
#pragma once

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>
#include <functional/functional.ixx>

#include <draw/style.ixx>

#include <draw/element.ixx>
#include <draw/detail/component.ixx>

#endif

namespace gal::prometheus::draw
{
	namespace detail::component
	{
		enum class ButtonOption
		{
			RADIO,
		};

		template<typename T>
		concept button_option_t = std::is_same_v<T, ButtonOption>;

		struct button_options
		{
			options<ButtonOption::RADIO> radio{};
		};
	}

	namespace component
	{
		constexpr auto button = detail::component::button_options{};
	}

	namespace detail
	{
		namespace component
		{
			template<ButtonOption>
			class Button;

			template<>
			class Button<ButtonOption::RADIO> final : public Component
			{
			public:
				using entries_type = std::span<std::string>;
				using size_type = entries_type::size_type;
				using callback_type = std::function<void()>;

			private:
				entries_type entries_;

				size_type hovered_;
				size_type selected_;

				callback_type on_clicked_;

				std::vector<rect_type> entries_rect_;
				rect_type self_rect_;

				[[nodiscard]] static auto make_entry_element(const std::string& entry, const bool selected, const bool hovered) noexcept -> element_type
				{
					auto prefix = make(draw::element::text)(selected ? "[*]" : "[ ]");
					auto content = make(draw::element::text)(entry);
					if (hovered)
					{
						content |= draw::element::decorator.bold;
					}

					return make(draw::element::box.horizontal)(
						       std::move(prefix),
						       std::move(content)
					       ) | draw::element::border;
				}

			public:
				explicit Button(const entries_type entries, callback_type on_click) noexcept
					: entries_{entries},
					  hovered_{0},
					  selected_{0},
					  on_clicked_{std::move(on_click)}
				{
					entries_rect_.resize(entries_.size());
				}

				Button(const Button&) noexcept = default;
				auto operator=(const Button&) noexcept -> Button& = default;
				Button(Button&&) noexcept = default;
				auto operator=(Button&&) noexcept -> Button& = default;
				~Button() noexcept override = default;

				// =================================

				auto render() noexcept -> element_type override
				{
					elements_type es{};
					es.reserve(entries_.size());

					std::ranges::for_each(
						std::views::zip(entries_, entries_rect_),
						[
							this,
							&es,
							&hovered_entry = entries_[hovered_],
							&selected_entry = entries_[selected_]
						](const std::tuple<const std::string&, rect_type&>& pack) noexcept -> void
						{
							auto& [entry, rect] = pack;
							es.emplace_back(
								make_entry_element(entry, entry == selected_entry, entry == hovered_entry) |
								make(draw::element::guardian.rect)(rect)
							);
						}
					);

					return make(draw::element::box.vertical)(std::move(es)) | make(draw::element::guardian.rect)(self_rect_);
				}

				// =================================

				auto on_event(const event_type event) noexcept -> bool override
				{
					if (not focused() or not try_capture_mouse(event))
					{
						return false;
					}

					if (const auto type = event.type();
						type == io::DeviceActionType::MOUSE_MOV)
					{
						const auto [x, y] = event.value<io::DeviceActionType::MOUSE_MOV>();
						if (
							const auto it = std::ranges::find_if(
								entries_rect_,
								[p = rect_type::point_type{x, y}](const auto& rect) noexcept -> bool
								{
									return rect.includes(p);
								}
							);
							it != std::ranges::end(entries_rect_))
						{
							hovered_ = std::ranges::distance(std::ranges::begin(entries_rect_), it);
							return true;
						}

						return false;
					}
					else if (type == io::DeviceActionType::MOUSE_BUTTON)
					{
						if (const auto [button, state] = event.value<io::DeviceActionType::MOUSE_BUTTON>();
							button == io::MouseButton::LEFT and state == io::MouseButtonStatus::PRESS)
						{
							selected_ = hovered_;
							return true;
						}

						return false;
					}
					else if (type == io::DeviceActionType::MOUSE_WHEEL)
					{
						if (const auto [x, y] = event.value<io::DeviceActionType::MOUSE_WHEEL>();
							y > 0)
						{
							hovered_ = (hovered_ + 1) % entries_.size();
						}
						else
						{
							hovered_ = hovered_ == 0 ? entries_.size() - 1 : hovered_ - 1;
						}

						return true;
					}

					// todo
					return false;
				}

				// =================================

				[[nodiscard]] auto focusable() const noexcept -> bool override
				{
					return not entries_.empty();
				}
			};

			struct button_radio_builder
			{
				[[nodiscard]] auto operator()(
					Button<ButtonOption::RADIO>::entries_type entries,
					Button<ButtonOption::RADIO>::callback_type on_click = [] {}
				) const noexcept -> auto
				{
					return make_component<Button<ButtonOption::RADIO>>(entries, std::move(on_click));
				}
			};
		}

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<component::button_option_t auto... Os>
		struct maker<Os...>
		{
			template<component::ButtonOption Option>
			[[nodiscard]] auto operator()(options<Option>) const noexcept -> auto
			{
				if constexpr (Option == component::ButtonOption::RADIO)
				{
					return component::button_radio_builder{};
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			template<component::ButtonOption Option, typename... Args>
			[[nodiscard]] auto operator()(options<Option> option, Args&&... args) const noexcept -> auto
			{
				return this->operator()(option)(std::forward<Args>(args)...);
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
