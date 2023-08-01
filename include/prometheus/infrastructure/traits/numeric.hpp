// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <numeric>

namespace gal::prometheus::inline infrastructure::traits
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
}
