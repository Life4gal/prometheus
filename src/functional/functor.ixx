// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:functional.functor;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <utility>
#include <type_traits>

#include <prometheus/macro.hpp>

#endif

#if __cpp_static_call_operator >= 202207L
#define FUNCTOR_WORKAROUND_OPERATOR_STATIC static
#define FUNCTOR_WORKAROUND_OPERATOR_CONST
#define FUNCTOR_WORKAROUND_OPERATOR_THIS(type) type::
#else
#define FUNCTOR_WORKAROUND_OPERATOR_STATIC
#define FUNCTOR_WORKAROUND_OPERATOR_CONST const
#define FUNCTOR_WORKAROUND_OPERATOR_THIS(type) this->
#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_INTERNAL(functional)
{
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
		[[nodiscard]] constexpr FUNCTOR_WORKAROUND_OPERATOR_STATIC auto operator()(const Args&... args) FUNCTOR_WORKAROUND_OPERATOR_CONST
			noexcept((std::is_nothrow_invocable_r_v<bool, decltype(DefaultFunctor), Args> and ...)) -> bool //
			requires(std::is_invocable_r_v<bool, decltype(DefaultFunctor), Args> and ...)
		{
			if constexpr (sizeof...(Args) == 0) { return true; }

			if constexpr (FoldType == InvokeFoldType::ALL) { return (DefaultFunctor(args) and ...); }
			else if constexpr (FoldType == InvokeFoldType::ANY) { return (DefaultFunctor(args) or ...); }
			else if constexpr (FoldType == InvokeFoldType::NONE) { return not(DefaultFunctor(args) or ...); }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<typename Function, typename... Args>
		[[nodiscard]] constexpr FUNCTOR_WORKAROUND_OPERATOR_STATIC auto
		operator()(
			Function function,
			const Args&... args
		) FUNCTOR_WORKAROUND_OPERATOR_CONST noexcept((std::is_nothrow_invocable_r_v<bool, Function, Args> and ...)) -> bool //
			requires(std::is_invocable_r_v<bool, Function, Args> and ...)
		{
			if constexpr (sizeof...(Args) == 0) { return true; }

			if constexpr (FoldType == InvokeFoldType::ALL) { return (function(args) and ...); }
			else if constexpr (FoldType == InvokeFoldType::ANY) { return (function(args) or ...); }
			else if constexpr (FoldType == InvokeFoldType::NONE) { return not(function(args) or ...); }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};

	template<auto DefaultFunctor>
	struct binary_invoker
	{
		template<typename Lhs, typename Rhs, typename... Reset>
		[[nodiscard]] constexpr FUNCTOR_WORKAROUND_OPERATOR_STATIC auto
		operator()(
			const Lhs& lhs,
			const Rhs& rhs,
			const Reset&... reset //
		) FUNCTOR_WORKAROUND_OPERATOR_CONST
			noexcept(
				noexcept(DefaultFunctor(lhs, rhs)) and //
				noexcept((DefaultFunctor(lhs, reset) and ...)) and //
				noexcept((DefaultFunctor(rhs, reset) and ...)) //
			) -> const auto&
		{
			if constexpr (sizeof...(reset) == 0) { return DefaultFunctor(lhs, rhs) ? lhs : rhs; }
			else
			{
				return
						FUNCTOR_WORKAROUND_OPERATOR_THIS(binary_invoker)operator()(
							FUNCTOR_WORKAROUND_OPERATOR_THIS(binary_invoker)operator()(lhs, rhs),
							reset...
						);
			}
		}

		template<typename Function, typename Lhs, typename Rhs, typename... Reset>
			requires std::is_invocable_r_v<bool, Function, Lhs, Rhs>
		[[nodiscard]] constexpr FUNCTOR_WORKAROUND_OPERATOR_STATIC auto operator()(
			Function function,
			const Lhs& lhs,
			const Rhs& rhs,
			const Reset&... reset //
		) FUNCTOR_WORKAROUND_OPERATOR_CONST
			noexcept(
				noexcept(function(lhs, rhs)) and //
				noexcept((function(lhs, reset) and ...)) and //
				noexcept((function(rhs, reset) and ...)) //
			) -> const auto&
		{
			if constexpr (sizeof...(reset) == 0) { return function(lhs, rhs) ? lhs : rhs; }
			else
			{
				return
						FUNCTOR_WORKAROUND_OPERATOR_THIS(binary_invoker)operator()(
							FUNCTOR_WORKAROUND_OPERATOR_THIS(binary_invoker)operator()(lhs, rhs),
							reset...
						);
			}
		}
	};

	[[maybe_unused]] constexpr auto as_boolean = [](const auto& i) noexcept((static_cast<bool>(i))) -> bool { return static_cast<bool>(i); };
	[[maybe_unused]] constexpr auto compare_greater_than = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs > rhs)) -> bool
	{
		return lhs > rhs;
	};
	[[maybe_unused]] constexpr auto compare_greater_equal = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs >= rhs)) -> bool
	{
		return lhs >= rhs;
	};
	[[maybe_unused]] constexpr auto compare_less_than = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs < rhs)) -> bool
	{
		return lhs < rhs;
	};
	[[maybe_unused]] constexpr auto compare_less_equal = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs <= rhs)) -> bool
	{
		return lhs <= rhs;
	};
	[[maybe_unused]] constexpr auto compare_equal = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs == rhs)) -> bool
	{
		return lhs == rhs;
	};
	[[maybe_unused]] constexpr auto compare_not_equal = [](const auto& lhs, const auto& rhs) noexcept(noexcept(lhs != rhs)) -> bool
	{
		return lhs != rhs;
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(functional)
{
	template<typename FunctionType>
	struct y_combinator
	{
		using function_type = FunctionType;

		function_type function;

		template<typename... Args>
		constexpr auto operator()(Args&&... args) const
			noexcept(std::is_nothrow_invocable_v<function_type, decltype(*this), Args...>) ->
			// https://github.com/llvm/llvm-project/issues/97680
			#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
			decltype(std::invoke(function, *this, std::forward<Args>(args)...))
			#else
			decltype(auto) //
			#endif
		{
			// we pass ourselves to function, the lambda should take the first argument as `auto&& self` or similar.
			return std::invoke(function, *this, std::forward<Args>(args)...);
		}
	};

	template<typename... Ts>
	struct overloaded : Ts...
	{
		constexpr explicit overloaded(Ts&&... ts) noexcept((std::is_nothrow_constructible_v<Ts, decltype(ts)> and ...))
			: Ts{std::forward<Ts>(ts)}... {}
	};

	namespace functor
	{
		constexpr GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::unary_invoker<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::as_boolean, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::InvokeFoldType::ALL> all;
		constexpr GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::unary_invoker<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::as_boolean, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::InvokeFoldType::ANY> any;
		constexpr GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::unary_invoker<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::as_boolean, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::InvokeFoldType::NONE> none;

		constexpr GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::binary_invoker<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::compare_greater_than> max;
		constexpr GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::binary_invoker<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::compare_less_than> min;
	}
}

#undef FUNCTOR_WORKAROUND_OPERATOR_STATIC
#undef FUNCTOR_WORKAROUND_OPERATOR_CONST
#undef FUNCTOR_WORKAROUND_OPERATOR_THIS
