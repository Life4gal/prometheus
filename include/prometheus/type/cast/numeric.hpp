// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <prometheus/type/traits/numeric.hpp>
#include <prometheus/type/compare/numeric.hpp>
#include <prometheus/debug/exception.hpp>

namespace gal::prometheus::type::cast
{
	/**
	 * @brief Cast a number to a type that will be able to represent all values without loss of precision.
	 * @tparam Out The numeric type to cast to.
	 * @tparam In The numeric type to cast from.
	 * @param input The input value.
	 * @return The output value.
	 */
	template<traits::arithmetic Out, traits::arithmetic In>
		requires traits::type_in_range<Out, In>
	[[nodiscard]] constexpr auto wide_cast(const In input) noexcept -> Out { return static_cast<Out>(input); }

	/**
	 * @brief Cast a numeric value to an integer saturating on overflow.
	 * @tparam Out The signed-integer or unsigned-integer type to cast to.
	 * @tparam In The numeric type to cast from.
	 * @param input The input value.
	 * @return The converted value, which is saturated if @b input is overflowing or under-flowing.
	 */
	template<traits::integral Out, traits::arithmetic In>
	[[nodiscard]] constexpr auto saturate_cast(const In input) noexcept -> Out
	{
		if constexpr (std::is_floating_point_v<In>) { if (std::isnan(input)) { return Out{0}; } }

		if (compare::three_way_compare(input, std::numeric_limits<Out>::lowest()) != std::strong_ordering::greater) { return std::numeric_limits<Out>::lowest(); }

		if (compare::three_way_compare(input, std::numeric_limits<Out>::max()) != std::strong_ordering::less) { return std::numeric_limits<Out>::max(); }

		return static_cast<Out>(input);
	}

	/**
	 * @brief Cast numeric values without loss of precision.
	 * @tparam Out The numeric type to cast to.
	 * @tparam In The numeric type to cast from
	 * @param input The input value.
	 * @return The value cast to a different type without loss of precision.
	 *
	 * @note It is undefined behavior to cast a value which will cause a loss of precision.
	 */
	template<traits::arithmetic Out, traits::arithmetic In>
	[[nodiscard]] constexpr auto narrow_cast(const In input) noexcept -> Out
	{
		constexpr auto narrow_validate = [](const Out o, const In i) noexcept -> bool
		{
			// in-value and out-value compares the same, after converting out-value back to in-type.
			auto result = (i == static_cast<In>(o));

			// If the types have different signs we need to do an extra test to make sure the actual sign
			// of the values are the same as well.
			if constexpr (std::numeric_limits<Out>::is_signed != std::numeric_limits<In>::is_signed) { result &= (i < In{0}) == (o < Out{0}); }

			return result;
		};

		if constexpr (traits::type_in_range<Out, In>) { return static_cast<Out>(input); }
		else
		{
			const auto out = static_cast<Out>(input);
			GAL_PROMETHEUS_DEBUG_ASSUME(narrow_validate(out, input), "Invalid narrow cast!");
			return out;
		}
	}

	template<traits::arithmetic Out, traits::arithmetic In>
	[[nodiscard]] constexpr auto round_cast(const In input) noexcept -> Out
	{
		if constexpr (std::is_floating_point_v<In>) { return narrow_cast<Out>(std::round(input)); }
		else { return narrow_cast<Out>(input); }
	}

	template<traits::arithmetic Out, traits::arithmetic In>
	[[nodiscard]] constexpr auto floor_cast(const In input) noexcept -> Out
	{
		if constexpr (std::is_floating_point_v<In>) { return narrow_cast<Out>(std::floor(input)); }
		else { return narrow_cast<Out>(input); }
	}

	template<traits::arithmetic Out, traits::arithmetic In>
	[[nodiscard]] constexpr auto ceil_cast(const In input) noexcept -> Out
	{
		if constexpr (std::is_floating_point_v<In>) { return narrow_cast<Out>(std::ceil(input)); }
		else { return narrow_cast<Out>(input); }
	}

	/**
	 * @brief Cast a character.
	 * @tparam Out The value type of the result.
	 * @tparam In The value type of the character.
	 * @param input The value of the character.
	 * @return The cast value.
	 *
	 * @note Both the input and output types are interpreted as unsigned values, even if
	 * 	they are signed values. For example `char` may be either signed or unsigned,
	 * 	but you have to treat those as unsigned values.
	 * @note @b input value after casting, must fit in the output type.
	 */
	template<traits::integral Out, traits::integral In>
	[[nodiscard]] constexpr auto char_cast(const In input) noexcept -> Out
	{
		using unsigned_in_type = std::make_unsigned_t<In>;
		using unsigned_out_type = std::make_unsigned_t<Out>;

		// We cast to unsigned of the same type, so that we don't accidentally sign extent 'char'.
		auto unsigned_in  = static_cast<unsigned_in_type>(input);
		auto unsigned_out = narrow_cast<unsigned_out_type>(unsigned_in);
		return static_cast<Out>(unsigned_out);
	}

	template<traits::integral Out>
	[[nodiscard]] constexpr auto char_cast(const std::byte input) noexcept -> Out
	{
		static_assert(sizeof(std::byte) == sizeof(std::uint8_t));
		return char_cast<Out>(static_cast<std::uint8_t>(input));
	}

	/**
	 * @brief Return the low half of the input value.
	 */
	template<traits::unsigned_integral Out, traits::unsigned_integral In>
		requires(sizeof(Out) * 2 == sizeof(In))// Return value of low_bit_cast must be half the size of the input
	[[nodiscard]] constexpr auto low_bit_cast(const In input) noexcept -> Out { return static_cast<Out>(input); }

	/**
	 * @brief Return the high half of the input value.
	 */
	template<traits::unsigned_integral Out, traits::unsigned_integral In>
		requires(sizeof(Out) * 2 == sizeof(In))// Return value of high_bit_cast must be half the size of the input
	[[nodiscard]] constexpr auto high_bit_cast(const In input) noexcept -> Out { return static_cast<Out>(input >> sizeof(Out) * std::numeric_limits<unsigned char>::digits); }

	/**
	 * @brief Return the upper half of the input value.
	 */
	template<traits::unsigned_integral Out, traits::unsigned_integral In>
		requires(sizeof(Out) * 2 == sizeof(In))// Return value of merge_bit_cast must be half the size of the input
	[[nodiscard]] constexpr auto merge_bit_cast(const In high, const In low) noexcept -> Out
	{
		auto result = static_cast<Out>(high);
		result <<= sizeof(In) * std::numeric_limits<unsigned char>::digits;
		result |= static_cast<Out>(low);
		return result;
	}

	/**
	 * @brief Return the low half of the input value.
	 */
	template<traits::signed_integral Out, traits::signed_integral In>
		requires(sizeof(Out) * 2 == sizeof(In))// Return value of low_bit_cast must be half the size of the input
	[[nodiscard]] constexpr auto low_bit_cast(const In input) noexcept -> Out
	{
		using unsigned_in_type = std::make_unsigned_t<In>;
		using unsigned_out_type = std::make_unsigned_t<Out>;

		return static_cast<Out>(low_bit_cast<unsigned_out_type>(static_cast<unsigned_in_type>(input)));
	}

	/**
	 * @brief Return the high half of the input value.
	 */
	template<traits::signed_integral Out, traits::signed_integral In>
		requires(sizeof(Out) * 2 == sizeof(In))// Return value of high_bit_cast must be half the size of the input
	[[nodiscard]] constexpr auto high_bit_cast(const In input) noexcept -> Out
	{
		using unsigned_in_type = std::make_unsigned_t<In>;
		using unsigned_out_type = std::make_unsigned_t<Out>;

		return static_cast<Out>(high_bit_cast<unsigned_out_type>(static_cast<unsigned_in_type>(input)));
	}

	/**
	 * @brief Return the upper half of the input value.
	 */
	template<traits::signed_integral Out, traits::signed_integral In>
		requires(sizeof(Out) * 2 == sizeof(In))// Return value of merge_bit_cast must be half the size of the input
	[[nodiscard]] constexpr auto merge_bit_cast(const In high, const In low) noexcept -> Out
	{
		using unsigned_in_type = std::make_unsigned_t<In>;
		using unsigned_out_type = std::make_unsigned_t<Out>;

		return static_cast<Out>(merge_bit_cast<unsigned_out_type>(static_cast<unsigned_in_type>(high), static_cast<unsigned_in_type>(low)));
	}

	/**
	 * @brief Return the low half of the input value.
	 */
	template<traits::unsigned_integral Out, traits::signed_integral In>
		requires(sizeof(Out) * 2 == sizeof(In))// Return value of low_bit_cast must be half the size of the input
	[[nodiscard]] constexpr auto low_bit_cast(const In input) noexcept -> Out
	{
		using unsigned_in_type = std::make_unsigned_t<In>;

		return static_cast<Out>(low_bit_cast<Out>(static_cast<unsigned_in_type>(input)));
	}

	/**
	 * @brief Return the high half of the input value.
	 */
	template<traits::unsigned_integral Out, traits::signed_integral In>
		requires(sizeof(Out) * 2 == sizeof(In))// Return value of high_bit_cast must be half the size of the input
	[[nodiscard]] constexpr auto high_bit_cast(const In input) noexcept -> Out
	{
		using unsigned_in_type = std::make_unsigned_t<In>;

		return static_cast<Out>(high_bit_cast<Out>(static_cast<unsigned_in_type>(input)));
	}

	/**
	 * @brief Return the upper half of the input value.
	 */
	template<traits::signed_integral Out, traits::unsigned_integral In>
		requires(sizeof(Out) * 2 == sizeof(In))// Return value of merge_bit_cast must be half the size of the input
	[[nodiscard]] constexpr auto merge_bit_cast(const In high, const In low) noexcept -> Out
	{
		using unsigned_out_type = std::make_unsigned_t<Out>;

		return narrow_cast<Out>(merge_bit_cast<unsigned_out_type>(high, low));
	}

	/**
	 * @brief Cast an integral to an unsigned integral of the same size.
	 */
	template<traits::integral In>
	[[nodiscard]] constexpr auto to_unsigned(const In input) noexcept -> std::make_unsigned_t<In> { return static_cast<std::make_unsigned_t<In>>(input); }

	/**
	 * @briefc Cast an integral to an signed integral of the same size.
	 */
	template<traits::integral In>
	[[nodiscard]] constexpr auto to_signed(const In input) noexcept -> std::make_signed_t<In> { return static_cast<std::make_signed_t<In>>(input); }

	/**
	 * @brief Cast between integral types truncating or zero-extending the result.
	 */
	template<traits::integral Out, traits::integral In>
	[[nodiscard]] constexpr auto truncate(const In input) noexcept -> Out { return static_cast<Out>(to_unsigned(input)); }
}
