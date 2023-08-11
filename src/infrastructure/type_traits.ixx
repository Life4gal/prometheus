// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.infrastructure:type_traits;

import std;

export namespace gal::prometheus::infrastructure::traits
{
	template<typename T>
	struct is_signed_integral : std::false_type { };

	template<typename T>
		requires std::is_signed_v<T> and std::is_integral_v<T>
	struct is_signed_integral<T> : std::true_type { };

	template<typename T>
	constexpr bool is_signed_integral_v = is_signed_integral<T>::value;

	template<typename T>
	concept signed_integral = is_signed_integral_v<T>;

	template<typename T>
	struct is_unsigned_integral : std::false_type { };

	template<typename T>
		requires std::is_unsigned_v<T> and std::is_integral_v<T>
	struct is_unsigned_integral<T> : std::true_type { };

	template<typename T>
	constexpr bool is_unsigned_integral_v = is_unsigned_integral<T>::value;

	template<typename T>
	concept unsigned_integral = is_unsigned_integral_v<T>;

	template<typename T>
	struct is_integral : std::false_type { };

	template<typename T>
		requires signed_integral<T> or unsigned_integral<T>
	struct is_integral<T> : std::true_type { };

	template<typename T>
	constexpr bool is_integral_v = is_integral<T>::value;

	template<typename T>
	concept integral = is_integral_v<T>;

	template<typename T>
	struct is_arithmetic : std::is_arithmetic<T> { };

	template<typename T>
	constexpr bool is_arithmetic_v = is_arithmetic<T>::value;

	template<typename T>
	concept arithmetic = is_arithmetic_v<T>;

	template<typename Out, typename In>
	struct is_type_in_range : std::false_type { };

	template<arithmetic Out, arithmetic In>
		requires(std::numeric_limits<Out>::digits >= std::numeric_limits<In>::digits) and
				(std::numeric_limits<Out>::is_signed == std::numeric_limits<In>::is_signed or
				std::numeric_limits<Out>::is_signed)
	struct is_type_in_range<Out, In> : std::true_type { };

	template<typename Out, typename In>
	constexpr bool is_type_in_range_v = is_type_in_range<Out, In>::value;

	template<typename Out, typename In>
	concept type_in_range = is_type_in_range_v<Out, In>;

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
