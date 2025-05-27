// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/internal/rect_pack.hpp>

#include <algorithm>
#include <ranges>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::gfx
{
	auto RectPackContext::rect_type::packed() const noexcept -> bool
	{
		return point != invalid_point;
	}

	auto RectPackContext::align_of(const PackPrefer pack_prefer) const noexcept -> std::uint32_t
	{
		if (pack_prefer == PackPrefer::FAST_FAIL)
		{
			return static_cast<std::uint32_t>(static_cast<std::size_t>(size_.width) + nodes_.size() - 1 / nodes_.size());
		}

		return 1;
	}

	auto RectPackContext::skyline_find_min_y(const rect_pack_node* head, const point_type::value_type x0, const extent_type::value_type width) noexcept -> find_y_result
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(head->point.x <= x0);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(head->next->point.x > x0);

		const auto x1 = x0 + width;
		const auto* current = head;

		find_y_result result{.y = 0, .waste = 0};
		extent_type::value_type visited_width = 0;

		while (current->point.x < x1)
		{
			if (current->point.y > result.y)
			{
				// raise min_y higher.
				// we've accounted for all waste up to min_y,
				// but we'll now add more waste for everything we've visited
				result.waste += static_cast<std::uint64_t>(visited_width) * (current->point.y - result.y);
				result.y = current->point.y;

				// the first time through, visited_width might be reduced
				visited_width += current->next->point.x - std::ranges::max(x0, current->point.x);
			}
			else
			{
				// add waste area
				const auto under_width = std::ranges::min(current->next->point.x - current->point.x, width - visited_width);

				result.waste += static_cast<std::uint64_t>(under_width) * (result.y - current->point.y);
				visited_width += under_width;
			}

			current = current->next;
		}

		return result;
	}

	auto RectPackContext::skyline_find_best_pos(extent_type size, const PackPrefer pack_prefer, const Heuristic heuristic) noexcept -> find_result
	{
		const auto& context_size = size_;

		// align to multiple of 'align'
		const auto align = align_of(pack_prefer);
		size.width = ((size.width + align - 1) / align) * align;

		// if it can't possibly fit, bail immediately
		if (size.width > context_size.width or size.height > context_size.height)
		{
			return {.point = {0, 0}, .prev_link = nullptr};
		}

		auto best_waste = std::numeric_limits<std::uint64_t>::max();
		auto best_y = std::numeric_limits<point_type::value_type>::max();

		rect_pack_node** best = nullptr;
		{
			auto* current = active_head_;
			auto** prev = &current;

			while (current->point.x + size.width <= context_size.width)
			{
				const auto [min_y, waste] = skyline_find_min_y(current, current->point.x, size.width);

				if (heuristic == Heuristic::SKYLINE_BOTTOM_LEFT)
				{
					if (min_y < best_y)
					{
						best_y = min_y;
						best = prev;
					}
				}
				else if (heuristic == Heuristic::SKYLINE_BEST_FIT)
				{
					// best-fit
					if (min_y + size.height <= context_size.height)
					{
						// can only use it if it first vertically
						if (min_y < best_y or (min_y == best_y and waste < best_waste))
						{
							best_y = min_y;
							best_waste = waste;
							best = prev;
						}
					}
				}
				else
				{
					GAL_PROMETHEUS_COMPILER_UNREACHABLE();
				}

				prev = &current->next;
				current = current->next;
			}
		}

		auto best_x = best == nullptr ? 0 : (*best)->point.x;

		// if doing best-fit (BF), we also have to try aligning right edge to each node position
		//
		// e.g, if fitting
		//
		//     ____________________
		//    |____________________|
		//
		//            into
		//
		//   |                         |
		//   |             ____________|
		//   |____________|
		//
		// then right-aligned reduces waste, but bottom-left BL is always chooses left-aligned
		//
		// This makes BF take about 2x the time

		if (heuristic == Heuristic::SKYLINE_BEST_FIT)
		{
			const auto* tail = active_head_;
			auto* current = active_head_;
			auto** prev = &current;

			// find first node that's admissible
			while (tail->point.x < size.width)
			{
				tail = tail->next;
			}

			while (tail)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(tail->point.x >= size.width);
				const auto x = tail->point.x - size.width;

				// find the left position that matches this
				while (current->next->point.x <= x)
				{
					prev = &current->next;
					current = current->next;
				}

				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current->next->point.x > x and current->point.x <= x);

				if (const auto [min_y, waste] = skyline_find_min_y(current, x, size.width);
					min_y + size.height <= context_size.height)
				{
					if (min_y <= best_y)
					{
						if (min_y < best_y or waste < best_waste or (waste == best_waste and x < best_x))
						{
							best_x = x;
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(min_y <= best_y);
							best_y = min_y;
							best_waste = waste;
							best = prev;
						}
					}
				}

				tail = tail->next;
			}
		}

		return {.point = {best_x, best_y}, .prev_link = best};
	}

	auto RectPackContext::skyline_pack_rectangle(extent_type size, PackPrefer pack_prefer, Heuristic heuristic) noexcept -> find_result
	{
		const auto context_size = size_;

		// find best position according to heuristic
		auto result = skyline_find_best_pos(size, pack_prefer, heuristic);

		// bail if:
		//    1. it failed
		//    2. the best node doesn't fit (we don't always check this)
		//    3. we're out of memory
		if (result.prev_link == nullptr or result.point.y + size.height > context_size.height or free_head_ == nullptr)
		{
			return {.point = result.point, .prev_link = nullptr};
		}

		// on success, create new node
		auto* head = free_head_;
		head->point = {result.point.x, result.point.y + size.height};
		free_head_ = head->next;

		// insert the new node into the right starting point,
		// and let 'current' point to the remaining nodes needing to be stitched back in
		auto* current = *result.prev_link;
		if (current->point.x < result.point.x)
		{
			// preserve the existing one, so start testing with the next one
			auto* next = current->next;
			current->next = head;
			current = next;
		}
		else
		{
			*result.prev_link = head;
		}

		// from here, traverse current and free the nodes, until we get to one
		// that shouldn't be freed
		while (current->next and current->next->point.x <= result.point.x + size.width)
		{
			auto* next = current->next;
			// move the current node to the free list
			current->next = free_head_;
			free_head_ = current;
			current = next;
		}

		// stitch the list back in
		head->next = current;

		current->point.x = std::ranges::max(current->point.x, result.point.x + size.width);

#if GAL_PROMETHEUS_COMPILER_DEBUG
		current = active_head_;
		while (current->point.x < context_size.width)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current->point.x < current->next->point.x);
			current = current->next;
		}
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current->next == nullptr);

		std::size_t count = 0;
		for (current = active_head_; current; current = current->next, count += 1) {}
		for (current = free_head_; current; current = current->next, count += 1) {}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(count == nodes_.size() + 2);
#endif

		return result;
	}

	RectPackContext::RectPackContext(const extent_type& size) noexcept
		: size_{size},
		  nodes_{size.width},
		  free_head_{nodes_.data()}
	{
		for (auto [index, node]: nodes_ | std::views::take(nodes_.size() - 1) | std::views::enumerate)
		{
			node.next = &nodes_[index + 1];
		}
		nodes_.back().next = nullptr;

		// node 0 is the full width, node 1 is the sentinel (lets us not store width explicitly)
		active_head_[0] = {.point = {0, 0}, .next = &active_head_[1]};
		active_head_[1] = {.point = {size_.width, std::numeric_limits<point_type::value_type>::max()}, .next = nullptr};
	}

	auto RectPackContext::pack(
		std::span<rect_type> in_out_rects,
		const PackPrefer pack_prefer,
		const Heuristic heuristic
	) noexcept -> bool
	{
		// we use the 'internal_status_' field internally to allow sorting/un-sorting
		for (auto [index, rect]: in_out_rects | std::views::enumerate)
		{
			rect.internal_status_ = static_cast<std::uint32_t>(index);
		}

		// sort according to heuristic
		std::ranges::sort(
			in_out_rects,
			[](const rect_type& r1, const rect_type& r2) noexcept -> bool
			{
				if (r1.size.height != r2.size.height)
				{
					return r1.size.height > r2.size.height;
				}

				return r1.size.width > r2.size.width;
			}
		);

		std::ranges::for_each(
			in_out_rects,
			[this, pack_prefer, heuristic](rect_type& rect) noexcept -> void
			{
				if (rect.size.width == 0 or rect.size.height == 0)
				{
					// empty rect needs no space
					rect.point = {0, 0};
				}
				else
				{
					if (const auto [point, prev_link] = skyline_pack_rectangle(rect.size, pack_prefer, heuristic); prev_link)
					{
						rect.point = point;
					}
					else
					{
						rect.point = invalid_point;
					}
				}
			}
		);

		// un-sort
		std::ranges::sort(
			in_out_rects,
			[](const rect_type& r1, const rect_type& r2) noexcept -> bool
			{
				return r1.internal_status_ < r2.internal_status_;
			}
		);

		return std::ranges::all_of(in_out_rects, &rect_type::packed);
	}
}
