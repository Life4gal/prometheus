// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gui/internal/mouse.hpp>
#include <gui/internal/context.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::gui::internal
{
	auto Mouse::is_down(const Context& context, MouseKey key) const noexcept -> bool
	{
		std::ignore = context;

		const auto index = static_cast<std::size_t>(key);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < mouse_key_count);

		return key_statuses[index].down;
	}

	auto Mouse::is_clicked(const Context& context, MouseKey key, const bool repeat) const noexcept -> bool
	{
		const auto& io = context.io();

		const auto index = static_cast<std::size_t>(key);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < mouse_key_count);

		const auto& key_status = key_statuses[index];
		const auto down_time = key_status.down_time;

		if (down_time == Mouse::time_just_start) // NOLINT(clang-diagnostic-float-equal)
		{
			return true;
		}

		if (repeat and down_time > io.mouse_repeat_click_delay)
		{
			const auto v1 = std::fmodf(down_time - io.mouse_repeat_click_delay, io.mouse_repeat_click_rate) > io.mouse_repeat_click_rate * .5f;
			const auto v2 = std::fmodf(down_time - io.delta_time, io.mouse_repeat_click_rate) > io.mouse_repeat_click_rate * .5f;

			return v1 != v2;
		}

		return false;
	}

	auto Mouse::is_double_clicked(const Context& context, MouseKey key) const noexcept -> bool
	{
		std::ignore = context;

		const auto index = static_cast<std::size_t>(key);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < mouse_key_count);

		const auto& key_status = key_statuses[index];

		return key_status.double_clicked;
	}

	auto Mouse::tick(const Context& context) noexcept -> void
	{
		// ----------------------
		// read context
		const auto& io = context.io();
		const auto time = context.current_time();

		position_current = io.mouse_position;
		wheel = io.mouse_wheel;

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(key_statuses.size() == io.mouse_button_state.state.size());
		key_statuses[static_cast<std::size_t>(MouseKey::LEFT)].down = io.mouse_button_state[MouseKey::LEFT];
		key_statuses[static_cast<std::size_t>(MouseKey::MIDDLE)].down = io.mouse_button_state[MouseKey::MIDDLE];
		key_statuses[static_cast<std::size_t>(MouseKey::RIGHT)].down = io.mouse_button_state[MouseKey::RIGHT];
		key_statuses[static_cast<std::size_t>(MouseKey::X1)].down = io.mouse_button_state[MouseKey::X1];
		key_statuses[static_cast<std::size_t>(MouseKey::X2)].down = io.mouse_button_state[MouseKey::X2];

		// ----------------------
		// update mouse state

		if (position_current.x < 0 and position_current.y < 0)
		{
			position_current = position_unknown;
		}

		if (position_current == position_unknown and position_previous == position_unknown)
		{
			// mouse just appeared or disappeared
			position_delta = {0, 0};
		}
		else
		{
			position_delta = (position_current - position_previous).to<extent_type>();
		}
		position_previous = position_current;

		static_assert(time_not_start < 0);
		std::ranges::for_each(
			key_statuses,
			[&](auto& key_status) noexcept -> void
			{
				if (key_status.down)
				{
					if (key_status.down_time < 0)
					{
						// not start
						key_status.down_time = time_just_start;
					}
					else
					{
						key_status.down_time += io.delta_time;
					}
				}
				else
				{
					key_status.down_time = time_not_start;
				}

				// just start
				key_status.clicked = key_status.down_time == time_just_start; // NOLINT(clang-diagnostic-float-equal)
				key_status.double_clicked = false;

				if (key_status.clicked)
				{
					// time_not_start < 0
					if (time - key_status.click_time < io.mouse_double_click_interval_threshold)
					{
						if (position_current.distance(key_status.click_position) < io.mouse_double_click_distance_threshold)
						{
							key_status.double_clicked = true;
						}

						// so the third click isn't turned into a double click
						key_status.click_time = time_not_start;
					}
					else
					{
						key_status.click_time = time;
						key_status.click_position = position_current;
					}
				}
			}
		);
	}
}
