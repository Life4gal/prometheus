// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.concurrency:unfair_mutex;

import std;
import gal.prometheus.meta;

import :thread;

#else
#pragma once

#include <format>

#include <prometheus/macro.hpp>
#include <concurrency/thread.ixx>
#include <meta/meta.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::concurrency)
{
	class UnfairMutex final
	{
		// format
		friend std::formatter<UnfairMutex>;

	public:
		using semaphore_type = std::atomic_unsigned_lock_free;
		using semaphore_value_type = semaphore_type::value_type;

		constexpr static bool check_deadlock = GAL_PROMETHEUS_COMPILER_DEBUG;

	private:
		enum class SemaphoreValue : semaphore_value_type
		{
			UNLOCKED = 0,
			LOCKED_NO_WAITER = 1,
			LOCKED = 2,
		};

		semaphore_type semaphore_;

		[[nodiscard]] auto holds_invariant() const noexcept -> bool;

		auto lock_contended(semaphore_value_type expected) noexcept -> void;

	public:
		constexpr UnfairMutex(const UnfairMutex&) noexcept = delete;
		constexpr UnfairMutex(UnfairMutex&&) noexcept = delete;
		constexpr auto operator=(const UnfairMutex&) noexcept -> UnfairMutex& = delete;
		constexpr auto operator=(UnfairMutex&&) noexcept -> UnfairMutex& = delete;

		UnfairMutex() noexcept;

		~UnfairMutex() noexcept;

		auto is_locked() const noexcept -> bool;

		auto lock() noexcept -> void;

		auto try_lock() noexcept -> bool;

		auto unlock() noexcept -> void;
	};

	class UnfairRecursiveMutex final
	{
		// format
		friend std::formatter<UnfairMutex>;

	public:
		using mutex_type = UnfairMutex;
		using count_type = std::uint32_t;

	private:
		mutex_type mutex_;
		std::atomic<thread_id> owner_;
		count_type count_;

	public:
		constexpr UnfairRecursiveMutex(const UnfairRecursiveMutex&) noexcept = delete;
		constexpr UnfairRecursiveMutex(UnfairRecursiveMutex&&) noexcept = delete;
		constexpr auto operator=(const UnfairRecursiveMutex&) noexcept -> UnfairRecursiveMutex& = delete;
		constexpr auto operator=(UnfairRecursiveMutex&&) noexcept -> UnfairRecursiveMutex& = delete;

		UnfairRecursiveMutex() noexcept;

		constexpr ~UnfairRecursiveMutex() noexcept = default;

		/**
		 * @brief Check if the lock is held by current thread.
		 * @return The number of recursive locks the current thread has taken.
		 * @retval 0 The current thread does not have a lock, or no-thread have a lock.
		 */
		[[nodiscard]] auto recurse_count() const noexcept -> count_type;

		auto lock() noexcept -> void;

		[[nodiscard]] auto try_lock() noexcept -> bool;

		auto unlock() noexcept -> void;
	};
} // namespace gal::prometheus::concurrency

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
{
	template<>
	struct formatter<gal::prometheus::concurrency::UnfairMutex>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::concurrency::UnfairMutex& mutex, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
					context.out(),
					"{}",
					gal::prometheus::meta::name_of<gal::prometheus::concurrency::UnfairMutex::SemaphoreValue>(
							mutex.semaphore_.load(memory_order::relaxed)
							)
					);
		}
	};

	template<>
	struct formatter<gal::prometheus::concurrency::UnfairRecursiveMutex>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::concurrency::UnfairRecursiveMutex& mutex, FormatContext& context) const noexcept -> auto
		{
			return formatter<gal::prometheus::concurrency::UnfairRecursiveMutex::mutex_type>::format(mutex.mutex_, context);
		}
	};
}
