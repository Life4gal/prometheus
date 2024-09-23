// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.guardian;

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
		enum class GuardianOption
		{
			RECT,
		};

		template<typename T>
		concept guardian_option_t = std::is_same_v<T, GuardianOption>;

		struct guardian_options
		{
			options<GuardianOption::RECT> rect{};
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace element
	{
		constexpr auto guardian = detail::element::guardian_options{};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		namespace element
		{
			template<GuardianOption Option>
			class Guardian;

			template<>
			class Guardian<GuardianOption::RECT> final : public Element
			{
			public:
				using reference = std::reference_wrapper<rect_type>;

			private:
				reference ref_rect_;

			public:
				Guardian(element_type element, const reference ref_rect) noexcept
					: Element{std::move(element)},
					  ref_rect_{ref_rect} {}

				auto calculate_requirement(const Style& style, Surface& surface) noexcept -> void override
				{
					Element::calculate_requirement(style, surface);
					requirement_ = children_[0]->requirement();
				}

				auto set_rect(const Style& style, const rect_type& rect) noexcept -> void override
				{
					ref_rect_.get() = rect;
					Element::set_rect(style, rect);
					children_[0]->set_rect(style, rect);
				}

				auto render(const Style& style, Surface& surface) noexcept -> void override
				{
					ref_rect_.get() = surface.rect().combine_min(ref_rect_);
					Element::render(style, surface);
				}
			};

			struct guardian_rect_builder
			{
				[[nodiscard]] auto operator()(element_type element, Guardian<GuardianOption::RECT>::reference ref_rect) const noexcept -> element_type
				{
					return make_element<Guardian<GuardianOption::RECT>>(std::move(element), ref_rect);
				}

				[[nodiscard]] auto operator()(element_type element) const noexcept -> auto
				{
					return [element](Guardian<GuardianOption::RECT>::reference ref_rect) noexcept -> element_type
					{
						return make_element<Guardian<GuardianOption::RECT>>(std::move(element), ref_rect);
					};
				}

				[[nodiscard]] auto operator()(Guardian<GuardianOption::RECT>::reference ref_rect) const noexcept -> auto
				{
					return [ref_rect](element_type element) noexcept -> element_type
					{
						return make_element<Guardian<GuardianOption::RECT>>(std::move(element), ref_rect);
					};
				}
			};
		}

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<element::guardian_option_t auto... Os>
		struct maker<Os...>
		{
			template<element::GuardianOption Option>
			[[nodiscard]] auto operator()(options<Option>) const noexcept -> auto
			{
				if constexpr (Option == element::GuardianOption::RECT)
				{
					return element::guardian_rect_builder{};
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
