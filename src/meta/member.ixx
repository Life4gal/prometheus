// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.meta:member;

import std;
import :string;

#else
#pragma once

#include <string>
#include <source_location>
#include <tuple>
#include <utility>

#include <prometheus/macro.hpp>
#include <meta/string.ixx>

#endif

namespace gal::prometheus::meta
{
	namespace member_detail
	{
		// fixme: dup
		// ReSharper disable once CppTemplateParameterNeverUsed
		template<auto... Vs> // DO NOT REMOVE `Vs`
		[[nodiscard]] constexpr auto get_full_function_name() noexcept -> std::string_view { return std::source_location::current().function_name(); }

		template<typename T>
		extern const T extern_any{};

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

		template<typename T>
		struct wrapper
		{
			T& ref; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
		};

		template<typename T>
		wrapper(T&) -> wrapper<T>;

		template<typename>
		struct is_tuple_like : std::false_type {};

		template<typename T>
			requires requires
			{
				// lazy
				// ReSharper disable once CppUseTypeTraitAlias
				std::tuple_size<T>::value;
				// ReSharper disable once CppUseTypeTraitAlias
				typename std::tuple_element<0, T>::type;
			} and (requires(const T& t)
			       {
				       // member function
				       t.template get<0>();
			       } or
			       requires(const T& t)
			       {
				       // free function
				       get<0>(t);
			       }
			)
		struct is_tuple_like<T> : std::true_type {};

		template<typename T>
		constexpr auto is_tuple_like_v = is_tuple_like<T>::value;

		template<typename T>
		constexpr auto is_member_element_gettable_v =
				// user-defined structured-binding
				is_tuple_like_v<T> or
				// auto structured-binding
				std::is_aggregate_v<T>;

		template<typename T>
		concept member_element_gettable_t = is_member_element_gettable_v<T>;

		constexpr auto member_size_unknown = std::numeric_limits<std::size_t>::max();

		template<typename T, typename... Args>
		[[nodiscard]] constexpr auto member_size_impl() noexcept -> std::size_t //
		{
			if constexpr (is_tuple_like_v<T>) { return std::tuple_size_v<T>; }
			else if constexpr (std::is_aggregate_v<T>)
			{
				if constexpr (sizeof...(Args) > sizeof(T)) { return member_size_unknown; }
				else if constexpr (requires { T{Args{}...}; } and not requires { T{Args{}..., any{}}; }
				) { return sizeof...(Args); }
				else { return member_size_impl<T, Args..., any>(); }
			}
			else { return member_size_unknown; }
		}

		template<typename T>
		[[nodiscard]] constexpr auto member_size() noexcept -> std::size_t //
		{
			return member_size_impl<std::remove_cvref_t<T>>();
		}

		template<typename T>
		[[nodiscard]] constexpr auto member_size(T&&) noexcept -> std::size_t //
		{
			return member_size<std::remove_cvref_t<T>>();
		}

		template<typename T>
		constexpr auto is_member_size_gettable_v = member_size<T>() != member_size_unknown;
		template<typename T>
		concept member_size_gettable_t = is_member_size_gettable_v<T>;

		template<typename T>
		constexpr auto is_member_gettable_v = is_member_element_gettable_v<T> and is_member_size_gettable_v<T>;
		template<typename T>
		concept member_gettable_t = //
				member_element_gettable_t<T> and
				member_size_gettable_t<T>;

		template<typename Function, typename T>
			requires member_gettable_t<std::remove_cvref_t<T>>
		[[nodiscard]] constexpr auto visit(Function&& function, T&& value) noexcept -> decltype(auto)
		{
			#define MEMBER_NAME_VISIT_DO_FORWARD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__) // std::forward
			#define MEMBER_NAME_VISIT_DO_FORWARD_LIKE(...) static_cast<std::conditional_t<std::is_lvalue_reference_v<T>, std::add_lvalue_reference_t<std::remove_reference_t<decltype(__VA_ARGS__)>>, std::add_rvalue_reference_t<std::remove_reference_t<decltype(__VA_ARGS__)>>>>(__VA_ARGS__)// forward like



			#if __cpp_structured_bindings >= 202401L
			auto&& [... vs] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
			return std::invoke(MEMBER_NAME_VISIT_DO_FORWARD(function), MEMBER_NAME_VISIT_DO_FORWARD_LIKE(vs)...);
			#else
			if constexpr (constexpr auto size = member_size<T>();
				size == 1)
			{
				auto&& [m0] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0)
						);
			}
			else if constexpr (size == 2)
			{
				auto&& [m0, m1] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1)
						);
			}
			else if constexpr (size == 3)
			{
				auto&& [m0, m1, m2] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2)
						);
			}
			else if constexpr (size == 4)
			{
				auto&& [m0, m1, m2, m3] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3)
						);
			}
			else if constexpr (size == 5)
			{
				auto&& [m0, m1, m2, m3, m4] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4)
						);
			}
			else if constexpr (size == 6)
			{
				auto&& [m0, m1, m2, m3, m4, m5] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5)
						);
			}
			else if constexpr (size == 7)
			{
				auto&& [m0, m1, m2, m3, m4, m5, m6] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6)
						);
			}
			else if constexpr (size == 8)
			{
				auto&& [m0, m1, m2, m3, m4, m5, m6, m7] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7)
						);
			}
			else if constexpr (size == 9)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8)
						);
			}
			else if constexpr (size == 10)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9)
						);
			}
			else if constexpr (size == 11)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10)
						);
			}
			else if constexpr (size == 12)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11)
						);
			}
			else if constexpr (size == 13)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12)
						);
			}
			else if constexpr (size == 14)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13)
						);
			}
			else if constexpr (size == 15)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14)
						);
			}
			else if constexpr (size == 16)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15)
						);
			}
			else if constexpr (size == 17)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16)
						);
			}
			else if constexpr (size == 18)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17)
						);
			}
			else if constexpr (size == 19)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18)
						);
			}
			else if constexpr (size == 20)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19)
						);
			}
			else if constexpr (size == 21)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20)
						);
			}
			else if constexpr (size == 22)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6,m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21)
						);
			}
			else if constexpr (size == 23)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22)
						);
			}
			else if constexpr (size == 24)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23)
						);
			}
			else if constexpr (size == 25)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23,
					m24
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24)
						);
			}
			else if constexpr (size == 26)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6,m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23,
					m24, m25
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25)
						);
			}
			else if constexpr (size == 27)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23,
					m24, m25,m26
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26)
						);
			}
			else if constexpr (size == 28)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6,m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23,
					m24, m25,m26,m27
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27)
						);
			}
			else if constexpr (size == 29)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23,
					m24, m25,m26,m27, m28
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28)
						);
			}
			else if constexpr (size == 30)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6,m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23,
					m24, m25,m26,m27, m28, m29
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29)
						);
			}
			else if constexpr (size == 31)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6,
					m7, m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23,
					m24, m25,m26,m27, m28, m29, m30
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30)
						);
			}
			else if constexpr (size == 32)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23,
					m24, m25,m26,m27, m28, m29, m30, m31
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31)
						);
			}
			else if constexpr (size == 33)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6,m7,
					m8, m9, m10, m11, m12, m13,m14, m15,
					m16, m17, m18, m19,m20, m21, m22, m23,
					m24, m25,m26,m27, m28, m29, m30, m31,
					m32
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32)
						);
			}
			else if constexpr (size == 34)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6,m7,
					m8, m9, m10, m11, m12,m13, m14, m15,
					m16, m17, m18,m19, m20, m21, m22, m23,
					m24,m25, m26,m27, m28, m29, m30, m31,
					m32, m33
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33)
						);
			}
			else if constexpr (size == 35)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34)
						);
			}
			else if constexpr (size == 36)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35)
						);
			}
			else if constexpr (size == 37)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36)
						);
			}
			else if constexpr (size == 38)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37)
						);
			}
			else if constexpr (size == 39)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38)
						);
			}
			else if constexpr (size == 40)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39)
						);
			}
			else if constexpr (size == 41)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40)
						);
			}
			else if constexpr (size == 42)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7, m8,
					m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30,
					m31, m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41)
						);
			}
			else if constexpr (size == 43)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42)
						);
			}
			else if constexpr (size == 44)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43)
						);
			}
			else if constexpr (size == 45)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44)
						);
			}
			else if constexpr (size == 46)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45)
						);
			}
			else if constexpr (size == 47)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46)
						);
			}
			else if constexpr (size == 48)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47)
						);
			}
			else if constexpr (size == 49)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48)
						);
			}
			else if constexpr (size == 50)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49)
						);
			}
			else if constexpr (size == 51)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50)
						);
			}
			else if constexpr (size == 52)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50,m51
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51)
						);
			}
			else if constexpr (size == 53)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50,m51, m52
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52)
						);
			}
			else if constexpr (size == 54)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53)
						);
			}
			else if constexpr (size == 55)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54)
						);
			}
			else if constexpr (size == 56)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54, m55
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m55)
						);
			}
			else if constexpr (size == 57)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54, m55,
					m56
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m55),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m56)
						);
			}
			else if constexpr (size == 58)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54, m55,
					m56, m57
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m55),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m56),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m57)
						);
			}
			else if constexpr (size == 59)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54, m55,
					m56, m57, m58
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m55),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m56),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m57),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m58)
						);
			}
			else if constexpr (size == 60)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54, m55,
					m56, m57, m58, m59
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m55),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m56),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m57),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m58),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m59)
						);
			}
			else if constexpr (size == 61)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54, m55,
					m56, m57, m58, m59, m60
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m55),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m56),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m57),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m58),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m59),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m60)
						);
			}
			else if constexpr (size == 62)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23, m24,
					m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54, m55,
					m56, m57, m58, m59, m60, m61
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m55),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m56),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m57),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m58),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m59),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m60),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m61)
						);
			}
			else if constexpr (size == 63)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54, m55,
					m56, m57, m58, m59, m60, m61, m62
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m55),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m56),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m57),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m58),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m59),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m60),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m61),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m62)
						);
			}
			else if constexpr (size == 64)
			{
				auto&& [
					m0, m1, m2, m3, m4, m5, m6, m7,
					m8, m9, m10, m11, m12, m13, m14, m15,
					m16, m17, m18, m19, m20, m21, m22, m23,
					m24, m25, m26,m27, m28, m29, m30, m31,
					m32, m33, m34, m35, m36, m37, m38, m39,
					m40, m41, m42, m43, m44, m45, m46, m47,
					m48, m49, m50, m51, m52,m53, m54, m55,
					m56, m57, m58, m59, m60, m61, m62, m63
				] = MEMBER_NAME_VISIT_DO_FORWARD_LIKE(value);
				return std::invoke(
						MEMBER_NAME_VISIT_DO_FORWARD(function),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m0),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m1),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m2),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m3),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m4),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m5),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m6),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m7),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m8),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m9),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m10),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m11),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m12),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m13),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m14),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m15),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m16),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m17),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m18),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m19),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m20),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m21),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m22),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m23),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m24),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m25),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m26),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m27),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m28),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m29),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m30),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m31),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m32),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m33),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m34),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m35),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m36),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m37),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m38),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m39),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m40),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m41),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m42),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m43),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m44),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m45),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m46),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m47),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m48),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m49),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m50),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m51),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m52),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m53),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m54),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m55),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m56),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m57),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m58),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m59),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m60),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m61),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m62),
						MEMBER_NAME_VISIT_DO_FORWARD_LIKE(m63)
						);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("too much members."); }
			#endif

			#undef MEMBER_NAME_VISIT_DO_FORWARD_LIKE
		}

		template<std::size_t N>
		constexpr auto nth_element_impl = []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> decltype(auto) //
		{
			return []<typename NthType>(placeholder<Index>&&..., NthType&& nth, auto&&...) noexcept -> decltype(auto) //
			{
				return std::forward<NthType>(nth);
			};
		}(std::make_index_sequence<N>{});

		template<std::size_t N, typename... Args>
			requires (N < sizeof...(Args))
		[[nodiscard]] constexpr auto nth_element(Args&&... args) noexcept -> decltype(auto) //
		{
			return nth_element_impl<N>(std::forward<Args>(args)...);
		}

		template<std::size_t Index, typename Function, typename... Args>
		constexpr auto invoke(Function&& function, Args&&... args) noexcept -> void
		{
			if constexpr (requires { std::forward<Function>(function).template operator()<Index>(std::forward<Args>(args)...); }) //
			{
				std::forward<Function>(function).template operator()<Index>(std::forward<Args>(args)...);
			}
			else if constexpr (requires { std::forward<Function>(function)(std::forward<Args>(args)...); }) //
			{
				std::forward<Function>(function)(std::forward<Args>(args)...);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
		template<typename T>
		constexpr auto is_member_gettable_v = member_detail::is_member_gettable_v<T>;
		template<typename T>
		concept member_gettable_t = member_detail::member_gettable_t<T>;

		template<member_detail::member_size_gettable_t T>
		[[nodiscard]] constexpr auto member_size() noexcept -> std::size_t //
		{
			return member_detail::member_size<T>();
		}

		template<std::size_t N, typename T>
			requires member_gettable_t<std::remove_cvref_t<T>>
		[[nodiscard]] constexpr auto member_of_index(T&& value) noexcept -> decltype(auto) //
		{
			return member_detail::visit(
					[]<typename... Ts>(Ts&&... args) noexcept -> decltype(auto) //
					{
						return member_detail::nth_element<N>(std::forward<Ts>(args)...);
					},
					std::forward<T>(value));
		}

		template<std::size_t N, typename T>
			requires member_gettable_t<std::remove_cvref_t<T>>
		struct member_type_of_index
		{
			using type = std::decay_t<decltype(member_of_index<N>(std::declval<T>()))>;
		};

		template<std::size_t N, typename T>
		using member_type_of_index_t = typename member_type_of_index<N, T>::type;

		template<typename Function, typename T>
			requires member_gettable_t<std::remove_cvref_t<T>>
		constexpr auto member_view_all(Function function, T&& value) noexcept -> void
		{
			[function]<std::size_t... Index, typename U>(
					std::index_sequence<Index...>,
					U&& u
					) mutable noexcept -> void //
					{
						function(member_of_index<Index>(std::forward<U>(u)...));
					}(std::make_index_sequence<member_size<T>()>{}, std::forward<T>(value));
		}

		template<typename Function, typename T, typename... Ts>
			requires
			(
				// type
				member_gettable_t<std::remove_cvref_t<T>> and
				(sizeof...(Ts) == 0 or ((member_gettable_t<std::remove_cvref_t<Ts>>) and ...)) and
				// size
				(sizeof...(Ts) == 0 or ((member_size<T>() == member_size<Ts>()) and ...))
			)
		constexpr auto member_for_each(
				Function function,
				T&& value,
				Ts&&... optional_extra_values
				) noexcept -> void
		{
			[function] <std::size_t... Index, typename... Us>(
					std::index_sequence<Index...>,
					Us&&... us
					) mutable noexcept -> void //
					{
						(
							member_detail::invoke<Index>( //
									function,
									member_of_index<Index>(std::forward<Us>(us))...
									), //
							... //
						);
					}(std::make_index_sequence<member_size<T>()>{}, std::forward<T>(value), std::forward<Ts>(optional_extra_values)...);
		}

		template<typename Function, typename T, typename... Ts>
			requires
			(
				// type
				member_gettable_t<std::remove_cvref_t<T>> and
				(sizeof...(Ts) == 0 or ((member_gettable_t<std::remove_cvref_t<Ts>>) and ...)) and
				// size
				(sizeof...(Ts) == 0 or ((member_size<T>() == member_size<Ts>()) and ...))
			)
		constexpr auto member_for_each_until(
				Function function,
				T&& value,
				Ts&&... optional_extra_values
				) noexcept -> void
		{
			[function] <std::size_t... Index, typename... Us>(
					std::index_sequence<Index...>,
					Us&&... us
					) mutable noexcept -> void //
					{
						(
							member_detail::invoke<Index>( //
									function,
									member_of_index<Index>(std::forward<Us>(us))...
									) and //
							... //
						);
					}(std::make_index_sequence<member_size<T>()>{}, std::forward<T>(value), std::forward<Ts>(optional_extra_values)...);
		}

		template<typename T, std::size_t N>
			requires member_gettable_t<std::remove_cvref_t<T>>
		[[nodiscard]] constexpr auto name_of_member() noexcept -> std::string_view
		{
			constexpr auto full_function_name = member_detail::get_full_function_name<
				member_detail::visit(
						[]<typename... Ts>(Ts&&... args) noexcept -> auto //
						{
							return member_detail::wrapper{member_detail::nth_element<N>(std::forward<Ts>(args)...)};
						},
						member_detail::extern_any<std::remove_cvref_t<T>>) //
			>();

			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			// MSVC
			// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<struct `namespace`::member_name::wrapper<`member_type` const >{const `member_type`&:`namespace`::member_name::extern_any<struct `my_struct`>->`member_name`}>(void) noexcept
			constexpr auto full_function_name_size = full_function_name.size();

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
			constexpr auto full_function_name_size = full_function_name.size();

			constexpr std::string_view splitter{"extern_any."};
			constexpr auto             splitter_size = splitter.size();

			// std::string_view `namespace`::get_full_function_name() [Vs = <decltype(_Invoker1<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &, const `member_type` &>::_Call(static_cast<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &>(_Obj), static_cast<const `member_type` &>(_Arg1))){extern_any.
			static_assert(full_function_name.find(splitter) != std::string_view::npos);
			constexpr auto full_function_name_prefix_size = full_function_name.find(splitter) + splitter_size;

			// }>]
			constexpr std::string_view suffix{"}>]"};
			constexpr auto             full_function_name_suffix_size = suffix.size();
			#else
			// GCC
			// constexpr std::string_view `namespace`::get_full_function_name() [with auto ...<anonymous> = {`namespace`::member_name::wrapper<const bool>{`namespace`::member_name::extern_any<`my_struct`>.`my_struct`::`member_name`}}; std::string_view = std::basic_string_view<char>]
			constexpr auto full_function_name_size = full_function_name.size();

			// fixme: find a suitable splitter.
			// extern_any<`my_struct`>.`my_struct`::`member_name`
			constexpr std::string_view type_name = name_of<BareType>();
			constexpr auto             type_name_size = type_name.size() + 2; // 2 == `::`
			constexpr std::string_view splitter{">."};
			constexpr auto             splitter_size = splitter.size();

			// constexpr std::string_view `namespace`::get_full_function_name() [with auto ...<anonymous> = {`namespace`::member_name::wrapper<const bool>{`namespace`::member_name::extern_any<`my_struct`>.`my_struct`::
			static_assert(full_function_name.find(splitter) != std::string_view::npos);
			constexpr auto full_function_name_prefix_size = full_function_name.find(splitter) + splitter_size + type_name_size;

			// }}; std::string_view = std::basic_string_view<char>]
			constexpr std::string_view suffix{"}}; std::string_view = std::basic_string_view<char>]"};
			constexpr auto             full_function_name_suffix_size = suffix.size();
			#endif

			auto name = full_function_name;
			name.remove_prefix(full_function_name_prefix_size);
			name.remove_suffix(full_function_name_suffix_size);
			return name;
		}
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace member_detail
	{
		constexpr auto index_not_found = static_cast<std::size_t>(-1);

		template<typename T, basic_fixed_string Name>
		[[nodiscard]] constexpr auto index_of_member_name() noexcept -> std::size_t
		{
			return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> std::size_t
			{
				std::size_t index = index_not_found;

				(
					[&index]<std::size_t I>() noexcept //
					{
						if constexpr (name_of_member<T, I>() == Name) { index = I; }
					}.
					template operator()<Index>(),
					...);

				return index;
			}(std::make_index_sequence<member_size<T>()>{});
		}

		template<typename T>
		[[nodiscard]] constexpr auto index_of_member_name(const std::string_view name) noexcept -> std::size_t
		{
			return [name]<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> std::size_t
			{
				std::size_t index = index_not_found;

				([&index, name]<std::size_t I>() noexcept //
					{
						if constexpr (name_of_member<T, I>() == name) { index = I; }
					}.template operator()<Index>(),
					...);

				return index;
			}(std::make_index_sequence<member_size<T>()>{});
		}
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
		template<typename T, basic_fixed_string Name>
			requires member_gettable_t<std::remove_cvref_t<T>>
		[[nodiscard]] constexpr auto has_member_of_name() noexcept -> bool //
		{
			return member_detail::index_of_member_name<std::remove_cvref_t<T>, Name>() != member_detail::index_not_found;
		}

		template<typename T>
			requires member_gettable_t<std::remove_cvref_t<T>>
		[[nodiscard]] constexpr auto has_member_of_name(const std::string_view name) noexcept -> bool //
		{
			return member_detail::index_of_member_name<std::remove_cvref_t<T>>(name) != member_detail::index_not_found;
		}

		template<basic_fixed_string Name, typename T>
		concept has_member_of_name_t = has_member_of_name<T, Name>();

		template<basic_fixed_string Name, typename T>
			requires has_member_of_name_t<Name, T>
		[[nodiscard]] constexpr auto member_of_name(T&& value) noexcept -> decltype(auto)
		{
			return member_detail::visit(
					[]<typename... Ts>(Ts&&... args) noexcept -> decltype(auto)
					{
						constexpr auto index = member_detail::index_of_member_name<T, Name>();

						return member_detail::nth_element<index>(std::forward<Ts>(args)...);
					},
					std::forward<T>(value));
		}
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
