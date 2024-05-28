// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.concurrency:unfair_mutex;

import std;
import gal.prometheus.error;
import gal.prometheus.meta;
import :thread;

#else
#include <atomic>
#include <memory>
#include <mutex>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <error/error.hpp>
#include <meta/meta.hpp>
#include <concurrency/thread.hpp>
#endif

namespace gal::prometheus::concurrency
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

		[[nodiscard]] auto holds_invariant() const noexcept -> bool //
		{
			const auto v = static_cast<SemaphoreValue>(semaphore_.load(std::memory_order::relaxed));
			return v == SemaphoreValue::UNLOCKED or v == SemaphoreValue::LOCKED_NO_WAITER or v == SemaphoreValue::LOCKED;
		}

		auto lock_contended(semaphore_value_type expected) noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());

			do
			{
				const auto should_wait = expected == std::to_underlying(SemaphoreValue::LOCKED);

				expected = std::to_underlying(SemaphoreValue::LOCKED_NO_WAITER);
				if (should_wait or semaphore_.compare_exchange_strong(expected, std::to_underlying(SemaphoreValue::LOCKED)))
				{
					GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());
					semaphore_.wait(std::to_underlying(SemaphoreValue::LOCKED));
				}

				GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());
				expected = std::to_underlying(SemaphoreValue::UNLOCKED);
			} while (not semaphore_.compare_exchange_strong(expected, std::to_underlying(SemaphoreValue::LOCKED)));
		}

		using deadlock_mutex_type = std::mutex;
		using deadlock_graph_node_type = std::pair<const UnfairMutex*, const UnfairMutex*>;
		using deadlock_graph_type = std::vector<deadlock_graph_node_type>;
		using deadlock_stack_type = std::vector<const UnfairMutex*>;

		inline static deadlock_mutex_type deadlock_mutex_;
		inline static deadlock_graph_type deadlock_graph_;
		inline thread_local static deadlock_stack_type deadlock_stack_;

		/**
		 * @brief The order in which mutexes where locked, each pair gives a first before second order.
		 */
		static auto deadlock_check_graph(const UnfairMutex* const self) noexcept -> const UnfairMutex*
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(self);

			const auto lock = std::scoped_lock{deadlock_mutex_};

			for (const auto before: deadlock_stack_)
			{
				const auto correct_order = deadlock_graph_node_type{before, self};
				const auto reverse_order = deadlock_graph_node_type{self, before};

				if (std::ranges::binary_search(deadlock_graph_, correct_order))
				{
					// `self` has been locked in the correct order in comparison to `before`
					continue;
				}

				if (std::ranges::binary_search(deadlock_graph_, reverse_order))
				{
					// `self` has been locked in the reverse order in comparison to `before`
					return before;
				}

				// insert the new correct order in the sorted graph
				const auto target_place = std::ranges::upper_bound(deadlock_graph_, correct_order);
				deadlock_graph_.insert(target_place, correct_order);
			}

			return nullptr;
		}

		#if GAL_PROMETHEUS_COMPILER_DEBUG

	public:
		#endif

		/**
		 * @brief Lock a mutex on this thread.
		 * @param self The mutex that is being locked.
		 * @return
		 *	nullptr: success
		 *	self: the mutex was already locked
		 *	other_mutex: potential deadlock is found
		 */
		static auto deadlock_lock(const UnfairMutex* const self) noexcept -> const UnfairMutex*
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(self);

			if (std::ranges::contains(deadlock_stack_, self))
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

			deadlock_stack_.push_back(self);
			return nullptr;
		}

		/**
		 * @brief Unlock an object on this thread.
		 * @param self The mutex that is being locked.
		 */
		static auto deadlock_unlock(const UnfairMutex* const self) noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(self);

			if (deadlock_stack_.empty())
			{
				// trying to unlock `self`, but nothing on this thread was locked
				return false;
			}

			if (deadlock_stack_.back() != self)
			{
				// trying to unlock `self`, but unlocking in different order
				return false;
			}

			deadlock_stack_.pop_back();
			return true;
		}

		/**
		 * @brief Remove the object from the detection.
		 * @param self The mutex to remove from the lock order graph.
		 */
		static auto deadlock_remove(const UnfairMutex* const self) noexcept -> bool
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(self);

			const auto lock = std::scoped_lock{deadlock_mutex_};
			const auto sub_range =
					std::ranges::remove_if(
							deadlock_graph_,
							[self](const auto& pair) noexcept { return pair.first == self or pair.second == self; }
							);
			if (sub_range.empty())
			{
				// removed by hand
				return false;
			}
			deadlock_graph_.erase(sub_range.begin(), sub_range.end());
			return true;
		}

		#if GAL_PROMETHEUS_COMPILER_DEBUG
		static auto deadlock_stack_empty() noexcept -> bool { return deadlock_stack_.empty(); }

		static auto deadlock_graph_empty() noexcept -> bool
		{
			std::scoped_lock lock{deadlock_mutex_};
			return deadlock_graph_.empty();
		}

	private:
		#endif

	public:
		constexpr UnfairMutex(const UnfairMutex&) noexcept = delete;
		constexpr UnfairMutex(UnfairMutex&&) noexcept = delete;
		constexpr auto operator=(const UnfairMutex&) noexcept -> UnfairMutex& = delete;
		constexpr auto operator=(UnfairMutex&&) noexcept -> UnfairMutex& = delete;

		constexpr UnfairMutex() noexcept
			: semaphore_{std::to_underlying(SemaphoreValue::UNLOCKED)} {}

		~UnfairMutex() noexcept
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not is_locked());
			if constexpr (check_deadlock) { deadlock_remove(this); }
		}

		auto is_locked() const noexcept -> bool //
		{
			GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());
			return semaphore_.load(std::memory_order::relaxed) != std::to_underlying(SemaphoreValue::UNLOCKED);
		}

		auto lock() noexcept -> void
		{
			if constexpr (check_deadlock)
			{
				const auto deadlock_other = deadlock_lock(this);
				GAL_PROMETHEUS_DEBUG_ASSUME(deadlock_other != this, "this mutex is already locked");
				GAL_PROMETHEUS_DEBUG_ASSUME(deadlock_other == nullptr, "Potential deadlock because of different lock ordering of mutexes");
			}

			GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());

			if (auto expected = std::to_underlying(SemaphoreValue::UNLOCKED);
				not semaphore_.compare_exchange_strong(expected, std::to_underlying(SemaphoreValue::LOCKED_NO_WAITER), std::memory_order::acquire))
			[[unlikely]]
			{
				lock_contended(expected);
			}

			GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());
		}

		auto try_lock() noexcept -> bool
		{
			if constexpr (check_deadlock)
			{
				if constexpr (check_deadlock)
				{
					const auto deadlock_other = deadlock_lock(this);
					GAL_PROMETHEUS_DEBUG_ASSUME(deadlock_other != this, "this mutex is already locked");
					GAL_PROMETHEUS_DEBUG_ASSUME(deadlock_other == nullptr, "Potential deadlock because of different lock ordering of mutexes");
				}
			}

			GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());

			if (auto expected = std::to_underlying(SemaphoreValue::UNLOCKED);
				not semaphore_.compare_exchange_strong(expected, std::to_underlying(SemaphoreValue::LOCKED_NO_WAITER), std::memory_order::acquire))
			[[unlikely]]
			{
				GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());

				if constexpr (check_deadlock) { GAL_PROMETHEUS_DEBUG_ASSUME(deadlock_unlock(this), "Unlock failed in reverse order"); }

				return false;
			}

			GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());
			return true;
		}

		auto unlock() noexcept -> void
		{
			if constexpr (check_deadlock) { GAL_PROMETHEUS_DEBUG_ASSUME(deadlock_unlock(this), "Unlock failed in reverse order"); }

			GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());

			if (semaphore_.exchange(std::to_underlying(SemaphoreValue::UNLOCKED), std::memory_order::relaxed) !=
			    std::to_underlying(SemaphoreValue::LOCKED_NO_WAITER))
			[[unlikely]]
			{
				semaphore_.notify_one();
			}
			else { std::atomic_thread_fence(std::memory_order::release); }

			GAL_PROMETHEUS_DEBUG_AXIOM(holds_invariant());
		}
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

		constexpr UnfairRecursiveMutex() noexcept
			:
			owner_{invalid_thread_id},
			count_{0} {}

		constexpr ~UnfairRecursiveMutex() noexcept = default;

		/**
		 * @brief Check if the lock is held by current thread.
		 * @return The number of recursive locks the current thread has taken.
		 * @retval 0 The current thread does not have a lock, or no-thread have a lock.
		 */
		[[nodiscard]] auto recurse_count() const noexcept -> count_type
		{
			if (owner_.load(std::memory_order::acquire) == this_thread::get_id()) { return count_; }
			return 0;
		}

		auto lock() noexcept -> void
		{
			if (const auto id = this_thread::get_id(); owner_.load(std::memory_order::acquire) == id)
			{
				// owned by current thread
				GAL_PROMETHEUS_DEBUG_AXIOM(count_ != 0);
				count_ += 1;
			}
			else
			{
				mutex_.lock();

				// first lock
				GAL_PROMETHEUS_DEBUG_AXIOM(count_ == 0);
				count_ = 1;

				GAL_PROMETHEUS_DEBUG_AXIOM(owner_.load(std::memory_order::relaxed) == invalid_thread_id);
				owner_.store(id, std::memory_order::release);
			}
		}

		[[nodiscard]] auto try_lock() noexcept -> bool
		{
			if (const auto id = this_thread::get_id();
				owner_.load(std::memory_order::acquire) == id)
			{
				// owned by current thread
				GAL_PROMETHEUS_DEBUG_AXIOM(count_ != 0);
				count_ += 1;

				return true;
			}
			else if (mutex_.try_lock())
			{
				// first lock
				GAL_PROMETHEUS_DEBUG_AXIOM(count_ == 0);
				count_ = 1;

				GAL_PROMETHEUS_DEBUG_AXIOM(owner_.load(std::memory_order::relaxed) == invalid_thread_id);
				owner_.store(id, std::memory_order::release);

				return true;
			}

			// owned by another thread
			return false;
		}

		auto unlock() noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_AXIOM(count_ != 0);

			count_ -= 1;
			if (count_ == 0)
			{
				owner_.store(invalid_thread_id, std::memory_order::release);
				mutex_.unlock();
			}
		}
	};
}

// ReSharper disable once CppRedundantNamespaceDefinition
namespace gal::prometheus::meta
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	template<>
	struct user_defined::enum_name_policy<concurrency::UnfairMutex::SemaphoreValue>
	{
		constexpr static auto value = NamePolicy::WITH_ENUM_NAME;
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_BEGIN
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

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_STD_END
