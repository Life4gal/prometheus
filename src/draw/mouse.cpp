// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <draw/mouse.hpp>

namespace gal::prometheus::draw
{
	Mouse::Mouse(const time_type double_click_interval_threshold, const value_type double_click_distance_threshold) noexcept
		: double_click_interval_threshold_{double_click_interval_threshold},
		  double_click_distance_threshold_{double_click_distance_threshold},
		  position_current_{std::numeric_limits<value_type>::min(), std::numeric_limits<value_type>::min()},
		  position_previous_{std::numeric_limits<value_type>::min(), std::numeric_limits<value_type>::min()},
		  position_clicked_{std::numeric_limits<value_type>::min(), std::numeric_limits<value_type>::min()},
		  down_{false},
		  clicked_{false},
		  double_clicked_{false},
		  pad_{false},
		  down_duration_{0},
		  click_duration_{0}
	{
		std::ignore = pad_;
	}

	auto Mouse::position() const noexcept -> point_type
	{
		return position_current_;
	}

	auto Mouse::position_delta() const noexcept -> extent_type
	{
		const auto delta = position_current_ - position_previous_;

		return delta.to<extent_type>();
	}

	auto Mouse::down() const noexcept -> bool
	{
		return down_;
	}

	auto Mouse::clicked() const noexcept -> bool
	{
		return clicked_;
	}

	auto Mouse::double_clicked() const noexcept -> bool
	{
		return double_clicked_;
	}

	auto Mouse::move(const point_type position) noexcept -> void
	{
		position_current_ = position;
	}

	auto Mouse::tick(const time_type tick_time) noexcept -> void
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
}
