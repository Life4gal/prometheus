// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.decorator;

import std;

import gal.prometheus.primitive;

import :surface;
import :style;

import :element.element;

#else
#pragma once

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>

#include <draw/surface.ixx>
#include <draw/style.ixx>

#include <draw/detail/element.ixx>

#endif

namespace gal::prometheus::draw
{
	namespace detail::element
	{
		enum class DecoratorOption
		{
			BOLD,
		};

		template<typename T>
		concept decorator_option_t = std::is_same_v<T, DecoratorOption>;

		struct decorator_options
		{
			options<DecoratorOption::BOLD> bold{};
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace element
	{
		constexpr auto decorator = detail::element::decorator_options{};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		namespace element
		{
			template<DecoratorOption Option>
			class Decorator final : public Element
			{
			public:
				constexpr static auto option = Option;
				constexpr static auto option_value = std::to_underlying(option);

				explicit Decorator(element_type element) noexcept
					: Element{std::move(element)} {}

				Decorator(const Decorator&) noexcept = default;
				auto operator=(const Decorator&) noexcept -> Decorator& = default;
				Decorator(Decorator&&) noexcept = default;
				auto operator=(Decorator&&) noexcept -> Decorator& = default;
				~Decorator() noexcept override = default;

				auto calculate_requirement(const Style& style, Surface& surface) noexcept -> void override
				{
					Element::calculate_requirement(style, surface);
					requirement_ = children_[0]->requirement();
				}

				auto set_rect(const Style& style, const rect_type& rect) noexcept -> void override
				{
					Element::set_rect(style, rect);
					children_[0]->set_rect(style, rect);
				}

				auto render(const Style& style, Surface& surface) noexcept -> void override
				{
					if constexpr (option == DecoratorOption::BOLD)
					{
						surface.draw_list().rect_filled(rect_, style.item_color_focused);
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}

					children_[0]->render(style, surface);
				}
			};
		}

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<element::decorator_option_t auto... Os>
		struct maker<Os...>
		{
			template<element::DecoratorOption Option>
			[[nodiscard]] auto operator()(options<Option>) const noexcept -> auto
			{
				return [](element_type element) noexcept -> element_type
				{
					return make_element<element::Decorator<Option>>(std::move(element));
				};
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
