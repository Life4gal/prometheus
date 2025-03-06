// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <meta/string.hpp>

namespace gal::prometheus::chars
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

	namespace def_detail
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
	constexpr auto chars_type_of = def_detail::type_of<T>::value;

	template<CharsType Type>
	using input_type_of = typename def_detail::io_selector<Type>::input;

	template<CharsType Type>
	using output_type_of = typename def_detail::io_selector<Type>::output;

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

	[[nodiscard]] auto width_of(EncodingType type) noexcept -> std::size_t;
	[[nodiscard]] auto bom_of(std::span<const char8_t> string) noexcept -> EncodingType;
	[[nodiscard]] auto bom_of(std::span<const char> string) noexcept -> EncodingType;

	namespace latin
	{
		using input_type = input_type_of<CharsType::LATIN>;
		using char_type = input_type::value_type;
		using size_type = input_type::size_type;
		using pointer_type = input_type::const_pointer;

		/**
		 * @brief Checks if there is at least one valid `ASCII` code point in the range of [@c current, @c end].
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto validate(pointer_type current, pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// LATIN => UTF8_CHAR

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// LATIN => UTF8

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// LATIN => UTF16_LE

		/**
		 * @brief If there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// LATIN => UTF16_BE

		/**
		 * @brief If there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// LATIN => UTF32

		/**
		 * @brief If there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;
	}

	namespace utf8_char
	{
		using input_type = input_type_of<CharsType::UTF8_CHAR>;
		using char_type = input_type::value_type;
		using size_type = input_type::size_type;
		using pointer_type = input_type::const_pointer;

		/**
		 * @brief Checks if there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto validate(pointer_type current, pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8 => LATIN

		/**
		 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8 => UTF16_LE

		/**
		 * @brief If there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8 => UTF16_BE

		/**
		 * @brief If there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8 => UTF32

		/**
		 * @brief If there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8_CHAR => UTF8

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;
	}

	namespace utf8
	{
		using input_type = input_type_of<CharsType::UTF8>;
		using char_type = input_type::value_type;
		using size_type = input_type::size_type;
		using pointer_type = input_type::const_pointer;

		/**
		 * @brief Checks if there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto validate(pointer_type current, pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8 => LATIN

		/**
		 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8 => UTF16_LE

		/**
		 * @brief If there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8 => UTF16_BE

		/**
		 * @brief If there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8 => UTF32

		/**
		 * @brief If there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF8_CHAR => UTF8

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;
	}

	namespace utf16
	{
		using input_type = input_type_of<CharsType::UTF16>;
		static_assert(std::is_same_v<input_type, input_type_of<CharsType::UTF16_LE>>);
		static_assert(std::is_same_v<input_type, input_type_of<CharsType::UTF16_BE>>);

		using char_type = input_type::value_type;
		using size_type = input_type::size_type;
		using pointer_type = input_type::const_pointer;

		/**
		 * @brief Checks if there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto validate_le(pointer_type current, pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Checks if there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto validate_be(pointer_type current, pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF16 => LATIN

		/**
		 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_latin_le(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_latin_be(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_latin_pure_le(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_latin_pure_be(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_latin_correct_le(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_latin_correct_be(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF16 => UTF8_CHAR

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF16 => UTF8

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8_le(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8_be(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure_le(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure_be(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct_le(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct_be(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF16 => UTF32

		/**
		 * @brief If there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf32_le(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief If there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf32_be(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf32_pure_le(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf32_pure_be(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf32_correct_le(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf32_correct_be(
			output_type_of<CharsType::UTF32>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;
	}

	namespace utf32
	{
		using input_type = input_type_of<CharsType::UTF32>;
		using char_type = input_type::value_type;
		using size_type = input_type::size_type;
		using pointer_type = input_type::const_pointer;

		/**
		 * @brief Checks if there is at least one valid `UTF32` code point in the range of [@c current, @c end].
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto validate(pointer_type current, pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF32 => LATIN

		/**
		 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF32 => UTF8_CHAR

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF32 => UTF8

		/**
		 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF32 => UTF16_LE

		/**
		 * @brief If there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		// =======================================================
		// UTF32 => UTF16_BE

		/**
		 * @brief If there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
		 * and it is ASCII.
		 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
		 */
		[[nodiscard]] auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

		/**
		 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
		 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
		 */
		[[nodiscard]] auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			pointer_type current,
			pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;
	}
}
