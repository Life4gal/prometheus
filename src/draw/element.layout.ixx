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

			const auto& container_padding = Style::instance().container_padding;
			const auto& container_spacing = Style::instance().container_spacing;
			const auto extra_x = 2 * container_padding.width + (children_.size() - 1) * container_spacing.width;
			const auto extra_y = 2 * container_padding.height;

			requirement_.min_width += extra_x;
			requirement_.min_height += extra_y;
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

			const auto& container_padding = Style::instance().container_padding;
			const auto& container_spacing = Style::instance().container_spacing;
			const auto extra_x = 2 * container_padding.width + (children_.size() - 1) * container_spacing.width;

			calculate(elements, rect.width() - extra_x);

			const auto& [point, extent] = rect;

			std::ranges::for_each(
				std::views::zip(children_, elements),
				[
					current_x = point.x + container_padding.width,
					current_y = point.y + container_padding.height,
					current_bottom = point.y + extent.height - container_padding.height,
					container_spacing = container_spacing.width
				](std::tuple<element_type&, const element_size&> pack) mutable noexcept -> void
				{
					auto& [child, element] = pack;

					const rect_type box
					{
							// left
							current_x,
							// top
							current_y,
							// right
							current_x + element.size,
							// bottom
							current_bottom
					};
					child->set_rect(box);
					current_x = current_x + element.size + container_spacing;
				}
			);
		}
	};

	class VerticalBox final : public impl::Element
	{
	public:
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

			const auto& container_padding = Style::instance().container_padding;
			const auto& container_spacing = Style::instance().container_spacing;
			const auto extra_x = 2 * container_padding.width;
			const auto extra_y = 2 * container_padding.height + (children_.size() - 1) * container_spacing.height;

			requirement_.min_width += extra_x;
			requirement_.min_height += extra_y;
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

			const auto& container_padding = Style::instance().container_padding;
			const auto& container_spacing = Style::instance().container_spacing;
			const auto extra_y = 2 * container_padding.height + (children_.size() - 1) * container_spacing.height;

			calculate(elements, rect.height() - extra_y);

			const auto& [point, extent] = rect;

			std::ranges::for_each(
				std::views::zip(children_, elements),
				[
					current_x = point.x + container_padding.width,
					current_y = point.y + container_padding.height,
					current_right = point.x + extent.width - container_padding.width,
					container_spacing = container_spacing.height
				](std::tuple<element_type&, const element_size&> pack) mutable noexcept -> void
				{
					auto& [child, element] = pack;

					const rect_type box
					{
							// left
							current_x,
							// top
							current_y,
							// right
							current_right,
							// bottom
							current_y + element.size
					};
					child->set_rect(box);
					current_y = current_y + element.size + container_spacing;
				}
			);
		}
	};

	class GridBox final : public impl::Element
	{
	public:
		using element_grid_type = element_matrix_type;
		using size_type = element_grid_type::size_type;

	private:
		element_grid_type element_grid_;
		size_type width_;
		size_type height_;

	public:
		explicit GridBox(element_grid_type element_grid) noexcept
			: Element{},
			  element_grid_{std::move(element_grid)},
			  width_{0},
			  height_{element_grid_.size()}
		{
			width_ = std::ranges::max_element(element_grid_, {}, &element_grid_type::value_type::size)->size();

			std::ranges::for_each(
				element_grid_,
				[this](auto& line) noexcept -> void
				{
					if (const auto diff = width_ - line.size();
						diff != 0)
					{
						std::ranges::generate_n(std::back_inserter(line), diff, &element::impl::flex_filler);
					}
				}
			);
		}

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			requirement_.reset();

			std::ranges::for_each(
				element_grid_,
				[&surface](auto& line) noexcept -> void
				{
					std::ranges::for_each(
						line,
						[&surface](auto& element) noexcept -> void
						{
							element->calculate_requirement(surface);
						}
					);
				}
			);

			// Compute the size of each column/row
			std::vector<float> size_x{};
			std::vector<float> size_y{};
			size_x.resize(width_);
			size_y.reserve(height_);

			std::ranges::for_each(
				element_grid_,
				[
					&size_x,
					&size_y
				](const auto& line) noexcept -> void
				{
					const auto view = line | std::views::transform([](const auto& element) noexcept -> const auto& { return element->requirement(); });
					const auto& max_y = std::ranges::max_element(view, {}, &impl::requirement_type::min_height);

					size_y.emplace_back(max_y.base()->get()->requirement().min_height);

					std::ranges::for_each(
						std::views::zip(size_x, line),
						[](std::tuple<float&, const element_type&> pack) noexcept -> void
						{
							auto& [x, element] = pack;
							x = std::ranges::max(x, element->requirement().min_width);
						}
					);
				}
			);

			const auto integrate = [](std::vector<float>& size) noexcept -> float
			{
				float accumulate = 0.f;
				std::ranges::for_each(
					size,
					[&accumulate](auto& s) noexcept -> void
					{
						accumulate += std::exchange(s, accumulate);
					}
				);

				return accumulate;
			};

			const auto& container_padding = Style::instance().container_padding;
			const auto& container_spacing = Style::instance().container_spacing;
			const auto extra_x = 2 * container_padding.width + (width_ - 1) * container_spacing.width;
			const auto extra_y = 2 * container_padding.height + (height_ - 1) * container_spacing.height;

			requirement_.min_width = integrate(size_x) + extra_x;
			requirement_.min_height = integrate(size_y) + extra_y;
		}

		auto set_rect(const rect_type& rect) noexcept -> void override
		{
			Element::set_rect(rect);

			std::vector<element_size> elements_x{width_, {.min_size = 0, .flex_grow = std::numeric_limits<float>::max(), .flex_shrink = std::numeric_limits<float>::max(), .size = 0}};
			std::vector<element_size> elements_y{height_, {.min_size = 0, .flex_grow = std::numeric_limits<float>::max(), .flex_shrink = std::numeric_limits<float>::max(), .size = 0}};

			std::ranges::for_each(
				std::views::zip(elements_y, element_grid_),
				[&elements_x](std::tuple<element_size&, const elements_type&> pack) noexcept -> void
				{
					auto& [y, line] = pack;

					std::ranges::for_each(
						std::views::zip(elements_x, line),
						[&y](std::tuple<element_size&, const element_type&> inner_pack) noexcept -> void
						{
							auto& [x, element] = inner_pack;

							const auto& [min_width, min_height, flex_grow_width, flex_grow_height, flex_shrink_width, flex_shrink_height] = element->requirement();

							x.min_size = std::ranges::max(x.min_size, min_width);
							x.flex_grow = std::ranges::min(x.flex_grow, flex_grow_width);
							x.flex_shrink = std::ranges::min(x.flex_shrink, flex_shrink_width);

							y.min_size = std::ranges::max(y.min_size, min_height);
							y.flex_grow = std::ranges::min(y.flex_grow, flex_grow_height);
							y.flex_shrink = std::ranges::min(y.flex_shrink, flex_shrink_height);
						}
					);
				}
			);

			const auto& container_padding = Style::instance().container_padding;
			const auto& container_spacing = Style::instance().container_spacing;
			const auto extra_x = 2 * container_padding.width + (width_ - 1) * container_spacing.width;
			const auto extra_y = 2 * container_padding.height + (height_ - 1) * container_spacing.height;

			calculate(elements_x, rect.width() - extra_x);
			calculate(elements_y, rect.height() - extra_y);

			const auto& [point, extent] = rect;

			std::ranges::for_each(
				std::views::zip(elements_y, element_grid_),
				[
					&elements_x ,
					x = point.x + container_padding.width,
					y = point.y + container_padding.height,
					container_spacing = container_spacing
				](std::tuple<const element_size&, elements_type&> pack) mutable noexcept -> void
				{
					auto& [element_y, line] = pack;

					std::ranges::for_each(
						std::views::zip(elements_x, line),
						[
							current_x = x,
							current_y = y,
							current_height = element_y.size,
							container_spacing
						](std::tuple<const element_size&, element_type&> inner_pack) mutable noexcept -> void
						{
							auto& [element_x, element] = inner_pack;

							const rect_type box
							{
									// left
									current_x,
									// top
									current_y,
									// right
									current_x + element_x.size,
									// bottom
									current_y + current_height
							};
							element->set_rect(box);
							current_x = current_x + element_x.size + container_spacing.width;
						}
					);

					y = y + element_y.size + container_spacing.height;
				}
			);
		}

		auto render(Surface& surface) noexcept -> void override
		{
			std::ranges::for_each(
				element_grid_,
				[&surface](auto& line) noexcept -> void
				{
					std::ranges::for_each(
						line,
						[&surface](auto& element) noexcept -> void
						{
							element->render(surface);
						}
					);
				}
			);
		}
	};

	namespace function
	{
		using impl::requirement_type;

		auto flex(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_width = Style::instance().flex_x;
			requirement.flex_grow_height = Style::instance().flex_y;
			requirement.flex_shrink_width = Style::instance().flex_x;
			requirement.flex_shrink_height = Style::instance().flex_y;
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
			requirement.flex_grow_width = Style::instance().flex_x;
			requirement.flex_grow_height = Style::instance().flex_y;
		}

		auto flex_shrink(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_shrink_width = Style::instance().flex_x;
			requirement.flex_shrink_height = Style::instance().flex_y;
		}

		auto horizontal_flex(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_width = Style::instance().flex_x;
			requirement.flex_shrink_width = Style::instance().flex_x;
		}

		auto horizontal_flex_grow(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_width = Style::instance().flex_x;
		}

		auto horizontal_flex_shrink(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_shrink_width = Style::instance().flex_x;
		}

		auto vertical_flex(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_height = Style::instance().flex_y;
			requirement.flex_shrink_height = Style::instance().flex_y;
		}

		auto vertical_flex_grow(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_grow_height = Style::instance().flex_y;
		}

		auto vertical_flex_shrink(requirement_type& requirement) noexcept -> void
		{
			requirement.flex_shrink_height = Style::instance().flex_y;
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
}

namespace gal::prometheus::draw::element::impl
{
	[[nodiscard]] auto box_horizontal(elements_type elements) noexcept -> element_type
	{
		return make_element<HorizontalBox>(std::move(elements));
	}

	[[nodiscard]] auto box_vertical(elements_type elements) noexcept -> element_type
	{
		return make_element<VerticalBox>(std::move(elements));
	}

	[[nodiscard]] auto box_grid(element_matrix_type elements_grid) noexcept -> element_type
	{
		return make_element<GridBox>(std::move(elements_grid));
	}

	[[nodiscard]] auto flex_filler() noexcept -> element_type
	{
		return make_element<Flex>(function::flex);
	}

	[[nodiscard]] auto flex_none(element_type element) noexcept -> element_type
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

	[[nodiscard]] auto flex_horizontal(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::horizontal_flex, std::move(element));
	}

	[[nodiscard]] auto flex_horizontal_grow(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::horizontal_flex_grow, std::move(element));
	}

	[[nodiscard]] auto flex_horizontal_shrink(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::horizontal_flex_shrink, std::move(element));
	}

	[[nodiscard]] auto flex_vertical(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::vertical_flex, std::move(element));
	}

	[[nodiscard]] auto flex_vertical_grow(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::vertical_flex_grow, std::move(element));
	}

	[[nodiscard]] auto flex_vertical_shrink(element_type element) noexcept -> element_type
	{
		return make_element<Flex>(function::vertical_flex_shrink, std::move(element));
	}

	[[nodiscard]] auto center_horizontal(element_type element) noexcept -> element_type
	{
		return element::box_horizontal(flex_filler(), std::move(element), flex_filler());
	}

	[[nodiscard]] auto center_vertical(element_type element) noexcept -> element_type
	{
		return element::box_vertical(flex_filler(), std::move(element), flex_filler());
	}

	[[nodiscard]] auto center(element_type element) noexcept -> element_type
	{
		return center_horizontal(center_vertical(std::move(element)));
	}
}
