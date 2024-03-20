#pragma once

#include <string>

struct dummy_struct_do_not_put_into_any_namespace {};

enum class DummyEnumDoNotPutIntoAnyNamespace
{
	DO_NOT_USE
};

namespace gal::prometheus::meta
{
	namespace name
	{
		template<typename>
		[[nodiscard]] constexpr auto get_full_function_name() noexcept -> std::string_view
		{
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			return {__FUNCSIG__, sizeof(__FUNCSIG__)};
			#else
			return {__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__)};
			#endif
		}

		template<auto>
		[[nodiscard]] constexpr auto get_full_function_name() noexcept -> std::string_view
		{
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			return {__FUNCSIG__, sizeof(__FUNCSIG__)};
			#else
			return {__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__)};
			#endif
		}
	}

	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	template<typename T>
	[[nodiscard]] constexpr auto name_of_type() noexcept -> std::string_view
	{
		// const class std::basic_string_view<char, struct std::char_traits<char>> `__calling_convention` `namespace`::get_full_function_name< `[struct]` `dummy_struct_do_not_put_into_any_namespace` >( `[void]` )
		constexpr std::string_view full_function_name      = name::get_full_function_name<dummy_struct_do_not_put_into_any_namespace>();
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
		// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<`DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE`>( `[void]` ) noexcept
		constexpr std::string_view full_function_name      = name::get_full_function_name<DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE>();
		constexpr auto             full_function_name_size = full_function_name.size();

		constexpr std::string_view dummy_enum_value_name      = "DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE";
		constexpr auto             dummy_enum_value_name_size = std::ranges::size(dummy_enum_value_name);

		// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<
		constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_enum_value_name);
		static_assert(full_function_name_prefix_size != std::string_view::npos);

		// >( `[void]` ) noexcept
		constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_enum_value_name_size;

		auto full_name = name::get_full_function_name<EnumValue>();
		full_name.remove_prefix(full_function_name_prefix_size);
		full_name.remove_suffix(full_function_name_suffix_size);
		return full_name;
	}

	GAL_PROMETHEUS_MODULE_EXPORT_END
}// namespace gal::prometheus::meta
