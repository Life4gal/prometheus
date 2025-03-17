// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <cmath>
#include <numbers>
#include <numeric>
#include <limits>

#include <prometheus/macro.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#if not defined(__cpp_lib_constexpr_cmath) or __cpp_lib_constexpr_cmath < 202306L
#define CMATH_WORKAROUND_REQUIRED
#endif

namespace gal::prometheus::math
{
	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto is_nan(const T value) noexcept -> bool
	{
		if constexpr (not std::is_floating_point_v<T>)
		{
			return false;
		}
		else
		{
			#if defined(CMATH_WORKAROUND_REQUIRED)
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				// ReSharper disable once CppIdenticalOperandsInBinaryExpression
				return value != value; // NOLINT(misc-redundant-expression)
			}
			#endif

			return std::isnan(value);
		}
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto abs(const T value) noexcept -> T
	{
		if constexpr (std::is_unsigned_v<T>)
		{
			return value;
		}
		else
		{
			#if defined(CMATH_WORKAROUND_REQUIRED)
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				return value > 0 ? value : -value;
			}
			#endif

			return std::abs(value);
		}
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto floor(const T value) noexcept -> T
	{
		if constexpr (not std::is_floating_point_v<T>)
		{
			return value;
		}
		else
		{
			#if defined(CMATH_WORKAROUND_REQUIRED)
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				if (value >= 0 or static_cast<T>(static_cast<unsigned long long>(value)) == value)
				{
					return static_cast<T>(static_cast<unsigned long long>(value));
				}

				return static_cast<T>(static_cast<unsigned long long>(value) - 1);
			}
			#endif

			return std::floor(value);
		}
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto ceil(const T value) noexcept -> T
	{
		if constexpr (not std::is_floating_point_v<T>)
		{
			return value;
		}
		else
		{
			#if defined(CMATH_WORKAROUND_REQUIRED)
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				if (value >= 0 or static_cast<T>(static_cast<unsigned long long>(value)) == value)
				{
					return static_cast<T>(static_cast<unsigned long long>(value));
				}

				return static_cast<T>(static_cast<unsigned long long>(value) + 1);
			}
			#endif

			return std::ceil(value);
		}
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	// ReSharper disable once IdentifierTypo
	[[nodiscard]] constexpr auto tgamma(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(value >= T{0});

		if constexpr (not std::is_floating_point_v<T>)
		{
			return value;
		}
		else
		{
			#if defined(CMATH_WORKAROUND_REQUIRED)
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				return (value <= 1) ? 1 : (value * math::tgamma(value - 1));
			}
			#endif

			return static_cast<T>(std::tgamma(value));
		}
	}

	template<std::integral T>
	[[nodiscard]] constexpr auto factorial(const T value) noexcept -> T //
	{
		return math::tgamma(value);
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto pow(const T base, const int exp) noexcept -> T
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(exp >= 0);

		#if defined(CMATH_WORKAROUND_REQUIRED)
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if (exp == 0)
			{
				return static_cast<T>(1);
			}

			return static_cast<T>(base * math::pow(base, exp - 1));
		}
		#endif

		return static_cast<T>(std::pow(base, exp));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto sqrt(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(value >= 0);

		#if defined(CMATH_WORKAROUND_REQUIRED)
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if (value == 0) // NOLINT(clang-diagnostic-float-equal)
			{
				return value;
			}

			auto prev = static_cast<T>(0);
			auto current = static_cast<T>(value / 2);

			while (current != prev) // NOLINT(clang-diagnostic-float-equal)
			{
				prev = current;
				current = (current + value / current) / 2;
			}

			return current;
		}
		#endif

		return static_cast<T>(std::sqrt(value));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	// ReSharper disable once IdentifierTypo
	[[nodiscard]] constexpr auto hypot(const T x, const std::type_identity_t<T> y) noexcept -> T
	{
		#if defined(CMATH_WORKAROUND_REQUIRED)
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			return math::sqrt(math::pow(x, 2) + math::pow(y, 2));
		}
		#endif

		return static_cast<T>(std::hypot(x, y));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	// ReSharper disable once IdentifierTypo
	[[nodiscard]] constexpr auto hypot(const T x, const std::type_identity_t<T> y, const std::type_identity_t<T> z) noexcept -> T
	{
		#if defined(CMATH_WORKAROUND_REQUIRED)
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			return math::sqrt(math::pow(x, 2) + math::pow(y, 2) + math::pow(z, 2));
		}
		#endif

		return static_cast<T>(std::hypot(x, y, z));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto normalize(const T x, const T y) noexcept -> auto
	{
		using value_type = std::conditional_t<sizeof(T) == sizeof(float), float, double>;
		using result_type = std::pair<value_type, value_type>;

		if (const auto length = math::hypot(static_cast<value_type>(x), static_cast<value_type>(y));
			length > std::numeric_limits<T>::epsilon())
		{
			return result_type{static_cast<value_type>(x) / length, static_cast<value_type>(y) / length};
		}

		return result_type{x, y};
	}

	#if defined(CMATH_WORKAROUND_REQUIRED)
	// ReSharper disable once IdentifierTypo
	namespace cmath_detail
	{
		template<typename T>
		[[nodiscard]] constexpr auto tan_series_exp(const T value) noexcept -> T
		{
			const auto z = value - std::numbers::pi_v<T> / 2;

			if (std::numeric_limits<T>::min() > math::abs(z))
			{
				return std::numbers::pi_v<T> / 2;
			}

			// this is based on a fourth-order expansion of tan(z) using Bernoulli numbers
			return -1 / z + (z / 3 + (math::pow(z, 3) / 45 + (2 * math::pow(z, 5) / 945 + math::pow(z, 7) / 4725)));
		}

		template<typename T>
		[[nodiscard]] constexpr auto tan_cf_recurse(const T value, const int current, const int max) noexcept -> T
		{
			const auto z = static_cast<T>(2 * current - 1);

			if (current < max) { return z - value / cmath_detail::tan_cf_recurse(value, current + 1, max); }

			return z;
		}

		template<typename T>
		[[nodiscard]] constexpr auto tan_cf_main(const T value) noexcept -> T
		{
			if (value > static_cast<T>(1.55) and value < static_cast<T>(1.6))
			{
				// deals with a singularity at tan(pi/2)
				return cmath_detail::tan_series_exp(value);
			}

			if (value > static_cast<T>(1.4))
			{
				return value / cmath_detail::tan_cf_recurse(value * value, 1, 45);
			}

			if (value > static_cast<T>(1))
			{
				return value / cmath_detail::tan_cf_recurse(value * value, 1, 35);
			}

			return value / cmath_detail::tan_cf_recurse(value * value, 1, 25);
		}

		template<typename T>
		[[nodiscard]] constexpr auto tan_begin(const T value, const int count = 0) noexcept -> T
		{
			if (value > std::numbers::pi_v<T>)
			{
				if (count > 1)
				{
					return std::numeric_limits<T>::quiet_NaN();
				}

				return cmath_detail::tan_begin(value - std::numbers::pi_v<T> * math::floor(value - std::numbers::pi_v<T>), count + 1);
			}

			return cmath_detail::tan_cf_main(value);
		}
	}
	#endif

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto tan(const T value) noexcept -> T
	{
		#if defined(CMATH_WORKAROUND_REQUIRED)
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if (math::is_nan(value))
			{
				return std::numeric_limits<T>::quiet_NaN();
			}

			if (value < static_cast<T>(0))
			{
				return -cmath_detail::tan_begin(-value);
			}

			return cmath_detail::tan_begin(value);
		}
		#endif

		return std::tan(value);
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto sin(const T value) noexcept -> T
	{
		#if defined(CMATH_WORKAROUND_REQUIRED)
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if (math::is_nan(value)) { return std::numeric_limits<T>::quiet_NaN(); }

			if (std::numeric_limits<T>::min() > math::abs(value - std::numbers::pi_v<T> / 2)) { return 1; }

			if (std::numeric_limits<T>::min() > math::abs(value + std::numbers::pi_v<T> / 2)) { return -1; }

			if (std::numeric_limits<T>::min() > math::abs(value - std::numbers::pi_v<T>)) { return 0; }

			if (std::numeric_limits<T>::min() > math::abs(value + std::numbers::pi_v<T>)) { return -0; }

			// sin(x) = 2tan(x/2) / (1 + tan²(x/2))
			const auto z = math::tan(value / static_cast<T>(2));
			return (static_cast<T>(2) * z) / (static_cast<T>(1) + z * z);
		}
		#endif

		return static_cast<T>(std::sin(value));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto cos(const T value) noexcept -> T
	{
		#if defined(CMATH_WORKAROUND_REQUIRED)
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			if (math::is_nan(value)) { return std::numeric_limits<T>::quiet_NaN(); }

			if (std::numeric_limits<T>::min() > math::abs(value - std::numbers::pi_v<T> / 2)) { return 0; }

			if (std::numeric_limits<T>::min() > math::abs(value + std::numbers::pi_v<T> / 2)) { return -0; }

			if (std::numeric_limits<T>::min() > math::abs(value - std::numbers::pi_v<T>)) { return -1; }

			if (std::numeric_limits<T>::min() > math::abs(value + std::numbers::pi_v<T>)) { return -1; }

			// cos(x) = (1 - tan²(x/2)) / (1 + tan²(x/2))
			const auto z = math::tan(value / static_cast<T>(2));
			return (static_cast<T>(1) - z * z) / (static_cast<T>(1) + z * z);
		}
		#endif

		return static_cast<T>(std::cos(value));
	}
}

#undef CMATH_WORKAROUND_REQUIRED
