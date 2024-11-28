// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <coroutine>
#include <exception>
#include <utility>

#include <prometheus/macro.hpp>

namespace gal::prometheus::coroutine
{
	template<typename ReturnType>
	class Task;

	namespace task_detail
	{
		template<typename ReturnType>
		class TaskPromise;

		template<typename ReturnType>
		class CoroutineReturn
		{
		public:
			using return_type = ReturnType;

		private:
			return_type value_;

		public:
			constexpr auto return_value(ReturnType value) noexcept -> void { value_ = std::move(value); }

			template<typename... Args>
				requires std::is_constructible_v<return_type, Args...>
			constexpr auto return_value(Args&&... args) noexcept(std::is_nothrow_constructible_v<return_type, Args...>) -> void
			{
				value_ = return_type{std::forward<Args>(args)...};
			}

			constexpr auto result() & -> ReturnType&
			{
				if (auto& self = *static_cast<const TaskPromise<ReturnType>*>(this);
					self.exception_) { std::rethrow_exception(self.exception_); }
				return value_;
			}

			constexpr auto result() && -> ReturnType&&
			{
				if (auto& self = *static_cast<TaskPromise<ReturnType>*>(this);
					self.exception_) { std::rethrow_exception(self.exception_); }
				return std::move(value_);
			}
		};

		template<>
		class CoroutineReturn<void>
		{
		public:
			constexpr auto return_void() const noexcept -> void { (void)this; }

			/* constexpr */
			auto result() -> void;
		};

		template<typename ReturnType>
		class TaskPromise final : public CoroutineReturn<ReturnType>
		{
			friend CoroutineReturn<ReturnType>;

		public:
			using return_type = ReturnType;
			using task_type = Task<return_type>;
			using promise_type = TaskPromise;
			using coroutine_handle = std::coroutine_handle<promise_type>;
			using continuation_type = std::coroutine_handle<>;

			struct awaitable
			{
				[[nodiscard]] constexpr auto await_ready() const noexcept -> bool
				{
					(void)this;
					return false;
				}

				template<typename PromiseType>
				[[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<PromiseType> coroutine) noexcept -> continuation_type
				{
					auto& promise = coroutine.promise();
					return promise.continuation_ ? promise.continuation_ : std::noop_coroutine();
				}

				constexpr auto await_resume() const noexcept -> void { (void)this; }
			};

		private:
			continuation_type continuation_{nullptr};
			std::exception_ptr exception_{nullptr};

		public:
			constexpr auto initial_suspend() const noexcept -> std::suspend_always // NOLINT(modernize-use-nodiscard)
			{
				(void)this;
				return {};
			}

			[[nodiscard]] constexpr auto final_suspend() noexcept -> awaitable
			{
				(void)this;
				return {};
			}

			[[nodiscard]] constexpr auto get_return_object() noexcept -> task_type { return task_type{coroutine_handle::from_promise(*this)}; }

			constexpr auto unhandled_exception() noexcept -> void { exception_ = std::current_exception(); }

			constexpr auto continuation(const continuation_type continuation) noexcept -> void { continuation_ = continuation; }

			[[nodiscard]] constexpr auto has_exception() const noexcept -> bool { return exception_.operator bool(); }
		};

		/* constexpr */
		inline auto CoroutineReturn<void>::result() -> void
		{
			if (const auto& self = *static_cast<TaskPromise<void>*>(this); // NOLINT
				self.exception_) { std::rethrow_exception(self.exception_); }
		}
	}

	template<typename ReturnType>
	class [[nodiscard]] Task final
	{
	public:
		using promise_type = task_detail::TaskPromise<ReturnType>;
		using task_type = Task;

		using return_type = typename promise_type::return_type;
		using coroutine_handle = std::coroutine_handle<promise_type>;
		using continuation_type = std::coroutine_handle<>;

		struct awaitable
		{
			coroutine_handle coroutine;

			[[nodiscard]] constexpr auto await_ready() const noexcept -> bool { return not coroutine or coroutine.done(); }

			template<typename PromiseType>
			[[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<PromiseType> continuation) noexcept -> continuation_type
			{
				coroutine.promise().continuation(continuation);
				return coroutine;
			}

			[[nodiscard]] constexpr auto await_resume() & -> decltype(auto) { return coroutine.promise().result(); }

			constexpr auto await_resume() && -> decltype(auto) { return std::move(coroutine.promise()).result(); }
		};

	private:
		coroutine_handle coroutine_;

	public:
		constexpr Task(const Task&) noexcept = delete;
		constexpr auto operator=(const Task&) noexcept -> Task& = delete;

		constexpr explicit Task() noexcept
			: coroutine_{nullptr} {}

		constexpr explicit Task(coroutine_handle handle) noexcept
			: coroutine_{handle} {}

		constexpr Task(Task&& other) noexcept
			: coroutine_{std::exchange(other.coroutine_, nullptr)} {}

		constexpr auto operator=(Task&& other) noexcept -> Task&
		{
			if (std::addressof(other) != this)
			{
				if (coroutine_) { coroutine_.destroy(); }

				coroutine_ = std::exchange(other.coroutine_, nullptr);
			}

			return *this;
		}

		constexpr ~Task() noexcept { if (coroutine_) { coroutine_.destroy(); } }

		[[nodiscard]] constexpr auto promise() & noexcept -> promise_type& { return coroutine_.promise(); }

		[[nodiscard]] constexpr auto promise() const & noexcept -> const promise_type& { return coroutine_.promise(); }

		[[nodiscard]] constexpr auto promise() && noexcept -> promise_type&& { return std::move(coroutine_.promise()); }

		[[nodiscard]] constexpr auto handle() -> coroutine_handle { return coroutine_; }

		[[nodiscard]] constexpr auto done() const noexcept -> bool { return not coroutine_ or coroutine_.done(); }

		constexpr auto operator()() const -> bool { return this->resume(); }

		constexpr auto resume() -> bool
		{
			if (not coroutine_.done()) { coroutine_.resume(); }
			return not coroutine_.done();
		}

		[[nodiscard]] constexpr auto destroy() const noexcept -> bool
		{
			if (coroutine_)
			{
				coroutine_.destroy();
				coroutine_ = nullptr;
				return true;
			}

			return false;
		}

		constexpr auto operator co_await() & noexcept -> awaitable { return {.coroutine = coroutine_}; }

		constexpr auto operator co_await() && noexcept -> awaitable { return {.coroutine = coroutine_}; }
	};
}
