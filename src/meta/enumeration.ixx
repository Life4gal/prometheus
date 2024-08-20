// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.meta:enumeration;

import std;

#else
#pragma once

#include <string_view>
#include <source_location>
#include <optional>
#include <limits>
#include <type_traits>
#include <vector>
#include <array>

#include <prometheus/macro.hpp>

#endif

enum class DummyEnumDoNotPutIntoAnyNamespace
{
	DO_NOT_USE
};

namespace gal::prometheus::meta
{
	namespace enumeration_detail
	{
		// ReSharper disable once CppTemplateParameterNeverUsed
		template<auto... Vs> // DO NOT REMOVE `Vs`
		[[nodiscard]] constexpr auto get_full_function_name() noexcept -> std::string_view { return std::source_location::current().function_name(); }

		template<auto EnumValue>
			requires std::is_enum_v<std::decay_t<decltype(EnumValue)>>
		[[nodiscard]] constexpr auto name_of() noexcept -> std::string_view
		{
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			// MSVC
			// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<`DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE`>(void) noexcept
			constexpr std::string_view full_function_name = get_full_function_name<DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE>();
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
			constexpr std::string_view full_function_name = get_full_function_name<DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE>();
			constexpr auto full_function_name_size = full_function_name.size();

			constexpr std::string_view dummy_struct_name{"DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE"};
			constexpr auto dummy_struct_name_size = std::ranges::size(dummy_struct_name);

			// std::string_view `namespace`::get_full_function_name() [V =
			constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_struct_name);
			static_assert(full_function_name_prefix_size != std::string_view::npos);

			// ]
			constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_struct_name_size;
			#else
			// GCC
			// constexpr std::string_view `namespace`::get_full_function_name() [with auto <anonymous> = `DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE`; std::string_view = std::basic_string_view<char>]
			constexpr std::string_view full_function_name = get_full_function_name<DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE>();
			constexpr auto full_function_name_size = full_function_name.size();

			constexpr std::string_view dummy_struct_name{"DummyEnumDoNotPutIntoAnyNamespace::DO_NOT_USE"};
			constexpr auto dummy_struct_name_size = std::ranges::size(dummy_struct_name);

			// std::string_view `namespace`::get_full_function_name() [V =
			constexpr auto full_function_name_prefix_size = full_function_name.find(dummy_struct_name);
			static_assert(full_function_name_prefix_size != std::string_view::npos);

			// ; std::string_view = std::basic_string_view<char>]
			constexpr auto full_function_name_suffix_size = full_function_name_size - full_function_name_prefix_size - dummy_struct_name_size;
			#endif

			auto full_name = get_full_function_name<EnumValue>();
			full_name.remove_prefix(full_function_name_prefix_size);
			full_name.remove_suffix(full_function_name_suffix_size);
			return full_name;
		}

		template<auto EnumValue>
			requires std::is_enum_v<std::decay_t<decltype(EnumValue)>>
		[[nodiscard]] constexpr auto is_valid_enum() noexcept -> bool
		{
			// fixme: check it
			// (enum MyEnum)0x1
			return not name_of<EnumValue>().starts_with('(');
		}

		template<typename EnumType, std::integral auto EnumValue>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto is_valid_enum() noexcept -> bool //
		{
			return is_valid_enum<static_cast<EnumType>(EnumValue)>();
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto default_min_value() noexcept -> std::underlying_type_t<EnumType>
		{
			using value_type = std::underlying_type_t<EnumType>;

			if constexpr (std::is_signed_v<value_type>)
			{
				return static_cast<value_type>(-128);
			}
			else
			{
				return static_cast<value_type>(0);
			}
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto default_max_value() noexcept -> std::underlying_type_t<EnumType>
		{
			using value_type = std::underlying_type_t<EnumType>;

			if constexpr (std::is_signed_v<value_type>)
			{
				return static_cast<value_type>(127);
			}
			else
			{
				return static_cast<value_type>(255);
			}
		}
	} // namespace enumeration_detail

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	enum class EnumNamePolicy
	{
		// namespace_A::namespace_B::namespace_C::enum_name::Value // scoped enum
		// namespace_A::namespace_B::namespace_C::Value
		FULL,
		// enum_name::Value // scoped enum
		// Value
		WITH_SCOPED_NAME,
		// Value
		VALUE_ONLY,
	};

	namespace user_defined
	{
		/**
		 * template<>
		 * struct enum_name_policy<MyEnum>
		 * {
		 *		constexpr static auto value = EnumNamePolicy::VALUE_ONLY;
		 * };
		 */
		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		struct enum_name_policy
		{
			constexpr static auto value = EnumNamePolicy::FULL;
		};

		/**
		 * template<>
		 * struct enum_range<MyEnum>
		 * {
		 *		constexpr static auto min = 0;
		 *		constexpr static auto max = 65535;
		 * };
		 */
		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		struct enum_range
		{
			constexpr static auto min = enumeration_detail::default_min_value<EnumType>();
			constexpr static auto max = enumeration_detail::default_max_value<EnumType>();
		};

		/**
		 * template<>
		 * struct enum_range<MyEnum> : std::true_type
		 * {
		 * };
		 */
		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		struct enum_is_flag {};

		/**
		 * template<>
		 * struct enum_name<MyEnum>
		 * {
		 *		constexpr static std::string_view value{"MY-ENUM"};
		 * };
		 */
		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		struct enum_name {};

		/**
		 * template<>
		 * struct enum_name<MyEnum::VALUE>
		 * {
		 *		constexpr static std::string_view value{"MY-ENUM-VALUE"};
		 * };
		 */
		template<auto EnumValue>
			requires std::is_enum_v<std::decay_t<decltype(EnumValue)>>
		struct enum_value_name {};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace enumeration_detail
	{
		template<typename EnumType, std::size_t Count, std::size_t Index>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto flag_get_bits_combination() noexcept -> std::underlying_type_t<EnumType>
		{
			using type = std::underlying_type_t<EnumType>;

			constexpr auto index_sequence = std::views::iota(static_cast<type>(0), static_cast<type>(std::numeric_limits<type>::digits));
			constexpr auto index_chunk = index_sequence | std::views::chunk(Count);

			constexpr auto indices = index_chunk[Index];
			constexpr auto bits = std::ranges::fold_left(indices, static_cast<type>(0), [](const auto total, const auto current) noexcept -> auto { return total | (1 << current); });

			return bits;
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto flag_dynamic_enum_values() noexcept -> std::vector<EnumType>
		{
			constexpr auto digits = std::numeric_limits<std::underlying_type_t<EnumType>>::digits;

			std::vector<EnumType> values{};

			// zero
			if constexpr (enumeration_detail::is_valid_enum<EnumType, 0>())
			{
				values.push_back(static_cast<EnumType>(0));
			}

			[&values]<std::size_t... Count>(std::index_sequence<Count...>) noexcept -> void
			{
				const auto traversal = [&values]<std::size_t ThisCount, std::size_t... Index>(std::index_sequence<Index...>) noexcept -> void
				{
					const auto check = [&values]<std::size_t ThisIndex>() noexcept -> void
					{
						if constexpr (constexpr auto bits = flag_get_bits_combination<EnumType, ThisCount, ThisIndex>();
							enumeration_detail::is_valid_enum<EnumType, bits>())
						{
							values.push_back(static_cast<EnumType>(bits));
						}
					};

					(check.template operator()<Index>(), ...);
				};

				constexpr auto this_range = []<std::size_t ThisCount>() noexcept -> std::size_t
				{
					return digits / ThisCount + (digits % ThisCount == 0 ? 0 : 1);
				};

				(traversal.template operator()<Count + 1>(std::make_index_sequence<this_range.template operator()<Count + 1>()>{}), ...);
			}(std::make_index_sequence<digits>{});

			return values;
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		struct cached_flag_dynamic_enum_values_size : std::integral_constant<std::size_t, flag_dynamic_enum_values<EnumType>().size()> {};

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto is_flag_with_user_defined() noexcept -> bool
		{
			if constexpr (requires { user_defined::enum_is_flag<EnumType>::value; })
			{
				return user_defined::enum_is_flag<EnumType>::value;
			}
			else
			{
				// We require at least 6 values (1 << 6 => 64) to be considered for inferring a flag.
				return cached_flag_dynamic_enum_values_size<EnumType>::value >= 6;
			}
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto enum_name_with_user_defined() noexcept -> std::string_view
		{
			if constexpr (requires { { user_defined::enum_name<EnumType>::value } -> std::convertible_to<std::string_view>; })
			{
				return user_defined::enum_name<EnumType>::value;
			}
			else { return meta::name_of<EnumType>(); }
		}

		template<auto EnumValue>
			requires std::is_enum_v<std::decay_t<decltype(EnumValue)>>
		[[nodiscard]] constexpr auto enum_value_name_with_user_defined() noexcept -> std::string_view
		{
			if constexpr (requires { { user_defined::enum_value_name<EnumValue>::value } -> std::convertible_to<std::string_view>; })
			{
				return user_defined::enum_value_name<EnumValue>::value;
			}
			else { return enumeration_detail::name_of<EnumValue>(); }
		}

		template<
			typename EnumType,
			std::underlying_type_t<EnumType> Min = user_defined::enum_range<EnumType>::min,
			std::underlying_type_t<EnumType> Max = user_defined::enum_range<EnumType>::max,
			std::size_t StrideShift = 5,
			bool Found = false
		>
		[[nodiscard]] constexpr auto enum_value_min() noexcept -> std::underlying_type_t<EnumType>
		{
			if constexpr (Min > Max)
			{
				return enum_value_min<
					EnumType,
					user_defined::enum_range<EnumType>::min,
					user_defined::enum_range<EnumType>::max,
					StrideShift - 1,
					Found
				>();
			}
			else
			{
				if constexpr (is_valid_enum<EnumType, Min>())
				{
					if constexpr (StrideShift == 0)
					{
						return Min;
					}
					else
					{
						return enum_value_min<EnumType, Min - (1 << StrideShift), Min, StrideShift, true>();
					}
				}
				else
				{
					if constexpr (Found)
					{
						if constexpr (StrideShift == 0)
						{
							return enum_value_min<EnumType, Max, Max, 0, true>();
						}
						else
						{
							constexpr auto s = StrideShift - 1;
							return enum_value_min<EnumType, Max - (1 << s), Max, s, true>();
						}
					}
					else
					{
						return enum_value_min<EnumType, Min + (1 << StrideShift), Max, StrideShift, false>();
					}
				}
			}
		}

		template<
			typename EnumType,
			std::underlying_type_t<EnumType> Min = user_defined::enum_range<EnumType>::min,
			std::underlying_type_t<EnumType> Max = user_defined::enum_range<EnumType>::max,
			std::size_t StrideShift = 5,
			bool Found = false
		>
		[[nodiscard]] constexpr auto enum_value_max() noexcept -> std::underlying_type_t<EnumType>
		{
			if constexpr (Min > Max)
			{
				return enum_value_max<
					EnumType,
					user_defined::enum_range<EnumType>::min,
					user_defined::enum_range<EnumType>::max,
					StrideShift - 1,
					Found
				>();
			}
			else
			{
				if constexpr (is_valid_enum<EnumType, Max>())
				{
					if constexpr (StrideShift == 0)
					{
						return Max;
					}
					else
					{
						return enum_value_max<EnumType, Max, Max + (1 << StrideShift), StrideShift, true>();
					}
				}
				else
				{
					if constexpr (Found)
					{
						if constexpr (StrideShift == 0)
						{
							return enum_value_max<EnumType, Min, Min, 0, true>();
						}
						else
						{
							constexpr auto s = StrideShift - 1;
							return enum_value_max<EnumType, Min, Min + (1 << s), s, true>();
						}
					}
					else
					{
						return enum_value_max<EnumType, Min, Max - (1 << StrideShift), StrideShift, false>();
					}
				}
			}
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		struct cached_enum_value_min : std::integral_constant<std::underlying_type_t<EnumType>, enum_value_min<EnumType>()> {};

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		struct cached_enum_value_max : std::integral_constant<std::underlying_type_t<EnumType>, enum_value_max<EnumType>()> {};

		template<typename EnumType, EnumNamePolicy Policy>
		[[nodiscard]] constexpr auto trim_full_name(const std::string_view name) noexcept -> std::string_view
		{
			if constexpr (Policy == EnumNamePolicy::FULL) { return name; }
			else if constexpr (Policy == EnumNamePolicy::WITH_SCOPED_NAME)
			{
				if constexpr (std::is_scoped_enum_v<EnumType>)
				{
					const auto last_double_colon = name.rfind("::");
					const auto optional_extra_double_colon = name.rfind("::", last_double_colon - 1);

					if (optional_extra_double_colon == std::string_view::npos)
					[[unlikely]]
					{
						// global namespace
						return name;
					}

					// namespace_A::namespace_B::...
					return name.substr(optional_extra_double_colon + 2);
				}
				else
				{
					return trim_full_name<EnumType, EnumNamePolicy::VALUE_ONLY>(name);
				}
			}
			else if constexpr (Policy == EnumNamePolicy::VALUE_ONLY)
			{
				const auto last_double_colon = name.rfind("::");

				if constexpr (std::is_scoped_enum_v<EnumType>)
				{
					GAL_PROMETHEUS_ERROR_ASSUME(last_double_colon != std::string_view::npos);

					return name.substr(last_double_colon + 2);
				}
				else
				{
					if (last_double_colon == std::string_view::npos)
					{
						// global namespace
						return name;
					}

					return name.substr(last_double_colon + 2);
				}
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<typename EnumType, EnumNamePolicy Policy>
			requires (std::is_enum_v<EnumType> and is_flag_with_user_defined<EnumType>())
		[[nodiscard]] constexpr auto generate_flag_names() noexcept -> auto
		{
			constexpr auto size = cached_flag_dynamic_enum_values_size<EnumType>::value;
			using return_type = std::array<std::pair<EnumType, std::string_view>, size>;

			return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> return_type
			{
				return //
						return_type{
								//
								typename return_type::value_type{
										//
										flag_dynamic_enum_values<EnumType>()[Index],
										enumeration_detail::trim_full_name<EnumType, Policy>(
											enumeration_detail::name_of<static_cast<EnumType>(flag_dynamic_enum_values<EnumType>()[Index])>()
										)
								}...
						};
			}(std::make_index_sequence<size>{});
		}

		template<typename EnumType, EnumNamePolicy Policy>
			requires(std::is_enum_v<EnumType> and is_flag_with_user_defined<EnumType>())
		constexpr auto names_of_flag = generate_flag_names<EnumType, Policy>();

		template<typename EnumType, EnumNamePolicy Policy>
			requires (std::is_enum_v<EnumType>)
		[[nodiscard]] constexpr auto generate_enum_names() noexcept ->
			std::array<std::pair<EnumType, std::string_view>, cached_enum_value_max<EnumType>::value - cached_enum_value_min<EnumType>::value + 1>
		{
			constexpr auto min = cached_enum_value_min<EnumType>::value;
			constexpr auto max = cached_enum_value_max<EnumType>::value;
			constexpr auto size = max - min + 1;
			using return_type = std::array<std::pair<EnumType, std::string_view>, size>;

			return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> return_type
			{
				return //
				{
						//
						typename return_type::value_type{
								static_cast<EnumType>(min + Index),
								enumeration_detail::trim_full_name<EnumType, Policy>(
									enumeration_detail::name_of<static_cast<EnumType>(min + Index)>()
								)
						}...
				};
			}(std::make_index_sequence<size>{});
		}

		template<typename EnumType, EnumNamePolicy Policy>
			requires(std::is_enum_v<EnumType>)
		constexpr auto names_of_enum = generate_enum_names<EnumType, Policy>();
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	template<typename EnumType>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto min_value_of() noexcept -> std::underlying_type_t<EnumType>
	{
		if constexpr (enumeration_detail::is_flag_with_user_defined<EnumType>())
		{
			return static_cast<std::underlying_type_t<EnumType>>(
				enumeration_detail::names_of_flag<EnumType, user_defined::enum_name_policy<EnumType>::value>.front().first
			);
		}
		else
		{
			return static_cast<std::underlying_type_t<EnumType>>(
				enumeration_detail::names_of_enum<EnumType, user_defined::enum_name_policy<EnumType>::value>.front().first
			);
		}
	}

	template<typename EnumType>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto max_value_of() noexcept -> std::underlying_type_t<EnumType>
	{
		if constexpr (enumeration_detail::is_flag_with_user_defined<EnumType>())
		{
			return static_cast<std::underlying_type_t<EnumType>>(
				enumeration_detail::names_of_flag<EnumType, user_defined::enum_name_policy<EnumType>::value>.back().first
			);
		}
		else
		{
			return static_cast<std::underlying_type_t<EnumType>>(
				enumeration_detail::names_of_enum<EnumType, user_defined::enum_name_policy<EnumType>::value>.back().first
			);
		}
	}

	// enum class MyEnum
	// {
	//		E1,
	//		E2,
	//		E3,
	// }
	//
	// names_of<MyEnum> => { "E1", "E2", "E3" }
	//
	// enum class MyFlag
	// {
	//		F1 = 0x0001,
	//		F2 = 0x0010,
	//		F3 = 0x0100,
	//		F4 = 0x1000,
	//		F5 = F1 | F2,
	// }
	//
	// names_of<MyFlag> => { "F1", "F2", "F3", "F4", "F5" }

	template<typename EnumType, EnumNamePolicy Policy>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto names_of() noexcept -> auto
	{
		if constexpr (enumeration_detail::is_flag_with_user_defined<EnumType>())
		{
			return enumeration_detail::names_of_flag<EnumType, Policy>;
		}
		else
		{
			return enumeration_detail::names_of_enum<EnumType, Policy>;
		}
	}

	template<typename EnumType>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto names_of() noexcept -> auto //
	{
		return names_of<EnumType, user_defined::enum_name_policy<EnumType>::value>();
	}

	constexpr std::string_view enum_name_not_found{"?"};

	// enum class MyEnum
	// {
	//		E1 = 0,
	//		E2 = 1,
	//		E3 = 2,
	// }
	//
	// name_of(MyEnum::E1) => "E1"
	// name_of(MyEnum::E2) => "E2"
	// name_of(static_cast<MyEnum>(42)) => "?" (enum_name_not_found)
	//
	// name_of<MyEnum>(0) => "E1"
	// name_of<MyEnum>(1) => "E2"
	// name_of<MyEnum>(42) => "?" (enum_name_not_found)
	//
	// enum class MyFlag
	// {
	//		F1 = 0x0001,
	//		F2 = 0x0010,
	//		F3 = 0x0100,
	//		F4 = 0x1000,
	//		F5 = F1 | F2,
	// }
	//
	// name_of(MyFlag::F1) => "F1"
	// name_of(MyFlag::F2) => "F2"
	// name_of(MyFlag::F5) => "F5"
	// name_of(static_cast<MyFlag>(0x1'0000)) => "?" (enum_name_not_found)
	//
	// name_of<MyFlag>(0x0001) => "F1"
	// name_of<MyFlag>(0x0010) => "F2"
	// name_of<MyFlag>(0x0001 | 0x0010) => "F5"
	// name_of<MyFlag>(0x1'0000) => "?" (enum_name_not_found)

	template<EnumNamePolicy Policy, typename EnumType>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto name_of(const EnumType enum_value) noexcept -> std::string_view
	{
		constexpr auto list = names_of<EnumType, Policy>();

		if (const auto it = std::ranges::find(list, enum_value, [](const auto& pair) noexcept -> EnumType { return pair.first; });
			it != std::ranges::end(list)) { return it->second; }
		return enum_name_not_found;
	}

	template<typename EnumType>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto name_of(const EnumType enum_value) noexcept -> std::string_view //
	{
		return meta::name_of<user_defined::enum_name_policy<EnumType>::value, EnumType>(enum_value);
	}

	template<typename EnumType, EnumNamePolicy Policy = user_defined::enum_name_policy<EnumType>::value>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto name_of(const std::integral auto enum_value) noexcept -> std::string_view
	{
		return meta::name_of<Policy, EnumType>(static_cast<EnumType>(enum_value));
	}

	// enum class MyFlag
	// {
	//		F1 = 0x0001,
	//		F2 = 0x0010,
	//		F3 = 0x0100,
	//		F4 = 0x1000,
	//		F5 = F1 | F2,
	// }
	//
	// full_name_of(MyFlag::F1) => "F1"
	// full_name_of(MyFlag::F2) => "F2"
	// full_name_of(MyFlag::F5) => "F1|F2"
	// full_name_of(MyFlag::F3 | MyFlag::F4) => "F3|F4"
	// full_name_of(static_cast<MyFlag>(0x1'0000)) => "?" (enum_name_not_found)
	//
	// name_of<MyFlag>(0x0001) => "F1"
	// name_of<MyFlag>(0x0010) => "F2"
	// name_of<MyFlag>(0x0001 | 0x0010) => "F1|F2"
	// name_of<MyFlag>(0x0100 | 0x1000) => "F3|F4"
	// name_of<MyFlag>(0x1'0000) => "?" (enum_name_not_found)

	template<EnumNamePolicy Policy, typename Allocator = std::allocator<char>, typename EnumType>
		requires (std::is_enum_v<EnumType> and enumeration_detail::is_flag_with_user_defined<EnumType>())
	[[nodiscard]] constexpr auto full_name_of(
		const EnumType enum_value,
		const std::string_view split = "|",
		const Allocator& allocator = {}
	) noexcept -> std::basic_string<char, std::char_traits<char>, Allocator>
	{
		const auto value = std::to_underlying(enum_value);
		const auto unsigned_value = static_cast<std::make_unsigned_t<std::underlying_type_t<EnumType>>>(value);

		std::basic_string<char, std::char_traits<char>, Allocator> result{allocator};

		if (std::popcount(unsigned_value) == 1)
		{
			const auto i = std::countr_zero(unsigned_value);
			result.append_range(name_of<EnumType, Policy>(1 << i));
			return result;
		}

		for (std::size_t i = 0; i < std::numeric_limits<std::underlying_type_t<EnumType>>::digits; ++i)
		{
			if (value & (1 << i))
			{
				result.append_range(name_of<EnumType, Policy>(1 << i));
				result.append_range(split);
			}
		}

		if (not result.empty())
		{
			result.erase(result.size() - split.size(), split.size());
		}

		return result;
	}

	template<typename Allocator = std::allocator<char>, typename EnumType>
		requires (std::is_enum_v<EnumType> and enumeration_detail::is_flag_with_user_defined<EnumType>())
	[[nodiscard]] constexpr auto full_name_of(
		const EnumType enum_value,
		const std::string_view split = "|",
		const Allocator& allocator = {}
	) noexcept -> std::basic_string<char, std::char_traits<char>, Allocator>
	{
		return meta::full_name_of<user_defined::enum_name_policy<EnumType>::value, Allocator, EnumType>(enum_value, split, allocator);
	}

	template<typename EnumType, EnumNamePolicy Policy = user_defined::enum_name_policy<EnumType>::value, typename Allocator = std::allocator<char>>
		requires (std::is_enum_v<EnumType> and enumeration_detail::is_flag_with_user_defined<EnumType>())
	[[nodiscard]] constexpr auto full_name_of(
		const std::integral auto enum_value,
		const std::string_view split = "|",
		const Allocator& allocator = {}
	) noexcept -> std::basic_string<char, std::char_traits<char>, Allocator>
	{
		return meta::full_name_of<Policy, Allocator, EnumType>(static_cast<EnumType>(enum_value), split, allocator);
	}

	// enum class MyEnum
	// {
	//		E1 = 0,
	//		E2 = 1,
	//		E3 = 2,
	// }
	//
	// value_of("E1") => E1
	// value_of("E2") => E2
	// value_of("E1|E2") => E1|E2
	// value_of("E1|E4") => null[Strict] / E1
	// value_of("?") => null
	//
	// enum class MyFlag
	// {
	//		F1 = 0x0001,
	//		F2 = 0x0010,
	//		F3 = 0x0100,
	//		F4 = 0x1000,
	//		F5 = F1 | F2,
	// }
	//
	// value_of("F1") => F1
	// value_of("F2") => F2
	// value_of("F1|F2") => F1|F2
	// value_of("F1|F0") => null[Strict] / F1
	// value_of("F5") => F5
	// value_of("?") => null

	template<typename EnumType, EnumNamePolicy Policy = user_defined::enum_name_policy<EnumType>::value, bool Strict = true>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto value_of(
		const std::string_view enum_name,
		const std::string_view split = "|"
	) noexcept -> std::optional<EnumType>
	{
		constexpr auto list = names_of<EnumType, Policy>();

		std::underlying_type_t<EnumType> result{};

		for (const auto& each: enum_name | std::views::split(split))
		{
			const std::string_view s{each};

			if (const auto it = std::ranges::find(list | std::views::values, s);
				it != std::ranges::end(list | std::views::values))
			{
				result |= std::to_underlying(it.base()->first);
			}
			else
			{
				if constexpr (Strict)
				{
					return std::nullopt;
				}
			}
		}

		return static_cast<EnumType>(result);
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
