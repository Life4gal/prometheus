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

	template<typename Allocator>
	concept allocator_t = requires(Allocator a)
	{
		typename Allocator::value_type;
		// typename Allocator::pointer;
		// typename Allocator::const_pointer;
		// typename Allocator::reference;
		// typename Allocator::const_reference;
		typename Allocator::size_type;
		typename Allocator::difference_type;

		// { a.allocate(std::declval<typename Allocator::size_type>()) } -> std::same_as<typename Allocator::pointer>;
		// { a.deallocate(std::declval<typename Allocator::pointer>(), std::declval<typename Allocator::size_type>()) } -> std::same_as<void>;
		// { a.construct(std::declval<typename Allocator::pointer>(), std::declval<typename Allocator::const_reference>()) } -> std::same_as<void>;
		// { a.destroy(std::declval<typename Allocator::pointer>()) } -> std::same_as<void>;
		{ a.allocate(std::declval<typename Allocator::size_type>()) } -> std::same_as<typename Allocator::value_type*>;
		{ a.deallocate(std::declval<typename Allocator::value_type*>(), std::declval<typename Allocator::size_type>()) } -> std::same_as<void>;
	};
}
