// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:coroutine.task;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>
#include <coroutine>
#include <exception>
#include <utility>

#include <prometheus/macro.hpp>

#endif

namespace gal::prometheus::coroutine
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
		template<typename ReturnType>
		class Task;
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	template<typename ReturnType>
	class P
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

		constexpr auto result() & -> ReturnType&;

		constexpr auto result() && -> ReturnType&&;
	};

	template<>
	class P<void>
	{
	public:
		constexpr auto return_void() const noexcept -> void { (void)this; }

		/* constexpr */
		auto result() -> void;
	};

	template<typename ReturnType>
	class TaskPromise final : public P<ReturnType>
	{
		friend P<ReturnType>;

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
		[[nodiscard]] constexpr auto initial_suspend() const noexcept -> std::suspend_always
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

	template<typename ReturnType>
	constexpr auto P<ReturnType>::result() & -> ReturnType&
	{
		if (auto& self = *static_cast<const TaskPromise<ReturnType>*>(this);
			self.exception_) { std::rethrow_exception(self.exception_); }
		return value_;
	}

	template<typename ReturnType>
	constexpr auto P<ReturnType>::result() && -> ReturnType&&
	{
		if (auto& self = *static_cast<TaskPromise<ReturnType>*>(this);
			self.exception_) { std::rethrow_exception(self.exception_); }
		return std::move(value_);
	}

	/* constexpr */
	inline auto P<void>::result() -> void
	{
		if (const auto& self = *static_cast<TaskPromise<void>*>(this); // NOLINT
			self.exception_) { std::rethrow_exception(self.exception_); }
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
		template<typename ReturnType>
		class [[nodiscard]] Task final
		{
		public:
			using promise_type = TaskPromise<ReturnType>;
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
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
} // namespace gal::prometheus::coroutine
