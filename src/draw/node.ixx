// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:node;

import std;
import gal.prometheus.primitive;

#else
#pragma once

#include <memory>
#include <vector>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw)
{
	namespace impl
	{
		class Node;
		class Surface;
	}

	using element_type = std::shared_ptr<impl::Node>;
	using elements_type = std::vector<element_type>;

	namespace impl
	{
		struct requirement_type
		{
			float min_width{0};
			float min_height{0};

			float flex_grow_width{0};
			float flex_grow_height{0};
			float flex_shrink_width{0};
			float flex_shrink_height{0};
		};

		class Node
		{
		public:
			using rect_type = primitive::basic_rect_2d<float>;

		protected:
			elements_type children_;
			requirement_type requirement_;
			rect_type rect_;

		public:
			Node(const Node&) noexcept = default;
			auto operator=(const Node&) noexcept -> Node& = default;
			Node(Node&&) noexcept = default;
			auto operator=(Node&&) noexcept -> Node& = default;

			explicit Node() noexcept = default;

			explicit Node(elements_type children) noexcept
				: children_{std::move(children)} {}

			virtual ~Node() noexcept = 0;

			[[nodiscard]] auto requirement() const noexcept -> requirement_type
			{
				return requirement_;
			}

			virtual auto calculate_requirement() noexcept -> void
			{
				std::ranges::for_each(
					children_,
					[](auto& node) noexcept -> void { node->calculate_requirement(); }
				);
			}

			virtual auto set_rect(const rect_type& rect) noexcept -> void
			{
				rect_ = rect;
			}

			virtual auto render(Surface& surface) noexcept -> void
			{
				std::ranges::for_each(
					children_,
					[&surface](auto& node) noexcept -> void { node->render(surface); }
				);
			}
		};
	}
}
