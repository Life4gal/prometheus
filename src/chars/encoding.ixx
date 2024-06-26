// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:encoding;

import std;
import gal.prometheus.meta;
import gal.prometheus.error;

#else
#pragma once

#include <utility>

#include <prometheus/macro.hpp>
#include <meta/meta.ixx>
#include <error/error.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::chars)
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
			case EncodingType::UTF16_LE: { return 2; }
			case EncodingType::UTF16_BE: { return 2; }
			case EncodingType::UTF32_LE: { return 4; }
			case EncodingType::UTF32_BE: { return 4; }
			default: { GAL_PROMETHEUS_DEBUG_UNREACHABLE(); }
		}
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
	class Converter;
}
