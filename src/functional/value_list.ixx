// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.functional:value_list;

import std;

#else
#pragma once

#include <type_traits>
#include <tuple>

#include <prometheus/macro.hpp>

#endif

#if defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG) or defined(GAL_PROMETHEUS_COMPILER_GNU)
	#define VALUE_LIST_WORKAROUND_BINDER(Value, Prediction) binder<Value, Prediction>::template rebind
#else
#define VALUE_LIST_WORKAROUND_BINDER(Value, Prediction) typename binder<Value, Prediction>::rebind
#endif

namespace gal::prometheus::functional
{
	namespace value_list_detail
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
			[[nodiscard]] consteval auto size() const noexcept -> std::size_t
			{
				(void)this;
				return values_size;
			}

			template<template<auto> typename Projection>
			[[nodiscard]] consteval auto projection() const noexcept -> list<Projection<Values>::value...> // NOLINT(modernize-type-traits)
			{
				(void)this;
				return {};
			}

			template<template<auto> typename Prediction>
			[[nodiscard]] consteval auto all() const noexcept -> bool
			{
				(void)this;
				return ((Prediction<Values>::value) and ...);
			}

			template<auto Value, template<auto, auto> typename Prediction>
			[[nodiscard]] consteval auto all() const noexcept -> bool
			{
				(void)this;
				return this->template all<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>();
			}

			template<auto Value>
			[[nodiscard]] consteval auto all() const noexcept -> bool { return this->all<Value, same_value>(); }

			template<template<auto> typename Prediction>
			[[nodiscard]] consteval auto any() const noexcept -> bool
			{
				(void)this;
				return ((Prediction<Values>::value) or ...);
			}

			template<auto Value, template<auto, auto> typename Prediction>
			[[nodiscard]] consteval auto any() const noexcept -> bool //
			{
				return this->template any<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>();
			}

			template<auto Value>
			[[nodiscard]] consteval auto any() const noexcept -> bool { return this->any<Value, same_value>(); }

			template<std::size_t Index>
				requires((values_size != 0) and (Index <= values_size - 1))
			[[nodiscard]] consteval auto nth_value() const noexcept -> auto
			{
				(void)this;
				return nth_value_impl<Index, Values...>::value;
			}

			template<template<auto> typename Prediction>
				requires(list{}.any<Prediction>())
			[[nodiscard]] consteval auto index_of() const noexcept -> std::size_t
			{
				(void)this;
				return index_of_impl<0, Prediction, Values...>::value;
			}

			template<auto Value, template<auto, auto> typename Prediction>
				requires requires(list l) { l.template index_of<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>(); }
			[[nodiscard]] consteval auto index_of() const noexcept -> std::size_t //
			{
				return this->template index_of<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>();
			}

			template<auto Value>
				requires requires(list l) { l.template index_of<Value, same_value>(); }
			[[nodiscard]] consteval auto index_of() const noexcept -> std::size_t //
			{
				return this->template index_of<Value, same_value>();
			}

			[[nodiscard]] consteval auto reverse() const noexcept -> auto //
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
				{
					return list<nth_value<values_size - 1 - Index>()...>{};
				}(std::make_index_sequence<values_size>{});
			}

			template<auto... Us>
			[[nodiscard]] consteval auto push_back() const noexcept -> list<Values..., Us...>
			{
				(void)this;
				return {};
			}

			template<auto... Us>
			[[nodiscard]] consteval auto push_back(const list<Us...>) const noexcept -> list<Values..., Us...> //
			{
				return this->template push_back<Us...>();
			}

			template<list_t auto List>
			[[nodiscard]] consteval auto push_back() const noexcept -> auto { return this->push_back(List); }

			template<auto... Us>
			[[nodiscard]] consteval auto push_front() const noexcept -> list<Us..., Values...>
			{
				(void)this;
				return {};
			}

			template<auto... Us>
			[[nodiscard]] consteval auto push_front(const list<Us...>) const noexcept -> list<Us..., Values...> //
			{
				return this->template push_front<Us...>();
			}

			template<list_t auto List>
			[[nodiscard]] consteval auto push_front() const noexcept -> auto { return this->push_front(List); }

			template<std::size_t N = 1>
				requires(N <= values_size)
			[[nodiscard]] consteval auto pop_back() const noexcept -> auto
			{
				(void)this;

				if constexpr (N == 0) { return list{}; }
				else if constexpr (N == values_size) { return list<>{}; }
				else
				{
					return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
					{
						return list<nth_value<Index>()...>{};
					}(std::make_index_sequence<values_size - N>{});
				}
			}

			template<std::size_t N = 1>
				requires(N <= values_size)
			[[nodiscard]] consteval auto pop_front() const noexcept -> auto
			{
				(void)this;

				if constexpr (N == 0) { return list{}; }
				else if constexpr (N == values_size) { return list<>{}; }
				else
				{
					return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
					{
						return list<nth_value<N + Index>...>{};
					}(std::make_index_sequence<values_size - N>{});
				}
			}

			template<std::size_t N = 1>
				requires(N <= values_size)
			[[nodiscard]] consteval auto back() const noexcept -> auto //
			{
				return this->template pop_front<values_size - N>();
			}

			template<std::size_t N = 1>
				requires(N <= values_size)
			[[nodiscard]] consteval auto front() const noexcept -> auto //
			{
				return this->template pop_back<values_size - N>();
			}

			template<std::size_t N = values_size>
				requires(N <= values_size)
			[[nodiscard]] consteval auto unique_front() const noexcept -> auto
			{
				(void)this;

				if constexpr (N <= 1) { return list{}; }
				else
				{
					using type = typename unique_impl<
						decltype(list{}.template front<N>().template pop_front<1>()),
						decltype(list{}.template front<1>())
					>::type;

					if constexpr (N == values_size) //
					{
						return type{};
					}
					else //
					{
						return type{}.template push_back<list{}.template back<values_size - N>()>();
					}
				}
			}

			template<std::size_t N = values_size>
				requires(N <= values_size)
			[[nodiscard]] consteval auto unique_back() const noexcept -> auto
			{
				(void)this;

				if constexpr (N <= 1) { return list{}; }
				else
				{
					using type = typename unique_impl<
						decltype(list{}.template back<N>().template pop_front<1>()),
						decltype(list{}.template back<N>().template front<1>())
					>::type;

					if constexpr (N == values_size) //
					{
						return type{};
					}
					else //
					{
						return this->template front<values_size - N>().template push_back<type{}>();
					}
				}
			}

			[[nodiscard]] consteval auto unique() const noexcept -> auto { return this->template unique_back<values_size>(); }

			template<template<auto> typename Prediction>
			[[nodiscard]] consteval auto sub_list() const noexcept -> typename sub_list_impl<Prediction, list, list<>>::type
			{
				(void)this;
				return {};
			}

			template<auto Value, template<auto, auto> typename Prediction>
				requires requires(list l) { l.template sub_list<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>(); }
			[[nodiscard]] consteval auto sub_list() const noexcept -> auto //
			{
				return this->template sub_list<VALUE_LIST_WORKAROUND_BINDER(Value, Prediction)>();
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
			[[nodiscard]] constexpr auto to_integral() const noexcept -> T
			{
				(void)this;

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
			[[nodiscard]] constexpr auto to_floating_point() const noexcept -> T
			{
				(void)this;

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

			[[nodiscard]] constexpr auto numerator_length() const noexcept -> std::size_t //
				requires(((Cs >= '0' and Cs <= '9') or Cs == '\'' or Cs == '.') and ...)
			{
				(void)this;

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

			[[nodiscard]] constexpr auto denominator_length() const noexcept -> std::size_t //
				requires(((Cs >= '0' and Cs <= '9') or Cs == '\'' or Cs == '.') and ...)
			{
				(void)this;

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

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
		template<auto... Values>
		constexpr auto value_list = value_list_detail::list<Values...>{};

		template<value_list_detail::list_t auto List>
		using value_list_type = std::decay_t<decltype(List)>;

		template<typename T>
		concept value_list_t = value_list_detail::list_t<T>;

		template<char... Cs>
		constexpr auto char_list = value_list_detail::char_list<Cs...>{};

		template<value_list_detail::char_list_t auto List>
		using char_list_type = std::decay_t<decltype(List)>;

		template<typename T>
		concept char_list_t = value_list_detail::char_list_t<T>;
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}

#undef VALUE_LIST_WORKAROUND_BINDER
