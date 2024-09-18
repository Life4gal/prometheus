// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.flex_box;

import std;

import gal.prometheus.primitive;

import :style;
import :element.element;

#else
#pragma once

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>

#include <draw/style.ixx>
#include <draw/detail/element.ixx>

#endif

namespace gal::prometheus::draw
{
	namespace detail::element
	{
		enum class FlexBoxDirectionOption
		{
			// Flex items are laid out in a row
			ROW,
			// Flex items are laid out in a row, but in reverse order
			ROW_INVERSE,
			// Flex items are laid out in a column
			COLUMN,
			// Flex items are laid out in a column, but in reverse order
			COLUMN_INVERSE,
		};

		enum class FlexBoxWrapOption
		{
			// Flex items will wrap onto multiple lines
			DEFAULT,
			// Flex items will all try to fit onto one line
			NO_WRAP,
			// Flex items will wrap onto multiple lines, but in reverse order
			INVERSE,
		};

		enum class FlexBoxJustifyOption
		{
			// Items are aligned to the start of FLEX_BOX's direction
			FLEX_START,
			// Items are aligned to the end of FLEX_BOX's direction
			FLEX_END,
			// Items are centered along the line
			CENTER,
			// Items are stretched to fill the line
			STRETCH,
			// Items are evenly distributed in the line, first item is on the start line, last item on the end line
			SPACE_BETWEEN,
			// Items are evenly distributed in the line with equal space around them
			SPACE_AROUND,
			// Items are distributed so that the spacing between any two items (and the space to the edges) is equal
			SPACE_EVENLY,
		};

		enum class FlexBoxAlignItemOption
		{
			// Items are placed at the start of the cross axis
			FLEX_START,
			// Items are placed at the end of the cross axis
			FLEX_END,
			// Items are centered along the cross axis
			CENTER,
			// Items are stretched to fill the cross axis
			STRETCH,
		};

		enum class FlexBoxAlignContentOption
		{
			// Items are placed at the start of the cross axis
			FLEX_START,
			// Items are placed at the end of the cross axis
			FLEX_END,
			// Items are centered along the cross axis
			CENTER,
			// Items are stretched to fill the cross axis
			STRETCH,
			// Items are evenly distributed in the cross axis
			SPACE_BETWEEN,
			// Items evenly distributed with equal space around each line
			SPACE_AROUND,
			// Items are evenly distributed in the cross axis with equal space around them
			SPACE_EVENLY,
		};

		template<typename T>
		concept flex_box_direction_option_t = std::is_same_v<T, FlexBoxDirectionOption>;

		template<typename T>
		concept flex_box_wrap_option_t = std::is_same_v<T, FlexBoxWrapOption>;

		template<typename T>
		concept flex_box_justify_option_t = std::is_same_v<T, FlexBoxJustifyOption>;

		template<typename T>
		concept flex_box_align_item_option_t = std::is_same_v<T, FlexBoxAlignItemOption>;

		template<typename T>
		concept flex_box_align_content_option_t = std::is_same_v<T, FlexBoxAlignContentOption>;

		template<typename T>
		concept flex_box_direction_or_wrap_or_justify_or_align_item_or_align_content_option_t =
				flex_box_direction_option_t<T> or
				flex_box_wrap_option_t<T> or
				flex_box_justify_option_t<T> or
				flex_box_align_item_option_t<T> or
				flex_box_align_content_option_t<T>;

		struct flex_box_options
		{
			options<FlexBoxDirectionOption::ROW> direction_row{};
			options<FlexBoxDirectionOption::ROW_INVERSE> direction_row_inverse{};
			options<FlexBoxDirectionOption::COLUMN> direction_column{};
			options<FlexBoxDirectionOption::COLUMN_INVERSE> direction_column_inverse{};

			options<FlexBoxWrapOption::DEFAULT> wrap_default{};
			options<FlexBoxWrapOption::NO_WRAP> wrap_no_wrap{};
			options<FlexBoxWrapOption::INVERSE> wrap_inverse{};

			options<FlexBoxJustifyOption::FLEX_START> justify_flex_start{};
			options<FlexBoxJustifyOption::FLEX_END> justify_flex_end{};
			options<FlexBoxJustifyOption::CENTER> justify_center{};
			options<FlexBoxJustifyOption::STRETCH> justify_stretch{};
			options<FlexBoxJustifyOption::SPACE_BETWEEN> justify_space_between{};
			options<FlexBoxJustifyOption::SPACE_AROUND> justify_space_around{};
			options<FlexBoxJustifyOption::SPACE_EVENLY> justify_space_evenly{};

			options<FlexBoxAlignItemOption::FLEX_START> align_item_flex_start{};
			options<FlexBoxAlignItemOption::FLEX_END> align_item_flex_end{};
			options<FlexBoxAlignItemOption::CENTER> align_item_center{};
			options<FlexBoxAlignItemOption::STRETCH> align_item_stretch{};

			options<FlexBoxAlignContentOption::FLEX_START> align_content_flex_start{};
			options<FlexBoxAlignContentOption::FLEX_END> align_content_flex_end{};
			options<FlexBoxAlignContentOption::CENTER> align_content_center{};
			options<FlexBoxAlignContentOption::STRETCH> align_content_stretch{};
			options<FlexBoxAlignContentOption::SPACE_BETWEEN> align_content_space_between{};
			options<FlexBoxAlignContentOption::SPACE_AROUND> align_content_space_around{};
			options<FlexBoxAlignContentOption::SPACE_EVENLY> align_content_space_evenly{};
		};

		enum class FlexBoxHackyOption
		{
			NONE,
		};

		template<typename T>
		concept flex_box_hacky_option_t = std::is_same_v<T, FlexBoxHackyOption>;

		enum class FlowOption
		{
			HORIZONTAL,
			VERTICAL,
		};

		template<typename T>
		concept flow_option_t = std::is_same_v<T, FlowOption>;

		struct flow_options
		{
			options<FlowOption::HORIZONTAL> horizontal{};
			options<FlowOption::VERTICAL> vertical{};
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace element
	{
		constexpr auto flex_box = detail::element::flex_box_options{};

		// hacky
		constexpr auto flex_box_auto = detail::options<detail::element::FlexBoxHackyOption::NONE>{};

		constexpr auto flow = detail::element::flow_options{};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		namespace element
		{
			#include <draw/detail/element.flex_box.calculate.inl>

			template<
				FlexBoxDirectionOption DirectionOption,
				FlexBoxWrapOption WrapOption,
				FlexBoxJustifyOption JustifyOption,
				FlexBoxAlignItemOption AlignItemOption,
				FlexBoxAlignContentOption AlignContentOption
			>
			class FlexBox final : public Element
			{
			public:
				constexpr static auto direction_option = DirectionOption;
				constexpr static auto wrap_option = WrapOption;
				constexpr static auto justify_option = JustifyOption;
				constexpr static auto align_item_option = AlignItemOption;
				constexpr static auto align_content_option = AlignContentOption;

			private:
				float required_width_or_height_;

				[[nodiscard]] constexpr static auto is_row_oriented() noexcept -> bool
				{
					return direction_option == FlexBoxDirectionOption::ROW or direction_option == FlexBoxDirectionOption::ROW_INVERSE;
				}

				[[nodiscard]] constexpr static auto is_column_oriented() noexcept -> bool
				{
					return direction_option == FlexBoxDirectionOption::COLUMN or direction_option == FlexBoxDirectionOption::COLUMN_INVERSE;
				}

				static_assert(is_row_oriented() or is_column_oriented());

				template<
					FlexBoxDirectionOption D,
					FlexBoxWrapOption W,
					FlexBoxJustifyOption J,
					FlexBoxAlignItemOption Ai,
					FlexBoxAlignContentOption Ac,
					std::ranges::output_range<element_block_size> Range
				>
				auto calculate_layout(const Style& style, Range& range, const float total_width, const float total_height, bool calculate_requirement) noexcept -> void
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
											.min_size = requirement.min_width,
											.flex_grow = calculate_requirement ? 0 : requirement.flex_grow_width,
											.flex_shrink = calculate_requirement ? 0 : requirement.flex_shrink_width,
											.size = 0,
											.position = 0,
									},
									.height = {
											.min_size = requirement.min_height,
											.flex_grow = calculate_requirement ? 0 : requirement.flex_grow_height,
											.flex_shrink = calculate_requirement ? 0 : requirement.flex_shrink_height,
											.size = 0,
											.position = 0,
									},
									.placed_line = 0,
									.placed_position = 0,
							};
						}
					);

					calculate<D, W, J, Ai, Ac>(style, range, total_width, total_height);
				}

			public:
				explicit FlexBox(elements_type children) noexcept
					: Element{std::move(children)},
					  required_width_or_height_{std::numeric_limits<float>::max()}
				{
					if constexpr (is_row_oriented())
					{
						requirement_.flex_grow_width = 1;
					}
					else if constexpr (is_column_oriented())
					{
						requirement_.flex_grow_height = 1;
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}

				FlexBox(const FlexBox& other) noexcept = default;
				FlexBox& operator=(const FlexBox& other) noexcept = default;
				FlexBox(FlexBox&& other) noexcept = default;
				FlexBox& operator=(FlexBox&& other) noexcept = default;
				~FlexBox() noexcept override = default;

				auto calculate_requirement(const Style& style, Surface& surface) noexcept -> void override
				{
					Element::calculate_requirement(style, surface);

					std::vector<element_block_size> element_blocks{};
					if constexpr (is_row_oriented())
					{
						calculate_layout<
							FlexBoxDirectionOption::ROW,
							FlexBoxWrapOption::DEFAULT,
							FlexBoxJustifyOption::FLEX_START,
							FlexBoxAlignItemOption::FLEX_START,
							FlexBoxAlignContentOption::FLEX_START
						>(style, element_blocks, std::numeric_limits<float>::max(), required_width_or_height_, true);
					}
					else if constexpr (is_column_oriented())
					{
						calculate_layout<
							FlexBoxDirectionOption::COLUMN,
							FlexBoxWrapOption::DEFAULT,
							FlexBoxJustifyOption::FLEX_START,
							FlexBoxAlignItemOption::FLEX_START,
							FlexBoxAlignContentOption::FLEX_START
						>(style, element_blocks, required_width_or_height_, std::numeric_limits<float>::max(), true);
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}

					// reset
					requirement_.min_width = 0;
					requirement_.min_height = 0;

					// calculate the union of all the blocks
					rect_type box{
							std::numeric_limits<float>::max(),
							std::numeric_limits<float>::max(),
							std::numeric_limits<float>::min(),
							std::numeric_limits<float>::min()
					};
					std::ranges::for_each(
						element_blocks,
						[&box](const element_block_size& block) noexcept -> void
						{
							const rect_type block_rect
							{
									block.width.position,
									block.height.position,
									block.width.position + block.width.size,
									block.height.position + block.height.size
							};

							box = box.combine_max(block_rect);
						}
					);
					requirement_.min_width = box.width();
					requirement_.min_height = box.height();
				}

				auto set_rect(const Style& style, const rect_type& rect) noexcept -> void override
				{
					Element::set_rect(style, rect);

					[[maybe_unused]] const auto previous = std::exchange(required_width_or_height_, is_row_oriented() ? rect.width() : rect.height());

					std::vector<element_block_size> element_blocks{};
					calculate_layout<
						direction_option,
						wrap_option,
						justify_option,
						align_item_option,
						align_content_option
					>(style, element_blocks, rect.width(), rect.height(), false);

					std::ranges::for_each(
						std::views::zip(children_, element_blocks),
						[&style, &rect](const std::tuple<element_type&, const element_block_size&>& pack) noexcept -> void
						{
							auto& [child, block] = pack;

							const auto start_point = rect.point;
							const rect_type block_rect
							{
									start_point.x + block.width.position,
									start_point.y + block.height.position,
									start_point.x + block.width.position + block.width.size,
									start_point.y + block.height.position + block.height.size
							};
							const auto intersects_box = rect.combine_min(block_rect);

							child->set_rect(style, intersects_box);
						}
					);
				}
			};

			template<
				FlexBoxDirectionOption DirectionOption = FlexBoxDirectionOption::ROW,
				FlexBoxWrapOption WrapOption = FlexBoxWrapOption::DEFAULT,
				FlexBoxJustifyOption JustifyOption = FlexBoxJustifyOption::FLEX_START,
				FlexBoxAlignItemOption AlignItemOption = FlexBoxAlignItemOption::FLEX_START,
				FlexBoxAlignContentOption AlignContentOption = FlexBoxAlignContentOption::FLEX_START
			>
			struct flex_box_maker
			{
				[[nodiscard]] constexpr auto operator()(elements_type elements) const noexcept -> element_type
				{
					return make_element<FlexBox<DirectionOption, WrapOption, JustifyOption, AlignItemOption, AlignContentOption>>(std::move(elements));
				}

				[[nodiscard]] constexpr auto operator()(derived_element_t auto... elements) const noexcept -> element_type
				{
					elements_type es{};
					es.reserve(sizeof...(elements));

					(es.emplace_back(std::move(elements)), ...);

					return this->operator()(std::move(es));
				}

				template<FlexBoxDirectionOption Option>
				[[nodiscard]] constexpr auto operator()(options<Option>) const noexcept -> flex_box_maker<Option, WrapOption, JustifyOption, AlignItemOption, AlignContentOption>
				{
					return {};
				}

				template<FlexBoxDirectionOption Option, typename... Args>
				[[nodiscard]] constexpr auto operator()(options<Option> option, Args&&... args) const noexcept -> auto
				{
					return this->operator()(option)(std::forward<Args>(args)...);
				}

				template<FlexBoxWrapOption Option>
				[[nodiscard]] constexpr auto operator()(options<Option>) const noexcept -> flex_box_maker<DirectionOption, Option, JustifyOption, AlignItemOption, AlignContentOption>
				{
					return {};
				}

				template<FlexBoxWrapOption Option, typename... Args>
				[[nodiscard]] constexpr auto operator()(options<Option> option, Args&&... args) const noexcept -> auto
				{
					return this->operator()(option)(std::forward<Args>(args)...);
				}

				template<FlexBoxJustifyOption Option>
				[[nodiscard]] constexpr auto operator()(options<Option>) const noexcept -> flex_box_maker<DirectionOption, WrapOption, Option, AlignItemOption, AlignContentOption>
				{
					return {};
				}

				template<FlexBoxJustifyOption Option, typename... Args>
				[[nodiscard]] constexpr auto operator()(options<Option> option, Args&&... args) const noexcept -> auto
				{
					return this->operator()(option)(std::forward<Args>(args)...);
				}

				template<FlexBoxAlignItemOption Option>
				[[nodiscard]] constexpr auto operator()(options<Option>) const noexcept -> flex_box_maker<DirectionOption, WrapOption, JustifyOption, Option, AlignContentOption>
				{
					return {};
				}

				template<FlexBoxAlignItemOption Option, typename... Args>
				[[nodiscard]] constexpr auto operator()(options<Option> option, Args&&... args) const noexcept -> auto
				{
					return this->operator()(option)(std::forward<Args>(args)...);
				}

				template<FlexBoxAlignContentOption Option>
				[[nodiscard]] constexpr auto operator()(options<Option>) const noexcept -> flex_box_maker<DirectionOption, WrapOption, JustifyOption, AlignItemOption, Option>
				{
					return {};
				}

				template<FlexBoxAlignContentOption Option, typename... Args>
				[[nodiscard]] constexpr auto operator()(options<Option> option, Args&&... args) const noexcept -> auto
				{
					return this->operator()(option)(std::forward<Args>(args)...);
				}
			};
		}

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<element::flex_box_direction_or_wrap_or_justify_or_align_item_or_align_content_option_t auto... Os>
		struct maker<Os...> : element::flex_box_maker<> {};

		// hacky
		template<element::flex_box_hacky_option_t auto... Os>
		struct maker<Os...> : element::flex_box_maker<> {};

		template<element::flow_option_t auto... Os>
			requires ((std::to_underlying(Os) == draw::element::flow.horizontal) and ...)
		struct maker<Os...> : element::flex_box_maker<> {};

		template<element::flow_option_t auto... Os>
			requires ((std::to_underlying(Os) == draw::element::flow.vertical) and ...)
		struct maker<Os...> : element::flex_box_maker<element::FlexBoxDirectionOption::COLUMN> {};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
