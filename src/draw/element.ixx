// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element;

import gal.prometheus.functional;

export import :element.element;
export import :element.text;
export import :element.separator;
export import :element.border;
export import :element.boundary;
export import :element.flex;
export import :element.box;

#else
#pragma once

#include <functional/functional.ixx>

#include <draw/detail/element.ixx>
#include <draw/detail/element.text.ixx>
#include <draw/detail/element.separator.ixx>
#include <draw/detail/element.border.ixx>
#include <draw/detail/element.boundary.ixx>
#include <draw/detail/element.flex.ixx>
#include <draw/detail/element.box.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw)
{
	template<typename Element, typename Decorator>
		requires (derived_element_t<std::decay_t<Element>> and derived_element_t<std::invoke_result_t<Decorator, Element>>)
	[[nodiscard]] constexpr auto operator|(Element&& element, Decorator&& decorator) noexcept -> element_type //
	{
		return std::invoke(std::forward<Decorator>(decorator), std::forward<Element>(element));
	}

	template<typename Decorator>
		requires derived_element_t<std::invoke_result_t<Decorator, elements_type::value_type>>
	[[nodiscard]] constexpr auto operator|(elements_type& elements, Decorator&& decorator) noexcept -> decltype(auto)
	{
		return std::invoke(std::forward<Decorator>(decorator), elements);
	}

	template<typename Decorator>
		requires derived_element_t<std::invoke_result_t<Decorator, elements_type::value_type>>
	[[nodiscard]] constexpr auto operator|(elements_type&& elements, Decorator&& decorator) noexcept -> decltype(auto)
	{
		return std::invoke(std::forward<Decorator>(decorator), std::move(elements));
	}

	template<detail::options_t... Os>
	[[nodiscard]] constexpr auto element(Os... os) noexcept -> decltype(auto)
	{
		constexpr auto maker = detail::select_maker<Os...>();

		if constexpr (requires { maker(os...); })
		{
			return maker(os...);
		}
		else
		{
			return maker;
		}
	}
}
