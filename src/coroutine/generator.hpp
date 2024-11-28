// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <coroutine>
#include <exception>
#include <iterator>

#include <prometheus/macro.hpp>

namespace gal::prometheus::coroutine
{
	template<typename ReturnType>
		requires(not std::is_void_v<ReturnType>)
	class Generator;

	namespace generator_detail
	{
		template<typename ReturnType>
		class GeneratorPromise final
		{
		public:
			using return_type = ReturnType;
			using pointer = std::add_pointer_t<return_type>;

			using generator_type = Generator<return_type>;

			using promise_type = GeneratorPromise;
			using coroutine_handle = std::coroutine_handle<promise_type>;

		private:
			pointer value_{nullptr};
			std::exception_ptr exception_{nullptr};

		public:
			constexpr auto initial_suspend() const noexcept -> std::suspend_always // NOLINT(modernize-use-nodiscard)
			{
				(void)this;
				return {};
			}

			[[nodiscard]] constexpr auto final_suspend() const noexcept -> std::suspend_always
			{
				(void)this;
				return {};
			}

			template<typename U>
			constexpr auto await_transform(U&&) -> std::suspend_never = delete;

			constexpr auto yield_value(return_type& value) noexcept -> std::suspend_always
			{
				value_ = std::addressof(value);
				return {};
			}

			constexpr auto yield_value(return_type&& value) noexcept -> std::suspend_always
			{
				value_ = std::addressof(value);
				return {};
			}

			[[nodiscard]] constexpr auto get_return_object() noexcept -> generator_type { return generator_type{coroutine_handle::from_promise(*this)}; }

			constexpr auto unhandled_exception() noexcept -> void { exception_ = std::current_exception(); }

			constexpr auto return_void() const noexcept -> void { (void)this; }

			[[nodiscard]] constexpr auto has_exception() const noexcept -> bool { return exception_.operator bool(); }

			constexpr auto rethrow_if_exception() const -> void { if (has_exception()) { std::rethrow_exception(exception_); } }

			constexpr auto result() const & -> const ReturnType&
			{
				rethrow_if_exception();

				return *value_;
			}

			constexpr auto result() && -> ReturnType&&
			{
				rethrow_if_exception();

				return std::move(*value_);
			}
		};

		using iterator_sentinel = std::default_sentinel_t;

		template<typename Generator>
		class Iterator
		{
			using generator_type = Generator;

		public:
			using iterator_concept = std::input_iterator_tag;
			using iterator_category = std::input_iterator_tag;

			using value_type = typename generator_type::return_type;
			using difference_type = typename generator_type::difference_type;
			using pointer = typename generator_type::pointer;
			using reference = typename generator_type::reference;

		private:
			using promise_type = GeneratorPromise<value_type>;
			using coroutine_handle = typename promise_type::coroutine_handle;

			coroutine_handle coroutine_;

		public:
			constexpr explicit Iterator(coroutine_handle coroutine = coroutine_handle{}) noexcept
				: coroutine_{coroutine} {}

			[[nodiscard]] friend constexpr auto operator==(const Iterator& lhs, iterator_sentinel) noexcept -> bool
			{
				return lhs.coroutine_ == nullptr or lhs.coroutine_.done();
			}

			[[nodiscard]] friend constexpr auto operator==(iterator_sentinel, const Iterator& rhs) noexcept -> bool { return rhs == iterator_sentinel{}; }

			constexpr auto operator++() -> Iterator&
			{
				coroutine_.resume();
				if (coroutine_.done()) { coroutine_.promise().rethrow_if_exception(); }

				return *this;
			}

			constexpr auto operator++(int) -> void { operator++(); }

			[[nodiscard]] constexpr auto operator*() const & -> decltype(auto) { return coroutine_.promise().result(); }

			[[nodiscard]] constexpr auto operator*() && noexcept -> decltype(auto) { return std::move(coroutine_.promise()).result(); }

			[[nodiscard]] constexpr auto operator->() const & noexcept -> decltype(auto) { return std::addressof(operator*()); }

			[[nodiscard]] constexpr auto operator->() && noexcept -> decltype(auto) { return std::addressof(std::move(*this).operator*()); }
		};
	}

	template<typename ReturnType>
		requires(not std::is_void_v<ReturnType>)
	class Generator
	{
	public:
		using return_type = std::remove_reference_t<ReturnType>;
		using difference_type = std::ptrdiff_t;
		using pointer = std::add_pointer_t<return_type>;
		using reference = std::add_lvalue_reference_t<return_type>;
		using iterator = generator_detail::Iterator<Generator<return_type>>;

		using promise_type = generator_detail::GeneratorPromise<return_type>;
		using coroutine_handle = typename promise_type::coroutine_handle;

	private:
		friend promise_type;

		coroutine_handle coroutine_;

		constexpr explicit Generator(coroutine_handle coroutine) noexcept
			: coroutine_{coroutine} {}

	public:
		constexpr Generator() noexcept
			: coroutine_{nullptr} {}

		constexpr Generator(const Generator&) noexcept = delete;
		constexpr auto operator=(const Generator&) noexcept -> Generator& = delete;

		constexpr Generator(Generator&& other) noexcept
			: coroutine_{std::exchange(other.coroutine_, nullptr)} {}

		constexpr auto operator=(Generator&& other) noexcept -> Generator&
		{
			coroutine_ = std::exchange(other.coroutine_, nullptr);

			return *this;
		}

		constexpr ~Generator() noexcept { if (coroutine_) { coroutine_.destroy(); } }

		[[nodiscard]] constexpr auto begin() noexcept -> iterator
		{
			// If coroutine_ is nullptr, this is equivalent to returning `end()`
			iterator it{coroutine_};

			if (coroutine_) { ++it; }

			return it;
		}

		[[nodiscard]] constexpr auto end() const noexcept -> generator_detail::iterator_sentinel
		{
			(void)this;
			return {};
		}
	};
}
