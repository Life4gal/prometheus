// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.layout;

import std;

import gal.prometheus.primitive;

import :surface;
import :element;

#else
#include <optional>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>
#include <draw/surface.ixx>
#include <draw/element.ixx>

#endif

namespace
{
	using namespace gal::prometheus;
	using namespace draw;

	struct element_size
	{
		float min_size = 0;
		float flex_grow = 0;
		float flex_shrink = 0;

		float size = 0;
	};

	template<std::ranges::output_range<element_size> Range>
	constexpr auto calculate_grow(Range& range, float extra_space, float flex_grow_sum) noexcept -> void
	{
		std::ranges::for_each(
			range,
			[extra_space, flex_grow_sum](element_size& element) mutable noexcept -> void
			{
				const auto added_space = extra_space * element.flex_grow / std::ranges::max(flex_grow_sum, 1.f);

				extra_space -= added_space;
				flex_grow_sum -= element.flex_grow;
				element.size = element.min_size + added_space;
			}
		);
	}

	template<std::ranges::output_range<element_size> Range>
	constexpr auto calculate_shrink_easy(Range& range, float extra_space, float flex_shrink_sum) noexcept -> void
	{
		std::ranges::for_each(
			range,
			[extra_space, flex_shrink_sum](element_size& element) mutable noexcept -> void
			{
				const auto added_space = extra_space * element.min_size * element.flex_shrink / std::ranges::max(flex_shrink_sum, 1.f);

				extra_space -= added_space;
				flex_shrink_sum -= element.flex_shrink * element.min_size;
				element.size = element.min_size + added_space;
			}
		);
	}

	template<std::ranges::output_range<element_size> Range>
	constexpr auto calculate_shrink_hard(Range& range, float extra_space, float size) noexcept -> void
	{
		std::ranges::for_each(
			range,
			[extra_space, size](element_size& element) mutable noexcept -> void
			{
				if (element.flex_shrink != 0)
				{
					element.size = 0;
					return;
				}

				const auto added_space = extra_space * element.min_size / std::ranges::max(size, 1.f);

				extra_space -= added_space;
				size -= element.min_size;
				element.size = element.min_size + added_space;
			}
		);
	}

	template<std::ranges::output_range<element_size> Range>
	constexpr auto calculate(Range& range, const float target_size) noexcept -> void
	{
		float size = 0;
		float flex_grow_sum = 0;
		float flex_shrink_sum = 0;
		float flex_shrink_size = 0;

		std::ranges::for_each(
			range,
			[&](const element_size& element) noexcept -> void
			{
				size += element.min_size;
				flex_grow_sum += element.flex_grow;
				if (element.flex_shrink != 0)
				{
					flex_shrink_sum += element.min_size * element.flex_shrink;
					flex_shrink_size += element.min_size;
				}
			}
		);

		if (const auto extra_space = target_size - size;
			extra_space >= 0)
		{
			calculate_grow(range, extra_space, flex_grow_sum);
		}
		else if (flex_shrink_size + extra_space >= 0)
		{
			calculate_shrink_easy(range, extra_space, flex_shrink_sum);
		}
		else
		{
			calculate_shrink_hard(range, extra_space + flex_shrink_size, size - flex_shrink_size);
		}
	}

	class HorizontalBox final : public impl::Element
	{
	public:
		using point_type = rect_type::point_type;

		explicit HorizontalBox(elements_type children) noexcept
			: Element{std::move(children)} {}

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			requirement_.reset();

			std::ranges::for_each(
				children_,
				[this, &surface](const auto& child) noexcept -> void
				{
					child->calculate_requirement(surface);

					requirement_.min_width += child->requirement().min_width;
					requirement_.min_height = std::ranges::max(
						requirement_.min_height,
						child->requirement().min_height
					);
				}
			);
		}

		auto set_rect(const rect_type& rect) noexcept -> void override
		{
			Element::set_rect(rect);

			std::vector<element_size> elements{};
			elements.reserve(children_.size());

			std::ranges::transform(
				children_,
				std::back_inserter(elements),
				[](const auto& child) noexcept -> element_size
				{
					const auto& r = child->requirement();
					return {
							.min_size = r.min_width,
							.flex_grow = r.flex_grow_width,
							.flex_shrink = r.flex_shrink_width,
							.size = 0
					};
				}
			);

			const auto target_size = rect.width() + 1;
			calculate(elements, target_size);

			auto x = rect.left_top().x;
			std::ranges::for_each(
				std::views::zip(children_, elements),
				[&x, &rect](std::tuple<element_type&, element_size&> pack) noexcept -> void
				{
					auto& [child, element] = pack;

					const rect_type box
					{
							point_type{x, rect.left_top().y},
							point_type{x + element.size - 1, rect.right_bottom().y}
					};
					child->set_rect(box);
					x = box.right_bottom().x + 1;
				}
			);
		}
	};

	class VerticalBox final : public impl::Element
	{
	public:
		using point_type = rect_type::point_type;

		explicit VerticalBox(elements_type children) noexcept
			: Element{std::move(children)} {}

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			requirement_.reset();

			std::ranges::for_each(
				children_,
				[this, &surface](const auto& child) noexcept -> void
				{
					child->calculate_requirement(surface);

					requirement_.min_height += child->requirement().min_height;
					requirement_.min_width = std::ranges::max(
						requirement_.min_width,
						child->requirement().min_width
					);
				}
			);
		}

		auto set_rect(const rect_type& rect) noexcept -> void override
		{
			Element::set_rect(rect);

			std::vector<element_size> elements{};
			elements.reserve(children_.size());

			std::ranges::transform(
				children_,
				std::back_inserter(elements),
				[](const auto& child) noexcept -> element_size
				{
					const auto& r = child->requirement();
					return {
							.min_size = r.min_height,
							.flex_grow = r.flex_grow_height,
							.flex_shrink = r.flex_shrink_height,
							.size = 0
					};
				}
			);

			const auto target_size = rect.width() + 1;
			calculate(elements, target_size);

			auto y = rect.left_top().y;
			std::ranges::for_each(
				std::views::zip(children_, elements),
				[&y, &rect](std::tuple<element_type&, element_size&> pack) noexcept -> void
				{
					auto& [child, element] = pack;

					const rect_type box
					{
							point_type{rect.left_top().x, y},
							point_type{rect.right_bottom().x, y + element.size - 1}
					};
					child->set_rect(box);
					y = box.right_bottom().y + 1;
				}
			);
		}
	};
}

namespace gal::prometheus::draw::element::impl
{
	[[nodiscard]] auto horizontal_box(elements_type elements) noexcept -> element_type
	{
		return make_element<HorizontalBox>(std::move(elements));
	}

	[[nodiscard]] auto vertical_box(elements_type elements) noexcept -> element_type
	{
		return make_element<VerticalBox>(std::move(elements));
	}
}
