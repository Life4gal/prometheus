// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <tuple>
#include <utility>

#include <prometheus/macro.hpp>

#include <meta/name.hpp>
#include <meta/string.hpp>

namespace gal::prometheus::meta
{
	template<typename T>
	struct extern_accessor
	{
		[[nodiscard]] constexpr static auto make() noexcept -> T
		{
			return T{};
		}
	};

	namespace member_detail
	{
		// note that this requires the target type to be default constructible
		template<typename T>
		extern const auto extern_any = extern_accessor<T>::make();

		template<std::size_t>
		struct placeholder
		{
			constexpr explicit(false) placeholder(auto&&...) noexcept {}
		};

		struct any
		{
			template<typename T>
			// ReSharper disable once CppFunctionIsNotImplemented
			// ReSharper disable once CppNonExplicitConversionOperator
			constexpr explicit(false) operator T() const noexcept;
		};

		template<class T>
		struct any_except_base_of
		{
			template<class U>
				requires(not std::is_base_of_v<U, T>)
			// ReSharper disable once CppFunctionIsNotImplemented
			// ReSharper disable once CppNonExplicitConversionOperator
			constexpr explicit(false) operator U() const noexcept;
		};

		template<typename T>
		struct wrapper
		{
			T& ref; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
		};

		template<typename T>
		wrapper(T&) -> wrapper<T>;

		template<typename>
		constexpr auto is_tuple_structured_binding_v = false;

		template<typename T>
			requires requires
			{
				// lazy
				// ReSharper disable once CppUseTypeTraitAlias
				std::tuple_size<T>::value;
				// ReSharper disable once CppUseTypeTraitAlias
				typename std::tuple_element<0, T>::type;
			} and (requires(const T& tuple)
			       {
				       // member function
				       tuple.template get<0>();
			       } or
			       requires(const T& tuple)
			       {
				       // free function
				       get<0>(tuple);
			       }
			)
		constexpr auto is_tuple_structured_binding_v<T> = true;

		template<typename>
		constexpr auto is_aggregate_structured_binding_v = false;

		template<typename T>
			requires std::is_aggregate_v<T>
		constexpr auto is_aggregate_structured_binding_v<T> = true;

		template<typename T>
		constexpr auto is_structured_binding_v = is_tuple_structured_binding_v<T> or is_aggregate_structured_binding_v<T>;

		template<typename T>
		concept structured_binding_t = is_structured_binding_v<T>;

		constexpr auto member_size_unknown = std::numeric_limits<std::size_t>::max();

		template<typename T, typename... Args>
		[[nodiscard]] constexpr auto member_size_impl() noexcept -> std::size_t
		{
			// the user defines tuple_element/tuple_size/get, and we believe the user
			if constexpr (is_tuple_structured_binding_v<T>)
			{
				return std::tuple_size_v<T>;
			}
			// the user does not define tuple_element/tuple_size/get, but the target type supports aggregate initialization.
			else if constexpr (is_aggregate_structured_binding_v<T>)
			{
				// the minimum size of each type is 1 (at least at this point), so the number of arguments cannot be larger than the type size
				if constexpr (sizeof...(Args) > sizeof(T))
				{
					return member_size_unknown;
				}
				else if constexpr (
					requires { T{Args{}...}; } and
					not requires { T{Args{}..., any{}}; }
				)
				{
					return (0 + ... + std::is_same_v<Args, any_except_base_of<T>>);
				}
				else if constexpr (
					requires { T{Args{}...}; } and
					not requires { T{Args{}..., any_except_base_of<T>{}}; }
				)
				{
					return member_size_impl<T, Args..., any>();
				}
				else
				{
					return member_size_impl<T, Args..., any_except_base_of<T>>();
				}
			}
			else
			{
				return member_size_unknown;
			}
		}

		template<typename T>
		[[nodiscard]] constexpr auto member_size() noexcept -> std::size_t
		{
			return member_size_impl<std::remove_cvref_t<T>>();
		}

		template<typename>
		constexpr auto is_known_member_size_v = false;

		template<typename T>
			requires (member_size<T>() != member_size_unknown)
		constexpr auto is_known_member_size_v<T> = true;

		template<typename T>
		concept known_member_size_t = is_known_member_size_v<T>;

		template<typename T>
		concept known_member_t = known_member_size_t<T> and structured_binding_t<T>;

		template<std::size_t N>
		constexpr auto nth_element_impl = []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> decltype(auto)
		{
			return []<typename NthType>(placeholder<Index>&&..., NthType&& nth, auto&&...) noexcept -> decltype(auto)
			{
				return std::forward<NthType>(nth);
			};
		}(std::make_index_sequence<N>{});

		template<std::size_t N, typename... Args>
			requires (N < sizeof...(Args))
		[[nodiscard]] constexpr auto nth_element(Args&&... args) noexcept -> decltype(auto)
		{
			return nth_element_impl<N>(std::forward<Args>(args)...);
		}

		// member.visit.inl
		template<typename Function, typename T>
		[[nodiscard]] constexpr auto visit(Function&& function, T&& object) noexcept -> decltype(auto);

		template<std::size_t Index, typename Function, typename... Args>
		constexpr auto invoke(const Function& function, Args&&... args) noexcept -> decltype(auto)
		{
			// function<Index>(args...)
			if constexpr (requires { function.template operator()<Index>(std::forward<Args>(args)...); })
			{
				return function.template operator()<Index>(std::forward<Args>(args)...);
			}
			// function(args...)
			else if constexpr (requires { function(std::forward<Args>(args)...); })
			{
				return function(std::forward<Args>(args)...);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	}

	template<typename T>
	concept known_member_t = member_detail::known_member_t<T>;

	template<member_detail::known_member_size_t T>
	[[nodiscard]] constexpr auto member_size() noexcept -> std::size_t
	{
		return member_detail::member_size<T>();
	}

	template<std::size_t N, typename T>
		requires (known_member_t<std::remove_cvref_t<T>> and N < member_size<std::remove_cvref_t<T>>())
	[[nodiscard]] constexpr auto member_of_index(T&& object) noexcept -> decltype(auto)
	{
		using bare_type = std::remove_cvref_t<T>;
		if constexpr (member_detail::is_tuple_structured_binding_v<bare_type>)
		{
			if constexpr (requires { std::forward<T>(object).template get<N>(); })
			{
				return std::forward<T>(object).template get<N>();
			}
			else if constexpr (requires { get<N>(std::forward<T>(object)); })
			{
				return get<N>(std::forward<T>(object));
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
		else if constexpr (member_detail::is_aggregate_structured_binding_v<bare_type>)
		{
			return member_detail::visit(
				[]<typename... Ts>(Ts&&... args) noexcept -> decltype(auto)
				{
					return member_detail::nth_element<N>(std::forward<Ts>(args)...);
				},
				std::forward<T>(object)
			);
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	namespace member_detail
	{
		template<std::size_t N, typename T>
			requires (member_detail::known_member_t<std::remove_cvref_t<T>> and N < member_size<std::remove_cvref_t<T>>())
		struct member_type_of_index
		{
			using type = std::decay_t<decltype(meta::member_of_index<N>(std::declval<T>()))>;
		};
	}

	template<std::size_t N, typename T>
	using member_type_of_index = typename member_detail::member_type_of_index<N, T>::type;

	template<std::size_t N, typename T>
		requires (known_member_t<std::remove_cvref_t<T>> and N < member_size<std::remove_cvref_t<T>>())
	[[nodiscard]] constexpr auto name_of_member() noexcept -> std::string_view
	{
		constexpr auto full_function_name = get_full_function_name<
			member_detail::visit(
				[]<typename... Ts>(Ts&&... args) noexcept -> auto //
				{
					return member_detail::wrapper
					{
							member_detail::nth_element<N>(std::forward<Ts>(args)...)
					};
				},
				member_detail::extern_any<std::remove_cvref_t<T>>) //
		>();

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		// MSVC
		// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<struct `namespace`::member_name::wrapper<`member_type` const >{const `member_type`&:`namespace`::member_name::extern_any<struct `my_struct`>->`member_name`}>(void) noexcept
		// constexpr auto full_function_name_size = full_function_name.size();

		constexpr std::string_view splitter{">->"};
		constexpr auto splitter_size = splitter.size();

		// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<struct `namespace`::member_name::wrapper<`member_type` const >{const `member_type`&:`namespace`::member_name::extern_any<struct `my_struct`>->
		static_assert(full_function_name.find(splitter) != std::string_view::npos);
		constexpr auto full_function_name_prefix_size = full_function_name.find(splitter) + splitter_size;

		// }>(void) noexcept
		constexpr std::string_view suffix{"}>(void) noexcept"};
		constexpr auto full_function_name_suffix_size = suffix.size();
		#elif defined(GAL_PROMETHEUS_COMPILER_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL)
		// CLANG/CLANG-CL
		// std::string_view `namespace`::get_full_function_name() [Vs = <decltype(_Invoker1<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &, const `member_type` &>::_Call(static_cast<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &>(_Obj), static_cast<const `member_type` &>(_Arg1))){extern_any.`member_name`}>]
		// constexpr auto full_function_name_size = full_function_name.size();

		constexpr std::string_view splitter{"extern_any."};
		constexpr auto splitter_size = splitter.size();

		// std::string_view `namespace`::get_full_function_name() [Vs = <decltype(_Invoker1<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &, const `member_type` &>::_Call(static_cast<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &>(_Obj), static_cast<const `member_type` &>(_Arg1))){extern_any.
		static_assert(full_function_name.find(splitter) != std::string_view::npos);
		constexpr auto full_function_name_prefix_size = full_function_name.find(splitter) + splitter_size;

		// }>]
		constexpr std::string_view suffix{"}>]"};
		constexpr auto full_function_name_suffix_size = suffix.size();
		#else
		// GCC
		// constexpr std::string_view `namespace`::get_full_function_name() [with auto ...<anonymous> = {`namespace`::member_name::wrapper<const bool>{`namespace`::member_name::extern_any<`my_struct`>.`my_struct`::`member_name`}}; std::string_view = std::basic_string_view<char>]
		// constexpr auto full_function_name_size = full_function_name.size();

		// fixme: find a suitable splitter.
		// extern_any<`my_struct`>.`my_struct`::`member_name`
		constexpr std::string_view type_name = name_of<std::remove_cvref_t<T>>();
		constexpr auto type_name_size = type_name.size() + 2; // 2 == `::`
		constexpr std::string_view splitter{">."};
		constexpr auto splitter_size = splitter.size();

		// constexpr std::string_view `namespace`::get_full_function_name() [with auto ...<anonymous> = {`namespace`::member_name::wrapper<const bool>{`namespace`::member_name::extern_any<`my_struct`>.`my_struct`::
		static_assert(full_function_name.find(splitter) != std::string_view::npos);
		constexpr auto full_function_name_prefix_size = full_function_name.find(splitter) + splitter_size + type_name_size;

		// }}; std::string_view = std::basic_string_view<char>]
		constexpr std::string_view suffix{"}}; std::string_view = std::basic_string_view<char>]"};
		constexpr auto full_function_name_suffix_size = suffix.size();
		#endif

		auto name = full_function_name;
		name.remove_prefix(full_function_name_prefix_size);
		name.remove_suffix(full_function_name_suffix_size);
		return name;
	}

	constexpr auto member_index_unknown = std::numeric_limits<std::size_t>::max();

	namespace member_detail
	{
		template<basic_fixed_string Name, typename T>
		[[nodiscard]] constexpr auto member_index() noexcept -> std::size_t
		{
			return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> std::size_t
			{
				const auto f = []<std::size_t I>() noexcept
				{
					if constexpr (name_of_member<I, T>() == Name)
					{
						return I;
					}
					else
					{
						return member_index_unknown;
					}
				};

				std::size_t index;
				(((index = f.template operator()<Index>()) == member_index_unknown) and ...);
				return index;
			}(std::make_index_sequence<member_size<T>()>{});
		}

		template<typename T>
		[[nodiscard]] constexpr auto member_index(const std::string_view name) noexcept -> std::size_t
		{
			return [name]<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> std::size_t
			{
				const auto f = [name]<std::size_t I>() noexcept
				{
					if (name_of_member<I, T>() == name)
					{
						return I;
					}

					return member_index_unknown;
				};

				std::size_t index;
				(((index = f.template operator()<Index>()) == member_index_unknown) and ...);
				return index;
			}(std::make_index_sequence<member_size<T>()>{});
		}
	}

	template<basic_fixed_string Name, typename T>
		requires (known_member_t<std::remove_cvref_t<T>>)
	[[nodiscard]] constexpr auto member_index() noexcept -> std::size_t
	{
		return member_detail::member_index<Name, T>();
	}

	template<typename T>
		requires (known_member_t<std::remove_cvref_t<T>>)
	[[nodiscard]] constexpr auto member_index(const std::string_view name) noexcept -> std::size_t
	{
		return member_detail::member_index<T>(name);
	}

	template<basic_fixed_string Name, typename T>
		requires (known_member_t<std::remove_cvref_t<T>>)
	[[nodiscard]] constexpr auto has_member() noexcept -> bool
	{
		return member_index<Name, T>() != member_index_unknown;
	}

	template<typename T>
		requires (known_member_t<std::remove_cvref_t<T>>)
	[[nodiscard]] constexpr auto has_member(const std::string_view name) noexcept -> bool
	{
		return member_index<T>(name) != member_index_unknown;
	}

	namespace member_detail
	{
		enum class FoldCategory : std::uint8_t
		{
			ALL,
			UNTIL_FALSE,
		};

		template<FoldCategory Category, typename Function, typename T, typename... Ts>
		constexpr auto member_walk(
			const Function& function,
			T&& object,
			Ts&&... optional_objects
		) noexcept -> void
		{
			[&function]<std::size_t... Index, typename... Us>(std::index_sequence<Index...>, Us&&... os) noexcept -> void
			{
				const auto f = [&function]<std::size_t I, typename... Os>(Os&&... o) noexcept -> decltype(auto)
				{
					return member_detail::invoke<I>(function, meta::member_of_index<I>(std::forward<Os>(o))...);
				};

				if constexpr (Category == FoldCategory::ALL)
				{
					(f.template operator()<Index>(std::forward<Us>(os)...), ...);
				}
				else if constexpr (Category == FoldCategory::UNTIL_FALSE)
				{
					(f.template operator()<Index>(std::forward<Us>(os)...) and ...);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}(std::make_index_sequence<meta::member_size<std::remove_cvref_t<T>>()>{}, std::forward<T>(object), std::forward<Ts>(optional_objects)...);
		}
	}

	template<typename Function, typename T, typename... Ts>
		requires
		(
			known_member_t<std::remove_cvref_t<T>> and
			(
				sizeof...(Ts) == 0 or
				(
					(
						known_member_t<std::remove_cvref_t<Ts>> and
						member_size<std::remove_cvref_t<Ts>>() == member_size<std::remove_cvref_t<T>>()
					)
					and ...
				)
			)
		)
	constexpr auto member_walk(
		const Function& function,
		T&& object,
		Ts&&... optional_objects
	) noexcept -> void
	{
		member_detail::member_walk<member_detail::FoldCategory::ALL>(function, std::forward<T>(object), std::forward<Ts>(optional_objects)...);
	}

	template<typename Function, typename T, typename... Ts>
		requires
		(
			known_member_t<std::remove_cvref_t<T>> and
			(
				sizeof...(Ts) == 0 or
				(
					(
						known_member_t<std::remove_cvref_t<Ts>> and
						member_size<std::remove_cvref_t<Ts>>() == member_size<std::remove_cvref_t<T>>()
					)
					and ...
				)
			)
		)
	constexpr auto member_walk_until(
		const Function& function,
		T&& object,
		Ts&&... optional_objects
	) noexcept -> void
	{
		member_detail::member_walk<member_detail::FoldCategory::UNTIL_FALSE>(function, std::forward<T>(object), std::forward<Ts>(optional_objects)...);
	}
}

#include <meta/member.visit.inl>
