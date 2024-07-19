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
#include <bit>
#include <optional>
#include <limits>

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
	} // namespace name_detail

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
		enum class NamePolicy
		{
			// namespace_A::namespace_B::namespace_C::enum_name::Value // scoped enum
			// namespace_A::namespace_B::namespace_C::Value
			FULL,
			// this has the same effect as ENUM_VALUE_ONLY if it is an un-scoped enumeration
			// enum_name::Value
			WITH_ENUM_NAME,
			// Value
			ENUM_VALUE_ONLY,
		};

		namespace user_defined
		{
			/**
			 * template<>
			 * struct enum_name_policy<MyEnum>
			 * {
			 *		constexpr static auto value = NamePolicy::ENUM_VALUE_ONLY;
			 * };
			 */
			template<typename EnumType>
				requires std::is_enum_v<EnumType>
			struct enum_name_policy
			{
				constexpr static auto value = NamePolicy::FULL;
			};

			/**
			 * template<>
			 * struct enum_range<MyEnum>
			 * {
			 *		constexpr static enum_range_size_type min = 0;
			 *		constexpr static enum_range_size_type max = 65535;
			 * };
			 */
			template<typename EnumType>
				requires std::is_enum_v<EnumType>
			struct enum_range
			{
				using value_type = std::underlying_type_t<EnumType>;

				constexpr static auto min_max = []() noexcept
				{
					if constexpr (std::is_signed_v<value_type>) { return std::pair<value_type, value_type>{-128, 127}; }
					else { return std::pair<value_type, value_type>{0, 255}; }
				}();

				constexpr static value_type min = min_max.first;
				constexpr static value_type max = min_max.second;
			};

			/**
			 * template<>
			 * struct enum_range<MyEnum> : std::true_type
			 * {
			 * };
			 */
			template<typename>
			struct enum_is_flag : std::false_type {};

			/**
			 * template<>
			 * struct enum_name<MyEnum>
			 * {
			 *		constexpr static std::string_view value{"MY-ENUM"};
			 * };
			 */
			template<typename>
			struct enum_name {};

			/**
			 * template<>
			 * struct enum_name<MyEnum::VALUE>
			 * {
			 *		constexpr static std::string_view value{"MY-ENUM-VALUE"};
			 * };
			 */
			template<auto>
			struct enum_value_name {};
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto name_of() noexcept -> std::string_view
		{
			if constexpr (requires { { user_defined::enum_name<EnumType>::value } -> std::convertible_to<std::string_view>; })
			{
				return user_defined::enum_name<EnumType>::value;
			}
			else { return meta::name_of<EnumType>(); }
		}

		template<auto EnumValue>
			requires std::is_enum_v<std::decay_t<decltype(EnumValue)>>
		[[nodiscard]] constexpr auto name_of() noexcept -> std::string_view
		{
			if constexpr (requires { { user_defined::enum_value_name<EnumValue>::value } -> std::convertible_to<std::string_view>; })
			{
				return user_defined::enum_name<std::decay_t<decltype(EnumValue)>>::value;
			}
			else { return enumeration_detail::name_of<EnumValue>(); }
		}
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace enumeration_detail
	{
		template<std::unsigned_integral auto Value>
			requires(Value > 0) // 1 << 0 => 1
		[[nodiscard]] constexpr auto lower_bound_shift() noexcept -> std::size_t { return std::bit_width(Value) - 1; }

		template<std::unsigned_integral auto Value>
			requires(Value > 0) // 1 << 0 => 1
		[[nodiscard]] constexpr auto upper_bound_shift() noexcept -> std::size_t { return std::bit_width(Value); }

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

		/**
		 * Category:
		 *	-> Enum => [Min, Max] ------ [Min] -> [Min + n] -> [Max]
		 *	-> Flag => [1 << Min, 1 << Max] ------ [1 << Min] -> [1 << n] -> [1 << Max]
		 */
		enum class EnumCategory
		{
			ENUM,
			FLAG,
		};

		template<typename EnumType, EnumCategory Category, std::integral auto Min, std::integral auto Max>
		struct maybe_valid_flag;

		template<typename EnumType, std::integral auto Min, std::integral auto Max>
		struct maybe_valid_flag<EnumType, EnumCategory::ENUM, Min, Max> : std::true_type {};

		template<typename EnumType, std::integral auto Min, std::integral auto Max>
		struct maybe_valid_flag<EnumType, EnumCategory::FLAG, Min, Max> : std::bool_constant< //
					(std::is_unsigned_v<std::decay_t<decltype(Min)>> and std::is_unsigned_v<std::decay_t<decltype(Max)>>) and // valid flag
					(Max <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits - 1) // valid Max
				> {};

		/**
		 * @brief Starting at Min and ending at Max, all values in between are tested to see if they are valid enumeration values (this always assumes that the enumeration values are consecutive).
		 * @tparam EnumType the type of the enumeration.
		 * @tparam Min the start value/shift of the enumeration.
		 * @tparam Max the end value/shift of the enumeration.
		 * @return true iff all values in between are valid enumeration.
		 */
		template<
			typename EnumType,
			std::integral auto Min,
			std::integral auto Max,
			EnumCategory Category,
			typename MinType = std::decay_t<decltype(Min)>,
			typename MaxType = std::decay_t<decltype(Max)>>
			requires //
			std::is_enum_v<EnumType> and // enum
			std::is_same_v<MinType, MaxType> and // same type
			(std::numeric_limits<MinType>::digits <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits) and // valid value
			maybe_valid_flag<EnumType, Category, Min, Max>::value
		[[nodiscard]] constexpr auto is_valid_enum() noexcept -> bool //
		{
			using underlying_type = std::underlying_type_t<MinType>;

			if constexpr (Min > Max) { return false; }

			if constexpr (Category == EnumCategory::ENUM)
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool //
				{
					return (is_valid_enum<static_cast<MinType>(Min + Index)>() and ...);
				}(std::make_index_sequence<Max - Min>{});
			}
			else if constexpr (Category == EnumCategory::FLAG)
			{
				constexpr auto lower = lower_bound_shift<Min>();
				constexpr auto upper = upper_bound_shift<Max>();

				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool //
				{
					return (is_valid_enum<static_cast<MinType>(static_cast<underlying_type>(1) << (lower + Index))>() and ...);
				}(std::make_index_sequence<upper - lower>{});
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<typename EnumType>
		constexpr auto valid_enum_value_not_found = std::numeric_limits<std::underlying_type_t<EnumType>>::max();
		template<std::integral ShiftType>
		constexpr auto valid_enum_shift_not_found = std::numeric_limits<ShiftType>::max();

		// ================================
		// enum class MyEnum
		// {
		//		ONE,
		//		TWO,
		//		THREE,
		// };
		// => [MyEnum::ONE, MyEnum::THREE]
		//
		// enum class MyEnum
		// {
		//		ONE = 0x0000'0001,
		//		TWO = 0x0000'0010,
		//		THREE = 0x0000'0100,
		// };
		// => [MyEnum::ONE, MyEnum::THREE]
		// ================================

		/**
		 * @brief Starting at MinShift and ending at MaxShift, find an enumeration of type EnumType whose value is equal to (1 << n).
		 * @tparam EnumType type of the enumeration.
		 * @tparam MinShift the start of the shift, usually obtained from lower_bound_shift.
		 * @tparam MaxShift the end of the shift, usually obtained from upper_bound_shift.
		 * @return (1 << n) if found, else valid_enum_value_not_found.
		 */
		template<typename EnumType, std::size_t MinShift, std::size_t MaxShift>
			requires std::is_enum_v<EnumType> and (MaxShift <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits - 1)
		[[nodiscard]] constexpr auto begin_enum_value_from_shift() noexcept -> std::underlying_type_t<EnumType>
		{
			using underlying_type = std::underlying_type_t<EnumType>;

			if constexpr (MinShift > MaxShift) { return valid_enum_value_not_found<EnumType>; }

			if constexpr (constexpr auto current = static_cast<underlying_type>(1) << MinShift;
				is_valid_enum<EnumType, current>()) { return current; }
			else { return begin_enum_value_from_shift<EnumType, MinShift + 1, MaxShift>(); }
		}

		/**
		 * @brief Starting at Min and ending at Max, find the first possible valid enumeration value.
		 * @tparam EnumType the type of the enumeration.
		 * @tparam Min the start value of the enumeration.
		 * @tparam Max the end value of the enumeration.
		 * @return the first possible valid enumeration value if found, else valid_enum_value_not_found.
		 */
		template<
			typename EnumType,
			std::integral auto Min,
			std::integral auto Max,
			EnumCategory Category,
			typename MinType = std::decay_t<decltype(Min)>,
			typename MaxType = std::decay_t<decltype(Max)>>
			requires //
			std::is_enum_v<EnumType> and // enum
			std::is_same_v<MinType, MaxType> and // same type
			(std::numeric_limits<MinType>::digits <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits) and // valid value
			maybe_valid_flag<EnumType, Category, Min, Max>::value
		[[nodiscard]] constexpr auto begin_enum_value_from_value() noexcept -> std::underlying_type_t<EnumType> //
		{
			using underlying_type = std::underlying_type_t<EnumType>;

			constexpr auto min = static_cast<underlying_type>(Min);
			constexpr auto max = static_cast<underlying_type>(Max);

			if constexpr (min > max) { return valid_enum_value_not_found<EnumType>; }

			if constexpr (Category == EnumCategory::ENUM)
			{
				if constexpr (is_valid_enum<EnumType, min>()) { return min; }
				else { return begin_enum_value_from_value<EnumType, min + 1, max, Category>(); }
			}
			else if constexpr (Category == EnumCategory::FLAG)
			{
				constexpr auto lower = lower_bound_shift<min>();
				constexpr auto upper = upper_bound_shift<max>();

				return begin_enum_value_from_shift<EnumType, lower, upper>();
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		/**
		 * @brief Starting at MaxShift and ending at MinShift, find an enumeration of type EnumType whose value is equal to (1 << n).
		 * @tparam EnumType type of the enumeration.
		 * @tparam MinShift the start of the shift, usually obtained from lower_bound_shift.
		 * @tparam MaxShift the end of the shift, usually obtained from upper_bound_shift.
		 * @return (1 << n) if found, else valid_enum_value_not_found.
		 */
		template<typename EnumType, std::size_t MinShift, std::size_t MaxShift>
			requires std::is_enum_v<EnumType> and (MaxShift <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits - 1)
		[[nodiscard]] constexpr auto end_enum_value_from_shift() noexcept -> std::underlying_type_t<EnumType>
		{
			using underlying_type = std::underlying_type_t<EnumType>;

			if constexpr (MinShift > MaxShift) { return valid_enum_value_not_found<EnumType>; }

			if constexpr (constexpr auto current = static_cast<underlying_type>(1) << MaxShift;
				is_valid_enum<EnumType, current>()) { return current; }
			else { return end_enum_value_from_shift<EnumType, MinShift, MaxShift - 1>(); }
		}

		/**
		 * @brief Starting at Max and ending at Min, find the first possible valid enumeration value.
		 * @tparam EnumType the type of the enumeration.
		 * @tparam Min the start value of the enumeration.
		 * @tparam Max the end value of the enumeration.
		 * @return the first possible valid enumeration value if found, else valid_enum_value_not_found.
		 */
		template<
			typename EnumType,
			std::integral auto Min,
			std::integral auto Max,
			EnumCategory Category,
			typename MinType = std::decay_t<decltype(Min)>,
			typename MaxType = std::decay_t<decltype(Max)>>
			requires //
			std::is_enum_v<EnumType> and // enum
			std::is_same_v<MinType, MaxType> and // same type
			(std::numeric_limits<MinType>::digits <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits) and // valid value
			maybe_valid_flag<EnumType, Category, Min, Max>::value
		[[nodiscard]] constexpr auto end_enum_value_from_value() noexcept -> std::underlying_type_t<EnumType> //
		{
			using underlying_type = std::underlying_type_t<EnumType>;

			constexpr auto min = static_cast<underlying_type>(Min);
			constexpr auto max = static_cast<underlying_type>(Max);

			if constexpr (min > max) { return valid_enum_value_not_found<EnumType>; }

			if constexpr (Category == EnumCategory::ENUM)
			{
				if constexpr (is_valid_enum<EnumType, max>()) { return max; }
				else { return end_enum_value_from_value<EnumType, min, max - 1, Category>(); }
			}
			else if constexpr (Category == EnumCategory::FLAG)
			{
				constexpr auto lower = lower_bound_shift<min>();
				constexpr auto upper = upper_bound_shift<max>();

				return end_enum_value_from_shift<EnumType, lower, upper>();
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		// ================================
		// enum class MyEnum
		// {
		//		ONE = 0x0000'0001,
		//		TWO = 0x0000'0010,
		//		THREE = 0x0000'0100,
		// };
		// => [0, 3]
		// (1 << 0) <= MyEnum::ONE
		// (1 << 3) <= MyEnum::THREE
		// ================================

		/**
		 * @brief Starting at MinShift and ending at MaxShift, find an enumeration of type EnumType whose value is equal to (1 << n).
		 * @tparam EnumType the type of the enumeration.
		 * @tparam MinShift the start of the shift, usually obtained from lower_bound_shift.
		 * @tparam MaxShift the end of the shift, usually obtained from upper_bound_shift.
		 * @return n if found, else valid_enum_shift_not_found.
		 */
		template<typename EnumType, std::size_t MinShift, std::size_t MaxShift>
			requires std::is_enum_v<EnumType> and (MaxShift <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits - 1)
		[[nodiscard]] constexpr auto begin_enum_shift_from_shift() noexcept -> std::size_t
		{
			using underlying_type = std::underlying_type_t<EnumType>;

			if constexpr (MinShift > MaxShift) { return valid_enum_shift_not_found<std::size_t>; }

			if constexpr (constexpr auto current = static_cast<underlying_type>(1) << MinShift;
				is_valid_enum<EnumType, current>()) { return MinShift; }
			else { return begin_enum_shift_from_shift<EnumType, MinShift + 1, MaxShift>(); }
		}

		/**
		 * @brief Starting at Min and ending at Max, find an enumeration of type EnumType whose value is equal to (1 << n).
		 * @tparam EnumType the type of the enumeration.
		 * @tparam Min the start of the enumeration (unsigned integer).
		 * @tparam Max the end of the enumeration (unsigned integer).
		 * @return n if found, else valid_enum_shift_not_found.
		 */
		template<
			typename EnumType,
			std::unsigned_integral auto Min,
			std::unsigned_integral auto Max,
			typename MinType = std::decay_t<decltype(Min)>,
			typename MaxType = std::decay_t<decltype(Max)>>
			requires //
			std::is_enum_v<EnumType> and // enum
			std::is_same_v<MinType, MaxType> and // same type
			(std::numeric_limits<MinType>::digits <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits) // valid value
		[[nodiscard]] constexpr auto begin_enum_shift_from_value() noexcept -> std::size_t //
		{
			using underlying_type = std::underlying_type_t<EnumType>;

			constexpr auto min = static_cast<underlying_type>(Min);
			constexpr auto max = static_cast<underlying_type>(Max);

			if constexpr (min > max) { return valid_enum_shift_not_found<std::size_t>; }

			constexpr auto lower = lower_bound_shift<min>();
			constexpr auto upper = upper_bound_shift<max>();

			return begin_enum_shift_from_shift<EnumType, lower, upper>();
		}

		/**
		 * @brief Starting at MaxShift and ending at MinShift, find an enumeration of type EnumType whose value is equal to (1 << n).
		 * @tparam EnumType the type of the enumeration.
		 * @tparam MinShift the start of the shift, usually obtained from lower_bound_shift.
		 * @tparam MaxShift the end of the shift, usually obtained from upper_bound_shift.
		 * @return n if found, else valid_enum_shift_not_found.
		 */
		template<typename EnumType, std::size_t MinShift, std::size_t MaxShift>
			requires std::is_enum_v<EnumType> and (MaxShift <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits - 1)
		[[nodiscard]] constexpr auto end_enum_shift_from_shift() noexcept -> std::size_t
		{
			using underlying_type = std::underlying_type_t<EnumType>;

			if constexpr (MinShift > MaxShift) { return valid_enum_shift_not_found<std::size_t>; }

			if constexpr (constexpr auto current = static_cast<underlying_type>(1) << MaxShift;
				is_valid_enum<EnumType, current>()) { return MaxShift; }
			else { return end_enum_value_from_shift<EnumType, MinShift, MaxShift - 1>(); }
		}

		/**
		 * @brief Starting at Max and ending at Min, find an enumeration of type EnumType whose value is equal to (1 << n).
		 * @tparam EnumType the type of the enumeration.
		 * @tparam Min the start of the enumeration (unsigned integer).
		 * @tparam Max the end of the enumeration (unsigned integer).
		 * @return n if found, else valid_enum_shift_not_found.
		 */
		template<
			typename EnumType,
			std::unsigned_integral auto Min,
			std::unsigned_integral auto Max,
			typename MinType = std::decay_t<decltype(Min)>,
			typename MaxType = std::decay_t<decltype(Max)>>
			requires //
			std::is_enum_v<EnumType> and // enum
			std::is_same_v<MinType, MaxType> and // same type
			(std::numeric_limits<MinType>::digits <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits) // valid value
		[[nodiscard]] constexpr auto end_enum_shift_from_value() noexcept -> std::underlying_type_t<EnumType> //
		{
			using underlying_type = std::underlying_type_t<MinType>;

			constexpr auto min = static_cast<underlying_type>(Min);
			constexpr auto max = static_cast<underlying_type>(Max);

			if constexpr (min > max) { return valid_enum_shift_not_found<std::size_t>; }

			constexpr auto lower = lower_bound_shift<min>();
			constexpr auto upper = upper_bound_shift<max>();

			return end_enum_shift_from_shift<EnumType, lower, upper>();
		}

		template<typename EnumType, NamePolicy Policy>
		[[nodiscard]] constexpr auto to_name(const std::string_view name) noexcept -> std::string_view
		{
			if constexpr (Policy == NamePolicy::FULL) { return name; }
			else if constexpr (Policy == NamePolicy::WITH_ENUM_NAME)
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
				else { return to_name<EnumType, NamePolicy::ENUM_VALUE_ONLY>(name); }
			}
			else if constexpr (Policy == NamePolicy::ENUM_VALUE_ONLY)
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

		// ================================
		// list
		// ================================

		template<
			typename EnumType,
			NamePolicy Policy,
			std::integral auto Min,
			std::integral auto Max,
			typename MinType = std::decay_t<decltype(Min)>,
			typename MaxType =std::decay_t<decltype(Max)>>
			requires //
			std::is_enum_v<EnumType> and // enum
			std::is_same_v<MinType, MaxType> and // same type
			(std::numeric_limits<MinType>::digits <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits) // valid value
		[[nodiscard]] constexpr auto generate_names_from_value() noexcept -> std::array<std::pair<EnumType, std::string_view>, Max - Min + 1>
		{
			return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept //
			{
				return std::array<std::pair<EnumType, std::string_view>, Max - Min + 1>{{
						//
						{
								//
								static_cast<EnumType>(Min + Index),
								to_name<EnumType, Policy>(meta::name_of<static_cast<EnumType>(Min + Index)>())
						}... //
				}};
			}(std::make_index_sequence<Max - Min + 1>{});
		}

		template<
			typename EnumType,
			NamePolicy Policy,
			std::integral auto Min,
			std::integral auto Max,
			typename MinType = std::decay_t<decltype(Min)>,
			typename MaxType =std::decay_t<decltype(Max)>>
			requires //
			std::is_enum_v<EnumType> and // enum
			std::is_same_v<MinType, MaxType> and // same type
			(std::numeric_limits<MinType>::digits <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits) // valid value
		constexpr auto names_from_value = generate_names_from_value<EnumType, Policy, Min, Max>();

		template<typename EnumType, NamePolicy Policy, std::size_t MinShift, std::size_t MaxShift>
			requires std::is_enum_v<EnumType> and (MaxShift <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits - 1)
		[[nodiscard]] constexpr auto generate_names_from_shift()
			noexcept -> std::array<std::pair<EnumType, std::string_view>, MaxShift - MinShift + 1>
		{
			return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept //
			{
				return std::array<std::pair<EnumType, std::string_view>, MaxShift - MinShift + 1>{{
						//
						{
								//
								static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(1) << (MinShift + Index)),
								to_name<EnumType, Policy>(
										meta::name_of<static_cast<EnumType>(
											static_cast<std::underlying_type_t<EnumType>>(1) << (MinShift + Index))>())
						}... //
				}};
			}(std::make_index_sequence<MaxShift - MinShift + 1>{});
		}

		template<typename EnumType, NamePolicy Policy, std::size_t MinShift, std::size_t MaxShift>
			requires std::is_enum_v<EnumType> and (MaxShift <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits - 1)
		constexpr auto names_from_shift = generate_names_from_shift<EnumType, Policy, MinShift, MaxShift>();

		// ================================
		// traits
		// ================================

		template<typename EnumType>
		struct range_min :
				std::integral_constant<decltype(user_defined::enum_range<EnumType>::min), user_defined::enum_range<EnumType>::min> {};

		template<typename EnumType>
		struct range_max :
				std::integral_constant<decltype(user_defined::enum_range<EnumType>::max), user_defined::enum_range<EnumType>::max> {};

		template<typename T>
		struct range_is_flag : user_defined::enum_is_flag<T> {};

		template<typename EnumType, EnumCategory Category, typename Underlying = std::underlying_type_t<EnumType>>
		[[nodiscard]] constexpr auto get_range_min() noexcept -> Underlying
		{
			if constexpr (Category == EnumCategory::ENUM)
			{
				// constexpr auto bound_0 = range_min<EnumType>::value;
				// constexpr auto bound_1 = std::numeric_limits<Underlying>::min();
				//
				// if constexpr (std::cmp_less(bound_0, bound_1)) { return bound_1; }
				// else { return bound_0; }
				return range_min<EnumType>::value;
			}
			else { return static_cast<Underlying>(0); }
		}

		template<typename EnumType, EnumCategory Category, typename Underlying = std::underlying_type_t<EnumType>>
		[[nodiscard]] constexpr auto get_range_max() noexcept -> Underlying
		{
			if constexpr (Category == EnumCategory::ENUM)
			{
				// constexpr auto bound_0 = range_max<EnumType>::value;
				// constexpr auto bound_1 = std::numeric_limits<Underlying>::max();
				//
				// if constexpr (std::cmp_less(bound_0, bound_1)) { return bound_1; }
				// else { return bound_0; }
				return range_max<EnumType>::value;
			}
			else { return std::numeric_limits<Underlying>::digits - 1; }
		}
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
		constexpr std::string_view enum_name_not_found{"?"};

		template<typename EnumType, NamePolicy Policy>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto names_of() noexcept -> auto
		{
			// fixme: auto deducting category
			constexpr auto is_flag = enumeration_detail::range_is_flag<EnumType>::value;
			constexpr auto category = is_flag ? enumeration_detail::EnumCategory::FLAG : enumeration_detail::EnumCategory::ENUM;

			constexpr auto min = enumeration_detail::get_range_min<EnumType, category>();
			constexpr auto max = enumeration_detail::get_range_max<EnumType, category>();
			static_assert(max > min);

			if constexpr (is_flag)
			{
				constexpr auto begin_shift =
						enumeration_detail::begin_enum_shift_from_value<EnumType, min, max>();
				constexpr auto end_shift =
						enumeration_detail::begin_enum_shift_from_value<EnumType, (static_cast<decltype(max)>(1) << begin_shift), max>();

				return enumeration_detail::names_from_shift<EnumType, Policy, begin_shift, end_shift>;
			}
			else
			{
				constexpr auto begin_value =
						enumeration_detail::begin_enum_value_from_value<EnumType, min, max, enumeration_detail::EnumCategory::ENUM>();
				constexpr auto end_value =
						enumeration_detail::end_enum_value_from_value<
							EnumType,
							static_cast<decltype(min)>(begin_value),
							max,
							category
						>();

				return enumeration_detail::names_from_value<EnumType, Policy, begin_value, end_value>;
			}
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto names_of() noexcept -> auto //
		{
			return names_of<EnumType, user_defined::enum_name_policy<EnumType>::value>();
		}

		template<NamePolicy Policy, typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto name_of(const EnumType enum_value) noexcept -> std::string_view
		{
			constexpr static auto list = names_of<EnumType, Policy>();

			if (const auto it = std::ranges::find(list, enum_value, [](const auto& pair) noexcept -> EnumType { return pair.first; });
				it != std::ranges::end(list)) { return it->second; }
			return enum_name_not_found;
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto name_of(const EnumType enum_value) noexcept -> std::string_view //
		{
			return name_of<user_defined::enum_name_policy<EnumType>::value, EnumType>(enum_value);
		}

		template<typename EnumType, NamePolicy Policy>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto name_of(const std::integral auto enum_value) noexcept -> std::string_view
		{
			constexpr static auto list = names_of<EnumType, Policy>();

			if (const auto it = std::ranges::find(list, enum_value, [](const auto& pair) noexcept -> auto { return std::to_underlying(pair.first); });
				it != std::ranges::end(list)) { return it->second; }
			return enum_name_not_found;
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto name_of(const std::integral auto enum_value) noexcept -> std::string_view //
		{
			return name_of<EnumType, user_defined::enum_name_policy<EnumType>::value>(enum_value);
		}

		template<typename EnumType, NamePolicy Policy>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto value_of(const std::string_view enum_name) noexcept -> std::optional<EnumType>
		{
			constexpr static auto list = names_of<EnumType, Policy>();

			if (const auto it = std::ranges::find(list, enum_name, [](const auto& pair) noexcept -> std::string_view { return pair.second; });
				it != std::ranges::end(list)) { return it->first; }
			return std::nullopt;
		}

		template<typename EnumType>
			requires std::is_enum_v<EnumType>
		[[nodiscard]] constexpr auto value_of(const std::string_view enum_name) noexcept -> std::optional<EnumType> //
		{
			return value_of<EnumType, user_defined::enum_name_policy<EnumType>::value>(enum_name);
		}
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
