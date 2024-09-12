// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>
#include <draw/detail/element.box.calculate.inl>

export module gal.prometheus.draw:element.box;

import std;

import gal.prometheus.primitive;
GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

import :style;
import :element.element;
import :element.flex; // for BoxGrid::fill_line

#else
#pragma once

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <draw/style.ixx>
#include <draw/detail/element.ixx>
#include <draw/detail/element.flex.ixx> // for BoxGrid::fill_line

#include <draw/detail/element.box.calculate.inl>

#endif

namespace gal::prometheus::draw
{
	namespace detail
	{
		enum class BoxOption
		{
			HORIZONTAL,
			VERTICAL,
		};

		template<typename T>
		concept box_option_t = std::is_same_v<T, BoxOption>;
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	constexpr auto box_horizontal = options<detail::BoxOption::HORIZONTAL>{};
	constexpr auto box_vertical = options<detail::BoxOption::VERTICAL>{};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		template<BoxOption Option>
		class Box final : public Element
		{
		public:
			constexpr static auto option = Option;
			constexpr static auto option_value = std::to_underlying(option);

			explicit Box(elements_type children) noexcept
				: Element{std::move(children)} {}

			Box(const Box&) noexcept = default;
			auto operator=(const Box&) noexcept -> Box& = default;
			Box(Box&&) noexcept = default;
			auto operator=(Box&&) noexcept -> Box& = default;
			~Box() noexcept override = default;

			auto calculate_requirement(Surface& surface) noexcept -> void override
			{
				requirement_.reset();

				std::ranges::for_each(
					children_,
					[this, &surface](const auto& child) noexcept -> void
					{
						child->calculate_requirement(surface);

						using functional::operators::operator|;
						if constexpr (option == BoxOption::HORIZONTAL)
						{
							requirement_.min_width += child->requirement().min_width;
							requirement_.min_height = std::ranges::max(
								requirement_.min_height,
								child->requirement().min_height
							);
						}
						else if constexpr (option == BoxOption::VERTICAL)
						{
							requirement_.min_height += child->requirement().min_height;
							requirement_.min_width = std::ranges::max(
								requirement_.min_width,
								child->requirement().min_width
							);
						}
						else
						{
							GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
						}
					}
				);

				const auto& container_padding = Style::instance().container_padding;
				const auto& container_spacing = Style::instance().container_spacing;
				const auto [extra_x, extra_y] = [&]() noexcept -> std::pair<float, float>
				{
					if constexpr (option == BoxOption::HORIZONTAL)
					{
						return std::make_pair(
							2 * container_padding.width + (children_.size() - 1) * container_spacing.width,
							2 * container_padding.height
						);
					}
					else if constexpr (option == BoxOption::VERTICAL)
					{
						return std::make_pair(
							2 * container_padding.width,
							2 * container_padding.height + (children_.size() - 1) * container_spacing.height
						);
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}();

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

						if constexpr (option == BoxOption::HORIZONTAL)
						{
							return {
									.min_size = r.min_width,
									.flex_grow = r.flex_grow_width,
									.flex_shrink = r.flex_shrink_width,
									.size = 0
							};
						}
						else if constexpr (option == BoxOption::VERTICAL)
						{
							return {
									.min_size = r.min_height,
									.flex_grow = r.flex_grow_height,
									.flex_shrink = r.flex_shrink_height,
									.size = 0
							};
						}
						else
						{
							GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
						}
					}
				);

				const auto& container_padding = Style::instance().container_padding;
				const auto& container_spacing = Style::instance().container_spacing;

				if constexpr (option == BoxOption::HORIZONTAL)
				{
					const auto extra_x = 2 * container_padding.width + (children_.size() - 1) * container_spacing.width;
					calculate(elements, rect.width() - extra_x);
				}
				else if constexpr (option == BoxOption::VERTICAL)
				{
					const auto extra_y = 2 * container_padding.height + (children_.size() - 1) * container_spacing.height;
					calculate(elements, rect.height() - extra_y);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}

				const auto& [point, extent] = rect;

				std::ranges::for_each(
					std::views::zip(children_, elements),
					[
						current_x = point.x + container_padding.width,
						current_y = point.y + container_padding.height,
						current_right = point.x + extent.width - container_padding.width,
						current_bottom = point.y + extent.height - container_padding.height,
						container_spacing
					](std::tuple<element_type&, const element_size&> pack) mutable noexcept -> void
					{
						auto& [child, element] = pack;

						if constexpr (option == BoxOption::HORIZONTAL)
						{
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
							current_x = current_x + element.size + container_spacing.width;
						}
						else if constexpr (option == BoxOption::VERTICAL)
						{
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
							current_y = current_y + element.size + container_spacing.height;
						}
						else
						{
							GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
						}
					}
				);
			}
		};

		class BoxGrid final : public Element
		{
		public:
			using element_matrix_type = std::vector<elements_type>;
			using size_type = element_matrix_type::size_type;

			using view_type = std::decay_t<decltype(std::declval<std::ranges::ref_view<elements_type>>() | std::views::chunk(1))>;
			using line_type = std::decay_t<decltype(std::declval<view_type>().begin().operator*())>;

		private:
			size_type width_;
			size_type height_;

			[[nodiscard]] constexpr static auto max_size(
				const derived_elements_t auto& r1,
				const derived_elements_t auto& r2,
				const derived_elements_t auto&... ranges
			) noexcept -> size_type
			{
				constexpr auto max = std::ranges::max(r1, r2, {}, [](const auto& r) noexcept -> auto { return std::ranges::size(r); });

				if constexpr (sizeof...(ranges) == 0)
				{
					return std::ranges::size(max);
				}
				else
				{
					return BoxGrid::max_size(max, ranges...);
				}
			}

			constexpr static auto fill_line(elements_type& dest, const size_type width, derived_elements_t auto&& elements) noexcept -> void
			{
				const auto size = std::ranges::size(elements);

				dest.reserve(dest.size() + size);
				dest.append_range(std::forward<decltype(elements)>(elements));
				if (const auto diff = width - size;
					diff != 0)
				{
					const auto filler = flex_maker{}(flex_horizontal)(flex_vertical)(flex_grow)(flex_shrink)();
					std::ranges::fill_n(std::back_inserter(dest), diff, filler);
				}
			}

			[[nodiscard]] constexpr auto make_view() noexcept -> view_type
			{
				const auto chunks = children_ | std::views::chunk(width_);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(chunks.size() == height_);
				return chunks;
			}

		public:
			explicit BoxGrid(elements_type elements, const size_type width, const size_type height) noexcept //
				: Element{std::move(elements)},
				  width_{width},
				  height_{height} {}

			explicit BoxGrid(derived_elements_t auto range, derived_elements_t auto... ranges) noexcept //
				: Element{},
				  width_{0},
				  height_{1 + sizeof...(ranges)}
			{
				if constexpr (sizeof...(ranges) == 0)
				{
					width_ = std::ranges::size(range);
				}
				else
				{
					width_ = BoxGrid::max_size(range, ranges);
				}

				BoxGrid::fill_line(children_, width_, std::move(range));
				(BoxGrid::fill_line(children_, width_, std::move(ranges)), ...);
			}

			explicit BoxGrid(element_matrix_type matrix) noexcept
				: Element{},
				  width_{0},
				  height_{matrix.size()}
			{
				width_ = std::ranges::max_element(matrix, {}, &element_matrix_type::value_type::size)->size();

				std::ranges::for_each(
					std::move(matrix),
					[this](element_matrix_type::value_type& line) noexcept -> void
					{
						BoxGrid::fill_line(children_, width_, std::move(line));
					}
				);
			}

			auto calculate_requirement(Surface& surface) noexcept -> void override
			{
				Element::calculate_requirement(surface);

				requirement_.reset();

				std::vector<float> size_x{};
				std::vector<float> size_y{};
				size_x.resize(width_);
				size_y.reserve(height_);

				const auto grid = make_view();
				std::ranges::for_each(
					grid,
					[
						&size_x,
						&size_y
					](const line_type& line) noexcept -> void
					{
						const auto view = line | std::views::transform([](const auto& element) noexcept -> const auto& { return element->requirement(); });
						const auto& max_y = std::ranges::max_element(view, {}, &requirement_type::min_height);

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
				const auto extra_x = 2 * container_padding.width + static_cast<float>(width_ - 1) * container_spacing.width;
				const auto extra_y = 2 * container_padding.height + static_cast<float>(height_ - 1) * container_spacing.height;

				requirement_.min_width = integrate(size_x) + extra_x;
				requirement_.min_height = integrate(size_y) + extra_y;
			}

			auto set_rect(const rect_type& rect) noexcept -> void override
			{
				Element::set_rect(rect);

				std::vector<element_size> elements_x{width_, {.min_size = 0, .flex_grow = std::numeric_limits<float>::max(), .flex_shrink = std::numeric_limits<float>::max(), .size = 0}};
				std::vector<element_size> elements_y{height_, {.min_size = 0, .flex_grow = std::numeric_limits<float>::max(), .flex_shrink = std::numeric_limits<float>::max(), .size = 0}};

				auto grid = make_view();
				std::ranges::for_each(
					std::views::zip(elements_y, grid),
					[&elements_x](std::tuple<element_size&, const line_type&> pack) noexcept -> void
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
				const auto extra_x = 2 * container_padding.width + static_cast<float>(width_ - 1) * container_spacing.width;
				const auto extra_y = 2 * container_padding.height + static_cast<float>(height_ - 1) * container_spacing.height;

				calculate(elements_x, rect.width() - extra_x);
				calculate(elements_y, rect.height() - extra_y);

				const auto& [point, extent] = rect;

				std::ranges::for_each(
					std::views::zip(elements_y, grid),
					[
						&elements_x ,
						x = point.x + container_padding.width,
						y = point.y + container_padding.height,
						container_spacing = container_spacing
					](const std::tuple<const element_size&, const line_type&>& pack) mutable noexcept -> void
					{
						auto& [element_y, line] = pack;

						std::ranges::for_each(
							std::views::zip(elements_x, line),
							[
								current_x = x,
								current_y = y,
								current_height = element_y.size,
								container_spacing
							](const std::tuple<const element_size&, element_type&>& inner_pack) mutable noexcept -> void
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
		};

		template<BoxOption Option>
		struct box_builder
		{
			[[nodiscard]] constexpr auto operator()(elements_type elements) const noexcept -> element_type
			{
				return make_element<Box<Option>>(std::move(elements));
			}

			[[nodiscard]] constexpr auto operator()(derived_element_t auto... elements) const noexcept -> element_type
			{
				elements_type es{};
				es.reserve(sizeof...(elements));

				(es.emplace_back(std::move(elements)), ...);

				return this->operator()(std::move(es));
			}
		};

		struct box_grid_builder
		{
			using size_type = BoxGrid::size_type;

			size_type width;
			size_type height;

			[[nodiscard]] auto operator()(elements_type elements) const noexcept -> element_type
			{
				return make_element<BoxGrid>(std::move(elements), width, height);
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<box_option_t auto... Os>
		struct element_maker<Os...>
		{
			template<BoxOption Option>
			[[nodiscard]] auto operator()(options<Option>) const noexcept -> box_builder<Option>
			{
				return {};
			}

			template<BoxOption Option, typename... Args>
			[[nodiscard]] auto operator()(options<Option> option, Args&&... args) const noexcept -> auto //
			{
				return this->operator()(option)(std::forward<Args>(args)...);
			}

			[[nodiscard]] auto operator()(const box_grid_builder::size_type width, const box_grid_builder::size_type height) const noexcept -> box_grid_builder
			{
				return {.width = width, .height = height};
			}

			template<typename... Args>
			[[nodiscard]] auto operator()(const box_grid_builder::size_type width, const box_grid_builder::size_type height, Args&&... args) const noexcept -> auto //
			{
				return this->operator()(width, height)(std::forward<Args>(args)...);
			}

			[[nodiscard]] auto operator()(BoxGrid::element_matrix_type matrix) const noexcept -> element_type
			{
				return make_element<BoxGrid>(std::move(matrix));
			}

			[[nodiscard]] auto operator()(derived_elements_t auto... elements) const noexcept -> element_type
			{
				return make_element<BoxGrid>(std::move(elements)...);
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
