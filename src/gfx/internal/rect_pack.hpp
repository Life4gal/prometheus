// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// Base on: stb_rect_pack.h - v1.01 - public domain - rectangle packing Sean Barrett 2014

#pragma once

#include <span>
#include <vector>

#include <primitive/point.hpp>
#include <primitive/extent.hpp>

namespace gal::prometheus::gfx
{
	enum class PackPrefer : std::uint8_t
	{
		FAST_FAIL = 0,
		SKIP = 1,

		DEFAULT = FAST_FAIL,
	};

	enum class Heuristic : std::uint8_t
	{
		SKYLINE_BOTTOM_LEFT = 0,
		SKYLINE_BEST_FIT = 1,

		DEFAULT = SKYLINE_BOTTOM_LEFT,
	};

	class RectPackContext final
	{
	public:
		using point_type = primitive::basic_point_2d<std::uint32_t>;
		using extent_type = primitive::basic_extent_2d<std::uint32_t>;

		constexpr static auto invalid_point = point_type{std::numeric_limits<point_type::value_type>::max(), std::numeric_limits<point_type::value_type>::max()};

		struct rect_type final
		{
			// INPUT
			extent_type size;

			// OUTPUT
			point_type point;

			[[nodiscard]] constexpr auto packed() const noexcept -> bool
			{
				return point != invalid_point;
			}
		};

	private:
		struct rect_pack_node final
		{
			point_type point;
			rect_pack_node* next;
		};

		struct find_y_result
		{
			point_type::value_type y;
			extent_type::value_type waste;
		};

		struct find_result
		{
			point_type point;
			rect_pack_node** prev_link;
		};

		extent_type size_;

		// size_.width + 2
		std::vector<rect_pack_node> nodes_;
		// size_.width
		rect_pack_node* free_head_;
		// 2
		rect_pack_node* active_head_;

		[[nodiscard]] auto nodes_count() const noexcept -> std::size_t;

		[[nodiscard]] auto align_of(PackPrefer pack_prefer) const noexcept -> std::uint32_t;

		// find minimum y position if it starts at x1
		[[nodiscard]] static auto skyline_find_min_y(const rect_pack_node* head, point_type::value_type x0, extent_type::value_type width) noexcept -> find_y_result;

		[[nodiscard]] auto skyline_find_best_pos(extent_type size, PackPrefer pack_prefer, Heuristic heuristic) noexcept -> find_result;

		[[nodiscard]] auto skyline_pack_rectangle(extent_type size, PackPrefer pack_prefer, Heuristic heuristic) noexcept -> find_result;

	public:
		explicit RectPackContext(const extent_type& size) noexcept;

		auto pack(
			std::span<rect_type> in_out_rects,
			PackPrefer pack_prefer = PackPrefer::DEFAULT,
			Heuristic heuristic = Heuristic::DEFAULT
		) noexcept -> bool;
	};
}
