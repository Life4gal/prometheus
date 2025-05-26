// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// Base on: stb_rect_pack.h - v1.01 - public domain - rectangle packing Sean Barrett 2014

#pragma once

#include <span>

#include <primitive/point.hpp>
#include <primitive/extent.hpp>

namespace gal::prometheus::gfx
{
	struct rect_pack_rect final
	{
		using point_type = primitive::basic_point_2d<std::uint32_t>;
		using extent_type = primitive::basic_extent_2d<std::uint32_t>;

		// INPUT
		extent_type size;

		// OUTPUT
		point_type point;

		// non-zero if valid packing
		std::uint32_t was_packed;
		// reserved for your use
		std::uint32_t id;
	};

	struct rect_pack_node final
	{
		using point_type = rect_pack_rect::point_type;

		point_type point;
		rect_pack_node* next;
	};

	enum class Heuristic : std::uint8_t
	{
		SKYLINE_BOTTOM_LEFT = 0,
		SKYLINE_BEST_FIT = 1,

		DEFAULT = SKYLINE_BOTTOM_LEFT,
	};

	class Context final
	{
	public:
		using point_type = rect_pack_rect::point_type;
		using extent_type = rect_pack_rect::extent_type;

	private:
		extent_type size_;

		Heuristic heuristic_;
		std::uint32_t align_;
		std::uint32_t nodes_count_;

		rect_pack_node* active_head_;
		rect_pack_node* free_head_;
		// we allocate two extra nodes so optimal user-node-count is 'size.width' not 'size.width+2'
		rect_pack_node extra_[2];

	public:
		Context(const extent_type& size, std::span<rect_pack_node> nodes) noexcept;

		auto set_heuristic(Heuristic heuristic) noexcept -> void;

		auto set_fast_fail(bool fast_fail) noexcept -> void;

		auto pack(std::span<rect_pack_rect> rects) noexcept -> bool;

		// =======================================
		// INTERNAL
		// =======================================

		[[nodiscard]] auto size() const noexcept -> const extent_type&;

		[[nodiscard]] auto heuristic() const noexcept -> Heuristic;

		[[nodiscard]] auto align() const noexcept -> std::uint32_t;

		[[nodiscard]] auto nodes_count() const noexcept -> std::uint32_t;

		[[nodiscard]] auto active_head() const noexcept -> rect_pack_node*;

		[[nodiscard]] auto free_head() const noexcept -> rect_pack_node*;
		auto free_head(rect_pack_node* node) noexcept -> void;
	};
}
