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
import :style;
import :element;

#else
#include <optional>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>
#include <draw/surface.ixx>
#include <draw/style.ixx>
#include <draw/element.ixx>

#endif

namespace
{
	using namespace gal::prometheus;
	using namespace draw;

	namespace function
	{
		using impl::requirement_type;

		auto flex(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_width = Style::instance().flex_pixel_x;
			requirement.flex_grow_height = Style::instance().flex_pixel_y;
			requirement.flex_shrink_width = Style::instance().flex_pixel_x;
			requirement.flex_shrink_height = Style::instance().flex_pixel_y;
		}

		auto no_flex(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_width = 0;
			requirement.flex_grow_height = 0;
			requirement.flex_shrink_width = 0;
			requirement.flex_shrink_height = 0;
		}

		auto flex_grow(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_width = Style::instance().flex_pixel_x;
			requirement.flex_grow_height = Style::instance().flex_pixel_y;
		}

		auto flex_shrink(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_shrink_width = Style::instance().flex_pixel_x;
			requirement.flex_shrink_height = Style::instance().flex_pixel_y;
		}

		auto horizontal_flex(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_width = Style::instance().flex_pixel_x;
			requirement.flex_shrink_width = Style::instance().flex_pixel_x;
		}

		auto horizontal_flex_grow(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_width = Style::instance().flex_pixel_x;
		}

		auto horizontal_flex_shrink(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_shrink_width = Style::instance().flex_pixel_x;
		}

		auto vertical_flex(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_height = Style::instance().flex_pixel_y;
			requirement.flex_shrink_height = Style::instance().flex_pixel_y;
		}

		auto vertical_flex_grow(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_height = Style::instance().flex_pixel_y;
		}

		auto vertical_flex_shrink(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_shrink_height = Style::instance().flex_pixel_y;
		}
	}

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

	class Flex final : public impl::Element
	{
	public:
		using function_type = void(*)(impl::requirement_type&);

	private:
		function_type function_;

	public:
		explicit Flex(const function_type function) noexcept
			: Element{},
			  function_{function} {}

		Flex(const function_type function, element_type element) noexcept
			: Element{elements_type{std::move(element)}},
			  function_{function} {}

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			requirement_.reset();

			if (not children_.empty())
			[[unlikely]]
			{
				children_[0]->calculate_requirement(surface);
				requirement_ = children_[0]->requirement();
			}

			function_(requirement_);
		}

		auto set_rect(const rect_type& rect) noexcept -> void override
		{
			if (children_.empty())
			[[unlikely]]
			{
				return;
			}

			children_[0]->set_rect(rect);
		}
	};

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

			const auto flex_pixel_x = Style::instance().flex_pixel_x;
			const auto target_size = rect.width() + flex_pixel_x;
			calculate(elements, target_size);

			auto x = rect.left_top().x;
			std::ranges::for_each(
				std::views::zip(children_, elements),
				[&x, &rect, flex_pixel_x](std::tuple<element_type&, const element_size&> pack) noexcept -> void
				{
					auto& [child, element] = pack;

					const rect_type box
					{
							point_type{x, rect.left_top().y},
							point_type{x + element.size - flex_pixel_x, rect.right_bottom().y}
					};
					child->set_rect(box);
					x = box.right_bottom().x + flex_pixel_x;
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

			const auto flex_pixel_y = Style::instance().flex_pixel_y;
			const auto target_size = rect.height() + flex_pixel_y;
			calculate(elements, target_size);

			auto y = rect.left_top().y;
			std::ranges::for_each(
				std::views::zip(children_, elements),
				[&y, &rect, flex_pixel_y](std::tuple<element_type&, const element_size&> pack) noexcept -> void
				{
					auto& [child, element] = pack;

					const rect_type box
					{
							point_type{rect.left_top().x, y},
							point_type{rect.right_bottom().x, y + element.size - flex_pixel_y}
					};
					child->set_rect(box);
					y = box.right_bottom().y + flex_pixel_y;
				}
			);
		}
	};

	class StackBox final : public impl::Element
	{
	public:
		explicit StackBox(elements_type children) noexcept
			: Element{std::move(children)} {}

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			requirement_.reset();

			std::ranges::for_each(
				children_,
				[this, &surface](const auto& child) noexcept -> void
				{
					child->calculate_requirement(surface);

					requirement_.min_width = std::ranges::max(
						requirement_.min_width,
						child->requirement().min_width
					);
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

			std::ranges::for_each(
				children_,
				[rect](auto& child) noexcept -> void
				{
					child->set_rect(rect);
				}
			);
		}
	};
}

namespace gal::prometheus::draw::element::impl
{
	[[nodiscard]] auto filler() noexcept -> element_type
	{
		return make_element<Flex>(function::flex);
	}

	[[nodiscard]] auto no_flex(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::no_flex, std::move(element));
	}

	[[nodiscard]] auto flex(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::flex, std::move(element));
	}

	[[nodiscard]] auto flex_grow(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::flex_grow, std::move(element));
	}

	[[nodiscard]] auto flex_shrink(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::flex_shrink, std::move(element));
	}

	[[nodiscard]] auto horizontal_flex(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::horizontal_flex, std::move(element));
	}

	[[nodiscard]] auto horizontal_flex_grow(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::horizontal_flex_grow, std::move(element));
	}

	[[nodiscard]] auto horizontal_flex_shrink(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::horizontal_flex_shrink, std::move(element));
	}

	[[nodiscard]] auto vertical_flex(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::vertical_flex, std::move(element));
	}

	[[nodiscard]] auto vertical_flex_grow(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::vertical_flex_grow, std::move(element));
	}

	[[nodiscard]] auto vertical_flex_shrink(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::vertical_flex_shrink, std::move(element));
	}

	[[nodiscard]] auto horizontal_box(elements_type elements) noexcept -> element_type
	{
		return make_element<HorizontalBox>(std::move(elements));
	}

	[[nodiscard]] auto vertical_box(elements_type elements) noexcept -> element_type
	{
		return make_element<VerticalBox>(std::move(elements));
	}

	[[nodiscard]] auto stack_box(elements_type elements) noexcept -> element_type
	{
		return make_element<StackBox>(std::move(elements));
	}

	[[nodiscard]] auto horizontal_center(element_type element) noexcept -> element_type
	{
		return element::horizontal_box(filler(), std::move(element), filler());
	}

	[[nodiscard]] auto vertical_center(element_type element) noexcept -> element_type
	{
		return element::vertical_box(filler(), std::move(element), filler());
	}

	[[nodiscard]] auto center(element_type element) noexcept -> element_type
	{
		return horizontal_center(vertical_center(std::move(element)));
	}
}
