// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.memory:read_write;

import std;
import gal.prometheus.error;
import gal.prometheus.functional;

#else
#include <type_traits>

#include <prometheus/macro.hpp>
#include <error/error.hpp>
#include <functional/functional.hpp>
#endif

namespace gal::prometheus::memory
{
	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	template<typename T, typename In>
		requires std::is_arithmetic_v<T> and (functional::type_list<char, const char, signed char, const signed char, unsigned char, const unsigned char, std::byte, const std::byte>.any<In>())
	[[nodiscard]] constexpr auto unaligned_load(const In* source) noexcept -> T
	{
		GAL_PROMETHEUS_DEBUG_NOT_NULL(source, "Cannot unaligned_load from null!");

		T result{};

		GAL_PROMETHEUS_IF_NOT_CONSTANT_EVALUATED
		{
			std::memcpy(&result, source, sizeof(T));
			return result;
		}

		if constexpr (std::endian::native == std::endian::little)// NOLINT
		{
			for (auto i = sizeof(T); i != 0; --i)
			{
				if constexpr (sizeof(result) > 1) { result <<= 8; }
				result |= static_cast<std::uint8_t>(source[i - 1]);
			}
		}
		else
		{
			for (auto i = 0; i != sizeof(T); ++i)
			{
				if constexpr (sizeof(result) > 1) { result <<= 8; }
				result |= static_cast<std::uint8_t>(source[i]);
			}
		}

		return result;
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	[[nodiscard]] constexpr auto unaligned_load(const void* source) noexcept -> T { return unaligned_load<T>(static_cast<const std::byte*>(source)); }

	template<typename T, typename Out>
		requires std::is_arithmetic_v<T> and (functional::type_list<char, const char, signed char, const signed char, unsigned char, const unsigned char, std::byte, const std::byte>.any<Out>())
	constexpr auto unaligned_store(const T value, Out* dest) noexcept -> void
	{
		GAL_PROMETHEUS_DEBUG_NOT_NULL(dest, "Cannot unaligned_store from null!");

		using unsigned_type = std::make_unsigned_t<T>;

		const auto unsigned_value = static_cast<unsigned_type>(value);

		GAL_PROMETHEUS_IF_NOT_CONSTANT_EVALUATED
		{
			std::memcpy(dest, &unsigned_value, sizeof(T));
			return;
		}

		if constexpr (std::endian::native == std::endian::little)// NOLINT
		{
			for (auto i = 0; i != sizeof(T); ++i)
			{
				dest[i] = static_cast<Out>(unsigned_value);
				unsigned_value >>= 8;
			}
		}
		else
		{
			for (auto i = sizeof(T); i != 0; --i)
			{
				dest[i - 1] = static_cast<Out>(unsigned_value);
				unsigned_value >>= 8;
			}
		}
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	constexpr auto unaligned_store(const T value, void* dest) noexcept -> void { unaligned_load<T>(value, static_cast<std::byte*>(dest)); }

	GAL_PROMETHEUS_MODULE_EXPORT_END
}// namespace gal::prometheus::memory
