// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.infrastructure:functor;

import std;

namespace gal::prometheus::infrastructure
{
	constexpr static auto as_boolean           = [](const auto& i) -> bool { return static_cast<bool>(i); };
	constexpr static auto compare_greater_than = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs > rhs)) -> bool { return lhs > rhs; };
	constexpr static auto compare_less_than    = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs < rhs)) -> bool { return lhs < rhs; };

	template<auto Functor = as_boolean>
	struct invoker_all
	{
		template<typename Function, typename... Ts>
		[[nodiscard]] constexpr auto operator()(
				Function     function,
				const Ts&... ts
				)
		const noexcept((std::is_nothrow_invocable_r_v<bool, Function, Ts> and ...))
			-> bool requires(std::is_invocable_r_v<bool, Function, Ts> and ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return (function(ts) and ...);
		}

		template<typename... Ts>
		[[nodiscard]] constexpr auto operator()(const Ts&... ts)
		const
			noexcept((std::is_nothrow_invocable_r_v<bool, decltype(Functor), Ts> and ...))
			-> bool requires((std::is_invocable_r_v<bool, decltype(Functor), Ts> and ...))
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return (Functor(ts) and ...);
		}
	};

	template<auto Functor = as_boolean>
	struct invoker_any
	{
		template<typename Function, typename... Ts>
		[[nodiscard]] constexpr auto operator()(
				Function     function,
				const Ts&... ts
				)
		const noexcept((std::is_nothrow_invocable_r_v<bool, Function, Ts> and ...))
			-> bool requires(std::is_invocable_r_v<bool, Function, Ts> and ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return (function(ts) or ...);
		}

		template<typename... Ts>
		[[nodiscard]] constexpr auto operator()(const Ts&... ts)
		const
			noexcept((std::is_nothrow_invocable_r_v<bool, decltype(Functor), Ts> and ...))
			-> bool requires(std::is_invocable_r_v<bool, decltype(Functor), Ts> and ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return (Functor(ts) or ...);
		}
	};

	template<auto Functor = as_boolean>
	struct invoker_none
	{
		template<typename Function, typename... Ts>
		[[nodiscard]] constexpr auto operator()(
				Function     function,
				const Ts&... ts
				)
		const noexcept((std::is_nothrow_invocable_r_v<bool, Function, Ts> and ...))
			-> bool requires(std::is_invocable_r_v<bool, Function, Ts> and ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return not(function(ts) or ...);
		}

		template<typename... Ts>
		[[nodiscard]] constexpr auto operator()(const Ts&... ts)
		const
			noexcept((std::is_nothrow_invocable_r_v<bool, decltype(Functor), Ts> and ...))
			-> bool requires(std::is_invocable_r_v<bool, decltype(Functor), Ts> and ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return not(Functor(ts) or ...);
		}
	};

	template<auto Functor>
	struct binary_invoker
	{
		template<typename L, typename R, typename... Ts>
		[[nodiscard]] constexpr auto operator()(
				const L&     lhs,
				const R&     rhs,
				const Ts&... reset
				)
		const noexcept(
			noexcept(Functor(lhs, rhs)) and            //
			noexcept((Functor(lhs, reset) and ...)) and//
			noexcept((Functor(rhs, reset) and ...))    //
		)
			-> const auto&
		{
			if constexpr (sizeof...(reset) == 0) { return Functor(lhs, rhs) ? lhs : rhs; }
			else { return this->operator()(this->operator()(lhs, rhs), reset...); }
		}
	};

	using invoker_max = binary_invoker<compare_greater_than>;
	using invoker_min = binary_invoker<compare_less_than>;

	export
	{
		namespace functor
		{
			constexpr invoker_all  all;
			constexpr invoker_any  any;
			constexpr invoker_none none;

			constexpr invoker_max max;
			constexpr invoker_min min;

			template<typename... Callable>
			struct overloaded : Callable...
			{
				using Callable::operator()...;
			};

			template<typename FunctionType>
			struct y_combinator
			{
				using function_type = FunctionType;

				function_type function;

				template<typename... Args>
				constexpr auto operator()(Args&&... args) const
					noexcept(std::is_nothrow_invocable_v<function_type, decltype(*this), Args...>) -> decltype(auto)//
				{
					// we pass ourselves to f, then the arguments.
					// the lambda should take the first argument as `auto&& self` or similar.
					return std::invoke(function, *this, std::forward<Args>(args)...);
				}
			};
		}
	}
}
