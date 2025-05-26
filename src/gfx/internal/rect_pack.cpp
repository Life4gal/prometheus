// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/internal/rect_pack.hpp>

#include <algorithm>
#include <ranges>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace
{
	using namespace gal::prometheus;
	using namespace gfx;

	// find minimum y position if it starts at x1
	[[nodiscard]] auto skyline_find_min_y(
		const rect_pack_node* first,
		const Context::point_type::value_type x0,
		const Context::extent_type::value_type width,
		std::uint64_t& waster_area
	) noexcept -> Context::point_type::value_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(first->point.x <= x0);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(first->next->point.x > x0);

		const auto x1 = x0 + width;
		const auto* current = first;

		Context::point_type::value_type min_y = 0;
		Context::extent_type::value_type visited_width = 0;

		while (current->point.x < x1)
		{
			if (current->point.y > min_y)
			{
				// raise min_y higher.
				// we've accounted for all waste up to min_y,
				// but we'll now add more waste for everything we've visited
				waster_area += static_cast<std::uint64_t>(visited_width) * (current->point.y - min_y);
				min_y = current->point.y;

				// the first time through, visited_width might be reduced
				visited_width += current->next->point.x - std::ranges::max(x0, current->point.x);
			}
			else
			{
				// add waste area
				const auto under_width = std::ranges::min(current->next->point.x - current->point.x, width - visited_width);

				waster_area += static_cast<std::uint64_t>(under_width) * (min_y - current->point.y);
				visited_width += under_width;
			}

			current = current->next;
		}

		return min_y;
	}

	struct find_result
	{
		Context::point_type point;
		rect_pack_node** prev_link;
	};

	auto skyline_find_best_pos(const Context& context, Context::extent_type size) noexcept -> find_result
	{
		// align to multiple of context->align
		const auto align = context.align();
		size.width = ((size.width + align - 1) / align) * align;

		// if it can't possibly fit, bail immediately
		if (size.width > context.size().width or size.height > context.size().height)
		{
			return {.point = {0, 0}, .prev_link = nullptr};
		}

		const auto& context_size = context.size();
		const auto heuristic = context.heuristic();

		auto best_waste = std::numeric_limits<std::uint64_t>::max();
		auto best_y = std::numeric_limits<Context::point_type::value_type>::max();

		rect_pack_node** best = nullptr;
		auto* current = context.active_head();
		auto** prev = &current;

		while (current->point.x + size.width <= context_size.width)
		{
			std::uint64_t waste = 0;
			const auto min_y = skyline_find_min_y(current, current->point.x, size.width, waste);

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
			const auto* tail = context.active_head();

			current = context.active_head();
			prev = &current;

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

				std::uint64_t waste = 0;
				if (const auto min_y = skyline_find_min_y(current, x, size.width, waste);
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

	auto skyline_pack_rectangle(Context& context, const Context::extent_type size) noexcept -> find_result
	{
		const auto context_size = context.size();

		// find best position according to heuristic
		auto result = skyline_find_best_pos(context, size);

		// bail if:
		//    1. it failed
		//    2. the best node doesn't fit (we don't always check this)
		//    3. we're out of memory
		if (result.prev_link == nullptr or result.point.y + size.height > context_size.height or context.free_head() == nullptr)
		{
			return {.point = result.point, .prev_link = nullptr};
		}

		// on success, create new node
		auto* head = context.free_head();
		head->point = {result.point.x, result.point.y + size.height};

		context.free_head(head->next);

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
			current->next = context.free_head();
			context.free_head(current);
			current = next;
		}

		// stitch the list back in
		head->next = current;

		current->point.x = std::ranges::max(current->point.x, result.point.x + size.width);

#if GAL_PROMETHEUS_COMPILER_DEBUG
		current = context.active_head();
		while (current->point.x < context_size.width)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current->point.x < current->next->point.x);
			current = current->next;
		}
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current->next == nullptr);

		std::uint32_t count = 0;
		for (current = context.active_head(); current; current = current->next, count += 1) {}
		for (current = context.free_head(); current; current = current->next, count += 1) {}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(count = context.nodes_count() + 2);
#endif

		return result;
	}
}

namespace gal::prometheus::gfx
{
	Context::Context(const extent_type& size, const std::span<rect_pack_node> nodes) noexcept
		: size_{size},
		  heuristic_{Heuristic::DEFAULT},
		  align_{0},
		  nodes_count_{static_cast<std::uint32_t>(nodes.size())},
		  active_head_{extra_},
		  free_head_{nodes.data()}
	{
		for (auto [index, node]: nodes | std::views::take(nodes.size() - 1) | std::views::enumerate)
		{
			node.next = &nodes[index + 1];
		}
		nodes[nodes.size() - 1].next = nullptr;

		set_fast_fail(true);

		// node 0 is the full width, node 1 is the sentinel (lets us not store width explicitly)
		extra_[0] = {.point = {0, 0}, .next = &extra_[1]};
		extra_[1] = {.point = {size.width, std::numeric_limits<point_type::value_type>::max()}, .next = nullptr};
	}

	auto Context::set_heuristic(const Heuristic heuristic) noexcept -> void
	{
		heuristic_ = heuristic;
	}

	auto Context::set_fast_fail(const bool fast_fail) noexcept -> void
	{
		if (fast_fail)
		{
			align_ = (size_.width + nodes_count_ - 1) / nodes_count_;
		}
		else
		{
			align_ = 1;
		}
	}

	auto Context::pack(std::span<rect_pack_rect> rects) noexcept -> bool
	{
		constexpr auto invalid_point = point_type{std::numeric_limits<point_type::value_type>::max(), std::numeric_limits<point_type::value_type>::max()};

		// we use the 'was_packed' field internally to allow sorting/un-sorting
		for (auto [index, rect]: rects | std::views::enumerate)
		{
			rect.was_packed = static_cast<std::uint32_t>(index);
		}

		// sort according to heuristic
		std::ranges::sort(
			rects,
			[](const rect_pack_rect& r1, const rect_pack_rect& r2) noexcept -> bool
			{
				return r1.size.height < r2.size.height or r1.size.width < r2.size.width;
			}
		);

		std::ranges::for_each(
			rects,
			[this](rect_pack_rect& rect) noexcept -> void
			{
				if (rect.size.width == 0 or rect.size.height == 0)
				{
					// empty rect needs no space
					rect.point = {0, 0};
				}
				else
				{
					if (const auto [point, prev_link] = skyline_pack_rectangle(*this, rect.size); prev_link)
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
			rects,
			[](const rect_pack_rect& r1, const rect_pack_rect& r2) noexcept -> bool
			{
				return r1.was_packed < r2.was_packed;
			}
		);

		// set was_packed flags
		std::ranges::for_each(
			rects,
			[](rect_pack_rect& rect) noexcept -> void
			{
				rect.was_packed = rect.point != invalid_point ? 1 : 0;
			}
		);

		return not std::ranges::contains(rects, std::uint32_t{0}, &rect_pack_rect::was_packed);
	}

	auto Context::size() const noexcept -> const extent_type&
	{
		return size_;
	}

	auto Context::heuristic() const noexcept -> Heuristic
	{
		return heuristic_;
	}

	auto Context::align() const noexcept -> std::uint32_t
	{
		return align_;
	}

	auto Context::nodes_count() const noexcept -> std::uint32_t
	{
		return nodes_count_;
	}

	auto Context::active_head() const noexcept -> rect_pack_node*
	{
		return active_head_;
	}

	auto Context::free_head() const noexcept -> rect_pack_node*
	{
		return free_head_;
	}

	auto Context::free_head(rect_pack_node* node) noexcept -> void
	{
		free_head_ = node;
	}
}
