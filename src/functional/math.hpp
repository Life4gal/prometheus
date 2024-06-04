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

#else

#include <type_traits>
#include <cmath>
#include <numbers>

#include <prometheus/macro.hpp>
#endif

namespace gal::prometheus::functional
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto is_nan(const T value) noexcept -> bool
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			//
			return value != value;
		}

		return std::isnan(value);
	}

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
	[[nodiscard]] constexpr auto floor(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if constexpr (std::is_integral_v<T>) { return value; }
			else
			{
				if (value >= 0 or static_cast<T>(static_cast<unsigned long long>(value)) == value)
				{
					return static_cast<T>(static_cast<unsigned long long>(value));
				}

				return static_cast<T>(static_cast<unsigned long long>(value) - 1);
			}
		}

		return std::floor(value);
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
			if (exp == 0) { return static_cast<T>(1); }

			return static_cast<T>(base * functional::pow(base, exp - 1));
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

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace math_detail
	{
		template<typename T>
		[[nodiscard]] constexpr auto tan_series_exp(const T value) noexcept -> T
		{
			const auto z = value - std::numbers::pi_v<T> / 2;

			if (std::numeric_limits<T>::min() > functional::abs(z)) { return std::numbers::pi_v<T> / 2; }

			// this is based on a fourth-order expansion of tan(z) using Bernoulli numbers
			return -1 / z + (z / 3 + (functional::pow(z, 3) / 45 + (2 * functional::pow(z, 5) / 945 + functional::pow(z, 7) / 4725)));
		}

		template<typename T>
		[[nodiscard]] constexpr auto tan_cf_recurse(const T value, const int current, const int max) noexcept -> T
		{
			const auto z = static_cast<T>(2 * current - 1);

			if (current < max) { return z - value / tan_cf_recurse(value, current + 1, max); }

			return z;
		}

		template<typename T>
		[[nodiscard]] constexpr auto tan_cf_main(const T value) noexcept -> T
		{
			if (value > static_cast<T>(1.55) and value < static_cast<T>(1.6))
			{
				// deals with a singularity at tan(pi/2)
				return tan_series_exp(value);
			}

			if (value > static_cast<T>(1.4)) { return value / tan_cf_recurse(value * value, 1, 45); }

			if (value > static_cast<T>(1)) { return value / tan_cf_recurse(value * value, 1, 35); }

			return value / tan_cf_recurse(value * value, 1, 25);
		}

		template<typename T>
		[[nodiscard]] constexpr auto tan_begin(const T value, const int count = 0) noexcept -> T
		{
			if (value > std::numbers::pi_v<T>)
			{
				if (count > 1) { return std::numeric_limits<T>::quiet_NaN(); }

				return tan_begin(value - std::numbers::pi_v<T> * functional::floor(value - std::numbers::pi_v<T>), count + 1);
			}

			return tan_cf_main(value);
		}
	} // namespace math_detail

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto tan(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if (functional::is_nan(value)) { return std::numeric_limits<T>::quiet_NaN(); }

			if (value < static_cast<T>(0)) { return -math_detail::tan_begin(-value); }

			return math_detail::tan_begin(value);
		}

		return std::tan(value);
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto sin(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if (functional::is_nan(value)) { return std::numeric_limits<T>::quiet_NaN(); }

			if (std::numeric_limits<T>::min() > functional::abs(value - std::numbers::pi_v<T> / 2)) { return 1; }

			if (std::numeric_limits<T>::min() > functional::abs(value + std::numbers::pi_v<T> / 2)) { return -1; }

			if (std::numeric_limits<T>::min() > functional::abs(value - std::numbers::pi_v<T>)) { return 0; }

			if (std::numeric_limits<T>::min() > functional::abs(value + std::numbers::pi_v<T>)) { return -0; }

			// sin(x) = 2tan(x/2) / (1 + tan²(x/2))
			const auto z = functional::tan(value / static_cast<T>(2));
			return (static_cast<T>(2) * z) / (static_cast<T>(1) + z * z);
		}

		return static_cast<T>(std::sin(value));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto cos(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if (functional::is_nan(value)) { return std::numeric_limits<T>::quiet_NaN(); }

			if (std::numeric_limits<T>::min() > functional::abs(value - std::numbers::pi_v<T> / 2)) { return 0; }

			if (std::numeric_limits<T>::min() > functional::abs(value + std::numbers::pi_v<T> / 2)) { return -0; }

			if (std::numeric_limits<T>::min() > functional::abs(value - std::numbers::pi_v<T>)) { return -1; }

			if (std::numeric_limits<T>::min() > functional::abs(value + std::numbers::pi_v<T>)) { return -1; }

			// cos(x) = (1 - tan²(x/2)) / (1 + tan²(x/2))
			const auto z = functional::tan(value / static_cast<T>(2));
			return (static_cast<T>(1) - z * z) / (static_cast<T>(1) + z * z);
		}

		return static_cast<T>(std::cos(value));
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
} // namespace gal::prometheus::functional
