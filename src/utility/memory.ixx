// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.utility:memory;

import std;
import gal.prometheus.error;

import :concepts;
import :cast;

namespace gal::prometheus::utility
{
	export
	{
		template<typename T>
		[[nodiscard]] constexpr auto is_aligned(const T* pointer, const std::size_t alignment = std::alignment_of_v<T>) noexcept -> bool { return reinterpret_cast<std::ptrdiff_t>(pointer) % alignment == 0; }

		template<typename T>
		[[nodiscard]] constexpr auto ceil_align(const T* pointer, const std::size_t alignment) noexcept -> T* { return GAL_PROMETHEUS_START_LIFETIME_AS(T, reinterpret_cast<T*>(ceil(reinterpret_cast<std::uintptr_t>(pointer), wide_cast<std::uintptr_t>(alignment)))); }

		template<typename T>
		[[nodiscard]] constexpr auto is_ceil_align(const T* pointer, const std::size_t alignment) noexcept -> bool { return pointer == ceil_align(pointer, alignment); }

		template<typename T>
		[[nodiscard]] constexpr auto floor_align(const T* pointer, const std::size_t alignment) noexcept -> T* { return GAL_PROMETHEUS_START_LIFETIME_AS(T, reinterpret_cast<T*>(floor(reinterpret_cast<std::uintptr_t>(pointer), wide_cast<std::uintptr_t>(alignment)))); }

		template<typename T>
		[[nodiscard]] constexpr auto is_floor_align(const T* pointer, const std::size_t alignment) noexcept -> bool { return pointer == floor_align(pointer, alignment); }

		/**
		 * @brief Make an unaligned load of an unsigned integer.
		 */
		template<concepts::arithmetic T, concepts::byte_like In>
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

		template<concepts::arithmetic T>
		[[nodiscard]] constexpr auto unaligned_load(const void* source) noexcept -> T { return unaligned_load<T>(static_cast<const std::byte*>(source)); }

		/**
		 * @brief Make an unaligned store of an unsigned integer.
		 */
		template<concepts::arithmetic T, concepts::byte_like In>
		constexpr auto unaligned_store(const T value, In* dest) noexcept -> void
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
					dest[i] = static_cast<In>(unsigned_value);
					unsigned_value >>= 8;
				}
			}
			else
			{
				for (auto i = sizeof(T); i != 0; --i)
				{
					dest[i - 1] = static_cast<In>(unsigned_value);
					unsigned_value >>= 8;
				}
			}
		}

		template<concepts::arithmetic T>
		constexpr auto unaligned_store(const T value, void* dest) noexcept -> void { unaligned_load<T>(value, static_cast<std::byte*>(dest)); }

		/**
		 * @brief Advance a pointer by a number of bytes.
		 * @tparam T The object type.
		 * @param pointer A pointer to the object.
		 * @param distance The number of bytes to advance the pointer, may be negative.
		 * @return A new point which advanced.
		 */
		template<typename T>
		[[nodiscard]] constexpr auto byte_advance(T* pointer, const std::ptrdiff_t distance) noexcept -> T*
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(pointer);
			return GAL_PROMETHEUS_START_LIFETIME_AS(T, reinterpret_cast<T*>(reinterpret_cast<char*>(pointer) + distance));
		}

		/**
		 * @brief Advance a pointer by a number of bytes.
		 * @tparam T The object type.
		 * @param pointer A pointer to the object.
		 * @param distance The number of bytes to advance the pointer, may be negative.
		 * @return A new point which advanced.
		 */
		template<typename T>
		[[nodiscard]] constexpr auto byte_advance(const T* pointer, const std::ptrdiff_t distance) noexcept -> const T*
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(pointer);
			return GAL_PROMETHEUS_START_LIFETIME_AS(T, reinterpret_cast<const T*>(reinterpret_cast<char*>(pointer) + distance));
		}
	}
}// namespace gal::prometheus::utility
