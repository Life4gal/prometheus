// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <tuple>

#include <prometheus/macro.hpp>

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
			[[nodiscard]] constexpr static auto size() noexcept -> std::size_t
			{
				return types_size;
			}

			template<template<typename> typename Projection>
			[[nodiscard]] constexpr static auto projection() noexcept -> list<typename Projection<Ts>::type...> // NOLINT(modernize-type-traits)
			{
				return {};
			}

			template<template<typename> typename Prediction>
			[[nodiscard]] constexpr static auto all() noexcept -> bool
			{
				return ((Prediction<Ts>::value) and ...);
			}

			template<template<typename, typename> typename Prediction, typename T>
			[[nodiscard]] constexpr static auto all() noexcept -> bool
			{
				return list::all<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>();
			}

			template<template<typename, typename> typename Prediction, typename... Us>
				requires(sizeof...(Us) > 1)
			[[nodiscard]] constexpr static auto all(const list<Us...>) noexcept -> bool
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
				{
					return ((Prediction<nth_type<Index>, typename list<Us...>::template nth_type<Index>>::value) and ...);
				}(std::make_index_sequence<sizeof...(Us)>{});
			}

			template<template<typename, typename> typename Prediction, typename... Us>
				requires(sizeof...(Us) > 1)
			[[nodiscard]] constexpr static auto all() noexcept -> bool
			{
				return list::all<Prediction, Us...>(list<Us...>{});
			}

			template<typename... Us>
			[[nodiscard]] constexpr static auto all() noexcept -> bool
			{
				return list::all<std::is_same, Us...>();
			}

			template<template<typename> typename Prediction>
			[[nodiscard]] constexpr static auto any() noexcept -> bool
			{
				return ((Prediction<Ts>::value) or ...);
			}

			template<template<typename, typename> typename Prediction, typename T>
			[[nodiscard]] constexpr static auto any() noexcept -> bool
			{
				return list::any<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>();
			}

			template<template<typename, typename> typename Prediction, typename... Us>
				requires(sizeof...(Us) > 1)
			[[nodiscard]] constexpr static auto any(const list<Us...>) noexcept -> bool
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
				{
					return ((Prediction<nth_type<Index>, typename list<Us...>::template nth_type<Index>>::value) or ...);
				}(std::make_index_sequence<sizeof...(Us)>{});
			}

			template<template<typename, typename> typename Prediction, typename... Us>
				requires(sizeof...(Us) > 1)
			[[nodiscard]] constexpr static auto any() noexcept -> bool
			{
				return list::any<Prediction, Us...>(list<Us...>{});
			}

			template<typename... Us>
			[[nodiscard]] constexpr static auto any() noexcept -> bool
			{
				return list::any<std::is_same, Us...>();
			}

			template<template<typename> typename Prediction>
				requires(list{}.any<Prediction>())
			[[nodiscard]] constexpr static auto index_of() noexcept -> std::size_t
			{
				return index_of_impl<0, Prediction, Ts...>::value;
			}

			template<typename T, template<typename, typename> typename Prediction>
				requires requires { list::index_of<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>(); }
			[[nodiscard]] constexpr static auto index_of() noexcept -> std::size_t
			{
				return list::index_of<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>();
			}

			template<typename T>
				requires requires { list::index_of<T, std::is_same>(); }
			[[nodiscard]] constexpr static auto index_of() noexcept -> std::size_t
			{
				return list::index_of<T, std::is_same>();
			}

			[[nodiscard]] constexpr static auto reverse() noexcept -> auto //
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> auto //
				{
					return list<std::tuple_element_t<types_size - 1 - Index, types>...>{};
				}(std::make_index_sequence<types_size>{});
			}

			template<typename... Us>
			[[nodiscard]] constexpr static auto push_back() noexcept -> list<Ts..., Us...>
			{
				return {};
			}

			template<typename... Us>
			[[nodiscard]] constexpr static auto push_back(const list<Us...>) noexcept -> list<Ts..., Us...>
			{
				return list::push_back<Us...>();
			}

			template<list_t auto List>
			[[nodiscard]] constexpr static auto push_back() noexcept -> auto
			{
				return list::push_back(List);
			}

			template<typename... Us>
			[[nodiscard]] constexpr static auto push_front() noexcept -> list<Us..., Ts...>
			{
				return {};
			}

			template<typename... Us>
			[[nodiscard]] constexpr static auto push_front(const list<Us...>) noexcept -> list<Us..., Ts...>
			{
				return list::push_front<Us...>();
			}

			template<list_t auto List>
			[[nodiscard]] constexpr static auto push_front() noexcept -> auto
			{
				return list::push_front(List);
			}

			template<std::size_t N = 1>
				requires(N <= types_size)
			[[nodiscard]] constexpr static auto pop_back() noexcept -> auto
			{
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
			[[nodiscard]] constexpr static auto pop_front() noexcept -> auto
			{
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
			[[nodiscard]] constexpr static auto back() noexcept -> auto
			{
				return list::pop_front<types_size - N>();
			}

			template<std::size_t N = 1>
				requires(N <= types_size)
			[[nodiscard]] constexpr static auto front() noexcept -> auto
			{
				return list::pop_back<types_size - N>();
			}

			template<std::size_t N = types_size>
				requires(N <= types_size)
			[[nodiscard]] constexpr static auto unique_front() noexcept -> auto
			{
				if constexpr (N <= 1) { return list{}; }
				else
				{
					using type = typename unique_impl<
						decltype(list::front<N>().template pop_front<1>()),
						decltype(list::front<1>())
					>::type;

					if constexpr (N == types_size) //
					{
						return type{};
					}
					else //
					{
						return type{}.template push_back<list::back<types_size - N>()>();
					}
				}
			}

			template<std::size_t N = types_size>
				requires(N <= types_size)
			[[nodiscard]] constexpr static auto unique_back() noexcept -> auto
			{
				if constexpr (N <= 1) { return list{}; }
				else
				{
					using type = typename unique_impl<
						decltype(list::back<N>().template pop_front<1>()),
						decltype(list::back<N>().template front<1>())
					>::type;

					if constexpr (N == types_size) //
					{
						return type{};
					}
					else //
					{
						return list::front<types_size - N>().template push_back<type{}>();
					}
				}
			}

			[[nodiscard]] constexpr static auto unique() noexcept -> auto
			{
				return list::unique_back<types_size>();
			}

			[[nodiscard]] constexpr static auto to_tuple() noexcept -> std::tuple<Ts...>
			{
				return {};
			}

			template<template<typename> typename Prediction>
			[[nodiscard]] constexpr static auto sub_list() noexcept -> typename sub_list_impl<Prediction, list, list<>>::type
			{
				return {};
			}

			template<typename T, template<typename, typename> typename Prediction>
				requires requires { list::sub_list<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>(); }
			[[nodiscard]] constexpr static auto sub_list() noexcept -> auto
			{
				return list::sub_list<TYPE_LIST_WORKAROUND_BINDER(T, Prediction)>();
			}
		};
	}

	template<typename... Ts>
	struct type_list_type : type_list_detail::list<Ts...> {};

	template<typename... Ts>
	constexpr auto type_list = type_list_type<Ts...>{};

	template<typename T>
	struct is_type_list : type_list_detail::is_list<T> {};

	template<typename T>
	constexpr auto is_type_list_v = type_list_detail::is_list_v<T>;

	template<typename T>
	concept type_list_t = type_list_detail::list_t<T>;

	// template<typename... Ts>
	// constexpr auto type_list<std::tuple<Ts...>> = type_list_type<Ts...>{};
	//
	// template<typename... Ts>
	// constexpr auto type_list<const std::tuple<Ts...>> = type_list_type<Ts...>{};

	template<typename... Ts>
	[[nodiscard]] constexpr auto to_type_list(const std::tuple<Ts...>) noexcept -> auto
	{
		return type_list<Ts...>;
	}
}

#undef TYPE_LIST_WORKAROUND_BINDER
