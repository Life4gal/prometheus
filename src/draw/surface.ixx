// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:surface;

import std;
GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

export import :draw_list;
export import :element;

#else
#pragma once

#include <prometheus/macro.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <draw/draw_list.ixx>
#include <draw/detail/element.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw)
{
	class Surface
	{
	public:
		using draw_list_type = DrawList;
		using rect_type = draw_list_type::rect_type;
		using point_type = draw_list_type::point_type;
		using extent_type = draw_list_type::extent_type;

	private:
		draw_list_type draw_list_;
		rect_type rect_;

	public:
		explicit Surface() noexcept = default;

		explicit Surface(const rect_type& rect) noexcept : rect_{rect} {}

		[[nodiscard]] auto draw_list(this auto&& self) noexcept -> auto&& { return std::forward<decltype(self)>(self).draw_list_; }

		[[nodiscard]] auto rect() const noexcept -> const rect_type& { return rect_; }

		auto set_rect(const rect_type& rect) noexcept -> void { rect_ = rect; }

		auto set_rect(const point_type& left_top, const point_type& right_bottom) noexcept -> void
		{
			set_rect({left_top, right_bottom});
		}

		auto new_frame() noexcept -> void
		{
			draw_list_.reset();
			draw_list_.push_clip_rect(rect_, false);
		}

		auto render(const Style& style, detail::Element& element) noexcept -> void;

		auto render(const Style& style, const element_type& element) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(element != nullptr);
			render(style, *element);
		}

		// todo
	};
}
