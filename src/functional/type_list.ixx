// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.functional:type_list;

import std;

#else
#pragma once

#include <type_traits>
#include <tuple>

#include <prometheus/macro.hpp>

#endif

#if defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG) or defined(GAL_PROMETHEUS_COMPILER_GNU)
#define TYPE_LIST_WORKAROUND_BINDER(T, Prediction) binder<T, Prediction>::template rebind
#else
#define TYPE_LIST_WORKAROUND_BINDER(T, Prediction) typename binder<T, Prediction>::rebind
#endif

namespace gal::prometheus::functional
{
	namespace type_list_detail
	{
		template<typename... Ts>
		struct list;

		template<typename>
		struct is_list : std::false_type {};

		template<typename... Ts>
		struct is_list<list<Ts...>> : std::true_type {};

		template<typename T>
		constexpr auto is_list_v = is_list<T>::value;

		template<typename T>
		concept list_t = is_list_v<T>;

		// =================================
		// nth type
		// =================================

		template<std::size_t Index, typename... Ts>
		struct nth_type_impl;

		template<typename T, typename... Ts>
		struct nth_type_impl<0, T, Ts...>
		{
			using type = T;
		};

		template<std::size_t Index, typename T, typename... Ts>
			requires(Index != 0)
		struct nth_type_impl<Index, T, Ts...>
		{
			using type = typename nth_type_impl<Index - 1, Ts...>::type;
		};

		// =================================
		// index of
		// =================================

		template<std::size_t Index, template<typename> typename Prediction, typename, typename... Ts>
		struct index_of_impl;

		template<std::size_t Index, template<typename> typename Prediction, typename T, typename... Ts>
			requires(Prediction<T>::value)
		struct index_of_impl<Index, Prediction, T, Ts...>
		{
			constexpr static auto value = Index;
		};

		template<std::size_t Index, template<typename> typename Prediction, typename, typename... Ts>
		struct index_of_impl
		{
			constexpr static auto value = index_of_impl<Index + 1, Prediction, Ts...>::value;
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

		template<typename Front, typename... RemainingTs, typename... CurrentTs>
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

		template<template<typename> typename Prediction, list_t Remaining, list_t Current>
		struct sub_list_impl;

		template<template<typename> typename Prediction, list_t Current>
		struct sub_list_impl<Prediction, list<>, Current>
		{
			using type = Current;
		};

		template<template<typename> typename Prediction, typename Front, typename... RemainingTs, typename... CurrentTs>
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

		template<typename... Ts>
		struct list
		{
			using types = std::tuple<Ts...>;

			template<std::size_t Index>
				requires((sizeof...(Ts) != 0) and (Index <= sizeof...(Ts) - 1))
			using nth_type = typename nth_type_impl<Index, Ts...>::type;

			constexpr static auto types_size = std::tuple_size_v<types>;

		private:
			template<typename T, template<typename, typename> typename Prediction>
			struct binder
			{
				template<typename U>
				using rebind = Prediction<T, U>;
			};

		public:
			[[nodiscard]] consteval auto size() const noexcept -> std::size_t
			{
				(void)this;
				return types_size;
			}

			template<template<typename> typename Projection>
			[[nodiscard]] consteval auto projection() const noexcept -> list<typename Projection<Ts>::type...> // NOLINT(modernize-type-traits)
			{
				(void)this;
				return {};
			}

			template<template<typename> typename Prediction>
			[[nodiscard]] consteval auto all() const noexcept -> bool
			{
				(void)this;
				return ((Prediction<Ts>::value) and ...);
			}

			template<template<typename, typename> typename Prediction, typename T>
			[[nodiscard]] consteval auto all() const noexcept -> bool
			{
				(void)this;
				return this->template all<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>();
			}

			template<template<typename, typename> typename Prediction, typename... Us>
				requires(sizeof...(Us) > 1)
			[[nodiscard]] consteval auto all(const list<Us...>) const noexcept -> bool
			{
				(void)this;
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
				{
					return ((Prediction<nth_type<Index>, typename list<Us...>::template nth_type<Index>>::value) and ...);
				}(std::make_index_sequence<sizeof...(Us)>{});
			}

			template<template<typename, typename> typename Prediction, typename... Us>
				requires(sizeof...(Us) > 1)
			[[nodiscard]] consteval auto all() const noexcept -> bool
			{
				return this->template all<Prediction, Us...>(list<Us...>{});
			}

			template<typename... Us>
			[[nodiscard]] consteval auto all() const noexcept -> bool { return this->all<std::is_same, Us...>(); }

			template<template<typename> typename Prediction>
			[[nodiscard]] consteval auto any() const noexcept -> bool
			{
				(void)this;
				return ((Prediction<Ts>::value) or ...);
			}

			template<template<typename, typename> typename Prediction, typename T>
			[[nodiscard]] consteval auto any() const noexcept -> bool { return this->template any<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>(); }

			template<template<typename, typename> typename Prediction, typename... Us>
				requires(sizeof...(Us) > 1)
			[[nodiscard]] consteval auto any(const list<Us...>) const noexcept -> bool
			{
				(void)this;
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
				{
					return ((Prediction<nth_type<Index>, typename list<Us...>::template nth_type<Index>>::value) or ...);
				}(std::make_index_sequence<sizeof...(Us)>{});
			}

			template<template<typename, typename> typename Prediction, typename... Us>
				requires(sizeof...(Us) > 1)
			[[nodiscard]] consteval auto any() const noexcept -> bool
			{
				return this->template any<Prediction, Us...>(list<Us...>{});
			}

			template<typename... Us>
			[[nodiscard]] consteval auto any() const noexcept -> bool
			{
				return this->any<std::is_same, Us...>();
			}

			template<template<typename> typename Prediction>
				requires(list{}.any<Prediction>())
			[[nodiscard]] consteval auto index_of() const noexcept -> std::size_t
			{
				(void)this;
				return index_of_impl<0, Prediction, Ts...>::value;
			}

			template<typename T, template<typename, typename> typename Prediction>
				requires requires(list l) { l.template index_of<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>(); }
			[[nodiscard]] consteval auto index_of() const noexcept -> std::size_t
			{
				return this->template index_of<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>();
			}

			template<typename T>
				requires requires(list l) { l.template index_of<T, std::is_same>(); }
			[[nodiscard]] consteval auto index_of() const noexcept -> std::size_t { return this->template index_of<T, std::is_same>(); }

			[[nodiscard]] consteval auto reverse() const noexcept -> auto //
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
				{
					return list<std::tuple_element_t<types_size - 1 - Index, types>...>{};
				}(std::make_index_sequence<types_size>{});
			}

			template<typename... Us>
			[[nodiscard]] consteval auto push_back() const noexcept -> list<Ts..., Us...>
			{
				(void)this;
				return {};
			}

			template<typename... Us>
			[[nodiscard]] consteval auto push_back(const list<Us...>) const noexcept -> list<Ts..., Us...> { return this->push_back<Us...>(); }

			template<list_t auto List>
			[[nodiscard]] consteval auto push_back() const noexcept -> auto { return this->push_back(List); }

			template<typename... Us>
			[[nodiscard]] consteval auto push_front() const noexcept -> list<Us..., Ts...>
			{
				(void)this;
				return {};
			}

			template<typename... Us>
			[[nodiscard]] consteval auto push_front(const list<Us...>) const noexcept -> list<Us..., Ts...> { return this->push_front<Us...>(); }

			template<list_t auto List>
			[[nodiscard]] consteval auto push_front() const noexcept -> auto { return this->push_front(List); }

			template<std::size_t N = 1>
				requires(N <= types_size)
			[[nodiscard]] consteval auto pop_back() const noexcept -> auto
			{
				(void)this;

				if constexpr (N == 0) { return list{}; }
				else if constexpr (N == types_size) { return list<>{}; }
				else
				{
					return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
					{
						return list<std::tuple_element_t<Index, types>...>{};
					}(std::make_index_sequence<types_size - N>{});
				}
			}

			template<std::size_t N = 1>
				requires (N <= types_size)
			[[nodiscard]] consteval auto pop_front() const noexcept -> auto
			{
				(void)this;

				if constexpr (N == 0) { return list{}; }
				else if constexpr (N == types_size) { return list<>{}; }
				else
				{
					return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
					{
						return list<std::tuple_element_t<N + Index, types>...>{};
					}(std::make_index_sequence<types_size - N>{});
				}
			}

			template<std::size_t N = 1>
				requires(N <= types_size)
			[[nodiscard]] consteval auto back() const noexcept -> auto { return this->template pop_front<types_size - N>(); }

			template<std::size_t N = 1>
				requires(N <= types_size)
			[[nodiscard]] consteval auto front() const noexcept -> auto { return this->template pop_back<types_size - N>(); }

			template<std::size_t N = types_size>
				requires(N <= types_size)
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

					if constexpr (N == types_size) //
					{
						return type{};
					}
					else //
					{
						return type{}.template push_back<list{}.template back<types_size - N>()>();
					}
				}
			}

			template<std::size_t N = types_size>
				requires(N <= types_size)
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

					if constexpr (N == types_size) //
					{
						return type{};
					}
					else //
					{
						return this->template front<types_size - N>().template push_back<type{}>();
					}
				}
			}

			[[nodiscard]] consteval auto unique() const noexcept -> auto { return this->template unique_back<types_size>(); }

			[[nodiscard]] consteval auto to_tuple() const noexcept -> std::tuple<Ts...>
			{
				(void)this;
				return {};
			}

			template<template<typename> typename Prediction>
			[[nodiscard]] consteval auto sub_list() const noexcept -> typename sub_list_impl<Prediction, list, list<>>::type
			{
				(void)this;
				return {};
			}

			template<typename T, template<typename, typename> typename Prediction>
				requires requires(list l) { l.sub_list<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>(); }
			[[nodiscard]] consteval auto sub_list() const noexcept -> auto
			{
				return this->sub_list<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>();
			}
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
	template<typename... Ts>
	constexpr auto type_list = type_list_detail::list<Ts...>{};

	template<type_list_detail::list_t auto List>
	using type_list_type = std::decay_t<decltype(List)>;

	template<typename T>
	concept type_list_t = type_list_detail::list_t<T>;

	template<typename... Ts>
	[[nodiscard]] constexpr auto to_type_list(const std::tuple<Ts...>) noexcept -> auto
	{
		return type_list<Ts...>;
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
} // namespace gal::prometheus::functional

#undef TYPE_LIST_WORKAROUND_BINDER
