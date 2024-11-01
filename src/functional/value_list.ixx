// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:functional.value_list;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>
#include <utility>

#include <prometheus/macro.hpp>

#endif

#if defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG) or defined(GAL_PROMETHEUS_COMPILER_GNU)
#define VALUE_LIST_WORKAROUND_BINDER(Value, Prediction) binder<Value, Prediction>::template rebind
#else
#define VALUE_LIST_WORKAROUND_BINDER(Value, Prediction) typename binder<Value, Prediction>::rebind
#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_INTERNAL(functional)::value_list_detail
{
	template<auto... Values>
	struct list;

	template<typename>
	struct is_list : std::false_type {};

	template<auto... Values>
	struct is_list<list<Values...>> : std::true_type {};

	template<typename T>
	constexpr auto is_list_v = is_list<T>::value;

	template<typename T>
	concept list_t = is_list_v<T>;

	// =================================
	// same_value
	// =================================

	template<auto L, auto R>
	struct same_value : std::bool_constant<L == R> {};

	// =================================
	// nth value
	// =================================

	template<std::size_t Index, auto... Values>
	struct nth_value_impl;

	template<auto Value, auto... Values>
	struct nth_value_impl<0, Value, Values...>
	{
		constexpr static auto value = Value;
	};

	template<std::size_t Index, auto Value, auto... Values>
		requires(Index != 0)
	struct nth_value_impl<Index, Value, Values...>
	{
		constexpr static auto value = nth_value_impl<Index - 1, Values...>::value;
	};

	// =================================
	// index of
	// =================================

	template<std::size_t Index, template<auto> typename Prediction, auto, auto... Values>
	struct index_of_impl;

	template<std::size_t Index, template<auto> typename Prediction, auto Value, auto... Values>
		requires(Prediction<Value>::value)
	struct index_of_impl<Index, Prediction, Value, Values...>
	{
		constexpr static auto value = Index;
	};

	template<std::size_t Index, template<auto> typename Prediction, auto, auto... Values>
	struct index_of_impl
	{
		constexpr static auto value = index_of_impl<Index + 1, Prediction, Values...>::value;
	};

	// =================================
	// unique
	// =================================

	template<list_t Remaining, list_t Current>
	struct unique_impl;

	template<list_t Current>
	struct unique_impl<list<>, Current>
	{
		using type = Current;
	};

	template<auto Front, auto... RemainingTs, auto... CurrentTs>
	struct unique_impl<list<Front, RemainingTs...>, list<CurrentTs...>>
	{
		using type = std::conditional_t<
			list<CurrentTs...>{}.template any<Front>(),
			typename unique_impl<list<RemainingTs...>, list<CurrentTs...>>::type,
			typename unique_impl<list<RemainingTs...>, list<CurrentTs..., Front>>::type>;
	};

	// =================================
	// sub-list
	// =================================

	template<template<auto> typename Prediction, list_t Remaining, list_t Current>
	struct sub_list_impl;

	template<template<auto> typename Prediction, list_t Current>
	struct sub_list_impl<Prediction, list<>, Current>
	{
		using type = Current;
	};

	template<template<auto> typename Prediction, auto Front, auto... RemainingTs, auto... CurrentTs>
	struct sub_list_impl<Prediction, list<Front, RemainingTs...>, list<CurrentTs...>>
	{
		using type = std::conditional_t<
			Prediction<Front>::value,
			typename sub_list_impl<Prediction, list<RemainingTs...>, list<CurrentTs..., Front>>::type,
			typename sub_list_impl<Prediction, list<RemainingTs...>, list<CurrentTs...>>::type>;
	};

	// =================================
	// list
	// =================================

	template<auto... Values>
	struct list
	{
		constexpr static auto values_size = sizeof...(Values);

	private:
		template<auto T, template<auto, auto> typename Prediction>
		struct binder
		{
			template<auto U>
			using rebind = Prediction<T, U>;
		};

	public:
		[[nodiscard]] constexpr static auto size() noexcept -> std::size_t
		{
			return values_size;
		}

		template<template<auto> typename Projection>
		[[nodiscard]] constexpr static auto projection() noexcept -> list<Projection<Values>::value...> // NOLINT(modernize-type-traits)
		{
			return {};
		}

		template<template<auto> typename Prediction>
		[[nodiscard]] constexpr static auto all() noexcept -> bool
		{
			return ((Prediction<Values>::value) and ...);
		}

		template<auto Value, template<auto, auto> typename Prediction>
		[[nodiscard]] constexpr static auto all() noexcept -> bool
		{
			return list::all<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>();
		}

		template<auto Value>
		[[nodiscard]] constexpr static auto all() noexcept -> bool
		{
			return list::all<Value, same_value>();
		}

		template<template<auto> typename Prediction>
		[[nodiscard]] constexpr static auto any() noexcept -> bool
		{
			return ((Prediction<Values>::value) or ...);
		}

		template<auto Value, template<auto, auto> typename Prediction>
		[[nodiscard]] constexpr static auto any() noexcept -> bool //
		{
			return list::any<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>();
		}

		template<auto Value>
		[[nodiscard]] constexpr static auto any() noexcept -> bool
		{
			return list::any<Value, same_value>();
		}

		template<std::size_t Index>
			requires((values_size != 0) and (Index <= values_size - 1))
		[[nodiscard]] constexpr static auto nth_value() noexcept -> auto
		{
			return nth_value_impl<Index, Values...>::value;
		}

		template<template<auto> typename Prediction>
			requires(list::any<Prediction>())
		[[nodiscard]] constexpr static auto index_of() noexcept -> std::size_t
		{
			return index_of_impl<0, Prediction, Values...>::value;
		}

		template<auto Value, template<auto, auto> typename Prediction>
			requires requires { list::index_of<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>(); }
		[[nodiscard]] constexpr static auto index_of() noexcept -> std::size_t //
		{
			return list::index_of<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>();
		}

		template<auto Value>
			requires requires { list::index_of<Value, same_value>(); }
		[[nodiscard]] constexpr static auto index_of() noexcept -> std::size_t //
		{
			return list::index_of<Value, same_value>();
		}

		[[nodiscard]] constexpr static auto reverse() noexcept -> auto //
		{
			return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
			{
				return list<list::nth_value<values_size - 1 - Index>()...>{};
			}(std::make_index_sequence<values_size>{});
		}

		template<auto... Us>
		[[nodiscard]] constexpr static auto push_back() noexcept -> list<Values..., Us...>
		{
			return {};
		}

		template<auto... Us>
		[[nodiscard]] constexpr static auto push_back(const list<Us...>) noexcept -> list<Values..., Us...> //
		{
			return list::push_back<Us...>();
		}

		template<list_t auto List>
		[[nodiscard]] constexpr static auto push_back() noexcept -> auto
		{
			return list::push_back(List);
		}

		template<auto... Us>
		[[nodiscard]] constexpr static auto push_front() noexcept -> list<Us..., Values...>
		{
			return {};
		}

		template<auto... Us>
		[[nodiscard]] constexpr static auto push_front(const list<Us...>) noexcept -> list<Us..., Values...> //
		{
			return list::push_front<Us...>();
		}

		template<list_t auto List>
		[[nodiscard]] constexpr static auto push_front() noexcept -> auto
		{
			return list::push_front(List);
		}

		template<std::size_t N = 1>
			requires(N <= values_size)
		[[nodiscard]] constexpr static auto pop_back() noexcept -> auto
		{
			if constexpr (N == 0) { return list{}; }
			else if constexpr (N == values_size) { return list<>{}; }
			else
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
				{
					return list<list::nth_value<Index>()...>{};
				}(std::make_index_sequence<values_size - N>{});
			}
		}

		template<std::size_t N = 1>
			requires(N <= values_size)
		[[nodiscard]] constexpr static auto pop_front() noexcept -> auto
		{
			if constexpr (N == 0) { return list{}; }
			else if constexpr (N == values_size) { return list<>{}; }
			else
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
				{
					return list<list::nth_value<N + Index>...>{};
				}(std::make_index_sequence<values_size - N>{});
			}
		}

		template<std::size_t N = 1>
			requires(N <= values_size)
		[[nodiscard]] constexpr static auto back() noexcept -> auto //
		{
			return list::pop_front<values_size - N>();
		}

		template<std::size_t N = 1>
			requires(N <= values_size)
		[[nodiscard]] constexpr static auto front() noexcept -> auto //
		{
			return list::pop_back<values_size - N>();
		}

		template<std::size_t N = values_size>
			requires(N <= values_size)
		[[nodiscard]] constexpr static auto unique_front() noexcept -> auto
		{
			if constexpr (N <= 1) { return list{}; }
			else
			{
				using type = typename unique_impl<
					decltype(list::front<N>().template pop_front<1>()),
					decltype(list::front<1>())
				>::type;

				if constexpr (N == values_size) //
				{
					return type{};
				}
				else //
				{
					return type{}.template push_back<list::back<values_size - N>()>();
				}
			}
		}

		template<std::size_t N = values_size>
			requires(N <= values_size)
		[[nodiscard]] constexpr static auto unique_back() noexcept -> auto
		{
			if constexpr (N <= 1) { return list{}; }
			else
			{
				using type = typename unique_impl<
					decltype(list::back<N>().template pop_front<1>()),
					decltype(list::back<N>().template front<1>())
				>::type;

				if constexpr (N == values_size) //
				{
					return type{};
				}
				else //
				{
					return list::front<values_size - N>().template push_back<type{}>();
				}
			}
		}

		[[nodiscard]] constexpr static auto unique() noexcept -> auto
		{
			return list::unique_back<values_size>();
		}

		template<template<auto> typename Prediction>
		[[nodiscard]] constexpr static auto sub_list() noexcept -> typename sub_list_impl<Prediction, list, list<>>::type
		{
			return {};
		}

		template<auto Value, template<auto, auto> typename Prediction>
			requires requires { list::sub_list<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>(); }
		[[nodiscard]] constexpr static auto sub_list() noexcept -> auto //
		{
			return list::sub_list<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>();
		}
	};

	template<char... Cs>
	struct char_list;

	template<typename>
	struct is_char_list : std::false_type {};

	template<char... Cs>
	struct is_char_list<char_list<Cs...>> : std::true_type {};

	template<typename T>
	constexpr auto is_char_list_v = is_char_list<T>::value;

	template<typename T>
	concept char_list_t = is_char_list_v<T>;

	template<char... Cs>
	struct char_list : list<Cs...>
	{
		template<std::integral T>
			requires(((Cs >= '0' and Cs <= '9') or Cs == '\'') and ...)
		[[nodiscard]] constexpr static auto to_integral() noexcept -> T
		{
			T result{0};

			(
				(
					result = Cs == '\''
						         ? //
						         result
						         : //
						         result * static_cast<T>(10) + static_cast<T>(Cs - '0') //
				),
				...);

			return result;
		}

		template<std::floating_point T>
			requires(((Cs >= '0' and Cs <= '9') or Cs == '\'' or Cs == '.') and ...)
		[[nodiscard]] constexpr static auto to_floating_point() noexcept -> T
		{
			auto result = static_cast<T>(0);
			auto fraction = static_cast<T>(0.1);

			bool past = false;
			((
					result = Cs == '\''
						         ? //
						         result
						         : //
						         (
							         Cs == '.'
								         ? //
								         (past = true, result)
								         : //
								         (past
									          ? //
									          [](T& f, const T r) noexcept -> T
									          {
										          const auto ret = r + static_cast<T>(Cs - '0') * f;
										          f *= static_cast<T>(0.1);
										          return ret;
									          }(fraction, result)
									          : //
									          result * static_cast<T>(10) + static_cast<T>(Cs - '0'))) //
				),
				...);

			return result;
		}

		[[nodiscard]] constexpr static auto numerator_length() noexcept -> std::size_t //
			requires(((Cs >= '0' and Cs <= '9') or Cs == '\'' or Cs == '.') and ...)
		{
			std::size_t length = 0;

			bool found = false;
			((
					Cs == '.'
						? //
						(found = true, (void)found) // NOLINT(clang-diagnostic-comma)
						: //
						(length += not found and Cs != '\'', (void)found)), // NOLINT(clang-diagnostic-comma)
				...);

			return length;
		}

		[[nodiscard]] constexpr static auto denominator_length() noexcept -> std::size_t //
			requires(((Cs >= '0' and Cs <= '9') or Cs == '\'' or Cs == '.') and ...)
		{
			std::size_t length = 0;

			bool found = false;
			((
					Cs == '.'
						? //
						(found = true, (void)found) // NOLINT(clang-diagnostic-comma)
						: //
						(length += found and Cs != '\'', (void)found)), // NOLINT(clang-diagnostic-comma)
				...);

			return length;
		}
	};
}

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: functional
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(functional)
#endif
{
	template<auto... Values>
	using value_list_type = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::value_list_detail::list<Values...>;

	template<auto... Values>
	constexpr auto value_list = value_list_type<Values...>{};

	template<typename T>
	concept value_list_t = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::value_list_detail::list_t<T>;

	template<char... Cs>
	using char_list_type = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::value_list_detail::char_list<Cs...>;

	template<char... Cs>
	constexpr auto char_list = char_list_type<Cs...>{};

	template<typename T>
	concept char_list_t = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::value_list_detail::char_list_t<T>;
}

#undef VALUE_LIST_WORKAROUND_BINDER
