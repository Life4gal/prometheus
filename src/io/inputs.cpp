// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <io/inputs.hpp>

#include <mutex>

#include <functional/functor.hpp>

namespace
{
	using namespace gal::prometheus;
	using namespace io;

	[[nodiscard]] auto current_time() noexcept -> time_point_type
	{
		return std::chrono::time_point_cast<duration_type>(time_point_type::clock::now());
	}
}

namespace gal::prometheus::io
{
	auto InputHandler::Mouse::reset(const time_point_type& time_point, const InputHandler& handler) noexcept -> void
	{
		std::ranges::for_each(
			states,
			[&](key_state_type& state) noexcept -> void
			{
				state.down_this_frame = 0;

				const auto view = state.press_records | std::views::reverse;
				const auto it = std::ranges::find_if(
					view,
					[&](const press_record_type& record) noexcept -> bool
					{
						if (const auto interval = record.time_point - time_point; interval > handler.mouse_double_click_interval_threshold_)
						{
							return true;
						}

						if (const auto distance = record.position.distance(position_current); distance > handler.mouse_double_click_distance_threshold_)
						{
							return true;
						}

						return false;
					}
				);
				state.press_records.erase(state.press_records.begin(), it.base());
			}
		);

		position_delta = {0, 0};
		wheel = {0, 0};
	}

	auto InputHandler::Mouse::is_up(const InputHandler& self, const MouseButton button) const noexcept -> bool
	{
		std::ignore = self;

		const auto index = static_cast<std::size_t>(button);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < states.size());

		const auto& state = states[index];

		return state.down_this_frame == 0;
	}

	auto InputHandler::Mouse::is_down(const InputHandler& self, const MouseButton button) const noexcept -> bool
	{
		std::ignore = self;

		const auto index = static_cast<std::size_t>(button);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < states.size());

		const auto& state = states[index];

		return state.down_this_frame != 0;
	}

	auto InputHandler::Mouse::is_clicked(const InputHandler& self, const MouseButton button, const bool repeat) const noexcept -> bool
	{
		std::ignore = self;

		const auto index = static_cast<std::size_t>(button);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < states.size());

		const auto& state = states[index];

		if (state.down_this_frame != 0)
		{
			return true;
		}

		if (repeat and state.down_time != 0)
		{
			const auto now = current_time();

			if (const auto interval = duration_type{now.time_since_epoch().count() - state.down_time};
				interval > self.mouse_repeat_click_delay_)
			{
				// todo: 1/60 second?
				constexpr auto tick_time{std::chrono::milliseconds{16}};
				const auto half = self.mouse_repeat_click_rate_ / 2;

				const auto v1 = (interval - self.mouse_repeat_click_delay_) % self.mouse_repeat_click_rate_;
				const auto v2 = (interval - tick_time) % self.mouse_repeat_click_rate_;

				return (v1 > half) != (v2 > half);
			}
		}

		return false;
	}

	auto InputHandler::Mouse::is_double_clicked(const InputHandler& self, const MouseButton button) const noexcept -> bool
	{
		const auto index = static_cast<std::size_t>(button);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < states.size());

		const auto& state = states[index];

		if (state.down_this_frame == 0)
		{
			return false;
		}

		const auto records_count = state.press_records.size();
		if (records_count < 2)
		{
			return false;
		}

		const auto& [c1_time_point, c1_position] = state.press_records[records_count - 2];
		const auto& [c2_time_point, c2_position] = state.press_records[records_count - 1];

		if (const auto distance = c2_position.distance(c1_position); distance > self.mouse_double_click_distance_threshold_)
		{
			return false;
		}

		if (const auto time = c2_time_point - c1_time_point; time > self.mouse_double_click_interval_threshold_)
		{
			return false;
		}

		return true;
	}

	auto InputHandler::Mouse::is_pressing(const InputHandler& self, const MouseButton button) const noexcept -> bool
	{
		std::ignore = self;

		const auto index = static_cast<std::size_t>(button);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < states.size());

		const auto& state = states[index];

		return state.down_time != 0;
	}

	auto InputHandler::Keyboard::reset(const time_point_type& time_point, const InputHandler& handler) noexcept -> void
	{
		std::ranges::for_each(
			states,
			[&](key_state_type& state) noexcept -> void
			{
				state.down_this_frame = 0;

				const auto view = state.press_records | std::views::reverse;
				const auto it = std::ranges::find_if(
					view,
					[&](const press_record_type& record) noexcept -> bool
					{
						if (const auto interval = record.time_point - time_point; interval > handler.mouse_double_click_interval_threshold_)
						{
							return true;
						}

						return false;
					}
				);
				state.press_records.erase(state.press_records.begin(), it.base());
			}
		);
	}

	auto InputHandler::Keyboard::is_up(const InputHandler& self, const KeyboardKeyCode code) const noexcept -> bool
	{
		std::ignore = self;

		const auto index = static_cast<std::size_t>(code);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < states.size());

		const auto& state = states[index];

		return state.down_this_frame == 0;
	}

	auto InputHandler::Keyboard::is_down(const InputHandler& self, const KeyboardKeyCode code) const noexcept -> bool
	{
		std::ignore = self;

		const auto index = static_cast<std::size_t>(code);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < states.size());

		const auto& state = states[index];

		return state.down_this_frame != 0;
	}

	auto InputHandler::Keyboard::is_pressing(const InputHandler& self, const KeyboardKeyCode code) const noexcept -> bool
	{
		std::ignore = self;

		const auto index = static_cast<std::size_t>(code);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < states.size());

		const auto& state = states[index];

		return state.down_time != 0;
	}

	auto InputHandler::Keyboard::is_combination_pressing(const InputHandler& self, const std::span<const KeyboardKeyCode> codes) const noexcept -> bool
	{
		const auto first = codes | std::views::take(codes.size() - 1);
		const auto last = codes.back();

		if (not std::ranges::all_of(
				first,
				[&](const KeyboardKeyCode code) noexcept -> bool
				{
					return is_pressing(self, code);
				}
			)
		)
		{
			return false;
		}

		return is_down(self, last);
	}

	auto InputHandler::Keyboard::is_combination_pressing(const InputHandler& self, std::initializer_list<const KeyboardKeyCode> codes) const noexcept -> bool
	{
		const std::span list{codes};

		return is_combination_pressing(self, list);
	}

	auto InputHandler::process_event(const input_event_type& event) noexcept -> void
	{
		// ============
		// MOUSE
		// ============
		const auto handle_mouse_move = [this](const MouseMoveEventData& data) noexcept -> void
		{
			mouse_.position_previous = mouse_.position_current;
			mouse_.position_current = data.position;
			mouse_.position_delta = (mouse_.position_current - mouse_.position_previous).to<extent_type>();
		};
		const auto handle_mouse_button = [this](const MouseButtonEventData& data) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.action != DeviceKeyAction::NONE);

			const auto button = static_cast<std::size_t>(data.button);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(button < mouse_.states.size());

			if (auto& [down_this_frame, down_time, press_records] = mouse_.states[button];
				data.action == DeviceKeyAction::DOWN)
			{
				const auto now = current_time();

				down_this_frame = 1;
				down_time = now.time_since_epoch().count();
				press_records.emplace_back(Mouse::press_record_type{.time_point = now, .position = mouse_.position_current});
			}
			else
			{
				down_this_frame = 0;
				down_time = 0;
			}
		};
		const auto handle_mouse_wheel = [this](const MouseWheelEventData& data) noexcept -> void
		{
			mouse_.wheel = data.value;
		};

		// ============
		// KEYBOARD
		// ============
		const auto handle_keyboard = [this](const KeyboardEventData& data) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.code != KeyboardKeyCode::NONE);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.action != DeviceKeyAction::NONE);

			const auto code = static_cast<std::size_t>(data.code);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(code < keyboard_.states.size());

			if (auto& [down_this_frame, down_time, press_records] = keyboard_.states[code];
				data.action == DeviceKeyAction::DOWN)
			{
				const auto now = current_time();

				down_this_frame = 1;
				down_time = now.time_since_epoch().count();
				press_records.emplace_back(Keyboard::press_record_type{.time_point = now});
			}
			else
			{
				down_this_frame = 0;
				down_time = 0;
			}
		};

		// ============
		// DISPLAY
		// ============
		const auto handle_display_move = [this](const DisplayMoveEventData& data) noexcept -> void
		{
			display_.position = data.position;
		};
		const auto handle_display_resize = [this](const DisplayResizeEventData& data) noexcept -> void
		{
			display_.size = data.size;
		};

		const auto visitor = functional::overloaded{
				handle_mouse_move,
				handle_mouse_button,
				handle_mouse_wheel,
				handle_keyboard,
				handle_display_move,
				handle_display_resize,
				[](const auto& data) noexcept -> void
				{
					std::ignore = data;
					GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();
				}
		};

		std::visit(visitor, event);
	}

	InputHandler::InputHandler() noexcept
		:
		// ReSharper disable once CppRedundantMemberInitializer
		mutex_{},
		// event_queue_pending_{},
		// event_queue_processing_{},
		mouse_
		{
				.position_current = {0, 0},
				.position_previous = {0, 0},
				.position_delta = {0, 0},
				.wheel = {0, 0},
				.states = {}
		},
		keyboard_
		{
				.states = {}
		},
		display_
		{
				.position = {0, 0},
				.size = {0, 0}
		},
		mouse_double_click_interval_threshold_{std::chrono::milliseconds{300}},
		mouse_double_click_distance_threshold_{6},
		mouse_repeat_click_delay_{std::chrono::milliseconds{275}},
		mouse_repeat_click_rate_{std::chrono::milliseconds{50}} {}

	auto InputHandler::begin_frame() noexcept -> void
	{
		{
			std::scoped_lock lock{mutex_};

			event_queue_processing_.append_range(std::move(event_queue_pending_));
			event_queue_pending_.clear();
		}

		std::ranges::for_each(
			event_queue_processing_,
			[this](const input_event_type& event) noexcept -> void
			{
				process_event(event);
			}
		);
		event_queue_processing_.clear();
	}

	auto InputHandler::end_frame() noexcept -> void
	{
		const auto now = current_time();

		// ============
		// MOUSE
		// ============
		mouse_.reset(now, *this);

		// ============
		// KEYBOARD
		// ============
		keyboard_.reset(now, *this);

		// ============
		// DISPLAY
		// ============
		// 
	}

	auto InputHandler::push_event(const input_event_type& event) noexcept -> void
	{
		std::scoped_lock lock{mutex_};
		event_queue_pending_.push_back(event);
	}

	auto InputHandler::mouse_proxy::position() const noexcept -> position_type
	{
		const auto& handler = self.get();
		const auto& mouse = handler.mouse_;

		return mouse.position_current;
	}

	auto InputHandler::mouse_proxy::position_delta() const noexcept -> extent_type
	{
		const auto& handler = self.get();
		const auto& mouse = handler.mouse_;

		return mouse.position_delta;
	}

	auto InputHandler::mouse_proxy::wheel() const noexcept -> extent_type
	{
		const auto& handler = self.get();
		const auto& mouse = handler.mouse_;

		return mouse.wheel;
	}

	auto InputHandler::mouse_proxy::is_up(const MouseButton button) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& mouse = handler.mouse_;

		return mouse.is_up(handler, button);
	}

	auto InputHandler::mouse_proxy::is_down(const MouseButton button) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& mouse = handler.mouse_;

		return mouse.is_down(handler, button);
	}

	auto InputHandler::mouse_proxy::is_click(const MouseButton button, const bool repeat) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& mouse = handler.mouse_;

		return mouse.is_clicked(handler, button, repeat);
	}

	auto InputHandler::mouse_proxy::is_double_click(const MouseButton button) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& mouse = handler.mouse_;

		return mouse.is_double_clicked(handler, button);
	}

	auto InputHandler::mouse_proxy::is_pressing(const MouseButton button) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& mouse = handler.mouse_;

		return mouse.is_pressing(handler, button);
	}

	auto InputHandler::mouse() const noexcept -> mouse_proxy
	{
		return {.self = *this};
	}

	auto InputHandler::keyboard_proxy::is_up(const KeyboardKeyCode code) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& keyboard = handler.keyboard_;

		return keyboard.is_up(handler, code);
	}

	auto InputHandler::keyboard_proxy::is_down(const KeyboardKeyCode code) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& keyboard = handler.keyboard_;

		return keyboard.is_down(handler, code);
	}

	auto InputHandler::keyboard_proxy::is_pressing(const KeyboardKeyCode code) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& keyboard = handler.keyboard_;

		return keyboard.is_pressing(handler, code);
	}

	auto InputHandler::keyboard_proxy::is_combination_pressing(const std::span<const KeyboardKeyCode> codes) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& keyboard = handler.keyboard_;

		return keyboard.is_combination_pressing(handler, codes);
	}

	auto InputHandler::keyboard_proxy::is_combination_pressing(const std::initializer_list<const KeyboardKeyCode> codes) const noexcept -> bool
	{
		const auto& handler = self.get();
		const auto& keyboard = handler.keyboard_;

		return keyboard.is_combination_pressing(handler, codes);
	}

	auto InputHandler::keyboard() const noexcept -> keyboard_proxy
	{
		return {.self = *this};
	}

	auto InputHandler::display_proxy::position() const noexcept -> position_type
	{
		const auto& handler = self.get();
		// ReSharper disable once CppUseStructuredBinding
		const auto& display = handler.display_;

		return display.position;
	}

	auto InputHandler::display_proxy::size() const noexcept -> extent_type
	{
		const auto& handler = self.get();
		// ReSharper disable once CppUseStructuredBinding
		const auto& display = handler.display_;

		return display.size;
	}

	auto InputHandler::display() const noexcept -> display_proxy
	{
		return {.self = *this};
	}

	auto InputHandler::config_proxy::set_mouse_double_click_interval_threshold(const duration_type interval) noexcept -> void
	{
		auto& handler = self.get();

		handler.mouse_double_click_interval_threshold_ = interval;
	}

	auto InputHandler::config_proxy::set_mouse_double_click_distance_threshold(const value_type distance) noexcept -> void
	{
		auto& handler = self.get();

		handler.mouse_double_click_distance_threshold_ = distance;
	}

	auto InputHandler::config_proxy::set_mouse_repeat_click_delay(const duration_type delay) noexcept -> void
	{
		auto& handler = self.get();

		handler.mouse_repeat_click_delay_ = delay;
	}

	auto InputHandler::config_proxy::set_mouse_repeat_click_rate(const duration_type rate) noexcept -> void
	{
		auto& handler = self.get();

		handler.mouse_repeat_click_rate_ = rate;
	}

	auto InputHandler::config() noexcept -> config_proxy
	{
		return {.self = *this};
	}
}
