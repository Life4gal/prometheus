// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.utility:functional;

import std;

namespace gal::prometheus::utility
{
	[[maybe_unused]] constexpr auto as_boolean            = [](const auto& i) -> bool { return static_cast<bool>(i); };
	[[maybe_unused]] constexpr auto compare_greater_than  = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs > rhs)) -> bool { return lhs > rhs; };
	[[maybe_unused]] constexpr auto compare_greater_equal = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs >= rhs)) -> bool { return lhs >= rhs; };
	[[maybe_unused]] constexpr auto compare_less_than     = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs < rhs)) -> bool { return lhs < rhs; };
	[[maybe_unused]] constexpr auto compare_less_equal    = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs <= rhs)) -> bool { return lhs <= rhs; };
	[[maybe_unused]] constexpr auto compare_equal         = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs == rhs)) -> bool { return lhs == rhs; };
	[[maybe_unused]] constexpr auto compare_not_equal     = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs != rhs)) -> bool { return lhs != rhs; };

	enum class InvokeFoldType
	{
		ALL,
		ANY,
		NONE,
	};

	template<auto DefaultFunctor, InvokeFoldType FoldType>
	struct unary_invoker
	{
		template<typename... Args>
		[[nodiscard]] constexpr auto operator()(const Args&... args) const noexcept((std::is_nothrow_invocable_r_v<bool, decltype(DefaultFunctor), Args> and ...)) -> bool//
			requires (std::is_invocable_r_v<bool, decltype(DefaultFunctor), Args> and ...)
		{
			if constexpr (sizeof...(Args) == 0) { return true; }

			if constexpr (FoldType == InvokeFoldType::ALL) { return (DefaultFunctor(args) and ...); }
			else if constexpr (FoldType == InvokeFoldType::ANY) { return (DefaultFunctor(args) or ...); }
			else if constexpr (FoldType == InvokeFoldType::NONE) { return not(DefaultFunctor(args) or ...); }
			else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
		}

		template<typename Function, typename... Args>
		[[nodiscard]] constexpr auto operator()(Function function, const Args&... args) const noexcept((std::is_nothrow_invocable_r_v<bool, Function, Args> and ...)) -> bool//
			requires (std::is_invocable_r_v<bool, Function, Args> and ...)
		{
			if constexpr (sizeof...(Args) == 0) { return true; }

			if constexpr (FoldType == InvokeFoldType::ALL) { return (function(args) and ...); }
			else if constexpr (FoldType == InvokeFoldType::ANY) { return (function(args) or ...); }
			else if constexpr (FoldType == InvokeFoldType::NONE) { return not(function(args) or ...); }
			else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
		}
	};

	template<auto DefaultFunctor>
	struct binary_invoker
	{
		template<typename Lhs, typename Rhs, typename... Reset>
		[[nodiscard]] constexpr auto operator()(
				const Lhs&      lhs,
				const Rhs&      rhs,
				const Reset&... reset//
				) const
			noexcept(
				noexcept(DefaultFunctor(lhs, rhs)) and            //
				noexcept((DefaultFunctor(lhs, reset) and ...)) and//
				noexcept((DefaultFunctor(rhs, reset) and ...))    //
			) -> const auto&
		{
			if constexpr (sizeof...(reset) == 0) { return DefaultFunctor(lhs, rhs) ? lhs : rhs; }
			else { return this->operator()(this->operator()(lhs, rhs), reset...); }
		}

		template<typename Function, typename Lhs, typename Rhs, typename... Reset>
			requires std::is_invocable_r_v<bool, Function, Lhs, Rhs>
		[[nodiscard]] constexpr auto operator()(
				Function        function,
				const Lhs&      lhs,
				const Rhs&      rhs,
				const Reset&... reset//
				) const
			noexcept(
				noexcept(function(lhs, rhs)) and            //
				noexcept((function(lhs, reset) and ...)) and//
				noexcept((function(rhs, reset) and ...))    //
			) -> const auto&
		{
			if constexpr (sizeof...(reset) == 0) { return function(lhs, rhs) ? lhs : rhs; }
			else { return this->operator()(this->operator()(lhs, rhs), reset...); }
		}
	};

	export namespace functor
	{
		constexpr unary_invoker<as_boolean, InvokeFoldType::ALL>  all;
		constexpr unary_invoker<as_boolean, InvokeFoldType::ANY>  any;
		constexpr unary_invoker<as_boolean, InvokeFoldType::NONE> none;

		constexpr binary_invoker<compare_greater_than> max;
		constexpr binary_invoker<compare_less_than>    min;

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
			constexpr auto operator()(Args&&... args) const noexcept(std::is_nothrow_invocable_v<function_type, decltype(*this), Args...>) -> decltype(auto)//
			{
				// we pass ourselves to function, the lambda should take the first argument as `auto&& self` or similar.
				return std::invoke(function, *this, std::forward<Args>(args)...);
			}
		};
	}
}
