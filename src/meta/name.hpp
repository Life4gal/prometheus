// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.meta:name;

import std;

#else
#include <string_view>
#include <source_location>

#include <prometheus/macro.hpp>
#endif

struct dummy_struct_do_not_put_into_any_namespace {};

enum class DummyEnumDoNotPutIntoAnyNamespace
{
	DO_NOT_USE
};

namespace gal::prometheus::meta
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace name
	{
		template<typename... Ts> // DO NOT REMOVE `Ts`
		[[nodiscard]] constexpr auto get_full_function_name() noexcept -> std::string_view { return std::source_location::current().function_name(); }

		template<auto... Vs> // DO NOT REMOVE `Vs`
		[[nodiscard]] constexpr auto get_full_function_name() noexcept -> std::string_view { return std::source_location::current().function_name(); }
	}

	template<typename T>
	[[nodiscard]] constexpr auto name_of_type() noexcept -> std::string_view
	{
		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		// MSVC
		// const class std::basic_string_view<char, struct std::char_traits<char>> `__calling_convention` `namespace`::get_full_function_name<struct `dummy_struct_do_not_put_into_any_namespace` >(void)
		constexpr std::string_view full_function_name = name::get_full_function_name<dummy_struct_do_not_put_into_any_namespace>();
		constexpr auto full_function_name_size = full_function_name.size();

		constexpr std::string_view dummy_struct_name{"struct dummy_struct_do_not_put_into_any_namespace"};
		constexpr auto dummy_struct_name_size = std::ranges::size(dummy_struct_name);

		// const class std::basic_string_view<char, struct std::char_traits<char>> `__calling_convention` `namespace`::get_full_function_name<
		constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_struct_name);
		static_assert(full_function_name_prefix_size != std::string_view::npos);

		// >(void)
		constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_struct_name_size;
		#elif defined(GAL_PROMETHEUS_COMPILER_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL)
		// CLANG/CLANG-CL
		// std::string_view `namespace`::get_full_function_name() [T = `dummy_struct_do_not_put_into_any_namespace`]
		constexpr std::string_view full_function_name      = name::get_full_function_name<dummy_struct_do_not_put_into_any_namespace>();
		constexpr auto             full_function_name_size = full_function_name.size();

		constexpr std::string_view dummy_struct_name{"dummy_struct_do_not_put_into_any_namespace"};
		constexpr auto             dummy_struct_name_size = std::ranges::size(dummy_struct_name);

		// std::string_view `namespace`::get_full_function_name() [T =
		constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_struct_name);
		static_assert(full_function_name_prefix_size != std::string_view::npos);

		// ]
		constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_struct_name_size;
		#else
		// GCC
		// constexpr std::string_view `namespace`::get_full_function_name() [with <template-parameter-1-1> = `dummy_struct_do_not_put_into_any_namespace`; std::string_view = std::basic_string_view<char>]
		constexpr std::string_view full_function_name      = name::get_full_function_name<dummy_struct_do_not_put_into_any_namespace>();
		constexpr auto             full_function_name_size = full_function_name.size();

		constexpr std::string_view dummy_struct_name{"dummy_struct_do_not_put_into_any_namespace"};
		constexpr auto             dummy_struct_name_size = std::ranges::size(dummy_struct_name);

		// constexpr std::string_view `namespace`::get_full_function_name() [with <template-parameter-1-1> =
		constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_struct_name);
		static_assert(full_function_name_prefix_size != std::string_view::npos);

		// ; std::string_view = std::basic_string_view<char>]
		constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_struct_name_size;
		#endif

		auto full_name = name::get_full_function_name<std::decay_t<T>>();
		full_name.remove_prefix(full_function_name_prefix_size);
		full_name.remove_suffix(full_function_name_suffix_size);
		return full_name;
	}

	template<typename T>
	[[nodiscard]] constexpr auto name_of() noexcept -> std::string_view { return name_of_type<T>(); }

	template<auto EnumValue, typename EnumType = std::decay_t<decltype(EnumValue)>>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto name_of_enum_value() noexcept -> std::string_view
	{
		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		// MSVC
		// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<`DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE`>(void) noexcept
		constexpr std::string_view full_function_name = name::get_full_function_name<DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE>();
		constexpr auto full_function_name_size = full_function_name.size();

		constexpr std::string_view dummy_enum_value_name = "DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE";
		constexpr auto dummy_enum_value_name_size = std::ranges::size(dummy_enum_value_name);

		// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<
		constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_enum_value_name);
		static_assert(full_function_name_prefix_size != std::string_view::npos);

		// >(void) noexcept
		constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_enum_value_name_size;
		#elif defined(GAL_PROMETHEUS_COMPILER_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL)
		// CLANG/CLANG-CL
		// std::string_view `namespace`::get_full_function_name() [V = `DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE`]
		constexpr std::string_view full_function_name      = name::get_full_function_name<DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE>();
		constexpr auto             full_function_name_size = full_function_name.size();

		constexpr std::string_view dummy_struct_name{"DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE"};
		constexpr auto             dummy_struct_name_size = std::ranges::size(dummy_struct_name);

		// std::string_view `namespace`::get_full_function_name() [V =
		constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_struct_name);
		static_assert(full_function_name_prefix_size != std::string_view::npos);

		// ]
		constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_struct_name_size;
		#else
		// GCC
		// constexpr std::string_view `namespace`::get_full_function_name() [with auto <anonymous> = `DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE`; std::string_view = std::basic_string_view<char>]
		constexpr std::string_view full_function_name      = name::get_full_function_name<DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE>();
		constexpr auto             full_function_name_size = full_function_name.size();

		constexpr std::string_view dummy_struct_name{"DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE"};
		constexpr auto             dummy_struct_name_size = std::ranges::size(dummy_struct_name);

		// std::string_view `namespace`::get_full_function_name() [V =
		constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_struct_name);
		static_assert(full_function_name_prefix_size != std::string_view::npos);

		// ; std::string_view = std::basic_string_view<char>]
		constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_struct_name_size;
		#endif

		auto full_name = name::get_full_function_name<EnumValue>();
		full_name.remove_prefix(full_function_name_prefix_size);
		full_name.remove_suffix(full_function_name_suffix_size);
		return full_name;
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
} // namespace gal::prometheus::meta
