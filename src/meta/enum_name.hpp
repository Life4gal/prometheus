// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.meta:enum_name;

import std;
import :name;

#else
#include <optional>
#include <concepts>
#include <limits>

#include <prometheus/macro.hpp>
#include <meta/name.hpp>
#endif

namespace gal::prometheus::meta
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace user_defined
	{
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
		else { return meta::name_of_type<EnumType>(); }
	}

	template<auto EnumValue, typename EnumType = std::decay_t<decltype(EnumValue)>>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto name_of() noexcept -> std::string_view
	{
		if constexpr (requires { { user_defined::enum_value_name<EnumValue>::value } -> std::convertible_to<std::string_view>; })
		{
			return user_defined::enum_name<EnumType>::value;
		}
		else { return meta::name_of_enum_value<EnumValue>(); }
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace enum_name_detail
	{
		// template<std::unsigned_integral auto Value, std::size_t CurrentShift = 0, typename ValueType = std::decay_t<decltype(Value)>>
		// 	requires(Value > 0) // 1 << 0 => 1
		// [[nodiscard]] constexpr auto lower_bound_shift() noexcept -> std::size_t
		// {
		// 	if constexpr ((static_cast<ValueType>(1) << CurrentShift) >= Value) { return CurrentShift; }
		// 	else { return lower_bound_shift<Value, CurrentShift + 1>(); }
		// }
		template<std::unsigned_integral auto Value>
			requires(Value > 0) // 1 << 0 => 1
		[[nodiscard]] constexpr auto lower_bound_shift() noexcept -> std::size_t { return std::bit_width(Value) - 1; }

		// template<std::unsigned_integral auto Value, std::size_t CurrentShift = std::numeric_limits<std::decay_t<decltype(Value)>>::digits - 1>
		// 	requires(Value > 0) // 1 << 0 => 1
		// [[nodiscard]] constexpr auto upper_bound_shift() noexcept -> std::size_t
		// {
		// 	// if constexpr (Value == 0) { return 0; }
		// 	if constexpr ((Value >> CurrentShift) == 0) { return upper_bound_shift<Value, CurrentShift - 1>(); }
		// 	else { return CurrentShift + 1; }
		// }
		template<std::unsigned_integral auto Value>
			requires(Value > 0) // 1 << 0 => 1
		[[nodiscard]] constexpr auto upper_bound_shift() noexcept -> std::size_t { return std::bit_width(Value); }

		template<auto EnumValue, typename EnumType = std::decay_t<decltype(EnumValue)>>
			requires std::is_enum_v<EnumType>
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

		// ================================
		// list
		// ================================

		template<
			typename EnumType,
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
								meta::name_of<static_cast<EnumType>(Min + Index)>()
						}... //
				}};
			}(std::make_index_sequence<Max - Min + 1>{});
		}

		template<
			typename EnumType,
			std::integral auto Min,
			std::integral auto Max,
			typename MinType = std::decay_t<decltype(Min)>,
			typename MaxType =std::decay_t<decltype(Max)>>
			requires //
			std::is_enum_v<EnumType> and // enum
			std::is_same_v<MinType, MaxType> and // same type
			(std::numeric_limits<MinType>::digits <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits) // valid value
		constexpr auto names_from_value = generate_names_from_value<EnumType, Min, Max>();

		template<typename EnumType, std::size_t MinShift, std::size_t MaxShift>
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
								meta::name_of<static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(1) << (MinShift + Index))>()
						}... //
				}};
			}(std::make_index_sequence<MaxShift - MinShift + 1>{});
		}

		template<typename EnumType, std::size_t MinShift, std::size_t MaxShift>
			requires std::is_enum_v<EnumType> and (MaxShift <= std::numeric_limits<std::underlying_type_t<EnumType>>::digits - 1)
		constexpr auto names_from_shift = generate_names_from_shift<EnumType, MinShift, MaxShift>();

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

	template<typename EnumType>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto names_of() noexcept -> auto
	{
		// fixme: auto deducting category
		constexpr auto is_flag = enum_name_detail::range_is_flag<EnumType>::value;
		constexpr auto category = is_flag ? enum_name_detail::EnumCategory::FLAG : enum_name_detail::EnumCategory::ENUM;

		constexpr auto min = enum_name_detail::get_range_min<EnumType, category>();
		constexpr auto max = enum_name_detail::get_range_max<EnumType, category>();
		static_assert(max > min);

		if constexpr (is_flag)
		{
			constexpr auto begin_shift = enum_name_detail::begin_enum_shift_from_value<EnumType, min, max>();
			constexpr auto end_shift = enum_name_detail::begin_enum_shift_from_value<EnumType, (static_cast<decltype(max)>(1) << begin_shift), max>();

			return enum_name_detail::names_from_shift<EnumType, begin_shift, end_shift>;
		}
		else
		{
			constexpr auto begin_value = enum_name_detail::begin_enum_value_from_value<EnumType, min, max, enum_name_detail::EnumCategory::ENUM>();
			constexpr auto end_value =
					enum_name_detail::end_enum_value_from_value<
						EnumType,
						static_cast<decltype(min)>(begin_value),
						max,
						category
					>();

			return enum_name_detail::names_from_value<EnumType, begin_value, end_value>;
		}
	}

	template<typename EnumType>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto name_of(const EnumType enum_value) noexcept -> std::string_view
	{
		constexpr auto list = names_of<EnumType>();

		if (const auto it = std::ranges::find(list, enum_value, [](const auto& pair) noexcept -> EnumType { return pair.first; });
			it != std::ranges::end(list)) { return it->second; }
		return enum_name_not_found;
	}

	template<typename EnumType>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto name_of(const std::integral auto enum_value) noexcept -> std::string_view
	{
		constexpr auto list = names_of<EnumType>();

		if (const auto it = std::ranges::find(list, enum_value, [](const auto& pair) noexcept -> auto { return std::to_underlying(pair.first); });
			it != std::ranges::end(list)) { return it->second; }
		return enum_name_not_found;
	}

	template<typename EnumType>
		requires std::is_enum_v<EnumType>
	[[nodiscard]] constexpr auto value_of(const std::string_view enum_name) noexcept -> std::optional<EnumType>
	{
		constexpr auto list = names_of<EnumType>();

		if (const auto it = std::ranges::find(list, enum_name, [](const auto& pair) noexcept -> std::string_view { return pair.second; });
			it != std::ranges::end(list)) { return it->first; }
		return std::nullopt;
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
