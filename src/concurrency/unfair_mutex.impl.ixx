// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:concurrency.unfair_mutex.impl;

import std;

#if GAL_PROMETHEUS_COMPILER_DEBUG
import :error;
#endif

import :concurrency.unfair_mutex;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#include <mutex>
#include <vector>
#include <algorithm>

#include <prometheus/macro.hpp>

#include <concurrency/unfair_mutex.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

namespace
{
	using gal::prometheus::concurrency::UnfairMutex;

	using deadlock_mutex_type = std::mutex;
	using deadlock_graph_node_type = std::pair<const UnfairMutex*, const UnfairMutex*>;
	using deadlock_graph_type = std::vector<deadlock_graph_node_type>;
	using deadlock_stack_type = std::vector<const UnfairMutex*>;

	deadlock_mutex_type deadlock_mutex;
	deadlock_graph_type deadlock_graph;
	thread_local deadlock_stack_type deadlock_stack;

	/**
	 * @brief The order in which mutexes where locked, each pair gives a first before second order.
	 */
	auto deadlock_check_graph(const UnfairMutex* const self) noexcept -> const UnfairMutex*
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(self != nullptr);

		const auto lock = std::scoped_lock{deadlock_mutex};

		for (const auto before: deadlock_stack)
		{
			const auto correct_order = deadlock_graph_node_type{before, self};
			const auto reverse_order = deadlock_graph_node_type{self, before};

			if (std::ranges::binary_search(deadlock_graph, correct_order))
			{
				// `self` has been locked in the correct order in comparison to `before`
				continue;
			}

			if (std::ranges::binary_search(deadlock_graph, reverse_order))
			{
				// `self` has been locked in the reverse order in comparison to `before`
				return before;
			}

			// insert the new correct order in the sorted graph
			const auto target_place = std::ranges::upper_bound(deadlock_graph, correct_order);
			deadlock_graph.insert(target_place, correct_order);
		}

		return nullptr;
	}

	/**
	 * @brief Lock a mutex on this thread.
	 * @param self The mutex that is being locked.
	 * @return
	 *	nullptr: success
	 *	self: the mutex was already locked
	 *	other_mutex: potential deadlock is found
	 */
	[[maybe_unused]] auto deadlock_lock(const UnfairMutex* const self) noexcept -> const UnfairMutex*
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(self != nullptr);

		if (std::ranges::contains(deadlock_stack, self))
		{
			// `self` already locked by the current thread
			return self;
		}

		if (const auto before = deadlock_check_graph(self);
			before != nullptr)
		{
			// trying to lock `self` after `before` in previously reversed order
			return before;
		}

		deadlock_stack.push_back(self);
		return nullptr;
	}

	/**
	 * @brief Unlock an object on this thread.
	 * @param self The mutex that is being locked.
	 */
	[[maybe_unused]] auto deadlock_unlock(const UnfairMutex* const self) noexcept -> bool
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(self != nullptr);

		if (deadlock_stack.empty())
		{
			// trying to unlock `self`, but nothing on this thread was locked
			return false;
		}

		if (deadlock_stack.back() != self)
		{
			// trying to unlock `self`, but unlocking in different order
			return false;
		}

		deadlock_stack.pop_back();
		return true;
	}

	/**
	 * @brief Remove the object from the detection.
	 * @param self The mutex to remove from the lock order graph.
	 */
	[[maybe_unused]] auto deadlock_remove(const UnfairMutex* const self) noexcept -> bool
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(self != nullptr);

		const auto lock = std::scoped_lock{deadlock_mutex};
		const auto sub_range =
				std::ranges::remove_if(deadlock_graph, [self](const auto& pair) noexcept { return pair.first == self or pair.second == self; });
		if (sub_range.empty())
		{
			// removed by hand
			return false;
		}
		deadlock_graph.erase(sub_range.begin(), sub_range.end());
		return true;
	}
}

GAL_PROMETHEUS_COMPILER_MODULE_IMPL_NAMESPACE(gal::prometheus::concurrency)
{
	auto UnfairMutex::holds_invariant() const noexcept -> bool
	{
		const auto v = static_cast<SemaphoreValue>(semaphore_.load(std::memory_order::relaxed));
		return v == SemaphoreValue::UNLOCKED or v == SemaphoreValue::LOCKED_NO_WAITER or v == SemaphoreValue::LOCKED;
	}

	auto UnfairMutex::lock_contended(semaphore_value_type expected) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());

		do
		{
			const auto should_wait = expected == std::to_underlying(SemaphoreValue::LOCKED);

			expected = std::to_underlying(SemaphoreValue::LOCKED_NO_WAITER);
			if (should_wait or semaphore_.compare_exchange_strong(expected, std::to_underlying(SemaphoreValue::LOCKED)))
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());
				semaphore_.wait(std::to_underlying(SemaphoreValue::LOCKED));
			}

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());
			expected = std::to_underlying(SemaphoreValue::UNLOCKED);
		} while (not semaphore_.compare_exchange_strong(expected, std::to_underlying(SemaphoreValue::LOCKED)));
	}

	UnfairMutex::UnfairMutex() noexcept : semaphore_{std::to_underlying(SemaphoreValue::UNLOCKED)} {}

	UnfairMutex::~UnfairMutex() noexcept
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not is_locked());
		if constexpr (check_deadlock) { deadlock_remove(this); }
	}

	auto UnfairMutex::is_locked() const noexcept -> bool
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());
		return semaphore_.load(std::memory_order::relaxed) != std::to_underlying(SemaphoreValue::UNLOCKED);
	}

	auto UnfairMutex::lock() noexcept -> void
	{
		if constexpr (check_deadlock)
		{
			const auto deadlock_other = deadlock_lock(this);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(deadlock_other != this, "this mutex is already locked");
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(deadlock_other == nullptr, "Potential deadlock because of different lock ordering of mutexes");
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());

		if (auto expected = std::to_underlying(SemaphoreValue::UNLOCKED);
			not semaphore_.compare_exchange_strong(expected, std::to_underlying(SemaphoreValue::LOCKED_NO_WAITER), std::memory_order::acquire))
		[[unlikely]]
		{
			lock_contended(expected);
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());
	}

	auto UnfairMutex::try_lock() noexcept -> bool
	{
		if constexpr (check_deadlock)
		{
			if constexpr (check_deadlock)
			{
				const auto deadlock_other = deadlock_lock(this);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(deadlock_other != this, "this mutex is already locked");
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(deadlock_other == nullptr, "Potential deadlock because of different lock ordering of mutexes");
			}
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());

		if (auto expected = std::to_underlying(SemaphoreValue::UNLOCKED);
			not semaphore_.compare_exchange_strong(expected, std::to_underlying(SemaphoreValue::LOCKED_NO_WAITER), std::memory_order::acquire))
		[[unlikely]]
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());

			if constexpr (check_deadlock)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(deadlock_unlock(this), "Unlock failed in reverse order");
			}

			return false;
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());
		return true;
	}

	auto UnfairMutex::unlock() noexcept -> void
	{
		if constexpr (check_deadlock)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(deadlock_unlock(this), "Unlock failed in reverse order");
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());

		if (semaphore_.exchange(
			    std::to_underlying(SemaphoreValue::UNLOCKED),
			    std::memory_order::relaxed
		    ) != std::to_underlying(SemaphoreValue::LOCKED_NO_WAITER))
		[[unlikely]]
		{
			semaphore_.notify_one();
		}
		else { std::atomic_thread_fence(std::memory_order::release); }

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(holds_invariant());
	}

	UnfairRecursiveMutex::UnfairRecursiveMutex() noexcept
		: owner_{invalid_thread_id},
		  count_{0} {}

	auto UnfairRecursiveMutex::recurse_count() const noexcept -> count_type
	{
		if (owner_.load(std::memory_order::acquire) == this_thread::get_id()) { return count_; }
		return 0;
	}

	auto UnfairRecursiveMutex::lock() noexcept -> void
	{
		if (const auto id = this_thread::get_id(); owner_.load(std::memory_order::acquire) == id)
		{
			// owned by current thread
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(count_ != 0);
			count_ += 1;
		}
		else
		{
			mutex_.lock();

			// first lock
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(count_ == 0);
			count_ = 1;

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(owner_.load(std::memory_order::relaxed) == invalid_thread_id);
			owner_.store(id, std::memory_order::release);
		}
	}

	auto UnfairRecursiveMutex::try_lock() noexcept -> bool
	{
		if (const auto id = this_thread::get_id(); owner_.load(std::memory_order::acquire) == id)
		{
			// owned by current thread
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(count_ != 0);
			count_ += 1;

			return true;
		}
		else if (mutex_.try_lock())
		{
			// first lock
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(count_ == 0);
			count_ = 1;

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(owner_.load(std::memory_order::relaxed) == invalid_thread_id);
			owner_.store(id, std::memory_order::release);

			return true;
		}

		// owned by another thread
		return false;
	}

	auto UnfairRecursiveMutex::unlock() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(count_ != 0);

		count_ -= 1;
		if (count_ == 0)
		{
			owner_.store(invalid_thread_id, std::memory_order::release);
			mutex_.unlock();
		}
	}
}
