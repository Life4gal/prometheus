// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <chars/def.hpp>

namespace gal::prometheus::chars_1
{
	namespace latin
	{
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

		namespace scalar
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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
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
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le_pure(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le_correct(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be_pure(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be_correct(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_pure(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
			 * assume that the @c input string is all valid `UTF32`.
			 * @return {@c length_for_utf32(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_correct(output, input);

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

			// =======================================================
			// LATIN => LATIN

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin(output, input);

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
			[[nodiscard]] inline auto write_latin(const input_type input) noexcept -> std::basic_string<char_type>
			{
				return scalar::write_latin<std::basic_string<char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin(const pointer_type input) noexcept -> std::basic_string<char_type>
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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_pure(output, input);

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
			[[nodiscard]] inline auto write_latin_pure(const input_type input) noexcept -> std::basic_string<char_type>
			{
				return scalar::write_latin_pure<std::basic_string<char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_pure(const pointer_type input) noexcept -> std::basic_string<char_type>
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
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_correct(output, input);

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
			[[nodiscard]] inline auto write_latin_correct(const input_type input) noexcept -> std::basic_string<char_type>
			{
				return scalar::write_latin_correct<std::basic_string<char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_correct(const pointer_type input) noexcept -> std::basic_string<char_type>
			{
				return scalar::write_latin_correct({input, std::char_traits<char_type>::length(input)});
			}
		}
	}

	namespace utf8_char
	{
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

		namespace scalar
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
			 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_latin(input_type input) noexcept -> size_type;

			/**
			 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_latin(pointer_type input) noexcept -> size_type;

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
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_pure(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_correct(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le_pure(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le_correct(output, input);

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
			// UTF8 => UTF16_BE

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be_pure(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be_correct(output, input);

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

			// =======================================================
			// UTF8 => UTF32

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_pure(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
			 * assume that the @c input string is all valid `UTF32`.
			 * @return {@c length_for_utf32(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_correct(output, input);

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

			// =======================================================
			// UTF8_CHAR => UTF8

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8(output, input);

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
			[[nodiscard]] inline auto write_utf8(const input_type input) noexcept -> std::basic_string<utf8::char_type>
			{
				return scalar::write_utf8<std::basic_string<utf8::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8(const pointer_type input) noexcept -> std::basic_string<utf8::char_type>
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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all `UTF8`.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
			 * @return {(error code), (the first invalid code point location)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 * @note Each input `ASCII` code always outputs an `ASCII` code,
			 * so the number of outputs is always equal to the number of inputs.
			 */
			auto write_utf8_pure(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure(output, input);

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
			[[nodiscard]] inline auto write_utf8_pure(const input_type input) noexcept -> std::basic_string<utf8::char_type>
			{
				return scalar::write_utf8_pure<std::basic_string<utf8::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_pure(const pointer_type input) noexcept -> std::basic_string<utf8::char_type>
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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct(output, input);

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
			[[nodiscard]] inline auto write_utf8_correct(const input_type input) noexcept -> std::basic_string<utf8::char_type>
			{
				return scalar::write_utf8_correct<std::basic_string<utf8::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_correct(const pointer_type input) noexcept -> std::basic_string<utf8::char_type>
			{
				return scalar::write_utf8_correct({input, std::char_traits<char_type>::length(input)});
			}
		}
	}

	namespace utf8
	{
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

		namespace scalar
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
			 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_latin(input_type input) noexcept -> size_type;

			/**
			 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_latin(pointer_type input) noexcept -> size_type;

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
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_pure(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_correct(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le_pure(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le_correct(output, input);

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
			// UTF8 => UTF16_BE

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be_pure(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be_correct(output, input);

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

			// =======================================================
			// UTF8 => UTF32

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_pure(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
			 * assume that the @c input string is all valid `UTF32`.
			 * @return {@c length_for_utf32(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_correct(output, input);

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

			// =======================================================
			// UTF8 => UTF8_CHAR

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8(output, input);

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
			[[nodiscard]] inline auto write_utf8(const input_type input) noexcept -> std::basic_string<utf8_char::char_type>
			{
				return scalar::write_utf8<std::basic_string<utf8_char::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8(const pointer_type input) noexcept -> std::basic_string<utf8_char::char_type>
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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all `UTF8`.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
			 * @return {(error code), (the first invalid code point location)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 * @note Each input `ASCII` code always outputs an `ASCII` code,
			 * so the number of outputs is always equal to the number of inputs.
			 */
			auto write_utf8_pure(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure(output, input);

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
			[[nodiscard]] inline auto write_utf8_pure(const input_type input) noexcept -> std::basic_string<utf8_char::char_type>
			{
				return scalar::write_utf8_pure<std::basic_string<utf8_char::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_pure(const pointer_type input) noexcept -> std::basic_string<utf8_char::char_type>
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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct(output, input);

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
			[[nodiscard]] inline auto write_utf8_correct(const input_type input) noexcept -> std::basic_string<utf8_char::char_type>
			{
				return scalar::write_utf8_correct<std::basic_string<utf8_char::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_correct(const pointer_type input) noexcept -> std::basic_string<utf8_char::char_type>
			{
				return scalar::write_utf8_correct({input, std::char_traits<char_type>::length(input)});
			}
		}
	}

	namespace utf16
	{
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

		namespace scalar
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
			 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf32(input_type input) noexcept -> size_type;

			/**
			 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf32(pointer_type input) noexcept -> size_type;

			// =======================================================
			// UTF16 => LATIN

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin_le(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin_le(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_le(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin_be(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_be(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_pure_le(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_pure_be(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct_le(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_correct_le(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct_be(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_correct_be(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8_le(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_le(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8_be(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_be(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure_le(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure_be(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct_le(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct_le(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct_be(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct_be(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8_le(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_le(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8_be(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_be(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
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
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure_le(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
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
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure_be(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct_le(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct_le(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct_be(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct_be(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32_le(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_le(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32_be(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_be(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_pure_le(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_pure_be(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
			 * assume that the @c input string is all valid `UTF32`.
			 * @return {@c length_for_utf32(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf32_correct_le(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_correct_le(output, input);

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
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
			 * assume that the @c input string is all valid `UTF32`.
			 * @return {@c length_for_utf32(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf32_correct_be(
				output_type_of<CharsType::UTF32>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = scalar::write_utf32_correct_le(output, input);

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
			// UTF16_LE => UTF16_BE
			// UTF16_BE => UTF16_LE

			// todo
		}
	}

	namespace utf32
	{
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

		namespace scalar
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

			// =======================================================
			// UTF32 => LATIN

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_pure(output, input);

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
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = scalar::write_latin_correct(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure(output, input);

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
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
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
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_pure(output, input);

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
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = scalar::write_utf8_correct(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le_pure(output, input);

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
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_le_correct(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be_pure(output, input);

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
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
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
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = scalar::write_utf16_be_correct(output, input);

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
	}
}
