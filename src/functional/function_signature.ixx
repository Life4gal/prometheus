// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.functional:function_signature;

import std;

#else
#pragma once

#include <utility>
#include <type_traits>
#include <tuple>

#include <prometheus/macro.hpp>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::functional)
{
	template<
		typename ReturnType,
		typename ParameterType,
		bool IsNoexcept = false,
		bool IsMember = false,
		bool IsMemberObject = false,
		bool IsObject = false
	>
	struct function_signature
	{
		using return_type = ReturnType;
		using parameter_type = ParameterType;
		constexpr static bool is_noexcept = IsNoexcept;
		constexpr static bool is_member = IsMember;
		constexpr static bool is_member_object = IsMemberObject;
		constexpr static bool is_object = IsObject;

		constexpr function_signature() noexcept = default;

		template<typename T>
		constexpr explicit function_signature(T&&) noexcept {} // NOLINT(bugprone-forwarding-reference-overload, cppcoreguidelines-missing-std-forward)
	};

	// normal function
	template<typename ReturnType, typename... ParameterTypes>
	function_signature(ReturnType (*)(ParameterTypes...)) -> function_signature<ReturnType, std::tuple<ParameterTypes...>>;
	template<typename ReturnType, typename... ParameterTypes>
	function_signature(ReturnType (*)(ParameterTypes...) noexcept) -> function_signature<ReturnType, std::tuple<ParameterTypes...>, true>;

	// object's member data
	template<typename ReturnType, typename ObjectType>
	function_signature(ReturnType ObjectType::*) -> function_signature<ReturnType, std::tuple<ObjectType&>, true, true, true>;

	// ***************************************************************************************
	// object's member function with cv-qualification
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
	// object's member function with cv-qualification && lvalue only(&)
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
	// object's member function with cv-qualification && rvalue only(&&)
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
		if constexpr (requires { &Function::operator(); })
		{
			return function_signature<
				typename decltype(function_signature{&std::decay_t<Function>::operator()})::return_type,
				typename decltype(function_signature{&std::decay_t<Function>::operator()})::parameter_type,
				decltype(function_signature{&std::decay_t<Function>::operator()})::is_noexcept,
				false,
				false,
				true>{};
		}
		else { return function_signature{function}; }
	}
}
