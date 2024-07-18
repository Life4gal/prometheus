// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>
#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

export module gal.prometheus.functional:math;

import std;
GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

#else
#pragma once

#include <cmath>
#include <numbers>
#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

#include <prometheus/macro.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

namespace gal::prometheus::functional
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

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
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				//
				return value != value;
			}

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
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				//
				return value > 0 ? value : -value;
			}

			return std::abs(value);
		}
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
	[[nodiscard]] constexpr auto ceil(const T value) noexcept -> T
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

				return static_cast<T>(static_cast<unsigned long long>(value) + 1);
			}
		}

		return std::ceil(value);
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto tgamma(const T value) noexcept -> T
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(value >= 0);

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
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(exp >= 0);

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
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(value >= 0);

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
	[[nodiscard]] constexpr auto hypot(const T x, const std::type_identity_t<T> y, const std::type_identity_t<T> z) noexcept -> T
	{
		GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
		{
			//
			return functional::sqrt(functional::pow(x, 2) + functional::pow(y, 2) + functional::pow(z, 2));
		}

		return static_cast<T>(std::hypot(x, y, z));
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
	}

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

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto normalize(const T x, const T y) noexcept -> std::pair<T, T>
	{
		const auto d = functional::pow(x, 2) + functional::pow(y, 2);
		const auto d2 = [d]
		{
			if constexpr (std::is_floating_point_v<T>)
			{
				return d;
			}
			else
			{
				if constexpr (sizeof(T) == sizeof(float))
				{
					return static_cast<float>(d);
				}
				else if constexpr (sizeof(T) == sizeof(double))
				{
					return static_cast<double>(d);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
		}();

		if (d2 > static_cast<decltype(d2)>(0))
		{
			// todo

			const auto inv_len = [d2]
			{
				if constexpr (sizeof(d2) == sizeof(float))
				{
					#if defined(__AVX512F__)
					__m512 d2_v = _mm512_set1_ps(d2);
					__m512 inv_len_v = _mm512_rsqrt14_ps(d2_v);
					return  _mm512_cvtss_f32(inv_len_v);
					#elif defined(__AVX__)
					__m256 d2_v = _mm256_set1_ps(d2);
					__m256 inv_len_v = _mm256_rsqrt_ps(d2_v);
					return _mm256_cvtss_f32(inv_len_v);
					#elif defined(__SSE4_1__) or defined(__SSE3__) or defined(__SSE__)
					__m128 d2_v = _mm_set_ss(d2);
					__m128 inv_len_v = _mm_rsqrt_ss(d2_v);
					return _mm_cvtss_f32(inv_len_v);
					#else
					return 1.0f / functional::sqrt(d2);
					#endif
				}
				else if constexpr (sizeof(d2) == sizeof(double))
				{
					#if defined(__AVX512F__)
					__m512d d2_v = _mm512_set1_pd(d2);
					__m512d sqrt_d2_v = _mm512_sqrt_pd(d2_v);
					__m512d inv_sqrt_d2_v = _mm512_div_pd(_mm512_set1_pd(1.0), sqrt_d2_v);
					return _mm512_cvtsd_f64(inv_sqrt_d2_v);
					#elif defined(__AVX__)
					__m256d d2_v = _mm256_set1_pd(d2);
					__m256d sqrt_d2_v = _mm256_sqrt_pd(d2_v);
					__m256d inv_sqrt_d2_v = _mm256_div_pd(_mm256_set1_pd(1.0), sqrt_d2_v);
					return _mm256_cvtsd_f64(inv_sqrt_d2_v);
					#elif defined(__SSE4_1__) or defined(__SSE3__) or defined(__SSE__)
					__m128d d2_v = _mm_set1_pd(d2);
					__m128d sqrt_d2_v = _mm_sqrt_pd(d2_v);
					__m128d inv_sqrt_d2_v = _mm_div_pd(_mm_set1_pd(1.0), sqrt_d2_v);
					return _mm_cvtsd_f64(inv_sqrt_d2_v);
					#else
					return 1.0 / functional::sqrt(d2);
					#endif
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}();

			return {static_cast<T>(x * inv_len), static_cast<T>(y * inv_len)};
		}

		return {x, y};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
