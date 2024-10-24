// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:chars.encoding;

import std;

import :meta;
#if GAL_PROMETHEUS_COMPILER_DEBUG
import :platform;
#endif

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <utility>
#include <span>

#include <prometheus/macro.hpp>
#include <meta/meta.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(chars)
{
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
		UTF32_BE,
	};

	[[nodiscard]] constexpr auto width_of(const EncodingType type) noexcept -> std::size_t
	{
		switch (type)
		{
			case EncodingType::UNKNOWN: { return 0; }
			case EncodingType::UTF8: { return 3; }
			case EncodingType::UTF16_LE:
			case EncodingType::UTF16_BE: { return 2; }
			case EncodingType::UTF32_LE:
			case EncodingType::UTF32_BE: { return 4; }
			default: { GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE(); } // NOLINT(clang-diagnostic-covered-switch-default)
		}
	}

	[[nodiscard]] constexpr auto bom_of(const std::span<const char8_t> string) noexcept -> EncodingType
	{
		// https://en.wikipedia.org/wiki/Byte_order_mark#Byte-order_marks_by_encoding

		const auto length = string.size();
		if (length < 2)
		{
			return EncodingType::UNKNOWN;
		}

		if (string[0] == 0xff and string[1] == 0xfe)
		{
			if (length >= 4 and string[2] == 0x00 and string[3] == 0x00)
			{
				return EncodingType::UTF32_LE;
			}
			return EncodingType::UTF16_LE;
		}

		if (string[0] == 0xfe and string[1] == 0xff)
		{
			return EncodingType::UTF16_BE;
		}

		if (length >= 4 and string[0] == 0x00 and string[1] == 0x00 and string[2] == 0xfe and string[3] == 0xff)
		{
			return EncodingType::UTF32_BE;
		}

		if (length >= 3 and string[0] == 0xef and string[1] == 0xbb and string[2] == 0xbf)
		{
			return EncodingType::UTF8;
		}

		return EncodingType::UNKNOWN;
	}

	[[nodiscard]] constexpr auto bom_of(const std::span<const char> string) noexcept -> EncodingType
	{
		static_assert(sizeof(char) == sizeof(char8_t));

		const auto* char8_string = GAL_PROMETHEUS_SEMANTIC_UNRESTRICTED_CHAR_POINTER_CAST(char8_t, string.data());
		return bom_of({char8_string, string.size()});
	}

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

		[[nodiscard]] constexpr auto has_error() const noexcept -> bool { return error != ErrorCode::NONE; }

		[[nodiscard]] constexpr explicit operator bool() const noexcept { return not has_error(); }
	};

	enum class CharsCategory
	{
		ASCII,

		UTF8_CHAR,
		UTF8,

		UTF16_LE,
		UTF16_BE,
		UTF16,

		UTF32,
	};

	template<CharsCategory Category>
	struct input_selector;

	template<CharsCategory Category>
	struct output_selector;

	// ======================================
	// ASCII
	// ======================================

	template<>
	struct input_selector<CharsCategory::ASCII>
	{
		using type = std::span<const char>;
		constexpr static auto value = CharsCategory::ASCII;
	};

	template<>
	struct output_selector<CharsCategory::ASCII>
	{
		using type = std::span<char>;
		constexpr static auto value = CharsCategory::ASCII;
	};

	// ======================================
	// UTF8_CHAR
	// ======================================

	template<>
	struct input_selector<CharsCategory::UTF8_CHAR>
	{
		using type = std::span<const char>;
		constexpr static auto value = CharsCategory::UTF8_CHAR;
	};

	template<>
	struct output_selector<CharsCategory::UTF8_CHAR>
	{
		using type = std::span<char>;
		constexpr static auto value = CharsCategory::UTF8_CHAR;
	};

	// ======================================
	// UTF8
	// ======================================

	template<>
	struct input_selector<CharsCategory::UTF8>
	{
		using type = std::span<const char8_t>;
		constexpr static auto value = CharsCategory::UTF8;
	};

	template<>
	struct output_selector<CharsCategory::UTF8>
	{
		using type = std::span<char8_t>;
		constexpr static auto value = CharsCategory::UTF8;
	};

	// ======================================
	// UTF16_LE
	// ======================================

	template<>
	struct input_selector<CharsCategory::UTF16_LE>
	{
		using type = std::span<const char16_t>;
		constexpr static auto value = CharsCategory::UTF16_LE;
	};

	template<>
	struct output_selector<CharsCategory::UTF16_LE>
	{
		using type = std::span<char16_t>;
		constexpr static auto value = CharsCategory::UTF16_LE;
	};

	// ======================================
	// UTF16_BE
	// ======================================

	template<>
	struct input_selector<CharsCategory::UTF16_BE>
	{
		using type = std::span<const char16_t>;
		constexpr static auto value = CharsCategory::UTF16_BE;
	};

	template<>
	struct output_selector<CharsCategory::UTF16_BE>
	{
		using type = std::span<char16_t>;
		constexpr static auto value = CharsCategory::UTF16_BE;
	};

	// ======================================
	// UTF16
	// ======================================

	template<>
	struct input_selector<CharsCategory::UTF16>
	{
		using type = std::span<const char16_t>;
		constexpr static auto value = CharsCategory::UTF16;
	};

	template<>
	struct output_selector<CharsCategory::UTF16>
	{
		using type = std::span<char16_t>;
		constexpr static auto value = CharsCategory::UTF16;
	};

	// ======================================
	// UTF32
	// ======================================

	template<>
	struct input_selector<CharsCategory::UTF32>
	{
		using type = std::span<const char32_t>;
		constexpr static auto value = CharsCategory::UTF32;
	};

	template<>
	struct output_selector<CharsCategory::UTF32>
	{
		using type = std::span<char32_t>;
		constexpr static auto value = CharsCategory::UTF32;
	};

	template<typename T>
	struct chars_category_of;

	template<typename T>
		requires requires
		{
			{
				T::value
			} -> std::same_as<CharsCategory>;
		}
	struct chars_category_of<T>
	{
		constexpr static auto value = T::value;
	};

	template<typename T>
	constexpr auto chars_category_of_v = chars_category_of<T>::value;

	template<CharsCategory Category>
	using input_type = typename input_selector<Category>::type;

	template<CharsCategory Category>
	using output_type = typename output_selector<Category>::type;

	enum class InputProcessPolicy
	{
		ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT,
		RETURN_RESULT_TYPE,
		ASSUME_VALID_INPUT,
	};

	template<meta::basic_fixed_string Name>
	class Scalar;

	template<meta::basic_fixed_string Name>
	class Simd;

	template<meta::basic_fixed_string Name>
	class Encoding;

	template<CharsCategory InputCategory>
	struct scalar_processor_of;

	template<CharsCategory InputCategory>
	using scalar_processor_of_t = typename scalar_processor_of<InputCategory>::type;

	template<CharsCategory InputCategory, meta::basic_fixed_string Name>
	struct simd_processor_of;

	template<CharsCategory InputCategory, meta::basic_fixed_string Name>
	using simd_processor_of_t = typename simd_processor_of<InputCategory, Name>::type;
}
