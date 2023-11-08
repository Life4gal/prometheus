// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.utility:concepts;

import std;

export namespace gal::prometheus::utility::concepts
{
	template<typename T, typename... Ts>
	constexpr bool is_any_of_v = (std::is_same_v<T, Ts> or ...);

	template<typename T, typename... Ts>
	concept any_of = is_any_of_v<T, Ts...>;

	template<typename T, typename... Ts>
	constexpr bool is_all_of_v = (std::is_same_v<T, Ts> and ...);

	template<typename T, typename... Ts>
	concept all_of = is_all_of_v<T, Ts...>;

	template<typename Allocator>
	concept allocator = requires(Allocator a)
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

	template<typename T>
	constexpr auto is_signed_integral_v = std::is_integral_v<T> and std::is_signed_v<T>;
	template<typename T>
	concept signed_integral = is_signed_integral_v<T>;
	template<typename T>
	constexpr auto is_unsigned_integral_v = std::is_integral_v<T> && std::is_unsigned_v<T>;
	template<typename T>
	concept unsigned_integral = is_unsigned_integral_v<T>;

	template<typename T>
	constexpr auto is_arithmetic_v = std::is_arithmetic_v<T>;
	template<typename T>
	concept arithmetic = is_arithmetic_v<T>;

	template<typename Out, typename In>
	constexpr auto is_type_in_range_v = is_arithmetic_v<Out> and is_arithmetic_v<In> and std::numeric_limits<Out>::digits >= std::numeric_limits<In>::digits and (std::numeric_limits<Out>::is_signed == std::numeric_limits<In>::is_signed or std::numeric_limits<Out>::is_signed);
	template<typename Out, typename In>
	concept type_in_range = is_type_in_range_v<Out, In>;

	template<typename T>
	constexpr auto is_byte_like_v =
			std::is_same_v<T, char> or				 //
			std::is_same_v<T, const char> or		 //
			std::is_same_v<T, signed char> or		 //
			std::is_same_v<T, const signed char> or	 //
			std::is_same_v<T, unsigned char> or		 //
			std::is_same_v<T, const unsigned char> or//
			std::is_same_v<T, std::byte> or			 //
			std::is_same_v<T, const std::byte>;		 //

	template<typename T>
	concept byte_like = is_byte_like_v<T>;
}
