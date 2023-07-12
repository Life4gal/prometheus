// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <compare>

#include <prometheus/type/traits/numeric.hpp>
#include <prometheus/macro.hpp>

namespace gal::prometheus::type::compare
{
	template<typename Left, typename Right>
	struct three_way_comparison;

	template<typename Left, typename Right>
		requires std::is_same_v<Left, Right> and
				(traits::unsigned_integral<Left> or
				traits::signed_integral<Left> or
				std::floating_point<Left>)
	struct three_way_comparison<Left, Right>
	{
		[[nodiscard]] constexpr auto operator()(const Left left, const Right right) const noexcept -> std::strong_ordering { return left <=> right; }
	};

	template<typename Left, typename Right>
		requires(std::signed_integral<Left> and std::unsigned_integral<Right>) or
				(std::unsigned_integral<Left> and std::signed_integral<Right>)
	struct three_way_comparison<Left, Right>
	{
		[[nodiscard]] constexpr auto operator()(const Left left, const Right right) const noexcept -> std::strong_ordering
		{
			// if (std::cmp_greater(left, right))
			// {
			// 	return std::strong_ordering::greater;
			// }

			if constexpr (std::signed_integral<Left>)
			{
				if (left < 0) { return std::strong_ordering::less; }
				return static_cast<Right>(left) <=> right;
			}
			else
			{
				if (right < 0) { return std::strong_ordering::greater; }
				return left <=> static_cast<Left>(right);
			}
		}
	};

	template<typename L, typename R>
		requires(std::floating_point<L> and std::integral<R>) or
				(std::integral<L> and std::floating_point<R>)
	struct three_way_comparison<L, R>
	{
		[[nodiscard]] constexpr auto operator()(const L l, const R r) const noexcept -> std::partial_ordering
		{
			if constexpr (std::floating_point<L>)
			{
				if constexpr (sizeof(R) < sizeof(float)) { return l <=> static_cast<float>(r); }
				else if constexpr (sizeof(R) < sizeof(double)) { return l <=> static_cast<double>(r); }
				else if constexpr (sizeof(R) < sizeof(long double)) { return l <=> static_cast<long double>(r); }
				else { GAL_PROMETHEUS_STATIC_UNREACHABLE("Invalid floating point type!"); }
			}
			else
			{
				if constexpr (sizeof(L) < sizeof(float)) { return static_cast<float>(l) <=> r; }
				else if constexpr (sizeof(L) < sizeof(double)) { return static_cast<double>(l) <=> r; }
				else if constexpr (sizeof(L) < sizeof(long double)) { return static_cast<long double>(l) <=> r; }
				else { GAL_PROMETHEUS_STATIC_UNREACHABLE("Invalid floating point type!"); }
			}
		}
	};

	template<typename Left, typename Right>
		requires std::is_arithmetic_v<Left> and std::is_arithmetic_v<Right>
	[[nodiscard]] constexpr auto three_way_compare(const Left left, const Right right) noexcept -> auto { return three_way_comparison<Left, Right>{}(left, right); }
}
