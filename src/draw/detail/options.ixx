// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
export module gal.prometheus.draw:options;

import std;

import gal.prometheus.functional;

#else
#pragma once

#include <type_traits>
#include <utility>

#include <functional/functional.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw)
{
	namespace detail
	{
		template<auto... Os>
		struct options : std::integral_constant<std::common_type_t<std::underlying_type_t<std::decay_t<decltype(Os)>>...>, (0 | ... | std::to_underlying(Os))> {};

		template<typename>
		struct is_options : std::false_type {};

		template<auto... Os>
		struct is_options<options<Os...>> : std::true_type {};

		template<typename T>
		constexpr auto is_options_v = is_options<T>::value;

		template<typename T>
		concept options_t = is_options_v<T>;

		template<typename...>
		struct options_builder;

		template<>
		struct options_builder<>
		{
			constexpr static auto value = functional::value_list<>;
		};

		template<auto... Os>
		struct options_builder<options<Os...>>
		{
			constexpr static auto value = functional::value_list<Os...>;
		};

		template<auto... Os1, auto... Os2>
		struct options_builder<options<Os1...>, options<Os2...>>
		{
			constexpr static auto value = functional::value_list<Os1..., Os2...>;
		};

		template<auto... Os1, auto... Os2, options_t... O>
		struct options_builder<options<Os1...>, options<Os2...>, O...>
		{
			constexpr static auto value = options_builder<options<Os1...>, options<Os2...>>::value.template push_back<options_builder<O...>::value>();
		};

		template<auto... Os>
		struct maker;

		template<auto... Os>
		struct maker_selector
		{
			using type = maker<Os...>;

			constexpr explicit maker_selector(functional::value_list_type<Os...>) noexcept {}
		};

		template<options_t... Os>
		[[nodiscard]] constexpr auto select_maker() noexcept -> auto
		{
			[[maybe_unused]] constexpr auto value = options_builder<Os...>::value;
			using selector_type = decltype(maker_selector{value});

			return typename selector_type::type{};
		}
	}

	// make(options) -> maker
	template<detail::options_t... Os>
	[[nodiscard]] constexpr auto make(Os... os) noexcept -> decltype(auto)
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
