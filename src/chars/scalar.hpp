// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <chars/def.hpp>

namespace gal::prometheus::chars
{
	namespace latin::scalar
	{
		/**
		 * @brief Checks if there are all valid `ASCII` code point in the range of @c input.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate(input_type input) noexcept -> result_error_input_type;

		/**
		 * @brief Checks if there are all valid `ASCII` code point in the range of [@c input, @c input+std::char_traits<char_type>::length(input)].
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate(pointer_type input) noexcept -> result_error_input_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_latin(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_latin(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf8(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf8(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf32(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf32(pointer_type input) noexcept -> size_type;

		// =======================================================
		// LATIN => UTF8_CHAR

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf8(input) ==> input.size()
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_pure<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_correct<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// LATIN => UTF8

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf8(input) ==> input.size()
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_pure<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_correct<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// LATIN => UTF16_LE

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_pure<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_correct<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// LATIN => UTF16_BE

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_pure<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_correct<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// LATIN => UTF32

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf32(input));

			std::ignore = scalar::write_utf32(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf32(input) ==> input.size()
			string.resize(length_for_utf32(input));

			std::ignore = scalar::write_utf32_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf32(input));

			std::ignore = scalar::write_utf32_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct({input, std::char_traits<char_type>::length(input)});
		}
	}

	namespace utf8_char::scalar
	{
		/**
		 * @brief Checks if there are all valid `UTF8` code point in the range of @c input.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate(input_type input) noexcept -> result_error_input_type;

		/**
		 * @brief Checks if there are all valid `UTF8` code point in the range of [@c input, @c input+std::char_traits<char_type>::length(input)].
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate(pointer_type input) noexcept -> result_error_input_type;

		/**
		 * @brief Finds the previous leading byte starting backward from @c current and validates from there.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note Used to pinpoint the location of an error when an invalid chunk is detected.
		 * @note We assume that the stream starts with a leading byte, and to check that it is the case,
		 * we ask that you pass a pointer to the start of the stream (@c begin).
		 */
		[[nodiscard]] auto rewind_and_validate(pointer_type begin, pointer_type current, pointer_type end) noexcept -> result_error_input_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_latin(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_latin(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf8(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf8(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf32(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf32(pointer_type input) noexcept -> size_type;

		// =======================================================
		// UTF8 => LATIN

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_latin(input));

			std::ignore = scalar::write_latin(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_latin(input) ==> input.size()
			string.resize(length_for_latin(input));

			std::ignore = scalar::write_latin_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_latin(input));

			std::ignore = scalar::write_latin_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF8 => UTF16_LE

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_pure<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_correct<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_correct({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto rewind_and_write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type furthest_possible_begin,
			input_type input
		) noexcept -> result_error_input_output_type;

		// =======================================================
		// UTF8 => UTF16_BE

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_pure<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_correct<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_correct({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto rewind_and_write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type furthest_possible_begin,
			input_type input
		) noexcept -> result_error_input_output_type;

		// =======================================================
		// UTF8 => UTF32

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf32(input));

			std::ignore = scalar::write_utf32(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf32(input) ==> input.size()
			string.resize(length_for_utf32(input));

			std::ignore = scalar::write_utf32_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf32(input));

			std::ignore = scalar::write_utf32_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto rewind_and_write_utf32(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type furthest_possible_begin,
			input_type input
		) noexcept -> result_error_input_output_type;

		// =======================================================
		// UTF8_CHAR => UTF8

		/**
		 * @brief Convert UTF8_CHAR string to UTF8 string up to the first invalid UTF8 code point.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Convert UTF8_CHAR string to UTF8 string up to the first invalid UTF8 code point.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Convert UTF8_CHAR string to UTF8 string up to the first invalid UTF8 code point.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf8(input) ==> input.size()
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8(string.data(), input);

			return string;
		}

		/**
		 * @brief Convert UTF8_CHAR string to UTF8 string up to the first invalid UTF8 code point.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Convert UTF8_CHAR string to UTF8 string up to the first invalid UTF8 code point.
		 */
		[[nodiscard]] inline auto write_utf8(const input_type input) noexcept -> std::basic_string<utf8::char_type>
		{
			return scalar::write_utf8<std::basic_string<utf8::char_type>>(input);
		}

		/**
		 * @brief Convert UTF8_CHAR string to UTF8 string up to the first invalid UTF8 code point.
		 */
		[[nodiscard]] inline auto write_utf8(const pointer_type input) noexcept -> std::basic_string<utf8::char_type>
		{
			return scalar::write_utf8({input, std::char_traits<char_type>::length(input)});
		}
	}

	namespace utf8::scalar
	{
		/**
		 * @brief Checks if there are all valid `UTF8` code point in the range of @c input.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate(input_type input) noexcept -> result_error_input_type;

		/**
		 * @brief Checks if there are all valid `UTF8` code point in the range of [@c input, @c input+std::char_traits<char_type>::length(input)].
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate(pointer_type input) noexcept -> result_error_input_type;

		/**
		 * @brief Finds the previous leading byte starting backward from @c current and validates from there.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note Used to pinpoint the location of an error when an invalid chunk is detected.
		 * @note We assume that the stream starts with a leading byte, and to check that it is the case,
		 * we ask that you pass a pointer to the start of the stream (@c begin).
		 */
		[[nodiscard]] auto rewind_and_validate(pointer_type begin, pointer_type current, pointer_type end) noexcept -> result_error_input_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_latin(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_latin(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf8(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf8(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf32(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf32(pointer_type input) noexcept -> size_type;

		// =======================================================
		// UTF8 => LATIN

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_latin(input));

			std::ignore = scalar::write_latin(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_latin(input) ==> input.size()
			string.resize(length_for_latin(input));

			std::ignore = scalar::write_latin_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_latin(input));

			std::ignore = scalar::write_latin_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF8 => UTF16_LE

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_pure<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_correct<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_correct({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto rewind_and_write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type furthest_possible_begin,
			input_type input
		) noexcept -> result_error_input_output_type;

		// =======================================================
		// UTF8 => UTF16_BE

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_pure<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_correct<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_correct({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto rewind_and_write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type furthest_possible_begin,
			input_type input
		) noexcept -> result_error_input_output_type;

		// =======================================================
		// UTF8 => UTF32

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf32(input));

			std::ignore = scalar::write_utf32(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf32(input) ==> input.size()
			string.resize(length_for_utf32(input));

			std::ignore = scalar::write_utf32_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf32(input));

			std::ignore = scalar::write_utf32_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto rewind_and_write_utf32(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type furthest_possible_begin,
			input_type input
		) noexcept -> result_error_input_output_type;

		// =======================================================
		// UTF8 => UTF8_CHAR

		/**
		 * @brief Convert UTF8 string to UTF8_CHAR string up to the first invalid UTF8 code point.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Convert UTF8 string to UTF8_CHAR string up to the first invalid UTF8 code point.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Convert UTF8 string to UTF8_CHAR string up to the first invalid UTF8 code point.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8(string.data(), input);

			return string;
		}

		/**
		 * @brief Convert UTF8 string to UTF8_CHAR string up to the first invalid UTF8 code point.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Convert UTF8 string to UTF8_CHAR string up to the first invalid UTF8 code point.
		 */
		[[nodiscard]] inline auto write_utf8(const input_type input) noexcept -> std::basic_string<utf8_char::char_type>
		{
			return scalar::write_utf8<std::basic_string<utf8_char::char_type>>(input);
		}

		/**
		 * @brief Convert UTF8 string to UTF8_CHAR string up to the first invalid UTF8 code point.
		 */
		[[nodiscard]] inline auto write_utf8(const pointer_type input) noexcept -> std::basic_string<utf8_char::char_type>
		{
			return scalar::write_utf8({input, std::char_traits<char_type>::length(input)});
		}
	}

	namespace utf16::scalar
	{
		/**
		 * @brief Checks if there are all valid `UTF16 (little-endian)` code point in the range of @c input.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate_le(input_type input) noexcept -> result_error_input_type;

		/**
		 * @brief Checks if there are all valid `UTF16 (little-endian)` code point in the range of [@c input, @c input+std::char_traits<char_type>::length(input)].
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate_le(pointer_type input) noexcept -> result_error_input_type;

		/**
		 * @brief Checks if there are all valid `UTF16 (big-endian)` code point in the range of @c input.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate_be(input_type input) noexcept -> result_error_input_type;

		/**
		 * @brief Checks if there are all valid `UTF16 (big-endian)` code point in the range of [@c input, @c input+std::char_traits<char_type>::length(input)].
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate_be(pointer_type input) noexcept -> result_error_input_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_le_for_latin(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_le_for_latin(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_be_for_latin(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_be_for_latin(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_le_for_utf8(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_le_for_utf8(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_be_for_utf8(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_be_for_utf8(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_le_for_utf32(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_le_for_utf32(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_be_for_utf32(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_be_for_utf32(pointer_type input) noexcept -> size_type;

		// =======================================================
		// UTF16 => LATIN

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin_le(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin_le(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_le_for_latin(input));

			std::ignore = scalar::write_latin_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_le<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin_be(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin_be(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_be_for_latin(input));

			std::ignore = scalar::write_latin_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_be<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure_le(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure_le(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_le_for_latin(input) ==> input.size()
			string.resize(length_le_for_latin(input));

			std::ignore = scalar::write_latin_pure_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_pure_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure_le<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure_be(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure_be(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_be_for_latin(input) ==> input.size()
			string.resize(length_be_for_latin(input));

			std::ignore = scalar::write_latin_pure_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_pure_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure_be<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct_le(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct_le(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_le_for_latin(input));

			std::ignore = scalar::write_latin_correct_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_correct_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct_le<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct_be(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct_be(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_be_for_latin(input));

			std::ignore = scalar::write_latin_correct_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_correct_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct_be<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct_be({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF16 => UTF8_CHAR

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_le_for_utf8(input));

			std::ignore = scalar::write_utf8_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_le<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_be_for_utf8(input));

			std::ignore = scalar::write_utf8_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_be<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_le_for_utf8(input) ==> input.size()
			string.resize(length_le_for_utf8(input));

			std::ignore = scalar::write_utf8_pure_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_pure_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_pure_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_pure_le<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_pure_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_pure_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_be_for_utf8(input) ==> input.size()
			string.resize(length_be_for_utf8(input));

			std::ignore = scalar::write_utf8_pure_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_pure_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_pure_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_pure_be<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_pure_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_pure_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_le_for_utf8(input));

			std::ignore = scalar::write_utf8_correct_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_correct_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_correct_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_correct_le<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_correct_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_correct_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_be_for_utf8(input));

			std::ignore = scalar::write_utf8_correct_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_correct_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_correct_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_correct_be<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_correct_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_correct_be({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF16 => UTF8

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8_le(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8_le(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_le_for_utf8(input));

			std::ignore = scalar::write_utf8_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_le<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8_be(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8_be(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_be_for_utf8(input));

			std::ignore = scalar::write_utf8_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_be<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure_le(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure_le(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_le_for_utf8(input) ==> input.size()
			string.resize(length_le_for_utf8(input));

			std::ignore = scalar::write_utf8_pure_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_pure_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_pure_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_pure_le<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_pure_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_pure_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure_be(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure_be(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_be_for_utf8(input) ==> input.size()
			string.resize(length_be_for_utf8(input));

			std::ignore = scalar::write_utf8_pure_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_pure_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_pure_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_pure_be<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_pure_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_pure_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct_le(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct_le(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_le_for_utf8(input));

			std::ignore = scalar::write_utf8_correct_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_correct_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_correct_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_correct_le<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_correct_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_correct_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct_be(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct_be(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_be_for_utf8(input));

			std::ignore = scalar::write_utf8_correct_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_correct_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_correct_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_correct_be<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_correct_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_correct_be({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF16 => UTF32

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32_le(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32_le(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_le_for_utf32(input));

			std::ignore = scalar::write_utf32_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_le<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32_be(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf32_be(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_be_for_utf32(input));

			std::ignore = scalar::write_utf32_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_be<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure_le(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure_le(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_le_for_utf32(input) ==> input.size()
			string.resize(length_le_for_utf32(input));

			std::ignore = scalar::write_utf32_pure_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_pure_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure_le<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure_be(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf32_pure_be(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_be_for_utf32(input) ==> input.size()
			string.resize(length_be_for_utf32(input));

			std::ignore = scalar::write_utf32_pure_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_pure_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_pure_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure_be<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_pure_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_pure_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct_le(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct_le(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_le_for_utf32(input));

			std::ignore = scalar::write_utf32_correct_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_correct_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct_le<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct_be(
			output_type_of<CharsType::UTF32>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
		 * assume that the @c input string is all valid `UTF32`.
		 * @return {@c length_for_utf32(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf32_correct_be(
			output_type_of<CharsType::UTF32>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_be_for_utf32(input));

			std::ignore = scalar::write_utf32_correct_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
			}
		[[nodiscard]] auto write_utf32_correct_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf32_correct_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct_be<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
		 */
		[[nodiscard]] inline auto write_utf32_correct_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
		{
			return scalar::write_utf32_correct_be({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF16_LE => UTF16_LE
		// UTF16_BE => UTF16_BE

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string up to the first invalid UTF16_LE code point.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string up to the first invalid UTF16_LE code point.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string up to the first invalid UTF16_LE code point.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_to_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string up to the first invalid UTF16_LE code point.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_to_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le_to_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string up to the first invalid UTF16_LE code point.
		 */
		[[nodiscard]] inline auto write_utf16_le_to_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_le_to_be<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string up to the first invalid UTF16_LE code point.
		 */
		[[nodiscard]] inline auto write_utf16_le_to_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_le_to_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Convert UTF16_BE string to UTF16_LE string up to the first invalid UTF16_BE code point.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Convert UTF16_BE string to UTF16_LE string up to the first invalid UTF16_BE code point.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Convert UTF16_BE string to UTF16_LE string up to the first invalid UTF16_BE code point.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_to_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Convert UTF16_BE string to UTF16_LE string up to the first invalid UTF16_BE code point.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_to_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be_to_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Convert UTF16_BE string to UTF16_LE string up to the first invalid UTF16_BE code point.
		 */
		[[nodiscard]] inline auto write_utf16_be_to_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_be_to_le<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Convert UTF16_BE string to UTF16_LE string up to the first invalid UTF16_BE code point.
		 */
		[[nodiscard]] inline auto write_utf16_be_to_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_be_to_le({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF16_LE => UTF16_BE
		// UTF16_BE => UTF16_LE

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string (or vice versa), assuming the input string is valid.
		 */
		auto flip(
			output_type_of<CharsType::UTF16>::pointer output,
			input_type input
		) noexcept -> void;

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string (or vice versa), assuming the input string is valid.
		 */
		auto flip(
			output_type_of<CharsType::UTF16>::pointer output,
			pointer_type input
		) noexcept -> void;

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string (or vice versa), assuming the input string is valid.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16>::pointer>;
			}
		[[nodiscard]] auto flip(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			scalar::flip(string.data(), input);

			return string;
		}

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string (or vice versa), assuming the input string is valid.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16>::pointer>;
			}
		[[nodiscard]] auto flip(const pointer_type input) noexcept -> StringType
		{
			return scalar::flip<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string (or vice versa), assuming the input string is valid.
		 */
		[[nodiscard]] inline auto flip(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16>::value_type>
		{
			return scalar::flip<std::basic_string<output_type_of<CharsType::UTF16>::value_type>>(input);
		}

		/**
		 * @brief Convert UTF16_LE string to UTF16_BE string (or vice versa), assuming the input string is valid.
		 */
		[[nodiscard]] inline auto flip(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16>::value_type>
		{
			return scalar::flip({input, std::char_traits<char_type>::length(input)});
		}
	}

	namespace utf32::scalar
	{
		/**
		 * @brief Checks if there are all valid `UTF32` code point in the range of @c input.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate(input_type input) noexcept -> result_error_input_type;

		/**
		 * @brief Checks if there are all valid `UTF32` code point in the range of [@c input, @c input+std::char_traits<char_type>::length(input)].
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 */
		[[nodiscard]] auto validate(pointer_type input) noexcept -> result_error_input_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_latin(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_latin(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf8(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF8` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf8(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf16(pointer_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf32(input_type input) noexcept -> size_type;

		/**
		 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
		 */
		[[nodiscard]] auto length_for_utf32(pointer_type input) noexcept -> size_type;

		// =======================================================
		// UTF32 => LATIN

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_latin(input));

			std::ignore = scalar::write_latin(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_latin(input) ==> input.size()
			string.resize(length_for_latin(input));

			std::ignore = scalar::write_latin_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
		 * assume that the @c input string is all valid `LATIN`.
		 * @return {@c length_for_latin(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_latin(input));

			std::ignore = scalar::write_latin_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
			}
		[[nodiscard]] auto write_latin_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_latin_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
		 */
		[[nodiscard]] inline auto write_latin_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
		{
			return scalar::write_latin_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF32 => UTF8_CHAR

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf8(input) ==> input.size()
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_pure<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_correct<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_char_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
		{
			return scalar::write_utf8_char_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF32 => UTF8

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf8(input) ==> input.size()
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_pure<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
		 * assume that the @c input string is all valid `UTF8`.
		 * @return {@c length_for_utf8(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf8(input));

			std::ignore = scalar::write_utf8_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
			}
		[[nodiscard]] auto write_utf8_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf8_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_correct<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
		 */
		[[nodiscard]] inline auto write_utf8_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
		{
			return scalar::write_utf8_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF32 => UTF16_LE

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_pure<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_le_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_le_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_le_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_correct<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_le_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
		{
			return scalar::write_utf16_le_correct({input, std::char_traits<char_type>::length(input)});
		}

		// =======================================================
		// UTF32 => UTF16_BE

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
		 * @return {(error code), (the first invalid code point location), (how many code points are output)}
		 */
		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c input.size()}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all `ASCII`.
		 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
		 * @return {(error code), (the first invalid code point location)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 * @note Each input `ASCII` code always outputs an `ASCII` code,
		 * so the number of outputs is always equal to the number of inputs.
		 */
		auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_pure(const input_type input) noexcept -> StringType
		{
			StringType string{};
			// OPT: length_for_utf16(input) ==> input.size()
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be_pure(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_pure(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be_pure<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_pure<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_pure({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			input_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
		 * assume that the @c input string is all valid `UTF16`.
		 * @return {@c length_for_utf16(input)}
		 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
		 */
		auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer output,
			pointer_type input
		) noexcept -> result_output_type;

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_correct(const input_type input) noexcept -> StringType
		{
			StringType string{};
			string.resize(length_for_utf16(input));

			std::ignore = scalar::write_utf16_be_correct(string.data(), input);

			return string;
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
			}
		[[nodiscard]] auto write_utf16_be_correct(const pointer_type input) noexcept -> StringType
		{
			return scalar::write_utf16_be_correct<StringType>({input, std::char_traits<char_type>::length(input)});
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_correct<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
		}

		/**
		 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
		 */
		[[nodiscard]] inline auto write_utf16_be_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
		{
			return scalar::write_utf16_be_correct({input, std::char_traits<char_type>::length(input)});
		}
	}

	class Scalar
	{
	public:
		// ===================================================
		// validate

		template<CharsType InputType>
		[[nodiscard]] constexpr static auto validate(
			const typename input_type_of<InputType>::const_pointer current,
			const typename input_type_of<InputType>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			if constexpr (InputType == CharsType::LATIN)
			{
				return latin::validate(current, end);
			}
			else if constexpr (InputType == CharsType::UTF8_CHAR)
			{
				return utf8_char::validate(current, end);
			}
			else if constexpr (InputType == CharsType::UTF8)
			{
				return utf8::validate(current, end);
			}
			else if constexpr (InputType == CharsType::UTF16_LE)
			{
				return utf16::validate_le(current, end);
			}
			else if constexpr (InputType == CharsType::UTF16_BE)
			{
				return utf16::validate_be(current, end);
			}
			else if constexpr (InputType == CharsType::UTF32)
			{
				return utf32::validate(current, end);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

	private:
		template<CharsType InputType, typename Input>
		[[nodiscard]] constexpr static auto do_validate(const Input input) noexcept -> result_error_input_type
		{
			if constexpr (InputType == CharsType::LATIN)
			{
				return latin::scalar::validate(input);
			}
			else if constexpr (InputType == CharsType::UTF8_CHAR)
			{
				return utf8_char::scalar::validate(input);
			}
			else if constexpr (InputType == CharsType::UTF8)
			{
				return utf8::scalar::validate(input);
			}
			else if constexpr (InputType == CharsType::UTF16_LE)
			{
				return utf16::scalar::validate_le(input);
			}
			else if constexpr (InputType == CharsType::UTF16_BE)
			{
				return utf16::scalar::validate_be(input);
			}
			else if constexpr (InputType == CharsType::UTF32)
			{
				return utf32::scalar::validate(input);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

	public:
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto validate(const input_type_of<InputType> input) noexcept -> result_error_input_type
		{
			return Scalar::do_validate<InputType>(input);
		}

		template<CharsType InputType>
		[[nodiscard]] constexpr static auto validate(const typename input_type_of<InputType>::const_pointer input) noexcept -> result_error_input_type
		{
			return Scalar::do_validate<InputType>(input);
		}

		// ===================================================
		// length

	private:
		template<CharsType InputType, CharsType OutputType, typename Input>
		[[nodiscard]] constexpr static auto do_length(const Input input) noexcept -> typename input_type_of<InputType>::size_type
		{
			if constexpr (InputType == CharsType::LATIN)
			{
				using namespace latin;

				if constexpr (OutputType == CharsType::LATIN)
				{
					return scalar::length_for_latin(input);
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					return scalar::length_for_utf8(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					return scalar::length_for_utf16(input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return scalar::length_for_utf32(input);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8_CHAR)
			{
				using namespace utf8_char;

				if constexpr (OutputType == CharsType::LATIN)
				{
					return scalar::length_for_latin(input);
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					return scalar::length_for_utf8(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					return scalar::length_for_utf16(input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return scalar::length_for_utf32(input);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8)
			{
				using namespace utf8;

				if constexpr (OutputType == CharsType::LATIN)
				{
					return scalar::length_for_latin(input);
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					return scalar::length_for_utf8(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					return scalar::length_for_utf16(input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return scalar::length_for_utf32(input);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_LE)
			{
				using namespace utf16;

				if constexpr (OutputType == CharsType::LATIN)
				{
					return scalar::length_le_for_latin(input);
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					return scalar::length_le_for_utf8(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					return scalar::length_for_utf16(input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return scalar::length_le_for_utf32(input);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_BE)
			{
				using namespace utf16;

				if constexpr (OutputType == CharsType::LATIN)
				{
					return scalar::length_be_for_latin(input);
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					return scalar::length_be_for_utf8(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					return scalar::length_for_utf16(input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return scalar::length_be_for_utf32(input);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF32)
			{
				using namespace utf32;

				if constexpr (OutputType == CharsType::LATIN)
				{
					return scalar::length_for_latin(input);
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					return scalar::length_for_utf8(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					return scalar::length_for_utf16(input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return scalar::length_for_utf32(input);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

	public:
		template<CharsType InputType, CharsType OutputType>
		[[nodiscard]] constexpr static auto length(const input_type_of<InputType> input) noexcept -> typename input_type_of<InputType>::size_type
		{
			return Scalar::do_length<InputType, OutputType>(input);
		}

		template<CharsType InputType, CharsType OutputType>
		[[nodiscard]] constexpr static auto length(const typename input_type_of<InputType>::const_pointer input) noexcept -> typename input_type_of<InputType>::size_type
		{
			return Scalar::do_length<InputType, OutputType>(input);
		}

		// ===================================================
		// write

		template<CharsType InputType, CharsType OutputType, bool Pure = false, bool Correct = false>
		[[nodiscard]] constexpr static auto convert(
			typename output_type_of<OutputType>::pointer& output,
			const typename input_type_of<InputType>::const_pointer current,
			const typename input_type_of<InputType>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			if constexpr (InputType == CharsType::LATIN)
			{
				if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return latin::write_utf8_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return latin::write_utf8_correct(output, current, end);
					}
					else
					{
						return latin::write_utf8(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return latin::write_utf16_le_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return latin::write_utf16_le_correct(output, current, end);
					}
					else
					{
						return latin::write_utf16_le(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return latin::write_utf16_be_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return latin::write_utf16_be_correct(output, current, end);
					}
					else
					{
						return latin::write_utf16_be(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return latin::write_utf32_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return latin::write_utf32_correct(output, current, end);
					}
					else
					{
						return latin::write_utf32(output, current, end);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8_CHAR)
			{
				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return utf8_char::write_latin_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8_char::write_latin_correct(output, current, end);
					}
					else
					{
						return utf8_char::write_latin(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return utf8_char::write_utf8_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8_char::write_utf8_correct(output, current, end);
					}
					else
					{
						return utf8_char::write_utf8(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return utf8_char::write_utf16_le_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8_char::write_utf16_le_correct(output, current, end);
					}
					else
					{
						return utf8_char::write_utf16_le(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return utf8_char::write_utf16_be_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8_char::write_utf16_be_correct(output, current, end);
					}
					else
					{
						return utf8_char::write_utf16_be(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return utf8_char::write_utf32_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8_char::write_utf32_correct(output, current, end);
					}
					else
					{
						return utf8_char::write_utf32(output, current, end);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8)
			{
				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return utf8::write_latin_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8::write_latin_correct(output, current, end);
					}
					else
					{
						return utf8::write_latin(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return utf8::write_utf8_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8::write_utf8_correct(output, current, end);
					}
					else
					{
						return utf8::write_utf8(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return utf8::write_utf16_le_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8::write_utf16_le_correct(output, current, end);
					}
					else
					{
						return utf8::write_utf16_le(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return utf8::write_utf16_be_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8::write_utf16_be_correct(output, current, end);
					}
					else
					{
						return utf8::write_utf16_be(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return utf8::write_utf32_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf8::write_utf32_correct(output, current, end);
					}
					else
					{
						return utf8::write_utf32(output, current, end);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_LE)
			{
				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return utf16::write_latin_pure_le(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf16::write_latin_correct_le(output, current, end);
					}
					else
					{
						return utf16::write_latin_le(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return utf16::write_utf8_pure_le(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf16::write_utf8_correct_le(output, current, end);
					}
					else
					{
						return utf16::write_utf8_le(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return utf16::write_utf32_pure_le(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf16::write_utf32_correct_le(output, current, end);
					}
					else
					{
						return utf16::write_utf32_le(output, current, end);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_BE)
			{
				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return utf16::write_latin_pure_be(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf16::write_latin_correct_be(output, current, end);
					}
					else
					{
						return utf16::write_latin_be(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return utf16::write_utf8_pure_be(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf16::write_utf8_correct_be(output, current, end);
					}
					else
					{
						return utf16::write_utf8_be(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return utf16::write_utf32_pure_be(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf16::write_utf32_correct_be(output, current, end);
					}
					else
					{
						return utf16::write_utf32_be(output, current, end);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF32)
			{
				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return utf32::write_latin_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf32::write_latin_correct(output, current, end);
					}
					else
					{
						return utf32::write_latin(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return utf32::write_utf8_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf32::write_utf8_correct(output, current, end);
					}
					else
					{
						return utf32::write_utf8(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return utf32::write_utf16_le_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf32::write_utf16_le_correct(output, current, end);
					}
					else
					{
						return utf32::write_utf16_le(output, current, end);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return utf32::write_utf16_be_pure(output, current, end);
					}
					else if constexpr (Correct)
					{
						return utf32::write_utf16_be_correct(output, current, end);
					}
					else
					{
						return utf32::write_utf16_be(output, current, end);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

	private:
		template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct, typename Input>
			requires (
				std::is_convertible_v<Input, input_type_of<InputType>> or
				std::is_convertible_v<Input, typename input_type_of<InputType>::const_pointer>
			)
		[[nodiscard]] constexpr static auto do_convert(
			const typename output_type_of<OutputType>::pointer output,
			const Input input
		) noexcept -> auto
		{
			if constexpr (InputType == CharsType::LATIN)
			{
				using namespace latin;

				if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct(output, input);
					}
					else
					{
						return scalar::write_utf8(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct(output, input);
					}
					else
					{
						return scalar::write_utf16_le(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct(output, input);
					}
					else
					{
						return scalar::write_utf16_be(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct(output, input);
					}
					else
					{
						return scalar::write_utf32(output, input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8_CHAR)
			{
				using namespace utf8_char;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct(output, input);
					}
					else
					{
						return scalar::write_latin(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8)
				{
					return scalar::write_utf8(output, input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct(output, input);
					}
					else
					{
						return scalar::write_utf16_le(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct(output, input);
					}
					else
					{
						return scalar::write_utf16_be(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct(output, input);
					}
					else
					{
						return scalar::write_utf32(output, input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8)
			{
				using namespace utf8;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct(output, input);
					}
					else
					{
						return scalar::write_latin(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR)
				{
					return scalar::write_utf8(output, input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct(output, input);
					}
					else
					{
						return scalar::write_utf16_le(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct(output, input);
					}
					else
					{
						return scalar::write_utf16_be(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct(output, input);
					}
					else
					{
						return scalar::write_utf32(output, input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_LE)
			{
				using namespace utf16;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure_le(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct_le(output, input);
					}
					else
					{
						return scalar::write_latin_le(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure_le(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct_le(output, input);
					}
					else
					{
						return scalar::write_utf8_le(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					return scalar::write_utf16_le(output, input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure_le(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct_le(output, input);
					}
					else
					{
						return scalar::write_utf32_le(output, input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_BE)
			{
				using namespace utf16;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure_be(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct_be(output, input);
					}
					else
					{
						return scalar::write_latin_be(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure_be(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct_be(output, input);
					}
					else
					{
						return scalar::write_utf8_be(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					return scalar::write_utf16_be(output, input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure_be(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct_be(output, input);
					}
					else
					{
						return scalar::write_utf32_be(output, input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF32)
			{
				using namespace utf32;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct(output, input);
					}
					else
					{
						return scalar::write_latin(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct(output, input);
					}
					else
					{
						return scalar::write_utf8(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct(output, input);
					}
					else
					{
						return scalar::write_utf16_le(output, input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure(output, input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct(output, input);
					}
					else
					{
						return scalar::write_utf16_be(output, input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

	public:
		template<CharsType InputType, CharsType OutputType, bool Pure = false, bool Correct = false>
		[[nodiscard]] constexpr static auto convert(
			const typename output_type_of<OutputType>::pointer output,
			const input_type_of<InputType> input
		) noexcept -> auto
		{
			return Scalar::do_convert<InputType, OutputType, Pure, Correct>(output, input);
		}

		template<CharsType InputType, CharsType OutputType, bool Pure = false, bool Correct = false>
		[[nodiscard]] constexpr static auto convert(
			const typename output_type_of<OutputType>::pointer output,
			const typename input_type_of<InputType>::const_pointer input
		) noexcept -> auto
		{
			return Scalar::do_convert<InputType, OutputType, Pure, Correct>(output, input);
		}

	private:
		template<CharsType InputType, CharsType OutputType, typename StringType, bool Pure, bool Correct, typename Input>
		[[nodiscard]] constexpr static auto do_convert(const Input input) noexcept -> StringType
		{
			if constexpr (InputType == CharsType::LATIN)
			{
				using namespace latin;

				if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf8<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf16_le<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf16_be<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf32<StringType>(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8_CHAR)
			{
				using namespace utf8_char;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct<StringType>(input);
					}
					else
					{
						return scalar::write_latin<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8)
				{
					return scalar::write_utf8<StringType>(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf16_le<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf16_be<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf32<StringType>(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8)
			{
				using namespace utf8;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct<StringType>(input);
					}
					else
					{
						return scalar::write_latin<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR)
				{
					return scalar::write_utf8<StringType>(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf16_le<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf16_be<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf32<StringType>(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_LE)
			{
				using namespace utf16;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure_le<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct_le<StringType>(input);
					}
					else
					{
						return scalar::write_latin_le<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure_le<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct_le<StringType>(input);
					}
					else
					{
						return scalar::write_utf8_le<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure_le<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct_le<StringType>(input);
					}
					else
					{
						return scalar::write_utf32_le<StringType>(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_BE)
			{
				using namespace utf16;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure_be<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct_be<StringType>(input);
					}
					else
					{
						return scalar::write_latin_be<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure_be<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct_be<StringType>(input);
					}
					else
					{
						return scalar::write_utf8_be<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure_be<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct_be<StringType>(input);
					}
					else
					{
						return scalar::write_utf32_be<StringType>(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF32)
			{
				using namespace utf32;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct<StringType>(input);
					}
					else
					{
						return scalar::write_latin<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf8<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf16_le<StringType>(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure<StringType>(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct<StringType>(input);
					}
					else
					{
						return scalar::write_utf16_be<StringType>(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

	public:
		template<CharsType InputType, CharsType OutputType, typename StringType, bool Pure = false, bool Correct = false>
		[[nodiscard]] constexpr static auto convert(const input_type_of<InputType> input) noexcept -> result_error_input_type
		{
			return Scalar::do_convert<InputType, OutputType, StringType, Pure, Correct>(input);
		}

		template<CharsType InputType, CharsType OutputType, typename StringType, bool Pure = false, bool Correct = false>
		[[nodiscard]] constexpr static auto convert(const typename input_type_of<InputType>::const_pointer input) noexcept -> result_error_input_type
		{
			return Scalar::do_convert<InputType, OutputType, StringType, Pure, Correct>(input);
		}

	private:
		template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct, typename Input>
		[[nodiscard]] constexpr static auto do_convert(const Input input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
		{
			if constexpr (InputType == CharsType::LATIN)
			{
				using namespace latin;

				if constexpr (OutputType == CharsType::UTF8_CHAR)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_char_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_char_correct(input);
					}
					else
					{
						return scalar::write_utf8_char(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct(input);
					}
					else
					{
						return scalar::write_utf8(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct(input);
					}
					else
					{
						return scalar::write_utf16_le(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct(input);
					}
					else
					{
						return scalar::write_utf16_be(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct(input);
					}
					else
					{
						return scalar::write_utf32(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8_CHAR)
			{
				using namespace utf8_char;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct(input);
					}
					else
					{
						return scalar::write_latin(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8)
				{
					return scalar::write_utf8(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct(input);
					}
					else
					{
						return scalar::write_utf16_le(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct(input);
					}
					else
					{
						return scalar::write_utf16_be(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct(input);
					}
					else
					{
						return scalar::write_utf32(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF8)
			{
				using namespace utf8;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct(input);
					}
					else
					{
						return scalar::write_latin(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR)
				{
					return scalar::write_utf8(input);
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct(input);
					}
					else
					{
						return scalar::write_utf16_le(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct(input);
					}
					else
					{
						return scalar::write_utf16_be(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct(input);
					}
					else
					{
						return scalar::write_utf32(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_LE)
			{
				using namespace utf16;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure_le(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct_le(input);
					}
					else
					{
						return scalar::write_latin_le(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_char_pure_le(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_char_correct_le(input);
					}
					else
					{
						return scalar::write_utf8_char_le(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure_le(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct_le(input);
					}
					else
					{
						return scalar::write_utf8_le(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					return scalar::write_utf16_le_to_be(input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure_le(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct_le(input);
					}
					else
					{
						return scalar::write_utf32_le(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF16_BE)
			{
				using namespace utf16;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure_be(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct_be(input);
					}
					else
					{
						return scalar::write_latin_be(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_char_pure_be(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_char_correct_be(input);
					}
					else
					{
						return scalar::write_utf8_char_be(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure_be(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct_be(input);
					}
					else
					{
						return scalar::write_utf8_be(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					return scalar::write_utf16_be_to_le(input);
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf32_pure_be(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf32_correct_be(input);
					}
					else
					{
						return scalar::write_utf32_be(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (InputType == CharsType::UTF32)
			{
				using namespace utf32;

				if constexpr (OutputType == CharsType::LATIN)
				{
					if constexpr (Pure)
					{
						return scalar::write_latin_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_latin_correct(input);
					}
					else
					{
						return scalar::write_latin(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_char_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_char_correct(input);
					}
					else
					{
						return scalar::write_utf8_char(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF8)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf8_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf8_correct(input);
					}
					else
					{
						return scalar::write_utf8(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_le_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_le_correct(input);
					}
					else
					{
						return scalar::write_utf16_le(input);
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_BE)
				{
					if constexpr (Pure)
					{
						return scalar::write_utf16_be_pure(input);
					}
					else if constexpr (Correct)
					{
						return scalar::write_utf16_be_correct(input);
					}
					else
					{
						return scalar::write_utf16_be(input);
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

	public:
		template<CharsType InputType, CharsType OutputType, bool Pure = false, bool Correct = false>
		[[nodiscard]] constexpr static auto convert(const input_type_of<InputType> input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
		{
			return Scalar::do_convert<InputType, OutputType, Pure, Correct>(input);
		}

		template<CharsType InputType, CharsType OutputType, bool Pure = false, bool Correct = false>
		[[nodiscard]] constexpr static auto convert(const typename input_type_of<InputType>::const_pointer input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
		{
			return Scalar::do_convert<InputType, OutputType, Pure, Correct>(input);
		}

	private:
		template<typename Input>
		constexpr static auto do_flip(
			const output_type_of<CharsType::UTF16>::pointer output,
			const Input input
		) noexcept -> void
		{
			using namespace utf16;

			scalar::flip(output, input);
		}

	public:
		constexpr static auto flip(
			const output_type_of<CharsType::UTF16>::pointer output,
			const input_type_of<CharsType::UTF16> input
		) noexcept -> void
		{
			return Scalar::do_flip(output, input);
		}

		constexpr static auto flip(
			const output_type_of<CharsType::UTF16>::pointer output,
			const input_type_of<CharsType::UTF16>::const_pointer input
		) noexcept -> void
		{
			return Scalar::do_flip(output, input);
		}

	private:
		template<typename StringType, typename Input>
		[[nodiscard]] constexpr static auto do_flip(const Input input) noexcept -> StringType
		{
			using namespace utf16;

			return scalar::flip<StringType>(input);
		}

	public:
		template<typename StringType>
		[[nodiscard]] constexpr static auto flip(const input_type_of<CharsType::UTF16> input) noexcept -> StringType
		{
			return Scalar::do_flip<StringType>(input);
		}

		template<typename StringType>
		[[nodiscard]] constexpr static auto flip(const input_type_of<CharsType::UTF16>::const_pointer input) noexcept -> StringType
		{
			return Scalar::do_flip<StringType>(input);
		}

	private:
		template<typename Input>
		[[nodiscard]] constexpr static auto do_flip(const Input input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16>::value_type>
		{
			using namespace utf16;

			return scalar::flip(input);
		}

	public:
		[[nodiscard]] constexpr static auto flip(const input_type_of<CharsType::UTF16> input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16>::value_type>
		{
			return Scalar::do_flip(input);
		}

		[[nodiscard]] constexpr static auto flip(const input_type_of<CharsType::UTF16>::const_pointer input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16>::value_type>
		{
			return Scalar::do_flip(input);
		}
	};
}
