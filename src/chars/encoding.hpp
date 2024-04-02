// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:encoding;

import std;
import gal.prometheus.meta;
import gal.prometheus.functional;

#else
#include <prometheus/macro.hpp>
#include <meta/meta.hpp>
#include <functional/functional.hpp>
#endif

namespace gal::prometheus::chars
{
	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	enum class EncodingType
	{
		UNKNOWN = 0b0000'0000,

		// BOM 0xef 0xbb 0xbf
		UTF8 = 0b0000'0001,
		// BOM 0xff 0xfe
		UTF16_LE = 0b0000'0010,
		// BOM 0xfe 0xff
		UTF16_BE = 0b0000'0100,
		// BOM 0xff 0xfe 0x00 0x00
		UTF32_LE = 0b0000'1000,
		// BOM 0x00 0x00 0xfe 0xff
		UTF32_BE = 0b0001'0000,
	};

	[[nodiscard]] constexpr auto size_of(const EncodingType type) noexcept -> std::size_t
	{
		switch (type)
		{
			case EncodingType::UNKNOWN: { return 0; }
			case EncodingType::UTF8: { return 3; }
			case EncodingType::UTF16_LE: { return 2; }
			case EncodingType::UTF16_BE: { return 2; }
			case EncodingType::UTF32_LE: { return 4; }
			case EncodingType::UTF32_BE: { return 4; }
			default: { GAL_PROMETHEUS_DEBUG_UNREACHABLE(); }
		}
	}

	class BomChecker
	{
	public:
		// [[nodiscard]] constexpr static auto operator()(const std::span<const char8_t> byte) noexcept -> EncodingType
		[[nodiscard]] constexpr static auto check(const std::span<const char8_t> byte) noexcept -> EncodingType
		{
			constexpr std::span bom_utf8{u8"\xef\xbb\xbf", size_of(EncodingType::UTF8)};
			constexpr std::span bom_utf16_le{u8"\xff\xfe", size_of(EncodingType::UTF16_LE)};
			constexpr std::span bom_utf16_be{u8"\xfe\xff", size_of(EncodingType::UTF16_BE)};
			constexpr std::span bom_utf32_le{u8"\xff\xfe\x00\x00", size_of(EncodingType::UTF32_LE)};
			constexpr std::span bom_utf32_be{u8"\x00\x00\xfe\xff", size_of(EncodingType::UTF32_BE)};

			if (const auto length = byte.size();
				length >= functional::functor::max(bom_utf8.size(), bom_utf32_le.size(), bom_utf32_be.size()))
			{
				if (std::ranges::equal(bom_utf32_le.begin(), bom_utf32_le.end(), byte.begin(), byte.begin() + bom_utf32_le.size())) { return EncodingType::UTF32_LE; }
				if (std::ranges::equal(bom_utf32_be.begin(), bom_utf32_be.end(), byte.begin(), byte.begin() + bom_utf32_be.size())) { return EncodingType::UTF32_BE; }
				if (std::ranges::equal(bom_utf8.begin(), bom_utf8.end(), byte.begin(), byte.begin() + bom_utf8.size())) { return EncodingType::UTF8; }
			}
			else if (length >= functional::functor::max(bom_utf16_le.size(), bom_utf16_be.size()))
			{
				if (std::ranges::equal(bom_utf16_le.begin(), bom_utf16_le.end(), byte.begin(), byte.begin() + bom_utf16_le.size())) { return EncodingType::UTF16_LE; }
				if (std::ranges::equal(bom_utf16_be.begin(), bom_utf16_be.end(), byte.begin(), byte.begin() + bom_utf16_be.size())) { return EncodingType::UTF16_BE; }
			}

			return EncodingType::UNKNOWN;
		}

		// [[nodiscard]] constexpr static auto operator()(const std::span<const char> byte) noexcept -> EncodingType
		[[nodiscard]] constexpr static auto check(const std::span<const char> byte) noexcept -> EncodingType
		{
			const auto data = std::span{GAL_PROMETHEUS_UNRESTRICTED_CHAR_POINTER_CAST(char8_t, byte.data()), byte.size()};
			// return operator()(data);
			return check(data);
		}
	};

	enum class ErrorCode
	{
		NONE = 0,

		// The decoded character must be not in U+D800...DFFF (UTF-8 or UTF-32) OR
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
		ErrorCode error;
		// In case of error, indicates the position of the error.
		// In case of success, indicates the number of code units validated/written.
		std::size_t count;

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

	GAL_PROMETHEUS_MODULE_EXPORT_END

	namespace encoding_detail
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
	}

	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	template<CharsCategory Type>
	using input_type = typename encoding_detail::input_type<Type>::type;

	template<CharsCategory Type>
	using output_type = typename encoding_detail::output_type<Type>::type;

	template<typename InputType>
	constexpr auto buffer_category = encoding_detail::buffer_category<InputType>::value;

	template<meta::basic_fixed_string Category>
	class Scalar;

	template<meta::basic_fixed_string Category>
	class Simd;

	template<meta::basic_fixed_string Category>
	class Converter;

	GAL_PROMETHEUS_MODULE_EXPORT_END
}
