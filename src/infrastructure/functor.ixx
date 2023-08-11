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
		const noexcept((std::is_nothrow_invocable_r_v<bool, Function, Ts> && ...))
			-> bool requires(std::is_invocable_r_v<bool, Function, Ts> && ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return (function(ts) && ...);
		}

		template<typename... Ts>
		[[nodiscard]] constexpr auto operator()(const Ts&... ts)
		const
			noexcept((std::is_nothrow_invocable_r_v<bool, decltype(Functor), Ts> && ...))
			-> bool requires((std::is_invocable_r_v<bool, decltype(Functor), Ts> && ...))
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return (Functor(ts) && ...);
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
		const noexcept((std::is_nothrow_invocable_r_v<bool, Function, Ts> && ...))
			-> bool requires(std::is_invocable_r_v<bool, Function, Ts> && ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return (function(ts) || ...);
		}

		template<typename... Ts>
		[[nodiscard]] constexpr auto operator()(const Ts&... ts)
		const
			noexcept((std::is_nothrow_invocable_r_v<bool, decltype(Functor), Ts> && ...))
			-> bool requires(std::is_invocable_r_v<bool, decltype(Functor), Ts> && ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return (Functor(ts) || ...);
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
		const noexcept((std::is_nothrow_invocable_r_v<bool, Function, Ts> && ...))
			-> bool requires(std::is_invocable_r_v<bool, Function, Ts> && ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return !(function(ts) || ...);
		}

		template<typename... Ts>
		[[nodiscard]] constexpr auto operator()(const Ts&... ts)
		const
			noexcept((std::is_nothrow_invocable_r_v<bool, decltype(Functor), Ts> && ...))
			-> bool requires(std::is_invocable_r_v<bool, decltype(Functor), Ts> && ...)
		{
			if constexpr (sizeof...(ts) == 0) { return true; }

			return !(Functor(ts) || ...);
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
			noexcept(Functor(lhs, rhs)) &&
			noexcept((Functor(lhs, reset) && ...)) &&
			noexcept((Functor(rhs, reset) && ...))
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
		}
	}
}
