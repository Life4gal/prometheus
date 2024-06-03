// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.functional:math;

import std;
import :functor;

#else

#include <type_traits>
#include <cmath>

#include <prometheus/macro.hpp>
#include <functional/functor.hpp>
#endif

namespace gal::prometheus::functional
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto abs(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			//
			return value > 0 ? value : -value;
		}

		return std::abs(value);
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto tgamma(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_DEBUG_AXIOM(value >= 0);

		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			//
			return (value <= 1) ? 1 : (value * functional::tgamma(value - 1));
		}

		return static_cast<T>(std::tgamma(value));
	}

	template<std::integral T>
	[[nodiscard]] constexpr auto factorial(const T value) noexcept -> T //
	{
		return functional::tgamma(value);
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto pow(const T base, const int exp) noexcept -> T
	{
		GAL_PROMETHEUS_DEBUG_AXIOM(exp >= 0);

		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			//
			return (exp == 0) ? static_cast<T>(1) : static_cast<T>(base * pow(base, exp - 1));
		}

		return static_cast<T>(std::pow(base, exp));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto sqrt(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_DEBUG_AXIOM(value >= 0);

		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if (value == 0) // NOLINT(clang-diagnostic-float-equal)
			{
				return value;
			}

			T prev = 0;
			T current = value / 2;

			while (current != prev) // NOLINT(clang-diagnostic-float-equal)
			{
				prev = current;
				current = (current + value / current) / 2;
			}

			return current;
		}

		return static_cast<T>(std::sqrt(value));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto hypot(const T x, const std::type_identity_t<T> y) noexcept -> T
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			//
			return functional::sqrt(functional::pow(x, 2) + functional::pow(y, 2));
		}

		return static_cast<T>(std::hypot(x, y));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto sin(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			constexpr auto taylor_series = functional::y_combinator{
					[](auto self, const T v, const int n) noexcept -> T //
					{
						return
								(n == 0)
									? v
									: (
										  functional::pow(-1, n) *
										  functional::pow(v, 2 * n + 1) /
										  functional::factorial(2 * n + 1)
									  ) +
									  self(v, n - 1);
					}
			};

			// todo
			constexpr auto n = 10;
			return taylor_series(value, n);
		}

		return static_cast<T>(std::sin(value));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto cos(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			constexpr auto taylor_series = functional::y_combinator
			{[](auto self, const T v, const int n) noexcept -> T //
					{
						return
								(n == 0)
									? 1
									: (
										  functional::pow(-1, n) *
										  functional::pow(v, 2 * n) /
										  functional::factorial(2 * n)
									  ) + self(v, n - 1);
					}
			};

			// todo
			constexpr auto n = 10;
			return taylor_series(value, n);
		}

		return static_cast<T>(std::cos(value));
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
} // namespace gal::prometheus::functional
