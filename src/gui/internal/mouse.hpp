// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gui/internal/common.hpp>

namespace gal::prometheus::gui::internal
{
	class Mouse final
	{
	public:
		using value_type = extent_type::value_type;

		// < 0
		constexpr static point_type position_unknown{-.999999f, -.999999f};
		// < 0
		constexpr static auto time_not_start = time_type{-.999999f};
		constexpr static auto time_just_start = time_type{0.f};

		struct key_status_type
		{
			bool down{false};
			bool pad{false};

			bool clicked{false};
			bool double_clicked{false};

			// Duration of the "pressed" state of the key
			time_type down_time{time_not_start};

			// Time of last press
			time_type click_time{time_not_start};
			// Position of last press
			point_type click_position{position_unknown};
		};

		point_type position_current{position_unknown};
		point_type position_previous{position_unknown};
		extent_type position_delta{0, 0};

		std::array<key_status_type, mouse_key_count> key_statuses{};

		value_type wheel{0};

		// ---------------------------------------------
		// STATE

		[[nodiscard]] auto is_down(const Context& context, MouseKey key) const noexcept -> bool;

		[[nodiscard]] auto is_clicked(const Context& context, MouseKey key, bool repeat = false) const noexcept -> bool;

		[[nodiscard]] auto is_double_clicked(const Context& context, MouseKey key) const noexcept -> bool;

		// ---------------------------------------------
		// UPDATE (PER FRAME)

		auto tick(const Context& context) noexcept -> void;
	};
}
