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
import gal.prometheus.functional;
// GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

import :surface;
import :style;
import :element;

#else
#include <optional>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>
#include <functional/functional.ixx>
#include <draw/surface.ixx>
#include <draw/style.ixx>
#include <draw/element.ixx>
// #include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

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

	struct element_block_size
	{
		struct size_type
		{
			float min_size = 0;
			float flex_grow = 0;
			float flex_shrink = 0;

			float size = 0;
			float position = 0;
		};

		size_type width = {};
		size_type height = {};

		// _________________________
		// | L0B1	L0B2		L0B3	   |
		// | L1B1  		L1B2	           |
		// |		L2B1		               |
		// |				                   |
		// |________________________|
		// 
		// L0B2 => 
		// placed_line = 0
		// placed_position = 1
		std::uint32_t placed_line = 0;
		std::uint32_t placed_position = 0;
	};

	using element_blocks_size = std::vector<element_block_size*>;

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

	template<std::underlying_type_t<element::impl::FlexBoxOption> OptionValue, std::ranges::output_range<element_block_size> Range>
	constexpr auto calculate_justify(Range& range, const float total_width, const float total_height) noexcept -> void
	{
		using option_type = element::impl::FlexBoxOption;

		const auto& container_padding = Style::instance().container_padding;
		const auto& container_spacing = Style::instance().container_spacing;

		std::vector<element_blocks_size> lines{};

		// layout all elements into rows
		{
			element_blocks_size this_line{};
			std::ranges::for_each(
				range,
				[
					&lines,
					&this_line,
					total_width,
					current_x = container_padding.width,
					container_padding_x = container_padding.width,
					container_spacing_x = container_spacing.width
				](element_block_size& block) mutable noexcept -> void
				{
					if (current_x + block.width.min_size > total_width)
					{
						current_x = container_padding_x;
						lines.emplace_back(std::exchange(this_line, element_blocks_size{}));
					}

					block.placed_line = static_cast<std::uint32_t>(lines.size());
					block.placed_position = static_cast<std::uint32_t>(this_line.size());
					this_line.emplace_back(std::addressof(block));

					current_x += block.width.min_size + container_spacing_x;
				}
			);
			if (not this_line.empty())
			{
				lines.emplace_back(std::move(this_line));
			}
		}

		// set position on the X axis
		{
			std::ranges::for_each(
				lines,
				[
					total_width,
					container_padding_x = container_padding.width,
					container_spacing_x = container_spacing.width
				](element_blocks_size& line) noexcept -> void
				{
					std::vector<element_size> elements{};
					elements.reserve(line.size());

					std::ranges::transform(
						line,
						std::back_inserter(elements),
						[](const element_block_size& block) noexcept -> element_size
						{
							// ReSharper disable once CppUseStructuredBinding
							const auto& s = block.width;

							return {
									.min_size = s.min_size,
									.flex_grow =
									s.flex_grow ? s.flex_grow : (OptionValue & std::to_underlying(option_type::JUSTIFY_STRETCH)) ? 1 : 0,
									.flex_shrink = s.flex_shrink,
									.size = s.size
							};
						}
					);

					const auto extra_x = 2 * container_padding_x + (line.size() - 1) * container_spacing_x;
					calculate(elements, total_width - extra_x);

					std::ranges::for_each(
						std::views::zip(line, elements),
						[
							current_x = container_padding_x,
							container_spacing_x
						](std::tuple<element_block_size&, const element_size&> pack) mutable noexcept -> void
						{
							auto& [block, element] = pack;

							block.width.size = element.size;
							block.width.position = current_x;

							current_x = current_x + element.size + container_spacing_x;
						}
					);
				}
			);
		}

		// distribute remaining space
		{
			std::ranges::for_each(
				lines,
				[total_width](element_blocks_size& line) noexcept -> void
				{
					const auto& back = line.back();
					auto remaining_space = total_width - back->width.position - back->width.size;

					if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_FLEX_END))
					{
						std::ranges::for_each(
							line,
							[remaining_space](element_block_size& block) noexcept -> void
							{
								block.width.position += remaining_space;
							}
						);
					}
					else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_CENTER))
					{
						std::ranges::for_each(
							line,
							[remaining_space](element_block_size& block) noexcept -> void
							{
								block.width.position += remaining_space / 2;
							}
						);
					}
					else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_SPACE_BETWEEN))
					{
						std::ranges::for_each(
							line | std::views::drop(1) | std::views::reverse | std::views::enumerate,
							[remaining_space, s = line.size()](std::tuple_element_t<std::ptrdiff_t, element_block_size&> pack) mutable noexcept -> void
							{
								const auto [index, block] = pack;
								const auto i = s - index - 1;

								block.width.position += remaining_space;
								remaining_space = remaining_space * (i - 1) / i;
							}
						);
					}
					else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_SPACE_AROUND))
					{
						std::ranges::for_each(
							line | std::views::reverse | std::views::enumerate,
							[remaining_space, s = line.size()](std::tuple_element_t<std::ptrdiff_t, element_block_size&> pack) mutable noexcept -> void
							{
								const auto [index, block] = pack;
								const auto i = s - index - 1;

								block.width.position += remaining_space * (2 * i + 1) / (2 * i + 2);
								remaining_space = remaining_space * (2 * i) / (2 * i + 2);
							}
						);
					}
					else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_SPACE_EVENLY))
					{
						std::ranges::for_each(
							line | std::views::reverse | std::views::enumerate,
							[remaining_space, s = line.size()](std::tuple_element_t<std::ptrdiff_t, element_block_size&> pack) mutable noexcept -> void
							{
								const auto [index, block] = pack;
								const auto i = s - index - 1;

								block.width.position += remaining_space * (i + 1) / (i + 2);
								remaining_space = remaining_space * (i + 1) / (i + 2);
							}
						);
					}
					else {}
				}
			);
		}

		// set position on the Y axis
		{
			std::vector<element_size> elements{};
			elements.reserve(lines.size());

			std::ranges::transform(
				lines,
				std::back_inserter(elements),
				[](const element_blocks_size& line) noexcept -> element_size
				{
					const auto min_size = std::ranges::max_element(line, {}, [](const element_block_size& block) noexcept -> auto { return block.height.min_size; });
					const auto flex_grow = std::ranges::max_element(line, {}, [](const element_block_size& block) noexcept -> auto { return block.height.flex_grow; });
					const auto flex_shrink = std::ranges::max_element(line, {}, [](const element_block_size& block) noexcept -> auto { return block.height.flex_shrink; });

					return {
							.min_size = min_size.operator*()->height.min_size,
							.flex_grow = flex_grow.operator*()->height.flex_grow,
							.flex_shrink = flex_shrink.operator*()->height.flex_shrink,
							.size = 0
					};
				}
			);

			const auto extra_y = 2 * container_padding.height + (lines.size() - 1) * container_spacing.height;
			calculate(elements, total_height - extra_y);

			std::vector<float> ys{};
			ys.reserve(elements.size());

			// align content
			auto current_y = container_padding.height;
			std::ranges::transform(
				elements,
				std::back_inserter(ys),
				[
					&current_y,
					container_spacing_y = container_spacing.height
				](const element_size& element) noexcept -> float
				{
					return std::exchange(current_y, current_y + element.size + container_spacing_y);
				}
			);

			auto remaining_space = std::ranges::max(0.f, total_height - current_y);

			if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_FLEX_END))
			{
				std::ranges::for_each(
					ys,
					[remaining_space](auto& y) noexcept -> void
					{
						y += remaining_space;
					}
				);
			}
			else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_CENTER))
			{
				std::ranges::for_each(
					ys,
					[remaining_space](auto& y) noexcept -> void
					{
						y += remaining_space / 2;
					}
				);
			}
			else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_STRETCH))
			{
				std::ranges::for_each(
					ys | std::views::reverse | std::views::enumerate,
					[&elements, remaining_space, s = ys.size()](std::tuple_element_t<std::ptrdiff_t, float&> pack) mutable noexcept -> void
					{
						auto& [index, y] = pack;
						const auto i = s - index - 1;

						const auto shifted = remaining_space * (i + 0) / (i + 1);
						y += shifted;

						const auto consumed = remaining_space - shifted;
						elements[i].size += consumed;
						remaining_space -= consumed;
					}
				);
			}
			else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_SPACE_BETWEEN))
			{
				std::ranges::for_each(
					ys | std::views::drop(1) | std::views::reverse | std::views::enumerate,
					[remaining_space, s = ys.size()](std::tuple_element_t<std::ptrdiff_t, float&> pack) mutable noexcept -> void
					{
						auto& [index, y] = pack;
						const auto i = s - index - 1;

						y += remaining_space;
						remaining_space = remaining_space * (i - 1) / i;
					}
				);
			}
			else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_SPACE_AROUND))
			{
				std::ranges::for_each(
					ys | std::views::reverse | std::views::enumerate,
					[remaining_space, s = ys.size()](std::tuple_element_t<std::ptrdiff_t, float&> pack) mutable noexcept -> void
					{
						auto& [index, y] = pack;
						const auto i = s - index - 1;

						y += remaining_space * (2 * i + 1) / (2 * i + 2);
						remaining_space = remaining_space * (2 * i) / (2 * i + 2);
					}
				);
			}
			else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_SPACE_EVENLY))
			{
				std::ranges::for_each(
					ys | std::views::reverse | std::views::enumerate,
					[remaining_space, s = ys.size()](std::tuple_element_t<std::ptrdiff_t, float&> pack) mutable noexcept -> void
					{
						auto& [index, y] = pack;
						const auto i = s - index - 1;

						y += remaining_space * (i + 1) / (i + 2);
						remaining_space = remaining_space * (i + 1) / (i + 2);
					}
				);
			}

			// align items
			std::ranges::for_each(
				std::views::zip(lines, elements, ys),
				[](std::tuple<element_blocks_size&, const element_size&, const float&> pack) noexcept -> void
				{
					auto& [line, element, y] = pack;
					std::ranges::for_each(
						line,
						[&element, y](element_block_size& block) noexcept -> void
						{
							// ReSharper disable once CppUseStructuredBinding
							auto& height = block.height;

							const auto stretch = height.flex_grow or (OptionValue & std::to_underlying(option_type::JUSTIFY_STRETCH));
							const auto size = stretch ? element.size : std::ranges::min(element.size, height.min_size);


							if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_FLEX_START))
							{
								height.size = size;
								height.position = y;
							}
							else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_FLEX_END))
							{
								height.size = size;
								height.position = y + element.size - size;
							}
							else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_CENTER))
							{
								height.size = size;
								height.position = y + (element.size - size) / 2;
							}
							else if constexpr (OptionValue & std::to_underlying(option_type::JUSTIFY_STRETCH))
							{
								height.size = element.size;
								height.position = y;
							}
						}
					);
				}
			);
		}
	}

	template<std::underlying_type_t<element::impl::FlexBoxOption> OptionValue, std::ranges::output_range<element_block_size> Range>
	constexpr auto calculate_wrap(Range& range, const float total_width, const float total_height) noexcept -> void
	{
		using option_type = element::impl::FlexBoxOption;

		if constexpr (OptionValue & std::to_underlying(option_type::WRAP_INVERSE))
		{
			const auto symmetry_y = [&range, total_height]() noexcept -> void
			{
				std::ranges::for_each(
					range,
					[total_height](element_block_size& block) noexcept -> void
					{
						block.height.position = (total_height - block.height.size) / 2;
					}
				);
			};

			symmetry_y();

			constexpr auto value = OptionValue & ~std::to_underlying(option_type::WRAP_INVERSE) | std::to_underlying(option_type::WRAP_DEFAULT);
			calculate_justify<value>(range, total_width, total_height);

			symmetry_y();
		}
		else
		{
			calculate_justify<OptionValue>(range, total_width, total_height);
		}
	}

	template<std::underlying_type_t<element::impl::FlexBoxOption> OptionValue, std::ranges::output_range<element_block_size> Range>
	constexpr auto calculate_direction(Range& range, const float total_width, const float total_height) noexcept -> void
	{
		using option_type = element::impl::FlexBoxOption;

		if constexpr (OptionValue & std::to_underlying(option_type::DIRECTION_ROW_INVERSE))
		{
			const auto symmetry_x = [&range, total_width]() noexcept -> void
			{
				std::ranges::for_each(
					range,
					[total_width](element_block_size& block) noexcept -> void
					{
						block.width.position = (total_width - block.width.size) / 2;
					}
				);
			};

			symmetry_x();

			constexpr auto value = OptionValue & ~std::to_underlying(option_type::DIRECTION_ROW_INVERSE) | std::to_underlying(option_type::DIRECTION_ROW);
			calculate_wrap<value>(range, total_width, total_height);

			symmetry_x();
		}
		else if constexpr (OptionValue & std::to_underlying(option_type::DIRECTION_ROW))
		{
			calculate_wrap<OptionValue>(range, total_width, total_height);
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<std::underlying_type_t<element::impl::FlexBoxOption> OptionValue, std::ranges::output_range<element_block_size> Range>
	constexpr auto calculate(Range& range, const float total_width, const float total_height) noexcept -> void
	{
		using option_type = element::impl::FlexBoxOption;

		if constexpr (
			(OptionValue & std::to_underlying(option_type::DIRECTION_COLUMN)) or
			(OptionValue & std::to_underlying(option_type::DIRECTION_COLUMN_INVERSE))
		)
		{
			const auto symmetry_xy = [&range]() noexcept -> void
			{
				std::ranges::for_each(
					range,
					[](element_block_size& block) noexcept -> void
					{
						using std::swap;

						swap(block.width, block.height);
					}
				);
			};

			symmetry_xy();

			if constexpr (OptionValue & std::to_underlying(option_type::DIRECTION_COLUMN))
			{
				constexpr auto value = OptionValue & ~std::to_underlying(option_type::DIRECTION_COLUMN) | std::to_underlying(option_type::DIRECTION_ROW);
				calculate_direction<value>(range, total_height, total_width);
			}
			else if constexpr (OptionValue & std::to_underlying(option_type::DIRECTION_COLUMN_INVERSE))
			{
				constexpr auto value = OptionValue & ~std::to_underlying(option_type::DIRECTION_COLUMN_INVERSE) | std::to_underlying(option_type::DIRECTION_ROW_INVERSE);
				calculate_direction<value>(range, total_height, total_width);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}

			symmetry_xy();
		}
		else if constexpr (
			(OptionValue & std::to_underlying(option_type::DIRECTION_ROW)) or
			(OptionValue & std::to_underlying(option_type::DIRECTION_ROW_INVERSE))
		)
		{
			calculate_direction<OptionValue>(range, total_width, total_height);
		}
	}

	template<element::impl::BoxOption Option>
		requires (Option == element::impl::BoxOption::HORIZONTAL or Option == element::impl::BoxOption::VERTICAL)
	class Box final : public impl::Element
	{
	public:
		using option_type = element::impl::BoxOption;

		constexpr static auto option = Option;
		constexpr static auto option_value = std::to_underlying(option);

		explicit Box(elements_type children) noexcept
			: Element{std::move(children)} {}

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
					if constexpr (option == option_type::HORIZONTAL)
					{
						requirement_.min_width += child->requirement().min_width;
						requirement_.min_height = std::ranges::max(
							requirement_.min_height,
							child->requirement().min_height
						);
					}
					else if constexpr (option == element::impl::BoxOption::VERTICAL)
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
				if constexpr (option == option_type::HORIZONTAL)
				{
					return std::make_pair(
						2 * container_padding.width + (children_.size() - 1) * container_spacing.width,
						2 * container_padding.height
					);
				}
				else if constexpr (option == element::impl::BoxOption::VERTICAL)
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

					if constexpr (option == option_type::HORIZONTAL)
					{
						return {
								.min_size = r.min_width,
								.flex_grow = r.flex_grow_width,
								.flex_shrink = r.flex_shrink_width,
								.size = 0
						};
					}
					else if constexpr (option == element::impl::BoxOption::VERTICAL)
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

			if constexpr (option == option_type::HORIZONTAL)
			{
				const auto extra_x = 2 * container_padding.width + (children_.size() - 1) * container_spacing.width;
				calculate(elements, rect.width() - extra_x);
			}
			else if constexpr (option == element::impl::BoxOption::VERTICAL)
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

					if constexpr (option == option_type::HORIZONTAL)
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
					else if constexpr (option == element::impl::BoxOption::VERTICAL)
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

	template<element::impl::FlexBoxOption Option>
	class FlexBox final : public impl::Element
	{
	public:
		using option_type = element::impl::FlexBoxOption;

		constexpr static auto option = Option;
		constexpr static auto option_value = std::to_underlying(option);
		constexpr static auto option_normalized_value = element::impl::options<
			(
				option_value & std::to_underlying(option_type::DIRECTION_ROW) or
				option_value & std::to_underlying(option_type::DIRECTION_ROW_INVERSE)
			)
				? option_type::DIRECTION_ROW
				: option_type::DIRECTION_COLUMN,
			option_type::WRAP_DEFAULT,
			option_type::JUSTIFY_FLEX_START
		>{}.value;

	private:
		template<std::ranges::output_range<element_block_size> Range>
		auto calculate_layout(Range& range, const float total_width, const float total_height, bool calculate_requirement) noexcept -> void
		{
			range.reserve(children_.size());

			std::ranges::transform(
				children_,
				std::back_inserter(range),
				[calculate_requirement](const auto& child) noexcept -> element_block_size
				{
					const auto& requirement = child->requirement();

					return {
							.width = {
									.size = {
											.min_size = requirement.min_width,
											.flex_grow = calculate_requirement ? 0 : requirement.flex_grow_width,
											.flex_shrink = calculate_requirement ? 0 : requirement.flex_shrink_width,
											.size = 0,
									},
									.position = 0,
							},
							.height = {
									.size = {
											.min_size = requirement.min_height,
											.flex_grow = calculate_requirement ? 0 : requirement.flex_grow_height,
											.flex_shrink = calculate_requirement ? 0 : requirement.flex_shrink_height,
									},
									.position = 0,
							},
							.placed_line = 0,
							.placed_position = 0,
					};
				}
			);

			calculate(range, total_width, total_height);
		}

	public:
		explicit FlexBox(elements_type children) noexcept
			: Element{std::move(children)}
		{
			if constexpr (
				(option_value & std::to_underlying(option_type::DIRECTION_ROW)) or
				(option_value & std::to_underlying(option_type::DIRECTION_ROW_INVERSE))
			)
			{
				requirement_.flex_grow_width = 1;
			}
			else if constexpr (
				(option_value & std::to_underlying(option_type::DIRECTION_COLUMN)) or
				(option_value & std::to_underlying(option_type::DIRECTION_COLUMN_INVERSE))
			)
			{
				requirement_.flex_grow_height = 1;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		~FlexBox() noexcept override = default;

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			Element::calculate_requirement(surface);
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

	template<element::impl::FixedOption Option>
	struct fixed_value;

	template<element::impl::FixedOption Option>
		requires (
			((std::to_underlying(Option) & std::to_underlying(element::impl::FixedOption::WIDTH)) != 0) and
			((std::to_underlying(Option) & std::to_underlying(element::impl::FixedOption::HEIGHT)) == 0)
		)
	struct fixed_value<Option>
	{
		float width;
	};

	template<element::impl::FixedOption Option>
		requires (
			((std::to_underlying(Option) & std::to_underlying(element::impl::FixedOption::WIDTH)) == 0) and
			((std::to_underlying(Option) & std::to_underlying(element::impl::FixedOption::HEIGHT)) != 0)
		)
	struct fixed_value<Option>
	{
		float height;
	};

	template<element::impl::FixedOption Option>
		requires (
			((std::to_underlying(Option) & std::to_underlying(element::impl::FixedOption::WIDTH)) != 0) and
			((std::to_underlying(Option) & std::to_underlying(element::impl::FixedOption::HEIGHT)) != 0)
		)
	struct fixed_value<Option>
	{
		float width;
		float height;
	};

	template<element::impl::FixedOption Option>
	class Fixed final : public impl::Element
	{
	public:
		using option_type = element::impl::FixedOption;
		using value_type = fixed_value<Option>;

		constexpr static auto option = Option;
		constexpr static auto option_value = std::to_underlying(option);

	private:
		value_type value_;

	public:
		Fixed(const value_type value, element_type element) noexcept
			: Element{elements_type{std::move(element)}},
			  value_{value} {}

		~Fixed() noexcept override = default;

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			Element::calculate_requirement(surface);

			requirement_ = children_[0]->requirement();
			if constexpr (option_value & std::to_underlying(option_type::WIDTH))
			{
				requirement_.flex_grow_width = 0;
				requirement_.flex_shrink_width = 0;

				if constexpr (option_value & std::to_underlying(option_type::LESS_THAN))
				{
					requirement_.min_width = std::ranges::min(requirement_.min_width, value_.width);
				}
				else if constexpr (option_value & std::to_underlying(option_type::EQUAL))
				{
					requirement_.min_width = value_.width;
				}
				else if constexpr (option_value & std::to_underlying(option_type::GREATER_THAN))
				{
					requirement_.min_width = std::ranges::max(requirement_.min_width, value_.width);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			if constexpr (option_value & std::to_underlying(option_type::HEIGHT))
			{
				requirement_.flex_grow_height = 0;
				requirement_.flex_shrink_height = 0;

				if constexpr (option_value & std::to_underlying(option_type::LESS_THAN))
				{
					requirement_.min_height = std::ranges::min(requirement_.min_height, value_.height);
				}
				else if constexpr (option_value & std::to_underlying(option_type::EQUAL))
				{
					requirement_.min_height = value_.height;
				}
				else if constexpr (option_value & std::to_underlying(option_type::GREATER_THAN))
				{
					requirement_.min_height = std::ranges::max(requirement_.min_height, value_.height);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
		}

		auto set_rect(const rect_type& rect) noexcept -> void override
		{
			Element::set_rect(rect);

			auto box = rect;
			if constexpr (
				(option_value & std::to_underlying(option_type::LESS_THAN)) or
				(option_value & std::to_underlying(option_type::EQUAL))
			)
			{
				if constexpr (option_value & std::to_underlying(option_type::WIDTH))
				{
					box.extent.width = std::ranges::min(box.extent.width, value_.width);
				}

				if constexpr (option_value & std::to_underlying(option_type::HEIGHT))
				{
					box.extent.height = std::ranges::min(box.extent.height, value_.height);
				}
			}

			children_[0]->set_rect(box);
		}
	};

	template<element::impl::FlexOption Option>
	class Flex final : public impl::Element
	{
	public:
		using option_type = element::impl::FlexOption;

		constexpr static auto option = Option;
		constexpr static auto option_value = std::to_underlying(option);

		explicit Flex() noexcept
			: Element{} {}

		explicit Flex(element_type element) noexcept
			: Element{elements_type{std::move(element)}} {}

		~Flex() noexcept override = default;

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			requirement_.reset();

			if (not children_.empty())
			[[likely]]
			{
				children_[0]->calculate_requirement(surface);
				requirement_ = children_[0]->requirement();
			}

			if constexpr (option == option_type::NONE)
			{
				requirement_.flex_grow_width = 0;
				requirement_.flex_grow_height = 0;
				requirement_.flex_shrink_width = 0;
				requirement_.flex_shrink_height = 0;
			}
			else
			{
				constexpr auto option_value = std::to_underlying(option);

				constexpr auto horizontal = option_value & std::to_underlying(option_type::HORIZONTAL);
				constexpr auto vertical = option_value & std::to_underlying(option_type::VERTICAL);

				constexpr auto grow = option_value & std::to_underlying(option_type::GROW);
				constexpr auto shrink = option_value & std::to_underlying(option_type::SHRINK);

				const auto set_gw = [this]() noexcept -> void { requirement_.flex_grow_width = 1; };
				const auto set_gh = [this]() noexcept -> void { requirement_.flex_grow_height = 1; };
				const auto set_sw = [this]() noexcept -> void { requirement_.flex_shrink_width = 1; };
				const auto set_sh = [this]() noexcept -> void { requirement_.flex_shrink_height = 1; };

				if constexpr (grow)
				{
					if constexpr (horizontal) { set_gw(); }
					if constexpr (vertical) { set_gh(); }
				}
				if constexpr (shrink)
				{
					if constexpr (horizontal) { set_sw(); }
					if constexpr (vertical) { set_sh(); }
				}
			}
		}

		auto set_rect(const rect_type& rect) noexcept -> void override
		{
			Element::set_rect(rect);

			if (not children_.empty())
			[[likely]]
			{
				children_[0]->set_rect(rect);
			}
		}
	};
}

namespace gal::prometheus::draw::element::impl
{
	[[nodiscard]] auto box_horizontal(elements_type elements) noexcept -> element_type
	{
		return make_element<Box<static_cast<BoxOption>(option_box_horizontal.value)>>(std::move(elements));
	}

	[[nodiscard]] auto box_vertical(elements_type elements) noexcept -> element_type
	{
		return make_element<Box<static_cast<BoxOption>(option_box_vertical.value)>>(std::move(elements));
	}

	[[nodiscard]] auto box_flex(elements_type elements_grid) noexcept -> element_type
	{
		// todo
		return nullptr;
	}

	[[nodiscard]] auto box_grid(element_matrix_type elements_grid) noexcept -> element_type
	{
		return make_element<GridBox>(std::move(elements_grid));
	}

	[[nodiscard]] auto fixed_less_than(const float width, const float height, element_type element) noexcept -> element_type
	{
		using fixed_type = Fixed<static_cast<FixedOption>(option_fixed_less_than.value)>;
		return make_element<fixed_type>(fixed_type::value_type{width, height}, std::move(element));
	}

	[[nodiscard]] auto fixed_width_less_than(const float width, element_type element) noexcept -> element_type
	{
		using fixed_type = Fixed<static_cast<FixedOption>(option_fixed_width_less_than.value)>;
		return make_element<fixed_type>(fixed_type::value_type{width}, std::move(element));
	}

	[[nodiscard]] auto fixed_height_less_than(const float height, element_type element) noexcept -> element_type
	{
		using fixed_type = Fixed<static_cast<FixedOption>(option_fixed_height_less_than.value)>;
		return make_element<fixed_type>(fixed_type::value_type{height}, std::move(element));
	}

	[[nodiscard]] auto fixed_equal(const float width, const float height, element_type element) noexcept -> element_type
	{
		using fixed_type = Fixed<static_cast<FixedOption>(option_fixed_equal.value)>;
		return make_element<fixed_type>(fixed_type::value_type{width, height}, std::move(element));
	}

	[[nodiscard]] auto fixed_width_equal(const float width, element_type element) noexcept -> element_type
	{
		using fixed_type = Fixed<static_cast<FixedOption>(option_fixed_width_equal.value)>;
		return make_element<fixed_type>(fixed_type::value_type{width}, std::move(element));
	}

	[[nodiscard]] auto fixed_height_equal(const float height, element_type element) noexcept -> element_type
	{
		using fixed_type = Fixed<static_cast<FixedOption>(option_fixed_height_equal.value)>;
		return make_element<fixed_type>(fixed_type::value_type{height}, std::move(element));
	}

	[[nodiscard]] auto fixed_greater_than(const float width, const float height, element_type element) noexcept -> element_type
	{
		using fixed_type = Fixed<static_cast<FixedOption>(option_fixed_greater_than.value)>;
		return make_element<fixed_type>(fixed_type::value_type{width, height}, std::move(element));
	}

	[[nodiscard]] auto fixed_width_greater_than(const float width, element_type element) noexcept -> element_type
	{
		using fixed_type = Fixed<static_cast<FixedOption>(option_fixed_width_greater_than.value)>;
		return make_element<fixed_type>(fixed_type::value_type{width}, std::move(element));
	}

	[[nodiscard]] auto fixed_height_greater_than(const float height, element_type element) noexcept -> element_type
	{
		using fixed_type = Fixed<static_cast<FixedOption>(option_fixed_height_greater_than.value)>;
		return make_element<fixed_type>(fixed_type::value_type{height}, std::move(element));
	}

	[[nodiscard]] auto flex_filler() noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_all.value)>>();
	}

	[[nodiscard]] auto flex_none(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_none.value)>>(std::move(element));
	}

	[[nodiscard]] auto flex(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_all.value)>>(std::move(element));
	}

	[[nodiscard]] auto flex_grow(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_grow.value)>>(std::move(element));
	}

	[[nodiscard]] auto flex_shrink(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_shrink.value)>>(std::move(element));
	}

	[[nodiscard]] auto flex_horizontal(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_horizontal.value)>>(std::move(element));
	}

	[[nodiscard]] auto flex_vertical(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_vertical.value)>>(std::move(element));
	}

	[[nodiscard]] auto flex_horizontal_grow(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_horizontal_grow.value)>>(std::move(element));
	}

	[[nodiscard]] auto flex_horizontal_shrink(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_horizontal_shrink.value)>>(std::move(element));
	}

	[[nodiscard]] auto flex_vertical_grow(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_vertical_grow.value)>>(std::move(element));
	}

	[[nodiscard]] auto flex_vertical_shrink(element_type element) noexcept -> element_type
	{
		return make_element<Flex<static_cast<FlexOption>(option_flex_vertical_shrink.value)>>(std::move(element));
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
