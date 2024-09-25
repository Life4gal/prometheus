// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

#if defined(__x86_64__) or defined(_M_X64) or defined(__i386__) or defined(_M_IX86)
#define GAL_PROMETHEUS_CONCURRENCY_QUEUE_X86
#include <emmintrin.h>
#elif defined(__arm__) or defined(__aarch64__) or defined(_M_ARM64)
#define GAL_PROMETHEUS_CONCURRENCY_QUEUE_ARM
#endif

export module gal.prometheus.concurrency:queue;

import std;

GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

#else
#pragma once

#include <thread>
#include <atomic>
#include <span>
#include <bit>

#if defined(__x86_64__) or defined(_M_X64) or defined(__i386__) or defined(_M_IX86)
#define GAL_PROMETHEUS_CONCURRENCY_QUEUE_X86
#include <emmintrin.h>
#elif defined(__arm__) or defined(__aarch64__) or defined(_M_ARM64)
#define GAL_PROMETHEUS_CONCURRENCY_QUEUE_ARM
#endif

#include <prometheus/macro.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

namespace gal::prometheus::concurrency
{
	namespace queue_detail
	{
		inline auto spin_loop_pause() noexcept -> void
		{
			#if defined(GAL_PROMETHEUS_CONCURRENCY_QUEUE_X86)
			_mm_pause();
			#elif defined(GAL_PROMETHEUS_CONCURRENCY_QUEUE_ARM)
			__yield();
			#else
			#error "fixme"
			#endif
		}

		constexpr auto cache_line_size = std::hardware_constructive_interference_size;

		template<std::size_t ElementsInCacheLine>
		struct cache_line_index_bits;

		template<>
		struct cache_line_index_bits<(1 << 1)> : std::integral_constant<std::size_t, 1> {};

		template<>
		struct cache_line_index_bits<(1 << 2)> : std::integral_constant<std::size_t, 2> {};

		template<>
		struct cache_line_index_bits<(1 << 3)> : std::integral_constant<std::size_t, 3> {};

		template<>
		struct cache_line_index_bits<(1 << 4)> : std::integral_constant<std::size_t, 4> {};

		template<>
		struct cache_line_index_bits<(1 << 5)> : std::integral_constant<std::size_t, 5> {};

		template<>
		struct cache_line_index_bits<(1 << 6)> : std::integral_constant<std::size_t, 6> {};

		template<>
		struct cache_line_index_bits<(1 << 7)> : std::integral_constant<std::size_t, 7> {};

		template<>
		struct cache_line_index_bits<(1 << 8)> : std::integral_constant<std::size_t, 8> {};

		template<std::size_t ElementCount, std::size_t ElementsInCacheLine>
		struct index_shuffle_bits
		{
			constexpr static auto index_bits = cache_line_index_bits<ElementsInCacheLine>::value;
			constexpr static auto min_size = static_cast<std::size_t>(1) << (index_bits * 2);

			constexpr static auto value = ElementCount < min_size ? 0 : index_bits;
		};

		template<std::size_t ShuffleBits>
		[[nodiscard]] constexpr auto mapping_index(const std::size_t index) noexcept -> std::size_t
		{
			if constexpr (ShuffleBits == 0)
			{
				return index;
			}
			else
			{
				constexpr auto mix_mask = (static_cast<std::size_t>(1) << ShuffleBits) - 1;

				const auto mix = (index ^ (index >> ShuffleBits)) & mix_mask;
				return index ^ mix ^ (mix << ShuffleBits);
			}
		}

		template<std::size_t ShuffleBits, typename T, std::size_t Extent>
		[[nodiscard]] constexpr auto mapping(std::span<T, Extent> elements, const std::size_t index) noexcept -> T&
		{
			return elements[mapping_index<ShuffleBits>(index)];
		}

		template<std::size_t ShuffleBits, typename T>
		[[nodiscard]] constexpr auto mapping(T* elements, const std::size_t index) noexcept -> T&
		{
			return elements[mapping_index<ShuffleBits>(index)];
		}

		template<typename Derived>
		class QueueImpl
		{
		public:
			using derived_type = Derived;

			using size_type = unsigned;
			using atomic_size_type = std::atomic<unsigned>;

			enum class State : std::uint8_t
			{
				EMPTY,
				LOADING,
				STORING,
				STORED,
			};

			using atomic_state_type = std::atomic<State>;

		protected:
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH

			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING(4324)
			#else
			#endif

			alignas(cache_line_size) atomic_size_type head_;
			alignas(cache_line_size) atomic_size_type tail_;

			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_POP

		private:
			[[nodiscard]] constexpr auto rep() noexcept -> derived_type&
			{
				return *static_cast<derived_type*>(this);
			}

			[[nodiscard]] constexpr auto rep() const noexcept -> const derived_type&
			{
				return *static_cast<derived_type*>(this);
			}

		public:
			QueueImpl() noexcept = default;

			QueueImpl(const QueueImpl& other) noexcept
				: head_{other.head_.load(std::memory_order::relaxed)},
				  tail_{other.tail_.load(std::memory_order::relaxed)} {}

			auto operator=(const QueueImpl& other) noexcept -> QueueImpl&
			{
				head_.store(other.head_.load(std::memory_order::relaxed), std::memory_order::relaxed);
				tail_.store(other.tail_.load(std::memory_order::relaxed), std::memory_order::relaxed);

				return *this;
			}

			QueueImpl(QueueImpl&&) noexcept = default;

			auto operator=(QueueImpl&&) noexcept -> QueueImpl& = default;

		protected:
			~QueueImpl() noexcept = default;

		public:
			auto swap(QueueImpl& other) noexcept -> void
			{
				other.head_.store(head_.exchange(other.head_.load(std::memory_order::relaxed)), std::memory_order::relaxed);
				other.tail_.store(tail_.exchange(other.tail_.load(std::memory_order::relaxed)), std::memory_order::relaxed);
			}

			friend auto swap(QueueImpl& lhs, QueueImpl& rhs) noexcept -> void
			{
				lhs.swap(rhs);
			}

		protected:
			template<typename T, T NilValue>
			constexpr static auto do_pop_atomic(std::atomic<T>& atomic_element) noexcept -> T
			{
				if constexpr (derived_type::is_single_producer_single_consumer)
				{
					while (true)
					{
						if (auto element = atomic_element.load(std::memory_order::acquire);
							element != NilValue)
						[[likely]]
						{
							atomic_element.store(NilValue, std::memory_order::relaxed);
							return element;
						}

						if constexpr (derived_type::maximize_throughput)
						{
							spin_loop_pause();
						}
					}
				}
				else
				{
					while (true)
					{
						if (auto element = atomic_element.exchange(NilValue, std::memory_order::acquire);
							element != NilValue)
						[[likely]]
						{
							return element;
						}

						spin_loop_pause();
						if constexpr (derived_type::maximize_throughput)
						{
							while (atomic_element.load(std::memory_order::relaxed) == NilValue)
							{
								spin_loop_pause();
							}
						}
					}
				}
			}

			template<typename T, T NilValue>
			constexpr static auto do_push_atomic(std::atomic<T>& atomic_element, T element) noexcept -> void
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(element != NilValue);

				if constexpr (derived_type::is_single_producer_single_consumer)
				{
					while (true)
					{
						if (atomic_element.load(std::memory_order::relaxed) == NilValue)
						[[likely]]
						{
							atomic_element.store(element, std::memory_order::release);
							return;
						}

						if constexpr (derived_type::maximize_throughput)
						{
							spin_loop_pause();
						}
					}
				}
				else
				{
					while (true)
					{
						if (auto expected = NilValue;
							atomic_element.compare_exchange_weak(expected, element, std::memory_order::release, std::memory_order::relaxed))
						[[likely]]
						{
							return;
						}

						spin_loop_pause();
						if constexpr (derived_type::maximize_throughput)
						{
							while (atomic_element.load(std::memory_order::relaxed) != NilValue)
							{
								spin_loop_pause();
							}
						}
					}
				}
			}

			template<typename T>
			constexpr static auto do_pop_atomic(atomic_state_type& atomic_state, T& in_element) noexcept -> T
			{
				if constexpr (derived_type::is_single_producer_single_consumer)
				{
					while (true)
					{
						if (atomic_state.load(std::memory_order::acquire) == State::STORED)
						[[likely]]
						{
							auto out_element = std::move(in_element);
							atomic_state.store(State::EMPTY, std::memory_order::release);
							return out_element;
						}

						if constexpr (derived_type::maximize_throughput)
						{
							spin_loop_pause();
						}
					}
				}
				else
				{
					while (true)
					{
						if (auto expected = State::STORED;
							atomic_state.compare_exchange_weak(expected, State::LOADING, std::memory_order::acquire, std::memory_order::relaxed))
						[[likely]]
						{
							auto out_element = std::move(in_element);
							atomic_state.store(State::EMPTY, std::memory_order::release);
							return out_element;
						}

						spin_loop_pause();
						if constexpr (derived_type::maximize_throughput)
						{
							while (atomic_state.load(std::memory_order::relaxed) != State::STORED)
							{
								spin_loop_pause();
							}
						}
					}
				}
			}

			template<typename T>
			constexpr static auto do_push_atomic(atomic_state_type& atomic_state, T& out_element, T&& in_element) noexcept -> void
			{
				if constexpr (derived_type::is_single_producer_single_consumer)
				{
					while (true)
					{
						if (atomic_state.load(std::memory_order::acquire) == State::EMPTY)
						[[likely]]
						{
							out_element = std::forward<T>(in_element);
							atomic_state.store(State::STORED, std::memory_order::release);
							return;
						}

						if constexpr (derived_type::maximize_throughput)
						{
							spin_loop_pause();
						}
					}
				}
				else
				{
					while (true)
					{
						if (auto expected = State::EMPTY;
							atomic_state.compare_exchange_weak(expected, State::STORING, std::memory_order::acquire, std::memory_order::relaxed))
						[[likely]]
						{
							out_element = std::forward<T>(in_element);
							atomic_state.store(State::STORED, std::memory_order::release);
							return;
						}

						spin_loop_pause();
						if constexpr (derived_type::maximize_throughput)
						{
							while (atomic_state.load(std::memory_order::relaxed) != State::EMPTY)
							{
								spin_loop_pause();
							}
						}
					}
				}
			}

		public:
			[[nodiscard]] constexpr auto try_pop() noexcept -> auto
			{
				auto tail = tail_.load(std::memory_order::relaxed);
				if constexpr (derived_type::is_single_producer_single_consumer)
				{
					if (std::cmp_less_equal(head_.load(std::memory_order::relaxed), tail))
					{
						return std::nullopt;
					}
					tail_.store(tail + 1, std::memory_order::relaxed);
				}
				else
				{
					while (true)
					{
						if (std::cmp_less_equal(head_.load(std::memory_order::relaxed), tail))
						{
							return std::nullopt;
						}

						if (tail_.compare_exchange_weak(tail, tail + 1, std::memory_order::relaxed, std::memory_order::relaxed))
						[[likely]]
						{
							break;
						}
					}
				}

				return std::optional{std::in_place, rep().do_pop(tail)};
			}

			template<typename... Args>
			[[nodiscard]] constexpr auto try_push(Args&&... args) noexcept -> bool
			{
				auto head = head_.load(std::memory_order::relaxed);
				if constexpr (derived_type::is_single_producer_single_consumer)
				{
					if (std::cmp_greater_equal(head, tail_.load(std::memory_order::relaxed) + rep().do_size()))
					{
						return false;
					}
					head_.store(head + 1, std::memory_order::relaxed);
				}
				else
				{
					while (true)
					{
						if (std::cmp_greater_equal(head, tail_.load(std::memory_order::relaxed) + rep().do_size()))
						{
							return false;
						}

						if (head_.compare_exchange_weak(head, head + 1, std::memory_order::relaxed, std::memory_order::relaxed))
						[[likely]]
						{
							break;
						}
					}
				}

				rep().do_push(head, std::forward<Args>(args)...);
				return true;
			}

			constexpr auto pop() noexcept -> auto
			{
				size_type tail;
				if constexpr (derived_type::is_single_producer_single_consumer)
				{
					tail = tail_.load(std::memory_order::relaxed);
					tail_.store(tail + 1, std::memory_order::relaxed);
				}
				else
				{
					constexpr auto order = derived_type::is_total_ordered ? std::memory_order::seq_cst : std::memory_order::relaxed;
					tail = tail_.fetch_add(1, order);
				}

				return rep().do_pop(tail);
			}

			template<typename... Args>
			constexpr auto push(Args&&... args) noexcept -> void
			{
				size_type head;
				if constexpr (derived_type::is_single_producer_single_consumer)
				{
					head = head_.load(std::memory_order::relaxed);
					head_.store(head + 1, std::memory_order::relaxed);
				}
				else
				{
					constexpr auto order = derived_type::is_total_ordered ? std::memory_order::seq_cst : std::memory_order::relaxed;
					head = head_.fetch_add(1, order);
				}

				rep().do_push(head, std::forward<Args>(args)...);
			}

			[[nodiscard]] auto size() const noexcept -> size_type
			{
				const auto head = head_.load(std::memory_order::relaxed);
				const auto tail = tail_.load(std::memory_order::relaxed);

				// tail_ can be greater than head_ because of consumers doing pop, rather that try_pop, when the queue is empty.
				if (head > tail)
				[[likely]]
				{
					return head - tail;
				}

				return 0;
			}

			[[nodiscard]] auto capacity() const noexcept -> size_type
			{
				return rep().do_size();
			}

			[[nodiscard]] auto empty() const noexcept -> bool
			{
				const auto head = head_.load(std::memory_order::relaxed);
				const auto tail = tail_.load(std::memory_order::relaxed);

				return head == tail;
			}

			[[nodiscard]] auto full() const noexcept -> bool
			{
				return size() >= capacity();
			}
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	template<
		typename T,
		std::size_t ElementCount,
		T NilValue = T{},
		bool MinimizeContention = true,
		bool MaximizeThroughput = true,
		bool IsTotalOrdered = false,
		bool IsSingleProducerSingleConsumer = false
	>
	class FixedAtomicQueue : public queue_detail::QueueImpl<FixedAtomicQueue<T, ElementCount, NilValue, MinimizeContention, MaximizeThroughput, IsTotalOrdered, IsSingleProducerSingleConsumer>>
	{
		using impl_type = queue_detail::QueueImpl<FixedAtomicQueue>;
		friend impl_type;

	public:
		using value_type = T;
		using atomic_value_type = std::atomic<value_type>;

		static_assert(atomic_value_type::is_always_lock_free, "Use `FixedQueue` instead");

		using size_type = typename impl_type::size_type;

		constexpr static auto nil_value = NilValue;
		constexpr static auto minimize_contention = MinimizeContention;
		constexpr static auto maximize_throughput = MaximizeThroughput;
		constexpr static auto is_total_ordered = IsTotalOrdered;
		constexpr static auto is_single_producer_single_consumer = IsSingleProducerSingleConsumer;

		constexpr static auto element_count = minimize_contention ? std::bit_ceil(ElementCount) : ElementCount;
		constexpr static auto elements_in_cache_line = queue_detail::cache_line_size / sizeof(atomic_value_type);
		constexpr static auto shuffle_bits = queue_detail::index_shuffle_bits<element_count, elements_in_cache_line>::value;

	private:
		alignas(queue_detail::cache_line_size) atomic_value_type elements_[element_count];

		constexpr auto do_pop(const size_type tail) noexcept -> value_type
		{
			auto& atomic_element = queue_detail::mapping<shuffle_bits>(elements_, tail % element_count);
			return impl_type::template do_pop_atomic<value_type, nil_value>(atomic_element);
		}

		template<typename... Args>
		constexpr auto do_push(const size_type head, Args&&... args) noexcept -> void //
			requires requires { value_type{std::forward<Args>(args)...}; }
		{
			auto& atomic_element = queue_detail::mapping<shuffle_bits>(elements_, head % element_count);
			impl_type::template do_push_atomic<value_type, nil_value>(atomic_element, value_type{std::forward<Args>(args)...});
		}

		template<typename U>
		constexpr auto do_push(const size_type head, U&& value) noexcept -> void //
			requires requires { static_cast<value_type>(std::forward<U>(value)); }
		{
			auto& atomic_element = queue_detail::mapping<shuffle_bits>(elements_, head % element_count);
			impl_type::template do_push_atomic<value_type, nil_value>(atomic_element, static_cast<value_type>(std::forward<U>(value)));
		}

		[[nodiscard]] constexpr auto do_size() const noexcept -> size_type
		{
			(void)this;
			return element_count;
		}

	public:
		FixedAtomicQueue() noexcept
			: impl_type{}
		{
			std::ranges::fill(elements_, nil_value);
		}

		FixedAtomicQueue(const FixedAtomicQueue&) noexcept = delete;
		auto operator=(const FixedAtomicQueue&) noexcept -> FixedAtomicQueue& = delete;
		FixedAtomicQueue(FixedAtomicQueue&&) noexcept = default;
		auto operator=(FixedAtomicQueue&&) noexcept -> FixedAtomicQueue& = default;
		~FixedAtomicQueue() noexcept = default;
	};

	template<
		typename T,
		std::size_t ElementCount,
		bool MinimizeContention = true,
		bool MaximizeThroughput = true,
		bool IsTotalOrdered = false,
		bool IsSingleProducerSingleConsumer = false
	>
	class FixedQueue : public queue_detail::QueueImpl<FixedQueue<T, ElementCount, MinimizeContention, MaximizeThroughput, IsTotalOrdered, IsSingleProducerSingleConsumer>>
	{
		using impl_type = queue_detail::QueueImpl<FixedQueue>;
		friend impl_type;

	public:
		using value_type = T;

		using state_type = typename impl_type::State;
		using atomic_state_type = typename impl_type::atomic_state_type;

		using size_type = typename impl_type::size_type;

		constexpr static auto minimize_contention = MinimizeContention;
		constexpr static auto maximize_throughput = MaximizeThroughput;
		constexpr static auto is_total_ordered = IsTotalOrdered;
		constexpr static auto is_single_producer_single_consumer = IsSingleProducerSingleConsumer;

		constexpr static auto element_count = minimize_contention ? std::bit_ceil(ElementCount) : ElementCount;
		constexpr static auto elements_in_cache_line = queue_detail::cache_line_size / sizeof(atomic_state_type);
		constexpr static auto shuffle_bits = queue_detail::index_shuffle_bits<element_count, elements_in_cache_line>::value;

	private:
		alignas(queue_detail::cache_line_size) atomic_state_type states_[element_count];
		alignas(queue_detail::cache_line_size) value_type elements_[element_count];

		constexpr auto do_pop(const size_type tail) noexcept -> value_type
		{
			const auto index = queue_detail::mapping_index<shuffle_bits>(tail % element_count);
			return impl_type::template do_pop_atomic<value_type>(states_[index], elements_[index]);
		}

		template<typename... Args>
		constexpr auto do_push(const size_type head, Args&&... args) noexcept -> void //
			requires requires { value_type{std::forward<Args>(args)...}; }
		{
			const auto index = queue_detail::mapping_index<shuffle_bits>(head % element_count);
			impl_type::template do_push_atomic(states_[index], elements_[index], value_type{std::forward<Args>(args)...});
		}

		template<typename U>
		constexpr auto do_push(const size_type head, U&& value) noexcept -> void //
			requires requires { static_cast<value_type>(std::forward<U>(value)); }
		{
			const auto index = queue_detail::mapping_index<shuffle_bits>(head % element_count);
			impl_type::template do_push_atomic(states_[index], elements_[index], static_cast<value_type>(std::forward<U>(value)));
		}

		[[nodiscard]] constexpr auto do_size() const noexcept -> size_type
		{
			(void)this;
			return element_count;
		}

	public:
		FixedQueue() noexcept
			: impl_type{}
		{
			std::ranges::fill(states_, state_type::EMPTY);
			std::ranges::fill(elements_, value_type{});
		}

		FixedQueue(const FixedQueue&) noexcept = delete;
		auto operator=(const FixedQueue&) noexcept -> FixedQueue& = delete;
		FixedQueue(FixedQueue&&) noexcept = default;
		auto operator=(FixedQueue&&) noexcept -> FixedQueue& = default;
		~FixedQueue() noexcept = default;
	};

	template<
		typename T,
		T NilValue = T{},
		typename Allocator = std::allocator<T>,
		bool MaximizeThroughput = true,
		bool IsTotalOrdered = false,
		bool IsSingleProducerSingleConsumer = false
	>
	class DynamicAtomicQueue : public queue_detail::QueueImpl<DynamicAtomicQueue<T, NilValue, Allocator, MaximizeThroughput, IsTotalOrdered, IsSingleProducerSingleConsumer>>
	{
		using impl_type = queue_detail::QueueImpl<DynamicAtomicQueue>;
		friend impl_type;

	public:
		using value_type = T;
		using atomic_value_type = std::atomic<value_type>;

		static_assert(atomic_value_type::is_always_lock_free, "Use `DynamicQueue` instead");

		using size_type = typename impl_type::size_type;

		using allocator_type = Allocator;
		using rebind_allocator_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<atomic_value_type>;

		constexpr static auto nil_value = NilValue;
		constexpr static auto maximize_throughput = MaximizeThroughput;
		constexpr static auto is_total_ordered = IsTotalOrdered;
		constexpr static auto is_single_producer_single_consumer = IsSingleProducerSingleConsumer;

		constexpr static auto elements_in_cache_line = queue_detail::cache_line_size / sizeof(atomic_value_type);
		constexpr static auto shuffle_bits = queue_detail::index_shuffle_bits<0, elements_in_cache_line>::index_bits;
		constexpr static auto min_size = queue_detail::index_shuffle_bits<0, elements_in_cache_line>::min_size;

	private:
		alignas(queue_detail::cache_line_size) size_type size_;
		atomic_value_type* elements_;

		GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS rebind_allocator_type allocator_;

		constexpr auto do_pop(const size_type tail) noexcept -> value_type
		{
			const auto mask = size_ - 1;
			const auto index = tail & mask;
			auto& atomic_element = queue_detail::mapping<shuffle_bits>(elements_, index);
			return impl_type::template do_pop_atomic<value_type, nil_value>(atomic_element);
		}

		template<typename... Args>
		constexpr auto do_push(const size_type head, Args&&... args) noexcept -> void //
			requires requires { value_type{std::forward<Args>(args)...}; }
		{
			const auto mask = size_ - 1;
			const auto index = head & mask;
			auto& atomic_element = queue_detail::mapping<shuffle_bits>(elements_, index);

			impl_type::template do_push_atomic<value_type, nil_value>(atomic_element, value_type{std::forward<Args>(args)...});
		}

		template<typename U>
		constexpr auto do_push(const size_type head, U&& value) noexcept -> void //
			requires requires { static_cast<value_type>(std::forward<U>(value)); }
		{
			const auto mask = size_ - 1;
			const auto index = head & mask;
			auto& atomic_element = queue_detail::mapping<shuffle_bits>(elements_, index);

			impl_type::template do_push_atomic<value_type, nil_value>(atomic_element, static_cast<value_type>(std::forward<U>(value)));
		}

		[[nodiscard]] constexpr auto do_size() const noexcept -> size_type
		{
			return size_;
		}

	public:
		explicit DynamicAtomicQueue(const size_type size, const allocator_type& allocator = allocator_type{}) noexcept
			: size_{std::ranges::max(static_cast<size_type>(min_size), std::bit_ceil(size))},
			  elements_{nullptr},
			  allocator_{allocator}
		{
			elements_ = allocator_.allocate(size_);
			std::ranges::uninitialized_fill_n(elements_, size_, nil_value);
		}

		DynamicAtomicQueue(const DynamicAtomicQueue&) noexcept = delete;
		auto operator=(const DynamicAtomicQueue&) noexcept -> DynamicAtomicQueue& = delete;
		DynamicAtomicQueue(DynamicAtomicQueue&& other) noexcept = default;
		auto operator=(DynamicAtomicQueue&&) noexcept -> DynamicAtomicQueue& = default;

		~DynamicAtomicQueue() noexcept
		{
			std::ranges::destroy_n(elements_, size_);
			allocator_.deallocate(elements_, size_);
		}

		auto swap(DynamicAtomicQueue& other) noexcept -> void
		{
			using std::swap;

			impl_type::swap(other);
			swap(size_, other.size_);
			swap(elements_, other.elements_);
			swap(allocator_, other.allocator_);
		}

		[[nodiscard]] auto get_allocator() const noexcept -> allocator_type
		{
			return allocator_;
		}
	};

	template<
		typename T,
		typename Allocator = std::allocator<T>,
		bool MaximizeThroughput = true,
		bool IsTotalOrdered = false,
		bool IsSingleProducerSingleConsumer = false
	>
	class DynamicQueue : public queue_detail::QueueImpl<DynamicQueue<T, Allocator, MaximizeThroughput, IsTotalOrdered, IsSingleProducerSingleConsumer>>
	{
		using impl_type = queue_detail::QueueImpl<DynamicQueue>;
		friend impl_type;

	public:
		using value_type = T;

		using state_type = typename impl_type::State;
		using atomic_state_type = typename impl_type::atomic_state_type;

		using size_type = typename impl_type::size_type;

		using allocator_type = Allocator;
		using rebind_allocator_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<std::uint8_t>;

		constexpr static auto maximize_throughput = MaximizeThroughput;
		constexpr static auto is_total_ordered = IsTotalOrdered;
		constexpr static auto is_single_producer_single_consumer = IsSingleProducerSingleConsumer;

		constexpr static auto elements_in_cache_line = queue_detail::cache_line_size / sizeof(atomic_state_type);
		constexpr static auto shuffle_bits = queue_detail::index_shuffle_bits<0, elements_in_cache_line>::index_bits;
		constexpr static auto min_size = queue_detail::index_shuffle_bits<0, elements_in_cache_line>::min_size;

	private:
		alignas(queue_detail::cache_line_size) size_type size_;
		atomic_state_type* states_;
		value_type* elements_;

		GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS rebind_allocator_type allocator_;

		constexpr auto do_pop(const size_type tail) noexcept -> value_type
		{
			const auto mask = size_ - 1;
			const auto index = tail & mask;
			const auto mapped_index = queue_detail::mapping_index<shuffle_bits>(index);
			return impl_type::template do_pop_atomic<value_type>(states_[mapped_index], elements_[mapped_index]);
		}

		template<typename... Args>
		constexpr auto do_push(const size_type head, Args&&... args) noexcept -> void //
			requires requires { value_type{std::forward<Args>(args)...}; }
		{
			const auto mask = size_ - 1;
			const auto index = head & mask;
			const auto mapped_index = queue_detail::mapping_index<shuffle_bits>(index);
			impl_type::template do_push_atomic<value_type>(states_[mapped_index], elements_[mapped_index], value_type{std::forward<Args>(args)...});
		}

		template<typename U>
		constexpr auto do_push(const size_type head, U&& value) noexcept -> void //
			requires requires { static_cast<value_type>(std::forward<U>(value)); }
		{
			const auto mask = size_ - 1;
			const auto index = head & mask;
			const auto mapped_index = queue_detail::mapping_index<shuffle_bits>(index);
			impl_type::template do_push_atomic<value_type>(states_[mapped_index], elements_[mapped_index], static_cast<value_type>(std::forward<U>(value)));
		}

		[[nodiscard]] constexpr auto do_size() const noexcept -> size_type
		{
			return size_;
		}

		template<typename U>
		[[nodiscard]] constexpr auto allocate(const size_type count) noexcept -> U*
		{
			static_assert(sizeof(typename rebind_allocator_type::value_type) == 1);
			return reinterpret_cast<U*>(std::allocator_traits<rebind_allocator_type>::allocate(allocator_, count * sizeof(U)));
		}

		template<typename U>
		constexpr auto deallocate(U* p, const size_type count) noexcept -> void
		{
			static_assert(sizeof(typename rebind_allocator_type::value_type) == 1);
			std::allocator_traits<rebind_allocator_type>::deallocate(allocator_, reinterpret_cast<typename rebind_allocator_type::value_type*>(p), count * sizeof(U));
		}

	public:
		explicit DynamicQueue(const size_type size, const allocator_type& allocator = allocator_type{}) noexcept
			: size_{std::ranges::max(static_cast<size_type>(min_size), std::bit_ceil(size))},
			  states_{nullptr},
			  elements_{nullptr},
			  allocator_{allocator}
		{
			states_ = DynamicQueue::allocate<atomic_state_type>(size_);
			elements_ = DynamicQueue::allocate<value_type>(size_);
			std::ranges::uninitialized_fill_n(states_, size_, state_type::EMPTY);
			std::ranges::uninitialized_fill_n(elements_, size_, value_type{});
		}

		DynamicQueue(const DynamicQueue&) noexcept = delete;
		auto operator=(const DynamicQueue&) noexcept -> DynamicQueue& = delete;
		DynamicQueue(DynamicQueue&& other) noexcept = default;
		auto operator=(DynamicQueue&&) noexcept -> DynamicQueue& = default;

		~DynamicQueue() noexcept
		{
			std::ranges::destroy_n(states_, size_);
			std::ranges::destroy_n(elements_, size_);
			DynamicQueue::deallocate(states_, size_);
			DynamicQueue::deallocate(elements_, size_);
		}

		auto swap(DynamicQueue& other) noexcept -> void
		{
			using std::swap;

			impl_type::swap(other);
			swap(size_, other.size_);
			swap(elements_, other.elements_);
			swap(allocator_, other.allocator_);
		}

		[[nodiscard]] auto get_allocator() const noexcept -> allocator_type
		{
			return allocator_;
		}
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
