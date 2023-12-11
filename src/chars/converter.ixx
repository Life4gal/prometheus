// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

export module gal.prometheus.chars:converter;

import std;
import gal.prometheus.string;

import :encoding;

namespace gal::prometheus::chars
{
	export
	{
		enum class ErrorCode
		{
			NONE = 0,

			// The decoded character must be not be in U+D800...DFFF (UTF-8 or UTF-32) OR
			// a high surrogate must be followed by a low surrogate and a low surrogate must be preceded by a high surrogate (UTF-16) OR
			// there must be no surrogate at all (Latin1)
			SURROGATE,

			// The leading byte must be followed by N-1 continuation bytes, where N is the UTF-8 character length.
			// This is also the error when the input is truncated.
			TOO_SHORT,
			// We either have too many consecutive continuation bytes or the string starts with a continuation byte.
			TOO_LONG,
			// The decoded character must be above U+7F for two-byte characters, U+7FF for three-byte characters, and U+FFFF for four-byte characters.
			OVERLONG,
			// The decoded character must be less than or equal to U+10FFFF, less than or equal than U+7F for ASCII OR less than equal than U+FF for Latin1
			TOO_LARGE,
			// Any byte must have fewer than 5 header bits.
			HEADER_BITS,
		};

		struct result_type
		{
			ErrorCode						 error;
			// In case of error, indicates the position of the error.
			// In case of success, indicates the number of code units validated/written.
			std::size_t						 count;

			[[nodiscard]] constexpr explicit operator bool() const noexcept { return error == ErrorCode::NONE; }
		};

		enum class CharsCategory
		{
			ASCII,

			UTF8,

			UTF16_LE,
			UTF16_BE,
			UTF16,

			UTF32,
		};

		enum class InputProcessCriterion
		{
			ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT,
			RETURN_RESULT_TYPE,
			ASSUME_VALID_INPUT,
		};
	}

	namespace converter_detail
	{
		template<CharsCategory Type>
		struct input_type;

		template<CharsCategory Type>
		struct output_type;

		template<typename InputType>
		struct buffer_category;

		template<>
		struct input_type<CharsCategory::ASCII>
		{
			using type = std::span<const char>;
		};

		template<>
		struct output_type<CharsCategory::ASCII>
		{
			using type = std::span<char>;
		};

		template<>
		struct buffer_category<input_type<CharsCategory::ASCII>::type>
		{
			constexpr static auto value = CharsCategory::ASCII;
		};

		template<>
		struct buffer_category<input_type<CharsCategory::ASCII>::type::value_type>
		{
			constexpr static auto value = CharsCategory::ASCII;
		};

		template<>
		struct input_type<CharsCategory::UTF8>
		{
			using type = std::span<const char8_t>;
		};

		template<>
		struct output_type<CharsCategory::UTF8>
		{
			using type = std::span<char8_t>;
		};

		template<>
		struct buffer_category<input_type<CharsCategory::UTF8>::type>
		{
			constexpr static auto value = CharsCategory::UTF8;
		};

		template<>
		struct buffer_category<input_type<CharsCategory::UTF8>::type::value_type>
		{
			constexpr static auto value = CharsCategory::UTF8;
		};

		template<>
		struct input_type<CharsCategory::UTF16_LE>
		{
			using type = std::span<const char16_t>;
		};

		template<>
		struct output_type<CharsCategory::UTF16_LE>
		{
			using type = std::span<char16_t>;
		};

		template<>
		struct input_type<CharsCategory::UTF16_BE>
		{
			using type = std::span<const char16_t>;
		};

		template<>
		struct output_type<CharsCategory::UTF16_BE>
		{
			using type = std::span<char16_t>;
		};

		template<>
		struct input_type<CharsCategory::UTF16>
		{
			using type = std::span<const char16_t>;
		};

		template<>
		struct output_type<CharsCategory::UTF16>
		{
			using type = std::span<char16_t>;
		};

		template<>
		struct buffer_category<input_type<CharsCategory::UTF16>::type>
		{
			constexpr static auto value = CharsCategory::UTF16;
		};

		template<>
		struct buffer_category<input_type<CharsCategory::UTF16>::type::value_type>
		{
			constexpr static auto value = CharsCategory::UTF16;
		};

		template<>
		struct input_type<CharsCategory::UTF32>
		{
			using type = std::span<const char32_t>;
		};

		template<>
		struct output_type<CharsCategory::UTF32>
		{
			using type = std::span<char32_t>;
		};

		template<>
		struct buffer_category<input_type<CharsCategory::UTF32>::type>
		{
			constexpr static auto value = CharsCategory::UTF32;
		};

		template<>
		struct buffer_category<input_type<CharsCategory::UTF32>::type::value_type>
		{
			constexpr static auto value = CharsCategory::UTF32;
		};
	}// namespace converter_detail

	export
	{
		template<CharsCategory Type>
		using input_type = typename converter_detail::input_type<Type>::type;

		template<CharsCategory Type>
		using output_type = typename converter_detail::output_type<Type>::type;

		template<typename InputType>
		constexpr auto buffer_category = converter_detail::buffer_category<InputType>::value;

		template<string::basic_fixed_string Category>
		class Converter;

		template<string::basic_fixed_string Category>
		class Simd;

		template<string::basic_fixed_string Category>
		class Scalar;
	}
}// namespace gal::prometheus::chars
