#pragma once

#include <string>

struct dummy_struct_do_not_put_into_any_namespace {};

namespace gal::prometheus::meta
{
	namespace type_name
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
	}

	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	template<typename T>
	[[nodiscard]] constexpr auto nameof() noexcept -> std::string_view
	{
		auto full_name = type_name::get_full_function_name<std::decay_t<T>>();
		full_name.remove_prefix(type_name::full_function_name_prefix_size);
		full_name.remove_suffix(type_name::full_function_name_suffix_size);
		return full_name;
	}

	GAL_PROMETHEUS_MODULE_EXPORT_END
}
