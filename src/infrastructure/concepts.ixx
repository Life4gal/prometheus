// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.infrastructure:concepts;

import std;

export namespace gal::prometheus::infrastructure::concepts
{
	template<typename T, typename... Ts>
	constexpr bool any_of_v = (std::is_same_v<T, Ts> || ...);

	template<typename T, typename... Ts>
	concept any_of_t = any_of_v<T, Ts...>;

	template<typename T, typename... Ts>
	constexpr bool all_of_v = (std::is_same_v<T, Ts> && ...);

	template<typename T, typename... Ts>
	concept all_of_t = all_of_v<T, Ts...>;
}
