// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.functional:functor;

import std;

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
	export
	{
		template<typename FunctionType>
		struct y_combinator
		{
			using function_type = FunctionType;

			function_type function;

			template<typename... Args>
			constexpr auto operator()(Args&&... args) const
				noexcept(std::is_nothrow_invocable_v<function_type, decltype(*this), Args...>) -> decltype(auto) //
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
	}

	namespace functor_detail
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

	export namespace functor
	{
		constexpr functor_detail::unary_invoker<functor_detail::as_boolean, functor_detail::InvokeFoldType::ALL> all;
		constexpr functor_detail::unary_invoker<functor_detail::as_boolean, functor_detail::InvokeFoldType::ANY> any;
		constexpr functor_detail::unary_invoker<functor_detail::as_boolean, functor_detail::InvokeFoldType::NONE> none;

		constexpr functor_detail::binary_invoker<functor_detail::compare_greater_than> max;
		constexpr functor_detail::binary_invoker<functor_detail::compare_less_than> min;
	}
}

#undef FUNCTOR_WORKAROUND_OPERATOR_STATIC
#undef FUNCTOR_WORKAROUND_OPERATOR_CONST
#undef FUNCTOR_WORKAROUND_OPERATOR_THIS
