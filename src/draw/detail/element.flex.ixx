// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.flex;

import std;

import :element.element;

#else
#pragma once

#include <prometheus/macro.hpp>

#include <draw/detail/element.ixx>

#endif

namespace gal::prometheus::draw
{
	namespace detail::element
	{
		enum class FlexTypeOption
		{
			NONE = 0x0000,

			GROW = 0x0001,
			SHRINK = 0x0010,
		};

		enum class FlexDirectionOption
		{
			NONE = 0x0000,

			HORIZONTAL = 0x0001,
			VERTICAL = 0x0010,
		};

		template<typename T>
		concept flex_type_option_t = std::is_same_v<T, FlexTypeOption>;

		template<typename T>
		concept flex_direction_option_t = std::is_same_v<T, FlexDirectionOption>;

		template<typename T>
		concept flex_type_or_direction_option_t = flex_type_option_t<T> or flex_direction_option_t<T>;

		struct flex_options
		{
			options<FlexTypeOption::GROW> grow{};
			options<FlexTypeOption::SHRINK> shrink{};
			options<FlexTypeOption::GROW, FlexTypeOption::SHRINK> dynamic{};

			options<FlexDirectionOption::HORIZONTAL> horizontal{};
			options<FlexDirectionOption::VERTICAL> vertical{};
			options<FlexDirectionOption::HORIZONTAL, FlexDirectionOption::VERTICAL> both{};
		};

		enum class FlexHackyOption
		{
			NONE,
		};

		template<typename T>
		concept flex_hack_option_t = std::is_same_v<T, FlexHackyOption>;
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace element
	{
		constexpr auto flex = detail::element::flex_options{};

		// hacky
		constexpr auto flex_auto = detail::options<detail::element::FlexHackyOption::NONE>{};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		namespace element
		{
			template<std::underlying_type_t<FlexTypeOption> Type, std::underlying_type_t<FlexDirectionOption> Direction>
			class Flex final : public Element
			{
			public:
				constexpr static auto type = Type;
				constexpr static auto direction = Direction;

				explicit Flex() noexcept
					: Element{} {}

				explicit Flex(element_type element) noexcept
					: Element{std::move(element)} {}

				Flex(const Flex& other) noexcept = default;
				Flex& operator=(const Flex& other) noexcept = default;
				Flex(Flex&& other) noexcept = default;
				Flex& operator=(Flex&& other) noexcept = default;
				~Flex() noexcept override = default;

				auto calculate_requirement(const Style& style, Surface& surface) noexcept -> void override
				{
					requirement_.reset();

					if (not children_.empty())
					[[likely]]
					{
						children_[0]->calculate_requirement(style, surface);
						requirement_ = children_[0]->requirement();
					}

					if constexpr (type == std::to_underlying(FlexTypeOption::NONE) and direction == std::to_underlying(FlexDirectionOption::NONE))
					{
						requirement_.flex_grow_width = 0;
						requirement_.flex_grow_height = 0;
						requirement_.flex_shrink_width = 0;
						requirement_.flex_shrink_height = 0;
					}
					else
					{
						constexpr auto horizontal = direction & std::to_underlying(FlexDirectionOption::HORIZONTAL);
						constexpr auto vertical = direction & std::to_underlying(FlexDirectionOption::VERTICAL);

						constexpr auto grow = type & std::to_underlying(FlexTypeOption::GROW);
						constexpr auto shrink = type & std::to_underlying(FlexTypeOption::SHRINK);

						const auto set_gw = [this, x = style.flex_x]() noexcept -> void { requirement_.flex_grow_width = x; };
						const auto set_gh = [this, y = style.flex_y]() noexcept -> void { requirement_.flex_grow_height = y; };
						const auto set_sw = [this, x = style.flex_x]() noexcept -> void { requirement_.flex_shrink_width = x; };
						const auto set_sh = [this, y = style.flex_y]() noexcept -> void { requirement_.flex_shrink_height = y; };

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

				auto set_rect(const Style& style, const rect_type& rect) noexcept -> void override
				{
					Element::set_rect(style, rect);

					if (not children_.empty())
					[[likely]]
					{
						children_[0]->set_rect(style, rect);
					}
				}
			};

			template<
				std::underlying_type_t<FlexTypeOption> Type = std::to_underlying(FlexTypeOption::NONE),
				std::underlying_type_t<FlexDirectionOption> Direction = std::to_underlying(FlexDirectionOption::NONE)
			>
			struct flex_maker
			{
				[[nodiscard]] constexpr auto operator()(element_type element) const noexcept -> element_type
				{
					return make_element<Flex<Type, Direction>>(std::move(element));
				}

				[[nodiscard]] constexpr auto operator()() const noexcept -> element_type
				{
					return make_element<Flex<Type, Direction>>();
				}

				template<FlexTypeOption... Option>
				[[nodiscard]] constexpr auto operator()(const options<Option...>) const noexcept -> flex_maker<(Type | ... | std::to_underlying(Option)), Direction>
				{
					return {};
				}

				template<FlexDirectionOption... Option>
				[[nodiscard]] constexpr auto operator()(const options<Option...>) const noexcept -> flex_maker<Type, (Direction | ... | std::to_underlying(Option))>
				{
					return {};
				}

				// hacky
				template<FlexHackyOption Option>
				[[nodiscard]] constexpr auto operator()(const options<Option>) const noexcept -> flex_maker<draw::element::flex.dynamic, draw::element::flex.both>
				{
					return {};
				}
			};
		}

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<element::flex_type_or_direction_option_t auto... Os>
		struct maker<Os...> : element::flex_maker<> {};

		template<element::flex_hack_option_t auto... Os>
		struct maker<Os...> : element::flex_maker<> {};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
