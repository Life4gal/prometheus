#pragma once

#include <draw/detail/element.box.calculate.inl>

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

using element_blocks_size = std::vector<std::reference_wrapper<element_block_size>>;

template<
	FlexBoxJustifyOption JustifyOption,
	FlexBoxAlignItemOption AlignItemOption,
	FlexBoxAlignContentOption AlignContentOption,
	std::ranges::output_range<element_block_size> Range
>
constexpr auto calculate(Range& range, const float total_width, const float total_height) noexcept -> void
{
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
				this_line.emplace_back(std::ref(block));

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
								.flex_grow = s.flex_grow ? s.flex_grow : (JustifyOption == FlexBoxJustifyOption::STRETCH) ? 1 : 0,
								.flex_shrink = s.flex_shrink,
								.size = s.size
						};
					}
				);

				const auto extra_x = 2 * container_padding_x + static_cast<float>(line.size() - 1) * container_spacing_x;
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
				// ReSharper disable once CppUseStructuredBinding
				const auto& back = line.back().get();
				auto remaining_space = total_width - back.width.position - back.width.size;

				if constexpr (JustifyOption == FlexBoxJustifyOption::FLEX_END)
				{
					std::ranges::for_each(
						line,
						[remaining_space](element_block_size& block) noexcept -> void
						{
							block.width.position += remaining_space;
						}
					);
				}
				else if constexpr (JustifyOption == FlexBoxJustifyOption::CENTER)
				{
					std::ranges::for_each(
						line,
						[remaining_space](element_block_size& block) noexcept -> void
						{
							block.width.position += remaining_space / 2;
						}
					);
				}
				else if constexpr (JustifyOption == FlexBoxJustifyOption::SPACE_BETWEEN)
				{
					using index_type = std::ranges::range_difference_t<std::decay_t<decltype(line | std::views::drop(1) | std::views::reverse | std::views::enumerate)>>;

					std::ranges::for_each(
						line | std::views::drop(1) | std::views::reverse | std::views::enumerate,
						[remaining_space, s = line.size()](std::tuple<index_type, element_block_size&> pack) mutable noexcept -> void
						{
							const auto [index, block] = pack;
							const auto i = s - index - 1;

							block.width.position += remaining_space;
							remaining_space = remaining_space * static_cast<float>(i - 1) / static_cast<float>(i);
						}
					);
				}
				else if constexpr (JustifyOption == FlexBoxJustifyOption::SPACE_AROUND)
				{
					using index_type = std::ranges::range_difference_t<std::decay_t<decltype(line | std::views::reverse | std::views::enumerate)>>;

					std::ranges::for_each(
						line | std::views::reverse | std::views::enumerate,
						[remaining_space, s = line.size()](std::tuple<index_type, element_block_size&> pack) mutable noexcept -> void
						{
							const auto [index, block] = pack;
							const auto i = s - index - 1;

							block.width.position += remaining_space * static_cast<float>(2 * i + 1) / static_cast<float>(2 * i + 2);
							remaining_space = remaining_space * static_cast<float>(2 * i) / static_cast<float>(2 * i + 2);
						}
					);
				}
				else if constexpr (JustifyOption == FlexBoxJustifyOption::SPACE_EVENLY)
				{
					using index_type = std::ranges::range_difference_t<std::decay_t<decltype(line | std::views::reverse | std::views::enumerate)>>;

					std::ranges::for_each(
						line | std::views::reverse | std::views::enumerate,
						[remaining_space, s = line.size()](std::tuple<index_type, element_block_size&> pack) mutable noexcept -> void
						{
							const auto [index, block] = pack;
							const auto i = s - index - 1;

							block.width.position += remaining_space * static_cast<float>(i + 1) / static_cast<float>(i + 2);
							remaining_space = remaining_space * static_cast<float>(i + 1) / static_cast<float>(i + 2);
						}
					);
				}
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
				const auto& min_size = std::ranges::max_element(line, {}, [](const element_block_size& block) noexcept -> auto { return block.height.min_size; })->get();
				const auto& flex_grow = std::ranges::max_element(line, {}, [](const element_block_size& block) noexcept -> auto { return block.height.flex_grow; })->get();
				const auto& flex_shrink = std::ranges::max_element(line, {}, [](const element_block_size& block) noexcept -> auto { return block.height.flex_shrink; })->get();

				return {
						.min_size = min_size.height.min_size,
						.flex_grow = flex_grow.height.flex_grow,
						.flex_shrink = flex_shrink.height.flex_shrink,
						.size = 0
				};
			}
		);

		const auto extra_y = 2 * container_padding.height + static_cast<float>(lines.size() - 1) * container_spacing.height;
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

		if constexpr (AlignContentOption == FlexBoxAlignContentOption::FLEX_END)
		{
			std::ranges::for_each(
				ys,
				[remaining_space](auto& y) noexcept -> void
				{
					y += remaining_space;
				}
			);
		}
		else if constexpr (AlignContentOption == FlexBoxAlignContentOption::CENTER)
		{
			std::ranges::for_each(
				ys,
				[remaining_space](auto& y) noexcept -> void
				{
					y += remaining_space / 2;
				}
			);
		}
		else if constexpr (AlignContentOption == FlexBoxAlignContentOption::STRETCH)
		{
			using index_type = std::ranges::range_difference_t<std::decay_t<decltype(ys | std::views::reverse | std::views::enumerate)>>;

			std::ranges::for_each(
				ys | std::views::reverse | std::views::enumerate,
				[&elements, remaining_space, s = ys.size()](std::tuple<index_type, float&> pack) mutable noexcept -> void
				{
					auto& [index, y] = pack;
					const auto i = s - index - 1;

					const auto shifted = remaining_space * static_cast<float>(i + 0) / static_cast<float>(i + 1);
					y += shifted;

					const auto consumed = remaining_space - shifted;
					elements[i].size += consumed;
					remaining_space -= consumed;
				}
			);
		}
		else if constexpr (AlignContentOption == FlexBoxAlignContentOption::SPACE_BETWEEN)
		{
			using index_type = std::ranges::range_difference_t<std::decay_t<decltype(ys | std::views::drop(1) | std::views::reverse | std::views::enumerate)>>;

			std::ranges::for_each(
				ys | std::views::drop(1) | std::views::reverse | std::views::enumerate,
				[remaining_space, s = ys.size()](std::tuple<index_type, float&> pack) mutable noexcept -> void
				{
					auto& [index, y] = pack;
					const auto i = s - index - 1;

					y += remaining_space;
					remaining_space = remaining_space * static_cast<float>(i - 1) / static_cast<float>(i);
				}
			);
		}
		else if constexpr (AlignContentOption == FlexBoxAlignContentOption::SPACE_AROUND)
		{
			using index_type = std::ranges::range_difference_t<std::decay_t<decltype(ys | std::views::reverse | std::views::enumerate)>>;

			std::ranges::for_each(
				ys | std::views::reverse | std::views::enumerate,
				[remaining_space, s = ys.size()](std::tuple<index_type, float&> pack) mutable noexcept -> void
				{
					auto& [index, y] = pack;
					const auto i = s - index - 1;

					y += remaining_space * static_cast<float>(2 * i + 1) / static_cast<float>(2 * i + 2);
					remaining_space = remaining_space * static_cast<float>(2 * i) / static_cast<float>(2 * i + 2);
				}
			);
		}
		else if constexpr (AlignContentOption == FlexBoxAlignContentOption::SPACE_EVENLY)
		{
			using index_type = std::ranges::range_difference_t<std::decay_t<decltype(ys | std::views::reverse | std::views::enumerate)>>;

			std::ranges::for_each(
				ys | std::views::reverse | std::views::enumerate,
				[remaining_space, s = ys.size()](std::tuple<index_type, float&> pack) mutable noexcept -> void
				{
					auto& [index, y] = pack;
					const auto i = s - index - 1;

					y += remaining_space * static_cast<float>(i + 1) / static_cast<float>(i + 2);
					remaining_space = remaining_space * static_cast<float>(i + 1) / static_cast<float>(i + 2);
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

						const auto stretch = height.flex_grow or (AlignItemOption == FlexBoxAlignItemOption::STRETCH);
						const auto size = stretch ? element.size : std::ranges::min(element.size, height.min_size);

						if constexpr (AlignItemOption == FlexBoxAlignItemOption::FLEX_START)
						{
							height.size = size;
							height.position = y;
						}
						else if constexpr (AlignItemOption == FlexBoxAlignItemOption::FLEX_END)
						{
							height.size = size;
							height.position = y + element.size - size;
						}
						else if constexpr (AlignItemOption == FlexBoxAlignItemOption::CENTER)
						{
							height.size = size;
							height.position = y + (element.size - size) / 2;
						}
						else if constexpr (AlignItemOption == FlexBoxAlignItemOption::STRETCH)
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

template<
	FlexBoxWrapOption WrapOption,
	FlexBoxJustifyOption JustifyOption,
	FlexBoxAlignItemOption AlignItemOption,
	FlexBoxAlignContentOption AlignContentOption,
	std::ranges::output_range<element_block_size> Range
>
constexpr auto calculate(Range& range, const float total_width, const float total_height) noexcept -> void
{
	if constexpr (WrapOption == FlexBoxWrapOption::INVERSE)
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

		calculate<JustifyOption, AlignItemOption, AlignContentOption>(range, total_width, total_height);

		symmetry_y();
	}
	else
	{
		calculate<JustifyOption, AlignItemOption, AlignContentOption>(range, total_width, total_height);
	}
}

template<
	FlexBoxDirectionOption DirectionOption,
	FlexBoxWrapOption WrapOption,
	FlexBoxJustifyOption JustifyOption,
	FlexBoxAlignItemOption AlignItemOption,
	FlexBoxAlignContentOption AlignContentOption,
	std::ranges::output_range<element_block_size> Range
>
constexpr auto calculate(Range& range, const float total_width, const float total_height) noexcept -> void
{
	if constexpr (DirectionOption == FlexBoxDirectionOption::ROW_INVERSE)
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

		calculate<WrapOption, JustifyOption, AlignItemOption, AlignContentOption>(range, total_width, total_height);

		symmetry_x();
	}
	else if constexpr (DirectionOption == FlexBoxDirectionOption::ROW)
	{
		calculate<WrapOption, JustifyOption, AlignItemOption, AlignContentOption>(range, total_width, total_height);
	}
	else
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

		if constexpr (DirectionOption == FlexBoxDirectionOption::COLUMN)
		{
			calculate<FlexBoxDirectionOption::ROW, WrapOption, JustifyOption, AlignItemOption, AlignContentOption>(range, total_height, total_width);
		}
		else if constexpr (DirectionOption == FlexBoxDirectionOption::COLUMN_INVERSE)
		{
			calculate<FlexBoxDirectionOption::ROW_INVERSE, WrapOption, JustifyOption, AlignItemOption, AlignContentOption>(range, total_height, total_width);
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}

		symmetry_xy();
	}
}
