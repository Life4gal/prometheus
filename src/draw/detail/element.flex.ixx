// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.flex;

import std;

import gal.prometheus.primitive;

import :element.element;

#else
#pragma once

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>

#include <draw/detail/element.ixx>

#endif

namespace gal::prometheus::draw
{
	namespace detail
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
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	constexpr auto flex_grow = options<detail::FlexTypeOption::GROW>{};
	constexpr auto flex_shrink = options<detail::FlexTypeOption::SHRINK>{};

	constexpr auto flex_horizontal = options<detail::FlexDirectionOption::HORIZONTAL>{};
	constexpr auto flex_vertical = options<detail::FlexDirectionOption::VERTICAL>{};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
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

			template<FlexTypeOption Option>
			[[nodiscard]] constexpr auto operator()(const options<Option>) const noexcept -> flex_maker<Type | std::to_underlying(Option), Direction>
			{
				return {};
			}

			template<FlexDirectionOption Option>
			[[nodiscard]] constexpr auto operator()(const options<Option>) const noexcept -> flex_maker<Type, Direction | std::to_underlying(Option)>
			{
				return {};
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<auto... Os>
			requires ((std::is_same_v<decltype(Os), FlexTypeOption> or std::is_same_v<decltype(Os), FlexDirectionOption>) and ...)
		struct element_maker<Os...> : flex_maker<> {};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
