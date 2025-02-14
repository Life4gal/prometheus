// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <meta/string.hpp>

namespace gal::prometheus::chars_1
{
	enum class EncodingType : std::uint8_t
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

	enum class CharsType : std::uint8_t
	{
		LATIN = 0b0000'0001,

		UTF8_CHAR = 0b0000'0010,
		UTF8 = 0b0000'0100,

		UTF16_LE = 0b0000'1000,
		UTF16_BE = 0b0001'0000,
		// Only for endianness-free functions, e.g. to calculate the length of a string
		UTF16 = UTF16_LE | UTF16_BE,

		UTF32 = 0b0010'0000,
	};

	namespace encoding_detail
	{
		template<CharsType Type>
		struct io_selector;

		// ======================================
		// LATIN
		// ======================================

		template<>
		struct io_selector<CharsType::LATIN>
		{
			using input = std::span<const char>;
			using output = std::span<char>;
			constexpr static auto value = CharsType::LATIN;
		};

		// ======================================
		// UTF8_CHAR
		// ======================================

		template<>
		struct io_selector<CharsType::UTF8_CHAR>
		{
			using input = std::span<const char>;
			using output = std::span<char>;
			constexpr static auto value = CharsType::UTF8_CHAR;
		};

		// ======================================
		// UTF8
		// ======================================

		template<>
		struct io_selector<CharsType::UTF8>
		{
			using input = std::span<const char8_t>;
			using output = std::span<char8_t>;
			constexpr static auto value = CharsType::UTF8;
		};

		// ======================================
		// UTF16_LE
		// ======================================

		template<>
		struct io_selector<CharsType::UTF16_LE>
		{
			using input = std::span<const char16_t>;
			using output = std::span<char16_t>;
			constexpr static auto value = CharsType::UTF16_LE;
		};

		// ======================================
		// UTF16_LE
		// ======================================

		template<>
		struct io_selector<CharsType::UTF16_BE>
		{
			using input = std::span<const char16_t>;
			using output = std::span<char16_t>;
			constexpr static auto value = CharsType::UTF16_BE;
		};

		// ======================================
		// UTF16
		// ======================================

		template<>
		struct io_selector<CharsType::UTF16>
		{
			using input = std::span<const char16_t>;
			using output = std::span<char16_t>;
			constexpr static auto value = CharsType::UTF16;
		};

		// ======================================
		// UTF32
		// ======================================

		template<>
		struct io_selector<CharsType::UTF32>
		{
			using input = std::span<const char32_t>;
			using output = std::span<char32_t>;
			constexpr static auto value = CharsType::UTF32;
		};

		template<typename>
		struct type_of;

		template<typename T>
			requires requires
			{
				{ T::value } -> std::same_as<CharsType>;
			}
		struct type_of<T>
		{
			constexpr static auto value = T::value;
		};
	}

	template<typename T>
	constexpr auto chars_type_of = encoding_detail::type_of<T>::value;

	template<CharsType Type>
	using input_type_of = typename encoding_detail::io_selector<Type>::input;

	template<CharsType Type>
	using output_type_of = typename encoding_detail::io_selector<Type>::output;

	// ReSharper disable CommentTypo

	enum class ErrorCode : std::uint8_t
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

		// The decoded character must be less than or equal to U+10FFFF, less than or equal than U+7F for ASCII OR less than equal than U+FF for Latin1.
		TOO_LARGE,

		// Any byte must have fewer than 5 header bits.
		HEADER_BITS,
	};

	// ReSharper restore CommentTypo

	enum class InputProcessPolicy : std::uint8_t
	{
		INTERNAL_INPUT = 0b0000'0001,
		INTERNAL_OUTPUT = 0b0000'0010,
		INTERNAL_ERROR = 0b0000'0100,

		// Write all characters until the first error character
		WRITE_ALL_CORRECT = INTERNAL_INPUT | INTERNAL_ERROR,
		// Write all characters until the first error character
		WRITE_ALL_CORRECT_2 = INTERNAL_INPUT | INTERNAL_OUTPUT | INTERNAL_ERROR,
		// Similar to WRITE_ALL_CORRECT, but the last block is not written
		// You can expect better performance with this policy (compared to WRITE_ALL_CORRECT), and the write length is a multiple of the block size
		WRITE_LAST_BLOCK = INTERNAL_INPUT | INTERNAL_ERROR,
		// Similar to WRITE_ALL_CORRECT, but the last block is not written
		// You can expect better performance with this policy (compared to WRITE_ALL_CORRECT), and the write length is a multiple of the block size
		WRITE_LAST_BLOCK_2 = INTERNAL_INPUT | INTERNAL_OUTPUT | INTERNAL_ERROR,
		// Assuming that the inputs are correct, this means that the characters are not checked for correctness during the write process
		ASSUME_ALL_CORRECT = INTERNAL_OUTPUT,

		DEFAULT = WRITE_ALL_CORRECT,
	};

	template<InputProcessPolicy ProcessPolicy>
	[[nodiscard]] constexpr auto write_all_correct() noexcept -> bool
	{
		return
				ProcessPolicy == InputProcessPolicy::WRITE_ALL_CORRECT or
				ProcessPolicy == InputProcessPolicy::WRITE_ALL_CORRECT_2;
	}

	template<InputProcessPolicy ProcessPolicy>
	[[nodiscard]] constexpr auto write_last_block() noexcept -> bool
	{
		return
				ProcessPolicy == InputProcessPolicy::WRITE_LAST_BLOCK or
				ProcessPolicy == InputProcessPolicy::WRITE_LAST_BLOCK_2;
	}

	template<InputProcessPolicy ProcessPolicy>
	[[nodiscard]] constexpr auto assume_all_correct() noexcept -> bool
	{
		return
				ProcessPolicy == InputProcessPolicy::ASSUME_ALL_CORRECT;
	}

	struct result_error_input_type
	{
		ErrorCode error;
		std::size_t input;

		[[nodiscard]] constexpr auto has_error() const noexcept -> bool
		{
			return error != ErrorCode::NONE;
		}

		[[nodiscard]] constexpr explicit operator bool() const noexcept
		{
			return not has_error();
		}
	};

	struct result_error_input_output_type
	{
		ErrorCode error;
		std::size_t input;
		std::size_t output;

		[[nodiscard]] constexpr auto has_error() const noexcept -> bool
		{
			return error != ErrorCode::NONE;
		}

		[[nodiscard]] constexpr explicit operator bool() const noexcept
		{
			return not has_error();
		}
	};

	struct result_output_type
	{
		std::size_t output;
	};

	// Use when the length is meaningless
	constexpr auto length_ignored = std::numeric_limits<std::size_t>::max();

	template<InputProcessPolicy ProcessPolicy>
	[[nodiscard]] constexpr auto make_result(const ErrorCode error, const std::size_t input, const std::size_t output) noexcept -> auto
	{
		if constexpr (
			ProcessPolicy == InputProcessPolicy::WRITE_ALL_CORRECT or
			ProcessPolicy == InputProcessPolicy::WRITE_LAST_BLOCK
		)
		{
			std::ignore = output;

			return result_error_input_type{.error = error, .input = input};
		}
		else if constexpr (
			ProcessPolicy == InputProcessPolicy::WRITE_ALL_CORRECT_2 or
			ProcessPolicy == InputProcessPolicy::WRITE_LAST_BLOCK_2
		)
		{
			return result_error_input_output_type{.error = error, .input = input, .output = output};
		}
		else if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_ALL_CORRECT)
		{
			std::ignore = error;
			std::ignore = input;

			return result_output_type{.output = output};
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}
}
