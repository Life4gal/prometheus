// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.utility:compiler;

import std;

struct dummy_struct_do_not_put_into_any_namespace {};

namespace gal::prometheus::utility::compiler
{
	template<typename T>
	[[nodiscard]] constexpr auto get_full_function_name() noexcept -> std::string_view
	{
		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		return {__FUNCSIG__, sizeof(__FUNCSIG__)};
		#else
		return {__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__)};
		#endif
	}

	// const class std::basic_string_view<char, struct std::char_traits<char>> `__calling_convention` `namespace`::get_full_function_name< `[struct]` `dummy_struct_do_not_put_into_any_namespace` >( `[void]` )
	constexpr std::string_view full_function_name      = get_full_function_name<dummy_struct_do_not_put_into_any_namespace>();
	constexpr auto             full_function_name_size = full_function_name.size();

	// `[struct]` `dummy_struct_do_not_put_into_any_namespace`
	constexpr std::string_view dummy_struct_name =
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			"struct "
			#endif
			"dummy_struct_do_not_put_into_any_namespace";
	constexpr auto dummy_struct_name_size = std::ranges::size(dummy_struct_name);

	// const class std::basic_string_view<char, struct std::char_traits<char>> `__calling_convention` `namespace`::get_full_function_name<
	constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_struct_name);
	static_assert(full_function_name_prefix_size != std::string_view::npos);

	// >( `[void]` )
	constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_struct_name_size;

	export
	{
		template<typename T>
		[[nodiscard]] constexpr auto type_name() noexcept -> std::string_view
		{
			auto full_name = get_full_function_name<std::decay_t<T>>();
			full_name.remove_prefix(full_function_name_prefix_size);
			full_name.remove_suffix(full_function_name_suffix_size);
			return full_name;
		}
	}
}
