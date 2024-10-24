// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:functional.function_signature;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <utility>
#include <type_traits>
#include <tuple>

#include <prometheus/macro.hpp>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(functional)
{
	template<
		typename ReturnType,
		typename ParameterType,
		bool IsNoexcept = false,
		bool IsMemberFunction = false,
		bool IsMemberObject = false
	>
	struct function_signature
	{
		using return_type = ReturnType;
		using parameter_type = ParameterType;
		constexpr static bool is_noexcept = IsNoexcept;
		constexpr static bool is_member_function = IsMemberFunction;
		constexpr static bool is_member_object = IsMemberObject;

		constexpr function_signature() noexcept = default;

		template<typename T>
		constexpr explicit function_signature(T&&) noexcept {} // NOLINT(bugprone-forwarding-reference-overload, cppcoreguidelines-missing-std-forward)
	};

	// free function
	//
	// template<typename T, std::size_t N>
	// constexpr auto foo(const T&) noexcept -> int { return 42; }
	// 
	// make_function_signature(foo<int, 42>)
	// => function_signature<int, std::tuple<const int&>, true, false, false>
	// { noexcept }
	template<typename ReturnType, typename... ParameterTypes>
	function_signature(ReturnType (*)(ParameterTypes...)) -> function_signature<ReturnType, std::tuple<ParameterTypes...>>;
	template<typename ReturnType, typename... ParameterTypes>
	function_signature(ReturnType (*)(ParameterTypes...) noexcept) -> function_signature<ReturnType, std::tuple<ParameterTypes...>, true>;

	// object's member data
	// struct foo { std::string name; };
	//
	// make_function_signature(&foo::name)
	// => function_signature<std::string, std::tuple<foo&>, true, false, true>
	// { noexcept, member_object }
	template<typename ReturnType, typename ObjectType>
	function_signature(ReturnType ObjectType::*) -> function_signature<ReturnType, std::tuple<ObjectType&>, true, false, true>;

	// lambda
	//
	// constexpr auto lambda = []<typename T, std::size_t N>(const T&) noexcept -> int { return 42; };
	//
	// make_function_signature(lambda)
	// => Unable to deduce the template parameters for "function_signature"
	// 
	// functional::make_function_signature(&decltype(lambda)::operator()<int, 42>)
	// => function_signature<int, std::tuple<const <lambda>&, const int&>, true, true, false>
	// { noexcept, member_function }
	template<typename Function>
		requires requires { &Function::operator(); }
	function_signature(Function) -> function_signature<
		typename decltype(function_signature{&std::decay_t<Function>::operator()})::return_type,
		typename decltype(function_signature{&std::decay_t<Function>::operator()})::parameter_type,
		decltype(function_signature{&std::decay_t<Function>::operator()})::is_noexcept,
		true
	>;

	// ***************************************************************************************
	// member function with cv-qualification
	// ***************************************************************************************
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...)) -> function_signature<ReturnType, std::tuple<ObjectType&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) noexcept) -> function_signature<ReturnType, std::tuple<ObjectType&, ParameterTypes...>, true, true>;
	// const
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const) -> function_signature<ReturnType, std::tuple<const ObjectType&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const noexcept) -> function_signature<ReturnType, std::tuple<const ObjectType&, ParameterTypes...>, true, true>;
	// volatile
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) volatile) -> function_signature<ReturnType, std::tuple<volatile ObjectType&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) volatile noexcept) -> function_signature<ReturnType, std::tuple<volatile ObjectType&, ParameterTypes...>, true, true>;
	// const & volatile
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const volatile) -> function_signature<ReturnType, std::tuple<const volatile ObjectType&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const volatile noexcept) -> function_signature<ReturnType, std::tuple<const volatile ObjectType&, ParameterTypes...>, true, true>;

	// ***************************************************************************************
	// member function with cv-qualification && lvalue only(&)
	// ***************************************************************************************
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) &) -> function_signature<ReturnType, std::tuple<ObjectType&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) & noexcept) -> function_signature<ReturnType, std::tuple<ObjectType&, ParameterTypes...>, true, true>;
	// const
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const &) -> function_signature<ReturnType, std::tuple<const ObjectType&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const & noexcept) -> function_signature<ReturnType, std::tuple<const ObjectType&, ParameterTypes...>, true, true>;
	// volatile
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) volatile &) -> function_signature<ReturnType, std::tuple<volatile ObjectType&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) volatile & noexcept) -> function_signature<ReturnType, std::tuple<volatile ObjectType&, ParameterTypes...>, true, true>;
	// const && volatile
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const volatile &) -> function_signature<ReturnType, std::tuple<const volatile ObjectType&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const volatile & noexcept) -> function_signature<ReturnType, std::tuple<const volatile ObjectType&, ParameterTypes...>, true, true>;

	// ***************************************************************************************
	// member function with cv-qualification && rvalue only(&&)
	// ***************************************************************************************
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) &&) -> function_signature<ReturnType, std::tuple<ObjectType&&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) && noexcept) -> function_signature<ReturnType, std::tuple<ObjectType&&, ParameterTypes...>, true, true>;
	// const
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const &&) -> function_signature<ReturnType, std::tuple<const ObjectType&&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const && noexcept) -> function_signature<ReturnType, std::tuple<const ObjectType&&, ParameterTypes...>, true, true>;
	// volatile
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) volatile &&) -> function_signature<ReturnType, std::tuple<volatile ObjectType&&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) volatile && noexcept) -> function_signature<ReturnType, std::tuple<volatile ObjectType&&, ParameterTypes...>, true, true>;
	// const && volatile
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const volatile &&) -> function_signature<ReturnType, std::tuple<const volatile ObjectType&&, ParameterTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParameterTypes>
	function_signature(ReturnType (ObjectType::*)(ParameterTypes...) const volatile && noexcept) -> function_signature<ReturnType, std::tuple<const volatile ObjectType&&, ParameterTypes...>, true, true>;

	template<typename Function>
	constexpr auto make_function_signature(const Function& function) noexcept -> auto
	{
		return function_signature{function};
	}
}
