// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:functional.hash;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>
#include <ranges>
#include <algorithm>
#include <string>

#include <prometheus/macro.hpp>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(functional)
{
	using hash_result_type = std::uint64_t;
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_INTERNAL(functional)
{
	template<typename T, typename Hash>
	struct hash
	{
		using value_type = T;
		using hash_type = Hash;

		[[nodiscard]] constexpr auto operator()(const value_type& value) const noexcept -> hash_result_type
		{
			return hash_type{}(value);
		}
	};

	template<std::ranges::contiguous_range String, typename Hash>
		requires std::is_same_v<typename String::value_typ, char>
	struct hash<String, Hash>
	{
		using is_transparent = int;
		using value_type = String;
		using hash_type = Hash;

		[[nodiscard]] constexpr auto operator()(const std::ranges::viewable_range auto string) const noexcept -> hash_result_type
		{
			// FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
			static_assert(sizeof(hash_result_type) == 8);
			constexpr hash_result_type hash_init{14695981039346656037ull};
			constexpr hash_result_type hash_prime{1099511628211ull};

			auto result = hash_init;
			std::ranges::for_each(
				string,
				[&result](const auto v) noexcept -> void
				{
					result ^= v;
					result *= hash_prime;
				}
			);
			return result;
		}

		template<typename S>
		[[nodiscard]] constexpr auto operator()(S&& string) const noexcept -> hash_result_type
		{
			return this->operator()(std::forward<S>(string) | std::views::all);
		}
	};

	template<typename Pointer, typename Hash>
		requires std::is_pointer_v<Pointer>
	struct hash<Pointer, Hash>
	{
		using is_transparent = int;
		using value_type = Pointer;
		using hash_type = Hash;

		[[nodiscard]] constexpr auto operator()(const value_type pointer) const noexcept -> hash_result_type
		{
			return hash_type{}(pointer);
		}

		template<typename U>
			requires std::is_base_of_v<typename std::pointer_traits<value_type>::element_type, typename std::pointer_traits<U>::element_type>
		[[nodiscard]] constexpr auto operator()(const U pointer) const noexcept -> hash_result_type
		{
			return hash_type{}(pointer);
		}
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(functional)
{
	template<typename T, typename Hash = std::hash<T>>
	constexpr auto hash = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::hash<T, Hash>{};

	[[nodiscard]] constexpr auto hash_combine_2(const hash_result_type hash1, const hash_result_type hash2) noexcept -> hash_result_type
	{
		static_assert(sizeof(hash_result_type) == 8);
		return hash1 + 0x9e3779b97f681800ull + (hash2 << 6) + (hash2 >> 2);
	}

	template<typename First, typename Second, typename... Reset>
	[[nodiscard]] constexpr auto hash_combine(First&& first, Second&& second, Reset&&... reset) noexcept -> hash_result_type requires requires
	{
		hash<std::remove_cvref_t<First>>;
		hash<std::remove_cvref_t<Second>>;
		(hash<std::remove_cvref_t<Reset>>, ...);
	}
	{
		if constexpr (sizeof...(Reset) == 0)
		{
			const auto hash1 = hash<std::remove_cvref_t<First>>(std::forward<First>(first));
			const auto hash2 = hash<std::remove_cvref_t<Second>>(std::forward<Second>(second));
			return hash_combine_2(hash1, hash2);
		}
		else
		{
			const auto hash1 = hash<std::remove_cvref_t<First>>(std::forward<First>(first));
			const auto hash2 = hash_combine(std::forward<Second>(second), std::forward<Reset>(reset)...);
			return hash_combine_2(hash1, hash2);
		}
	}
}
