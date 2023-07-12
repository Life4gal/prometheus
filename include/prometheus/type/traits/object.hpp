// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>

namespace gal::prometheus::type::traits
{
	template<typename T>
	struct is_byte_like : std::false_type { };

	template<>
	struct is_byte_like<char> : std::true_type { };

	template<>
	struct is_byte_like<const char> : std::true_type { };

	template<>
	struct is_byte_like<signed char> : std::true_type { };

	template<>
	struct is_byte_like<const signed char> : std::true_type { };

	template<>
	struct is_byte_like<unsigned char> : std::true_type { };

	template<>
	struct is_byte_like<const unsigned char> : std::true_type { };

	template<>
	struct is_byte_like<std::byte> : std::true_type { };

	template<>
	struct is_byte_like<const std::byte> : std::true_type { };

	template<typename T>
	constexpr bool is_byte_like_v = is_byte_like<T>::value;

	template<typename T>
	concept byte_like = is_byte_like_v<T>;

	template<typename To, typename From>
	struct keep_cv { };

	template<typename To, typename From>
		requires(not std::is_const_v<From> and not std::is_volatile_v<From>)
	struct keep_cv<To, From>
	{
		using type = std::remove_cv_t<To>;
	};

	template<typename To, typename From>
		requires(not std::is_const_v<From> and std::is_volatile_v<From>)
	struct keep_cv<To, From>
	{
		using type = std::add_volatile_t<std::remove_cv_t<To>>;
	};

	template<typename To, typename From>
		requires(std::is_const_v<From> and not std::is_volatile_v<From>)
	struct keep_cv<To, From>
	{
		using type = std::add_const_t<std::remove_cv_t<To>>;
	};

	template<typename To, typename From>
		requires(std::is_const_v<From> and std::is_volatile_v<From>)
	struct keep_cv<To, From>
	{
		using type = std::add_const_t<std::add_volatile_t<std::remove_cv_t<To>>>;
	};

	template<typename To, typename From>
	using keep_cv_t = typename keep_cv<To, From>::type;
}
