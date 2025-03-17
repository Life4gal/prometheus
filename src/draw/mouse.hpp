// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <draw/def.hpp>

namespace gal::prometheus::draw
{
	class Context;

	class Mouse final
	{
		friend Context;

	public:
		using rect_type = DrawListDef::rect_type;
		using point_type = DrawListDef::point_type;
		using extent_type = DrawListDef::extent_type;

		using value_type = point_type::value_type;

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
		bool pad_;

		time_type down_duration_;
		time_type click_duration_;

		Mouse(time_type double_click_interval_threshold, value_type double_click_distance_threshold) noexcept;

	public:
		[[nodiscard]] auto position() const noexcept -> point_type;

		[[nodiscard]] auto position_delta() const noexcept -> extent_type;

		[[nodiscard]] auto down() const noexcept -> bool;

		[[nodiscard]] auto clicked() const noexcept -> bool;

		[[nodiscard]] auto double_clicked() const noexcept -> bool;

	private:
		auto move(point_type position) noexcept -> void;

		auto tick(time_type tick_time) noexcept -> void;
	};
}
