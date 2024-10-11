// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:meta.string;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <utility>
#include <type_traits>
#include <algorithm>
#include <string>
#include <ranges>
#include <functional>

#include <prometheus/macro.hpp>

#endif

namespace gal::prometheus::meta
{
	namespace string_detail
	{
		template<typename Container, typename Getter, typename SizeType = typename Container::size_type>
		concept getter_t = requires(const Container& container, Getter&& getter)
		{
			std::invoke(std::forward<Getter>(getter), container, std::declval<SizeType>());
		};

		template<
			typename Container,
			typename Getter,
			typename Comparator,
			typename SizeType = typename Container::size_type,
			typename ValueType =typename Container::value_type>
		concept comparator_t = requires(const Container& container, Getter&& getter, Comparator&& comparator)
		{
			std::invoke(
					std::forward<Comparator>(comparator),
					std::invoke(std::forward<Getter>(getter), container, std::declval<SizeType>()),
					std::declval<ValueType>()
					);
		};

		template<typename Container, typename SizeType = typename Container::size_type>
			requires requires(const Container& container, const SizeType index)
			{
				container[index];
			}
		struct default_getter
		{
			[[nodiscard]] constexpr auto operator()(Container& container, const SizeType index) noexcept(noexcept(container[index])) -> decltype(auto)
			{
				return container[index];
			}

			[[nodiscard]] constexpr auto operator()(
					const Container& container,
					const SizeType index
					) const noexcept(noexcept(container[index])) -> decltype(auto) { return container[index]; }
		};

		template<typename Container, typename ValueType = typename Container::value_type>
		struct default_comparator
		{
			using const_reference = std::add_const_t<std::add_lvalue_reference_t<std::remove_cvref_t<ValueType>>>;

			[[nodiscard]] constexpr auto operator()(const_reference left, const_reference right) const noexcept(noexcept(left == right))
			{
				return left == right;
			}
		};

		template<typename ValueType>
		using default_view = std::basic_string_view<ValueType>;

		enum class MetaStringDerivedCategory
		{
			MEMBER,
			MEMBER_FUNCTION,
			STATIC,
			STATIC_FUNCTION,
		};

		/**
		 * std::is_member_object_pointer_v<decltype(&T::data)> and `not` std::is_member_function_pointer_v<decltype(&T::data)> -> member-data
		 * `not` std::is_member_object_pointer_v<decltype(&T::data)> and std::is_member_function_pointer_v<decltype(&T::data)> -> member-function
		 *
		 * `not` std::is_member_object_pointer_v<decltype(&T::data)> and `not` std::is_member_function_pointer_v<decltype(&T::data)> -> static-data
		 * `not` std::is_member_object_pointer_v<decltype(&T::data)> and `not` std::is_member_function_pointer_v<decltype(&T::data)> -> static-function(invocable)
		 */
		template<typename>
		struct pointer_traits;

		// member data
		template<typename T>
			requires
			(std::is_member_object_pointer_v<decltype(&T::value)> and not std::is_member_function_pointer_v<decltype(&T::value)>) and
			std::is_convertible_v<
				decltype(std::declval<T>().value),
				std::add_pointer_t<std::add_const_t<std::remove_pointer_t<std::decay_t<decltype(std::declval<const T&>().value)>>>>
			>
		struct pointer_traits<T>
		{
			// const pointer
			using type = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<std::decay_t<decltype(std::declval<const T&>().value)>>>>;
			constexpr static auto value = MetaStringDerivedCategory::MEMBER;
		};

		// member function
		template<typename T>
			requires(not std::is_member_object_pointer_v<decltype(&T::value)> and std::is_member_function_pointer_v<decltype(&T::value)>) and
			        requires(const T& derived)
			        {
				        {
					        derived.data()
				        } -> std::convertible_to<std::decay_t<decltype(derived.data())>>;
			        }
		struct pointer_traits<T>
		{
			// const pointer
			using type = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<std::decay_t<decltype(std::declval<const T&>().data())>>>>;
			constexpr static auto value = MetaStringDerivedCategory::MEMBER_FUNCTION;
		};

		// static data
		template<typename T>
			requires (not std::is_member_object_pointer_v<decltype(&T::value)> and not std::is_member_function_pointer_v<decltype(&T::value)>) and
			         (not std::is_invocable_v<decltype(T::value)>)
		struct pointer_traits<T>
		{
			// const pointer
			using type = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<std::decay_t<decltype(T::value)>>>>;
			constexpr static auto value = MetaStringDerivedCategory::STATIC;
		};

		// static function
		template<typename T>
			requires(not std::is_member_object_pointer_v<decltype(&T::value)> and not std::is_member_function_pointer_v<decltype(&T::value)>) and
			        (std::is_invocable_v<decltype(T::value)>)
		struct pointer_traits<T>
		{
			// const pointer
			using type = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<std::decay_t<decltype(T::data())>>>>;
			constexpr static auto value = MetaStringDerivedCategory::STATIC_FUNCTION;
		};

		template<typename DerivedType>
		struct lazy_pointer_traits
		{
			using pointer = typename pointer_traits<DerivedType>::type;

			constexpr static auto category = pointer_traits<DerivedType>::value;
			constexpr static auto is_static =
					(category == MetaStringDerivedCategory::STATIC) or
					(category == MetaStringDerivedCategory::STATIC_FUNCTION);
		};

		template<typename DerivedType>
		[[nodiscard]] consteval auto lazy_is_static() noexcept -> bool
		{
			using traits_type = lazy_pointer_traits<DerivedType>;

			return traits_type::is_static;
		}

		template<typename DerivedType, typename Pointer>
		[[nodiscard]] consteval auto lazy_is_pointer() noexcept -> bool
		{
			using traits_type = lazy_pointer_traits<DerivedType>;

			return std::is_same_v<typename traits_type::pointer, Pointer>;
		}

		template<typename DerivedType, typename ValueType, typename SizeType>
		struct meta_string_base;

		template<typename>
		struct is_meta_string_base : std::false_type {};

		template<typename DerivedType, typename ValueType, typename SizeType>
		struct is_meta_string_base<meta_string_base<DerivedType, ValueType, SizeType>> : std::true_type {};

		template<typename T>
			requires requires { typename T::base_type; }
		struct is_meta_string_base<T> : is_meta_string_base<typename T::base_type> {};

		template<typename T>
		constexpr auto is_meta_string_base_v = is_meta_string_base<T>::value;
		template<typename T>
		concept meta_string_base_t = is_meta_string_base_v<T>;

		template<typename DerivedType, typename ValueType, typename SizeType>
		struct meta_string_base
		{
			using derived_type = std::remove_cvref_t<DerivedType>;

			using value_type = ValueType;
			using size_type = SizeType;

		private:
			[[nodiscard]] constexpr auto rep_value() const noexcept -> auto //
				requires(not lazy_is_static<derived_type>())
			{
				if constexpr (lazy_pointer_traits<derived_type>::category == MetaStringDerivedCategory::MEMBER)
				{
					return static_cast<typename lazy_pointer_traits<derived_type>::pointer>(static_cast<const derived_type&>(*this).value);
				}
				else if constexpr (lazy_pointer_traits<derived_type>::category == MetaStringDerivedCategory::MEMBER_FUNCTION)
				{
					return static_cast<typename lazy_pointer_traits<derived_type>::pointer>(static_cast<const derived_type&>(*this).data());
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			[[nodiscard]] constexpr static auto rep_value() noexcept -> auto //
				requires(lazy_is_static<derived_type>())
			{
				if constexpr (lazy_pointer_traits<derived_type>::category == MetaStringDerivedCategory::STATIC)
				{
					return static_cast<typename lazy_pointer_traits<derived_type>::pointer>(derived_type::value);
				}
				else if constexpr (lazy_pointer_traits<derived_type>::category == MetaStringDerivedCategory::STATIC_FUNCTION)
				{
					return static_cast<typename lazy_pointer_traits<derived_type>::pointer>(derived_type::data());
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			[[nodiscard]] constexpr auto rep_size() const noexcept -> size_type //
				requires(not lazy_is_static<derived_type>())
			{
				if constexpr (lazy_pointer_traits<derived_type>::category == MetaStringDerivedCategory::MEMBER)
				{
					return static_cast<const derived_type&>(*this).size;
				}
				else if constexpr (lazy_pointer_traits<derived_type>::category == MetaStringDerivedCategory::MEMBER_FUNCTION)
				{
					return static_cast<const derived_type&>(*this).size();
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			[[nodiscard]] constexpr static auto rep_size() noexcept -> size_type //
				requires(lazy_is_static<derived_type>())
			{
				if constexpr (lazy_pointer_traits<derived_type>::category == MetaStringDerivedCategory::STATIC) { return derived_type::size; }
				else if constexpr (lazy_pointer_traits<derived_type>::category == MetaStringDerivedCategory::STATIC_FUNCTION)
				{
					return derived_type::size();
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			template<typename StringType>
			constexpr static auto is_constructible_from_data_v = std::is_constructible_v<
				StringType, typename lazy_pointer_traits<derived_type>::pointer, size_type
			>;
			template<typename StringType>
			constexpr static auto is_constructible_from_view_v = std::is_constructible_v<StringType, default_view<value_type>>;

		public:
			// =================================================

			template<typename StringType>
				requires is_constructible_from_data_v<StringType>
			[[nodiscard]] constexpr explicit operator StringType() const noexcept //
				requires(not lazy_is_static<derived_type>()) { return StringType{rep_value(), rep_size()}; }

			template<typename StringType>
				requires is_constructible_from_view_v<StringType> and (not is_constructible_from_data_v<StringType>)
			[[nodiscard]] constexpr explicit(not std::is_convertible_v<default_view<value_type>, StringType>) operator StringType() const noexcept //
				requires(not lazy_is_static<derived_type>()) { return StringType{default_view<value_type>{rep_value(), rep_size()}}; }

			template<typename StringType>
				requires is_constructible_from_data_v<StringType>
			[[nodiscard]] constexpr auto as() const noexcept -> StringType //
				requires(not lazy_is_static<derived_type>()) { return StringType{this->operator default_view<value_type>()}; }

			template<typename StringType>
				requires is_constructible_from_data_v<StringType>
			[[nodiscard]] constexpr static auto as() noexcept -> StringType //
				requires (lazy_is_static<derived_type>()) { return StringType{meta_string_base::rep_value(), meta_string_base::rep_size()}; }

			// =================================================

			template<typename Pointer>
			[[nodiscard]] friend constexpr auto operator==(const derived_type& lhs, Pointer string) noexcept -> bool //
				requires (lazy_is_pointer<derived_type, Pointer>()) //
			{
				return std::char_traits<value_type>::length(string) == lhs.rep_size() and
				       std::char_traits<value_type>::compare(
						       string,
						       lhs.rep_value(),
						       lhs.rep_size()
						       ) == 0;
			}

			template<typename Pointer>
			[[nodiscard]] constexpr auto match(Pointer string) const noexcept -> bool //
				requires(lazy_is_pointer<derived_type, Pointer>() and not lazy_is_static<derived_type>()) //
			{
				return *this == string;
			}

			template<typename Pointer>
			[[nodiscard]] constexpr static auto match(Pointer string) noexcept -> bool //
				requires(lazy_is_pointer<derived_type, Pointer>() and lazy_is_static<derived_type>()) //
			{
				return std::char_traits<value_type>::length(string) == rep_size() and
				       std::char_traits<value_type>::compare(
						       string,
						       rep_value(),
						       rep_size()
						       ) == 0;
			}

			template<meta_string_base_t Rhs>
			[[nodiscard]] friend constexpr auto operator==(const derived_type& lhs, const Rhs& rhs) noexcept -> bool //
			{
				return lhs.template as<default_view<value_type>>() == rhs.template as<default_view<value_type>>();
			}

			template<meta_string_base_t String>
				requires(not lazy_is_static<derived_type>())
			[[nodiscard]] constexpr auto match(const String& rhs) const noexcept -> bool //
			{
				return this->template as<default_view<value_type>>() == rhs.template as<default_view<value_type>>();
			}

			template<meta_string_base_t String>
				requires(lazy_is_static<derived_type>())
			[[nodiscard]] constexpr static auto match(const String& rhs) noexcept -> bool //
			{
				return meta_string_base::as<default_view<value_type>>() == rhs.template as<default_view<value_type>>();
			}

			template<typename String>
				requires (std::is_constructible_v<default_view<value_type>, String> and not is_meta_string_base_v<String>)
			[[nodiscard]] friend constexpr auto operator==(
					const derived_type& lhs,
					const String& rhs
					) noexcept(std::is_nothrow_constructible_v<default_view<value_type>, String>) -> bool //
			{
				//
				return lhs.template as<default_view<value_type>>() == default_view<value_type>{rhs};
			}

			template<typename String>
				requires(std::is_constructible_v<default_view<value_type>, String> and not is_meta_string_base_v<String>)
			[[nodiscard]] constexpr auto match(const String& rhs) const
				noexcept(std::is_nothrow_constructible_v<default_view<value_type>, String>) -> bool //
				requires(not lazy_is_static<derived_type>())
			{
				//
				return *this == rhs;
			}

			template<typename String>
				requires(std::is_constructible_v<default_view<value_type>, String> and not is_meta_string_base_v<String>)
			[[nodiscard]] constexpr static auto match(const String& rhs)
				noexcept(std::is_nothrow_constructible_v<default_view<value_type>, String>) -> bool //
				requires(lazy_is_static<derived_type>())
			{
				//
				return meta_string_base::as<default_view<value_type>>() == default_view<value_type>{rhs};
			}

			// container + getter + comparator
			template<typename Container, typename Getter, typename Comparator>
				requires getter_t<Container, Getter, size_type> and comparator_t<Container, Getter, Comparator, size_type, value_type>
			[[nodiscard]] constexpr auto match(
					const Container& container,
					Getter getter,
					Comparator comparator //
					) const noexcept(noexcept(comparator(getter(container, 0), rep_value()[0]))) -> bool //
				requires(not lazy_is_static<derived_type>())
			{
				if (std::ranges::equal(std::ranges::size(container), rep_size())) { return false; }

				return std::ranges::all_of(
						std::views::iota(static_cast<size_type>(0), rep_size()),
						[comparator](const auto& pair) noexcept(noexcept(comparator(pair.first, pair.second)))
						{
							return comparator(pair.first, pair.second);
						},
						[this, &container, getter](const auto index) noexcept(noexcept(getter(container, index)))
						{
							return std::make_pair(std::cref(getter(container, index)), std::cref(rep_value()[index]));
						});
			}

			// container + getter + comparator
			template<typename Container, typename Getter, typename Comparator>
				requires getter_t<Container, Getter, size_type> and comparator_t<Container, Getter, Comparator, size_type, value_type>
			[[nodiscard]] constexpr static auto match(
					const Container& container,
					Getter getter,
					Comparator comparator //
					) noexcept(noexcept(comparator(getter(container, 0), rep_value()[0]))) -> bool //
				requires(lazy_is_static<derived_type>())
			{
				if (std::ranges::equal(std::ranges::size(container), rep_size())) { return false; }

				return std::ranges::all_of(
						std::views::iota(static_cast<size_type>(0), rep_size()),
						[comparator](const auto& pair) noexcept(noexcept(comparator(pair.first, pair.second)))
						{
							return comparator(pair.first, pair.second);
						},
						[&container, getter](const auto index) noexcept(noexcept(getter(container, index)))
						{
							return std::make_pair(std::cref(getter(container, index)), std::cref(rep_value()[index]));
						});
			}

			// container + default_getter + comparator
			template<typename Container, typename Comparator>
				requires comparator_t<Container, default_getter<Container, size_type>, Comparator, size_type, value_type>
			[[nodiscard]] constexpr auto match(
					const Container& container,
					Comparator comparator //
					) const noexcept(noexcept(comparator(default_getter<Container, size_type>{}(container, 0), rep_value()[0]))) -> bool //
				requires(not lazy_is_static<derived_type>())
			{
				//
				return this->template match<Container, default_getter<Container, size_type>, Comparator>(
						container,
						default_getter<Container, size_type>{},
						comparator);
			}

			// container + default_getter + comparator
			template<typename Container, typename Comparator>
				requires comparator_t<Container, default_getter<Container, size_type>, Comparator, size_type, value_type>
			[[nodiscard]] constexpr static auto match(
					const Container& container,
					Comparator comparator //
					) noexcept(noexcept(comparator(default_getter<Container, size_type>{}(container, 0), rep_value()[0]))) -> bool //
				requires(lazy_is_static<derived_type>())
			{
				//
				return meta_string_base::match<Container, default_getter<Container, size_type>, Comparator>(
						container,
						default_getter<Container, size_type>{},
						comparator);
			}

			// container + getter + default_comparator
			template<typename Container, typename Getter>
				requires getter_t<Container, Getter, size_type>
			[[nodiscard]] constexpr auto match(
					const Container& container,
					Getter getter //
					) const noexcept(noexcept(default_comparator<Container, value_type>{}(getter(container, 0), rep_value()[0]))) -> bool //
				requires(not lazy_is_static<derived_type>())
			{
				//
				return this->template match<Container, Getter, default_comparator<Container, value_type>>(
						container,
						getter,
						default_comparator<Container, value_type>{});
			}

			// container + getter + default_comparator
			template<typename Container, typename Getter>
				requires getter_t<Container, Getter, size_type>
			[[nodiscard]] constexpr static auto match(
					const Container& container,
					Getter getter //
					) noexcept(noexcept(default_comparator<Container, value_type>{}(getter(container, 0), rep_value()[0]))) -> bool //
				requires(lazy_is_static<derived_type>())
			{
				//
				return meta_string_base::match<Container, Getter, default_comparator<Container, value_type>>(
						container,
						getter,
						default_comparator<Container, value_type>{});
			}

			// container + default_getter + default_comparator
			template<typename Container, typename Getter>
				requires getter_t<Container, Getter, size_type>
			[[nodiscard]] constexpr auto match(
					const Container& container //
					) const noexcept(noexcept(default_comparator<Container, value_type>{}(
					default_getter<Container, size_type>{}(container, 0),
					rep_value()[0]
					))) -> bool //
				requires(not lazy_is_static<derived_type>())
			{
				//
				return this->template match<Container, Getter, default_comparator<Container, value_type>>(
						container,
						default_getter<Container, size_type>{},
						default_comparator<Container, value_type>{});
			}

			// container + default_getter + default_comparator
			template<typename Container, typename Getter>
				requires getter_t<Container, Getter, size_type>
			[[nodiscard]] constexpr static auto match(
					const Container& container //
					) noexcept(noexcept(default_comparator<Container, value_type>{}(
					default_getter<Container, size_type>{}(container, 0),
					rep_value()[0]
					))) -> bool //
				requires(lazy_is_static<derived_type>())
			{
				//
				return meta_string_base::match<Container, Getter, default_comparator<Container, value_type>>(
						container,
						default_getter<Container, size_type>{},
						default_comparator<Container, value_type>{});
			}
		};

		template<typename T, T This, T... Cs>
		[[nodiscard]] consteval auto contains_zero() noexcept -> bool
		{
			if constexpr (sizeof...(Cs) == 0) { return This == static_cast<T>(0); }
			else { return contains_zero<T, Cs...>(); }
		}
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
		/**
		 * @brief An immutable array of fixed-length characters.
		 */
		template<typename T, T... Cs>
		struct basic_char_array;

		template<typename T>
		using basic_char_array_view = string_detail::default_view<T>;

		/**
		 * @brief A mutable array of fixed-length characters.
		 */
		template<typename T, std::size_t N>
		struct basic_fixed_string;

		template<typename T>
		using basic_fixed_string_view = string_detail::default_view<T>;

		template<typename>
		struct is_basic_char_array : std::false_type {};

		template<typename T, T... Cs>
		struct is_basic_char_array<basic_char_array<T, Cs...>> : std::true_type {};

		template<typename S>
		constexpr auto is_basic_char_array_v = is_basic_char_array<S>::value;

		template<typename>
		struct is_basic_fixed_string : std::false_type {};

		template<typename T, std::size_t N>
		struct is_basic_fixed_string<basic_fixed_string<T, N>> : std::true_type {};

		template<typename S>
		constexpr auto is_basic_fixed_string_v = is_basic_fixed_string<S>::value;

		template<typename S>
		concept basic_char_array_t = is_basic_char_array_v<S>;

		template<typename S>
		concept basic_fixed_string_t = is_basic_fixed_string_v<S>;

		template<typename T, T... Cs>
		struct basic_char_array : string_detail::meta_string_base<basic_char_array<T, Cs...>, T, std::size_t>
		{
			using base_type = string_detail::meta_string_base<basic_char_array, T, std::size_t>;

			using value_type = T;
			using size_type = std::size_t;

			using const_pointer = const value_type*;

			constexpr static size_type max_size = sizeof...(Cs);
			constexpr static value_type value[max_size]{Cs...};
			constexpr static size_type size = max_size - (value[max_size - 1] == '\0');

			[[nodiscard]] constexpr static const_pointer begin() noexcept { return std::ranges::begin(value); }

			[[nodiscard]] constexpr static const_pointer end() noexcept { return std::ranges::end(value); }

			template<std::size_t N = max_size>
				requires(N <= max_size)
			[[nodiscard]] constexpr static auto as_fixed_string() noexcept
			{
				return basic_fixed_string<value_type, N>{basic_fixed_string_view<value_type>{value, N}};
			}

			// // basic_char_array <=> basic_char_array
			// template<value_type... U>
			// [[nodiscard]] constexpr auto operator<=>(const basic_char_array<value_type, U...>& rhs) const noexcept -> auto
			// {
			// 	//
			// 	return this->template as<basic_char_array_view<value_type>>() <=> rhs.template as<basic_char_array_view<value_type>>();
			// }
			//
			// // basic_char_array <=> string
			// template<typename String>
			// 	requires(not basic_char_array_t<String>) and std::is_constructible_v<basic_char_array_view<value_type>, String>
			// [[nodiscard]] constexpr auto operator<=>(const String& rhs) const noexcept(std::is_nothrow_constructible_v<basic_char_array_view<value_type>, String>) -> auto
			// {
			// 	//
			// 	return this->template as<basic_char_array_view<value_type>>() <=> basic_char_array_view<value_type>{rhs};
			// }
			//
			// // string <=> basic_char_array
			// template<typename String>
			// 	requires(not basic_char_array_t<String>) and std::is_constructible_v<basic_char_array_view<value_type>, String>
			// [[nodiscard]] friend constexpr auto operator<=>(const String& lhs, const basic_char_array& rhs) noexcept(std::is_nothrow_constructible_v<basic_char_array_view<value_type>, String>) -> auto
			// {
			// 	//
			// 	return basic_char_array_view<value_type>{lhs} <=> rhs.template as<basic_char_array_view<value_type>>();
			// }
		};

		template<basic_fixed_string FixedString>
		[[nodiscard]] constexpr auto to_char_array() noexcept -> auto
		{
			using fixed_string_type = decltype(FixedString);

			return []<std::size_t... Index>(const std::index_sequence<Index...>) noexcept
			{
				return basic_char_array<typename fixed_string_type::value_type, FixedString.value[Index]...>{};
			}(std::make_index_sequence<FixedString.max_size>{});
		}

		template<char... Chars>
		using char_array = basic_char_array<char, Chars...>;
		template<wchar_t... Chars>
		using wchar_array = basic_char_array<wchar_t, Chars...>;
		template<char8_t... Chars>
		// ReSharper disable once CppInconsistentNaming
		using u8char_array = basic_char_array<char8_t, Chars...>;
		template<char16_t... Chars>
		// ReSharper disable once CppInconsistentNaming
		using u16char_array = basic_char_array<char16_t, Chars...>;
		template<char32_t... Chars>
		// ReSharper disable once CppInconsistentNaming
		using u32char_array = basic_char_array<char32_t, Chars...>;

		template<typename T, std::size_t N>
		struct basic_fixed_string : string_detail::meta_string_base<basic_fixed_string<T, N>, T, std::size_t>
		{
			using base_type = string_detail::meta_string_base<basic_fixed_string, T, std::size_t>;

			using value_type = T;
			using size_type = std::size_t;

			using pointer = value_type*;
			using const_pointer = const value_type*;

			constexpr static size_type max_size{N};
			constexpr static size_type size{max_size - 1};
			value_type value[max_size]{};

			constexpr basic_fixed_string() noexcept = default;

			template<std::size_t M>
				requires(M >= N)
			constexpr explicit(false) basic_fixed_string(const value_type (&string)[M]) noexcept
			{
				std::ranges::copy(std::ranges::begin(string), std::ranges::begin(string) + size, value);
			}

			template<value_type... Cs>
				requires(
					// DO NOT USE `basic_char_array<T, Cs...>::size`!
					sizeof...(Cs) - ((Cs == 0) or ...) >= N)
			constexpr explicit(false) basic_fixed_string(const basic_char_array<value_type, Cs...>& char_array) noexcept
			{
				std::ranges::copy(std::ranges::begin(char_array), std::ranges::begin(char_array) + N, value);
			}

			template<std::ranges::range String>
				requires std::is_same_v<std::ranges::range_value_t<String>, value_type>
			constexpr explicit basic_fixed_string(const String& string) noexcept
			{
				std::ranges::copy(std::ranges::begin(string), std::ranges::begin(string) + N, value);
			}

			[[nodiscard]] constexpr auto begin() noexcept -> pointer { return value; }

			[[nodiscard]] constexpr auto begin() const noexcept -> const_pointer { return value; }

			[[nodiscard]] constexpr auto end() noexcept -> pointer { return value + size; }

			[[nodiscard]] constexpr auto end() const noexcept -> const_pointer { return value + size; }

			// // basic_fixed_string <=> basic_fixed_string
			// template<size_type R>
			// [[nodiscard]] constexpr auto operator<=>(const basic_fixed_string<value_type, R>& rhs) const noexcept -> auto
			// {
			// 	//
			// 	return this->template as<basic_fixed_string_view<value_type>>() <=> rhs.template as<basic_fixed_string_view<value_type>>();
			// }
			//
			// // basic_fixed_string <=> string
			// template<typename String>
			// 	requires(not basic_fixed_string_t<String>) and std::is_constructible_v<basic_fixed_string_view<value_type>, String>
			// [[nodiscard]] constexpr auto operator<=>(const String& rhs) const noexcept(std::is_nothrow_constructible_v<basic_fixed_string_view<value_type>, String>) -> auto
			// {
			// 	//
			// 	return this->template as<basic_fixed_string_view<value_type>>() <=> basic_fixed_string_view<value_type>{rhs};
			// }
			//
			// // string <=> basic_fixed_string
			// template<typename String>
			// 	requires(not basic_fixed_string_t<String>) and std::is_constructible_v<basic_fixed_string_view<value_type>, String>
			// [[nodiscard]] friend constexpr auto operator<=>(const String& lhs, const basic_fixed_string& rhs) noexcept(std::is_nothrow_constructible_v<basic_fixed_string_view<value_type>, String>) -> auto
			// {
			// 	//
			// 	return basic_fixed_string_view<value_type>{lhs} <=> rhs.template as<basic_fixed_string_view<value_type>>();
			// }
		};

		template<typename T, std::size_t N>
		basic_fixed_string(const T (&string)[N]) -> basic_fixed_string<T, N>;

		template<typename T, T... Cs>
		basic_fixed_string(basic_char_array<T, Cs...> char_array) -> basic_fixed_string<
			T,
			// DO NOT USE `basic_char_array<T, Cs...>::size`!
			sizeof...(Cs) - string_detail::contains_zero<T, Cs...>()
		>;

		template<std::size_t N>
		using fixed_string = basic_fixed_string<char, N>;
		template<std::size_t N>
		// ReSharper disable once IdentifierTypo
		using fixed_wstring = basic_fixed_string<wchar_t, N>;
		template<std::size_t N>
		// ReSharper disable once CppInconsistentNaming
		using fixed_u8string = basic_fixed_string<char8_t, N>;
		template<std::size_t N>
		// ReSharper disable once CppInconsistentNaming
		using fixed_u16string = basic_fixed_string<char16_t, N>;
		template<std::size_t N>
		// ReSharper disable once CppInconsistentNaming
		using fixed_u32string = basic_fixed_string<char32_t, N>;

		using fixed_string_view = basic_fixed_string_view<char>;
		// ReSharper disable once IdentifierTypo
		using fixed_wstring_view = basic_fixed_string_view<wchar_t>;
		// ReSharper disable once CppInconsistentNaming
		using fixed_u8string_view = basic_fixed_string_view<char8_t>;
		// ReSharper disable once CppInconsistentNaming
		using fixed_u16string_view = basic_fixed_string_view<char16_t>;
		// ReSharper disable once CppInconsistentNaming
		using fixed_u32string_view = basic_fixed_string_view<char32_t>;
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
{
	// for `std::totally_ordered_with`
	template<gal::prometheus::meta::basic_char_array_t FixedString, typename String, template<typename> typename Q1, template<typename> typename Q2>
		requires std::is_constructible_v<gal::prometheus::meta::basic_char_array_view<typename FixedString::value_type>, String>
	struct basic_common_reference<FixedString, String, Q1, Q2> // NOLINT(cert-dcl58-cpp)
	{
		using type = gal::prometheus::meta::basic_char_array_view<typename FixedString::value_type>;
	};

	template<typename String, gal::prometheus::meta::basic_char_array_t FixedString, template<typename> typename Q1, template<typename> typename Q2>
		requires std::is_constructible_v<gal::prometheus::meta::basic_char_array_view<typename FixedString::value_type>, String>
	struct basic_common_reference<String, FixedString, Q1, Q2> // NOLINT(cert-dcl58-cpp)
	{
		using type = gal::prometheus::meta::basic_char_array_view<typename FixedString::value_type>;
	};

	// for `std::totally_ordered_with`
	template<gal::prometheus::meta::basic_fixed_string_t FixedString, typename String, template<typename> typename Q1, template<typename> typename Q2>
		requires std::is_constructible_v<gal::prometheus::meta::basic_fixed_string_view<typename FixedString::value_type>, String>
	struct basic_common_reference<FixedString, String, Q1, Q2> // NOLINT(cert-dcl58-cpp)
	{
		using type = gal::prometheus::meta::basic_fixed_string_view<typename FixedString::value_type>;
	};

	template<typename String, gal::prometheus::meta::basic_fixed_string_t FixedString, template<typename> typename Q1, template<typename> typename Q2>
		requires std::is_constructible_v<gal::prometheus::meta::basic_fixed_string_view<typename FixedString::value_type>, String>
	struct basic_common_reference<String, FixedString, Q1, Q2> // NOLINT(cert-dcl58-cpp)
	{
		using type = gal::prometheus::meta::basic_fixed_string_view<typename FixedString::value_type>;
	};
}
