// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <algorithm>
#include <format>

#include <prometheus/macro.hpp>

#include <meta/dimension.hpp>
#include <math/cmath.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::primitive
{
	template<std::size_t, typename>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point;

	template<typename T>
		requires(std::is_arithmetic_v<T>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point<2, T> final : meta::dimension<basic_point<2, T>>
	{
		using value_type = T;

		value_type x;
		value_type y;

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() const noexcept -> value_type
		{
			if constexpr (Index == 0) { return x; }
			else if constexpr (Index == 1) { return y; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 2)
		[[nodiscard]] constexpr auto get() noexcept -> value_type&
		{
			if constexpr (Index == 0) { return x; }
			else if constexpr (Index == 1) { return y; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr explicit operator basic_point<3, value_type>() const noexcept
		{
			return {.x = x, .y = y, .z = value_type{0}};
		}

		template<std::convertible_to<value_type> U = value_type>
		[[nodiscard]] constexpr auto distance(const basic_point<2, U>& other) const noexcept -> value_type //
		{
			return math::hypot(x - static_cast<value_type>(other.x), y - static_cast<value_type>(other.y));
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] constexpr auto clamp(
			const basic_point<2, Low>& low,
			const basic_point<2, High>& high
		) noexcept -> basic_point&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.x < high.x);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.y < high.y);

			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				x = std::ranges::min(std::ranges::max(x, static_cast<value_type>(low.x)), static_cast<value_type>(high.x));
				y = std::ranges::min(std::ranges::max(y, static_cast<value_type>(low.y)), static_cast<value_type>(high.y));
			}
			else
			{
				x = std::ranges::clamp(x, low.x, high.x);
				y = std::ranges::clamp(y, low.y, high.y);
			}

			return *this;
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] friend constexpr auto clamp(
			const basic_point& point,
			const basic_point<2, Low>& low,
			const basic_point<2, High>& high
		) noexcept -> basic_point
		{
			auto result{point};

			result.clamp(low, high);
			return result;
		}

		template<std::size_t Index, std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
			requires (Index < 2)
		[[nodiscard]] constexpr auto between(
			const basic_point<2, T1>& p1,
			const basic_point<2, T2>& p2
		) const noexcept -> bool
		{
			if constexpr (Index == 0)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(p1.x) < static_cast<value_type>(p2.x));

				return x >= static_cast<value_type>(p1.x) and x < static_cast<value_type>(p2.x);
			}
			else if constexpr (Index == 1)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(p1.y) < static_cast<value_type>(p2.y));

				return y >= static_cast<value_type>(p1.y) and y < static_cast<value_type>(p2.y);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
		[[nodiscard]] constexpr auto between(
			const basic_point<2, T1>& p1,
			const basic_point<2, T2>& p2
		) const noexcept -> bool
		{
			return
					this->template between<0>(p1, p2) and
					this->template between<1>(p1, p2);
		}
	};

	template<typename T>
		requires(std::is_arithmetic_v<T>)
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_point<3, T> final : meta::dimension<basic_point<3, T>>
	{
		using value_type = T;

		value_type x;
		value_type y;
		value_type z;

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() const noexcept -> value_type
		{
			if constexpr (Index == 0) { return x; }
			else if constexpr (Index == 1) { return y; }
			else if constexpr (Index == 2) { return z; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::size_t Index>
			requires(Index < 3)
		[[nodiscard]] constexpr auto get() noexcept -> value_type&
		{
			if constexpr (Index == 0) { return x; }
			else if constexpr (Index == 1) { return y; }
			else if constexpr (Index == 2) { return z; }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		[[nodiscard]] constexpr explicit operator basic_point<2, value_type>() const noexcept
		{
			return {.x = x, .y = y};
		}

		template<std::convertible_to<value_type> U = value_type>
		[[nodiscard]] constexpr auto distance(const basic_point<3, U>& other) const noexcept -> value_type
		{
			return math::hypot(x - static_cast<value_type>(other.x), y - static_cast<value_type>(other.y), z - static_cast<value_type>(other.z));
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] constexpr auto clamp(
			const basic_point<3, Low>& low,
			const basic_point<3, High>& high
		) noexcept -> basic_point&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.x < high.x);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.y < high.y);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(low.z < high.z);

			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				x = std::ranges::min(std::ranges::max(x, static_cast<value_type>(low.x)), static_cast<value_type>(high.x));
				y = std::ranges::min(std::ranges::max(y, static_cast<value_type>(low.y)), static_cast<value_type>(high.y));
				z = std::ranges::min(std::ranges::max(z, static_cast<value_type>(low.z)), static_cast<value_type>(high.z));
			}
			else
			{
				x = std::ranges::clamp(x, low.x, high.x);
				y = std::ranges::clamp(y, low.y, high.y);
				z = std::ranges::clamp(z, low.z, high.z);
			}

			return *this;
		}

		template<std::convertible_to<value_type> Low = value_type, std::convertible_to<value_type> High = value_type>
		[[nodiscard]] friend constexpr auto clamp(
			const basic_point& point,
			const basic_point<3, Low>& low,
			const basic_point<3, High>& high
		) noexcept -> basic_point
		{
			auto result{point};

			result.clamp(low, high);
			return result;
		}

		template<std::size_t Index, std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
			requires (Index < 3)
		[[nodiscard]] constexpr auto between(
			const basic_point<3, T1>& p1,
			const basic_point<3, T2>& p2
		) const noexcept -> bool
		{
			if constexpr (Index == 0)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(p1.x) < static_cast<value_type>(p2.x));

				return x >= static_cast<value_type>(p1.x) and x < static_cast<value_type>(p2.x);
			}
			else if constexpr (Index == 1)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(p1.y) < static_cast<value_type>(p2.y));

				return y >= static_cast<value_type>(p1.y) and y < static_cast<value_type>(p2.y);
			}
			else if constexpr (Index == 2)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(static_cast<value_type>(p1.z) < static_cast<value_type>(p2.z));

				return z >= static_cast<value_type>(p1.z) and z < static_cast<value_type>(p2.z);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<std::convertible_to<value_type> T1 = value_type, std::convertible_to<value_type> T2 = value_type>
		[[nodiscard]] constexpr auto between(
			const basic_point<3, T1>& p1,
			const basic_point<3, T2>& p2
		) const noexcept -> bool
		{
			return
					this->template between<0>(p1, p2) and
					this->template between<1>(p1, p2) and
					this->template between<2>(p1, p2);
		}
	};

	template<typename T>
	using basic_point_2d = basic_point<2, T>;

	template<typename T>
	using basic_point_3d = basic_point<3, T>;
}

namespace std
{
	template<std::size_t Index, std::size_t N, typename T>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_point<N, T>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<std::size_t N, typename T>
	struct tuple_size<gal::prometheus::primitive::basic_point<N, T>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, N> {};

	template<std::size_t N, typename T>
	struct formatter<gal::prometheus::primitive::basic_point<N, T>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_point<N, T>& point, FormatContext& context) const noexcept -> auto
		{
			if constexpr (N == 2)
			{
				return std::format_to(context.out(), "({},{})", point.x, point.y);
			}
			else if constexpr (N == 3)
			{
				return std::format_to(context.out(), "({},{},{})", point.x, point.y, point.z);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
	};
} // namespace std
