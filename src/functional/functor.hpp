// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <utility>
#include <type_traits>
#include <functional>

#include <prometheus/macro.hpp>

#if __cpp_static_call_operator >= 202207L
#define FUNCTOR_WORKAROUND_OPERATOR_STATIC static
#define FUNCTOR_WORKAROUND_OPERATOR_CONST
#define FUNCTOR_WORKAROUND_OPERATOR_THIS(type) type::
#else
#define FUNCTOR_WORKAROUND_OPERATOR_STATIC
#define FUNCTOR_WORKAROUND_OPERATOR_CONST const
#define FUNCTOR_WORKAROUND_OPERATOR_THIS(type) this->
#endif

namespace gal::prometheus::functional
{
	namespace functor_detail
	{
		enum class UnaryFoldCategory : std::uint8_t
		{
			ALL,
			ANY,
			NONE,
		};

		template<auto Default, UnaryFoldCategory Category>
		struct unary
		{
			template<typename... Args>
			[[nodiscard]] constexpr FUNCTOR_WORKAROUND_OPERATOR_STATIC auto operator()(const Args&... args) FUNCTOR_WORKAROUND_OPERATOR_CONST
				noexcept((std::is_nothrow_invocable_r_v<bool, decltype(Default), Args> and ...)) -> bool //
				requires(std::is_invocable_r_v<bool, decltype(Default), Args> and ...)
			{
				if constexpr (sizeof...(Args) == 0) { return true; }

				if constexpr (Category == UnaryFoldCategory::ALL) { return (Default(args) and ...); }
				else if constexpr (Category == UnaryFoldCategory::ANY) { return (Default(args) or ...); }
				else if constexpr (Category == UnaryFoldCategory::NONE) { return not(Default(args) or ...); }
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

				if constexpr (Category == UnaryFoldCategory::ALL) { return (function(args) and ...); }
				else if constexpr (Category == UnaryFoldCategory::ANY) { return (function(args) or ...); }
				else if constexpr (Category == UnaryFoldCategory::NONE) { return not(function(args) or ...); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
		};

		template<auto Default>
		struct binary
		{
			template<typename Lhs, typename Rhs, typename... Reset>
			[[nodiscard]] constexpr FUNCTOR_WORKAROUND_OPERATOR_STATIC auto
			operator()(
				const Lhs& lhs,
				const Rhs& rhs,
				const Reset&... reset //
			) FUNCTOR_WORKAROUND_OPERATOR_CONST
				noexcept(
					noexcept(Default(lhs, rhs)) and //
					noexcept((Default(lhs, reset) and ...)) and //
					noexcept((Default(rhs, reset) and ...)) //
				) -> const auto&
			{
				if constexpr (sizeof...(reset) == 0) { return Default(lhs, rhs) ? lhs : rhs; }
				else
				{
					return
							FUNCTOR_WORKAROUND_OPERATOR_THIS(binary)operator()(
								FUNCTOR_WORKAROUND_OPERATOR_THIS(binary)operator()(lhs, rhs),
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
							FUNCTOR_WORKAROUND_OPERATOR_THIS(binary)operator()(
								FUNCTOR_WORKAROUND_OPERATOR_THIS(binary)operator()(lhs, rhs),
								reset...
							);
				}
			}
		};

		[[maybe_unused]] constexpr auto to_boolean = [](const auto& object) noexcept(noexcept(static_cast<bool>(object))) -> bool
		{
			return static_cast<bool>(object);
		};
	}

	template<typename FunctionType>
	// [[deprecated("use `deducing this` instead")]]
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
		constexpr functor_detail::unary<functor_detail::to_boolean, functor_detail::UnaryFoldCategory::ALL> all{};
		constexpr functor_detail::unary<functor_detail::to_boolean, functor_detail::UnaryFoldCategory::ANY> any{};
		constexpr functor_detail::unary<functor_detail::to_boolean, functor_detail::UnaryFoldCategory::NONE> none{};

		constexpr functor_detail::binary<std::ranges::greater_equal{}> max{};
		constexpr functor_detail::binary<std::ranges::less_equal{}> min{};
	}
}

#undef FUNCTOR_WORKAROUND_OPERATOR_STATIC
#undef FUNCTOR_WORKAROUND_OPERATOR_CONST
#undef FUNCTOR_WORKAROUND_OPERATOR_THIS
