// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <chars/def.hpp>

namespace gal::prometheus::chars_1
{
	namespace latin_scalar_detail
	{
		using data_type = std::uint64_t;

		constexpr auto advance_latin = sizeof(data_type);
		constexpr auto advance_utf8 = sizeof(data_type);
		constexpr auto advance_utf16 = sizeof(data_type);
		constexpr auto advance_utf32 = sizeof(data_type);

		struct sign_latin
		{
		private:
			data_type data_;

		public:
			constexpr explicit sign_latin(const data_type data) noexcept
				: data_{data} {}

			/**
			 * @brief Get the underlying mask of the current block.
			 */
			[[nodiscard]] constexpr auto mask() const noexcept -> std::uint8_t
			{
				// MSB => LSB
				const auto msb = (data_ >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);
				const auto packed = msb * static_cast<data_type>(0x01'02'04'08'10'20'40'80);
				const auto mask = static_cast<std::uint8_t>(packed >> 56);

				return mask;
			}

			/**
			 * @brief Whether all sign bits are 0, in other words, whether the current block is all ASCII.
			 */
			[[nodiscard]] constexpr auto pure() const noexcept -> bool
			{
				return (data_ & 0x8080'8080'8080'8080) == 0;
			}

			/**
			 * @brief Get the number of non-ASCII in the current block.
			 */
			[[nodiscard]] constexpr auto count() const noexcept -> std::size_t
			{
				// 	const auto mask = this->mask(data_);
				// const auto count = std::popcount(mask);
				// return count;

				// MSB => LSB
				const auto msb = (data_ >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);

				// const auto packed = msb * static_cast<data_type>(0x01'01'01'01'01'01'01'01);
				// const auto count = static_cast<std::uint8_t>(packed >> 56);

				const auto count = std::popcount(msb);
				return count;
			}

			/**
			 * @brief Get the number of consecutive ASCII at the beginning.
			 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
			 * ^-----^ start_count
			 *                                                             ^----------^ end_count           
			 */
			[[nodiscard]] constexpr auto start_count() const noexcept -> std::size_t
			{
				const auto mask = this->mask();

				return std::countr_zero(mask);
			}

			/**
			 * @brief Get the number of consecutive ASCII at the ending.
			 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
			 * ^-----^ start_count
			 *                                                             ^----------^ end_count       
			 */
			[[nodiscard]] constexpr auto end_count() const noexcept -> std::size_t
			{
				const auto mask = this->mask();

				return std::countl_zero(mask);
			}
		};

		struct sign_utf8 : sign_latin {};

		struct sign_utf16
		{
		private:
			data_type data_;

		public:
			constexpr explicit sign_utf16(const data_type data) noexcept
				: data_{data} {}

			/**
			 * @brief Whether all sign bits are 0, in other words, whether the current block is all ASCII.
			 */
			[[nodiscard]] constexpr auto pure() const noexcept -> bool
			{
				return (data_ & 0xff80'ff80'ff80'ff80) == 0;
			}
		};

		struct sign_utf32
		{
		private:
			data_type data_;

		public:
			constexpr explicit sign_utf32(const data_type data) noexcept
				: data_{data} {}

			/**
			 * @brief Whether all sign bits are 0, in other words, whether the current block is all ASCII.
			 */
			[[nodiscard]] constexpr auto pure() const noexcept -> bool
			{
				return (data_ & 0xffff'ff80'ffff'ff80) == 0;
			}
		};
	}

	namespace scalar
	{
		using data_type = latin_scalar_detail::data_type;

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
				std::ignore = latin::write_utf8(output, input);

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
				return latin::write_utf8<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_char(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
			{
				return latin::write_utf8<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_char(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
			{
				return latin::write_utf8_char({input, std::char_traits<char_type>::length(input)});
			}

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
				std::ignore = latin::write_utf8_pure(output, input);

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
				return latin::write_utf8_pure<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_char_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
			{
				return latin::write_utf8_pure<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_char_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
			{
				return latin::write_utf8_char_pure({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				pointer_type current,
				pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				std::ignore = latin::write_utf8_correct(output, input);

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
				return latin::write_utf8_correct<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_char_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
			{
				return latin::write_utf8_correct<std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_char_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8_CHAR>::value_type>
			{
				return latin::write_utf8_char_correct({input, std::char_traits<char_type>::length(input)});
			}

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
				std::ignore = latin::write_utf8(output, input);

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
				return latin::write_utf8<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
			{
				return latin::write_utf8<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
			{
				return latin::write_utf8({input, std::char_traits<char_type>::length(input)});
			}

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
				std::ignore = latin::write_utf8_pure(output, input);

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
				return latin::write_utf8_pure<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
			{
				return latin::write_utf8_pure<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
			{
				return latin::write_utf8_pure({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
				pointer_type current,
				pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				std::ignore = latin::write_utf8_correct(output, input);

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
				return latin::write_utf8_correct<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
			{
				return latin::write_utf8_correct<std::basic_string<output_type_of<CharsType::UTF8>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF8>::value_type>
			{
				return latin::write_utf8_correct({input, std::char_traits<char_type>::length(input)});
			}

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
				std::ignore = latin::write_utf16_le(output, input);

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
				return latin::write_utf16_le<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return latin::write_utf16_le<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return latin::write_utf16_le({input, std::char_traits<char_type>::length(input)});
			}

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
				std::ignore = latin::write_utf16_le_pure(output, input);

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
				return latin::write_utf16_le_pure<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return latin::write_utf16_le_pure<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return latin::write_utf16_le_pure({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				pointer_type current,
				pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				std::ignore = latin::write_utf16_le_correct(output, input);

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
				return latin::write_utf16_le_correct<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return latin::write_utf16_le_correct<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return latin::write_utf16_le_correct({input, std::char_traits<char_type>::length(input)});
			}

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
				std::ignore = latin::write_utf16_be(output, input);

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
				return latin::write_utf16_be<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return latin::write_utf16_be<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return latin::write_utf16_be({input, std::char_traits<char_type>::length(input)});
			}

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
				std::ignore = latin::write_utf16_be_pure(output, input);

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
				return latin::write_utf16_be_pure<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return latin::write_utf16_be_pure<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return latin::write_utf16_be_pure({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				pointer_type current,
				pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				std::ignore = latin::write_utf16_be_correct(output, input);

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
				return latin::write_utf16_be_correct<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return latin::write_utf16_be_correct<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return latin::write_utf16_be_correct({input, std::char_traits<char_type>::length(input)});
			}

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
				std::ignore = latin::write_utf32(output, input);

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
				return latin::write_utf32<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return latin::write_utf32<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return latin::write_utf32({input, std::char_traits<char_type>::length(input)});
			}

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
				std::ignore = latin::write_utf32_pure(output, input);

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
				return latin::write_utf32_pure<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_pure(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return latin::write_utf32_pure<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_pure(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return latin::write_utf32_pure({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				pointer_type current,
				pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				std::ignore = latin::write_utf32_correct(output, input);

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
				return latin::write_utf32_correct<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_correct(const input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return latin::write_utf32_correct<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_correct(const pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return latin::write_utf32_correct({input, std::char_traits<char_type>::length(input)});
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
				std::ignore = latin::write_latin(output, input);

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
				return latin::write_latin<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin(const input_type input) noexcept -> std::basic_string<char_type>
			{
				return latin::write_latin<std::basic_string<char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin(const pointer_type input) noexcept -> std::basic_string<char_type>
			{
				return latin::write_latin({input, std::char_traits<char_type>::length(input)});
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
				std::ignore = latin::write_latin_pure(output, input);

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
				return latin::write_latin_pure<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_pure(const input_type input) noexcept -> std::basic_string<char_type>
			{
				return latin::write_latin_pure<std::basic_string<char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_pure(const pointer_type input) noexcept -> std::basic_string<char_type>
			{
				return latin::write_latin_pure({input, std::char_traits<char_type>::length(input)});
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
				std::ignore = latin::write_latin_correct(output, input);

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
				return latin::write_latin_correct<StringType>({input, std::char_traits<char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_correct(const input_type input) noexcept -> std::basic_string<char_type>
			{
				return latin::write_latin_correct<std::basic_string<char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_correct(const pointer_type input) noexcept -> std::basic_string<char_type>
			{
				return latin::write_latin_correct({input, std::char_traits<char_type>::length(input)});
			}
		}

		namespace utf8
		{
			namespace uc
			{
				using input_type = input_type_of<CharsType::UTF8_CHAR>;
				using char_type = input_type::value_type;
				using size_type = input_type::size_type;
				using pointer_type = input_type::const_pointer;
			}

			/**
			 * @brief Checks if there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto validate(uc::pointer_type current, uc::pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Checks if there are all valid `UTF8` code point in the range of @c input.
			 * @return {@c ErrorCode::NONE, @c input.size()}
			 * @return {(error code), (the first invalid code point location)}
			 */
			[[nodiscard]] auto validate(uc::input_type input) noexcept -> result_error_input_type;

			/**
			 * @brief Checks if there are all valid `UTF8` code point in the range of [@c input, @c input+std::char_traits<char_type>::length(input)].
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
			 * @return {(error code), (the first invalid code point location)}
			 */
			[[nodiscard]] auto validate(uc::pointer_type input) noexcept -> result_error_input_type;

			/**
			 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_latin(uc::input_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_latin(uc::pointer_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf16(uc::input_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf16(uc::pointer_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf32(uc::input_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf32(uc::pointer_type input) noexcept -> uc::size_type;

			// =======================================================
			// UTF8 => LATIN

			/**
			 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				uc::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = utf8::write_latin(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_latin<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_latin_pure(
				output_type_of<CharsType::LATIN>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				uc::input_type input
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
				uc::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin_pure(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = utf8::write_latin_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin_pure(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_latin_pure<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_pure(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin_pure<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_pure(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin_pure({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				uc::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin_correct(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = utf8::write_latin_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin_correct(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_latin_correct<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_correct(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin_correct<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_correct(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin_correct({input, std::char_traits<uc::char_type>::length(input)});
			}

			// =======================================================
			// UTF8 => UTF16_LE

			/**
			 * @brief If there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				uc::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_le(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_le<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf16_le_pure(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				uc::input_type input
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
				uc::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le_pure(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_le_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le_pure(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_le_pure<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_pure(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le_pure<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_pure(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le_pure({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				uc::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le_correct(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_le_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le_correct(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_le_correct<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_correct(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le_correct<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_correct(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le_correct({input, std::char_traits<uc::char_type>::length(input)});
			}

			// =======================================================
			// UTF8 => UTF16_BE

			/**
			 * @brief If there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				uc::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_be(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_be<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf16_be_pure(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				uc::input_type input
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
				uc::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be_pure(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_be_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be_pure(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_be_pure<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_pure(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be_pure<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_pure(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be_pure({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				uc::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be_correct(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_be_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be_correct(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_be_correct<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_correct(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be_correct<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_correct(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be_correct({input, std::char_traits<uc::char_type>::length(input)});
			}

			// =======================================================
			// UTF8 => UTF32

			/**
			 * @brief If there is at least one valid `UTF32` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				uc::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = utf8::write_utf32(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf32<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf32_pure(
				output_type_of<CharsType::UTF32>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				uc::input_type input
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
				uc::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32_pure(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = utf8::write_utf32_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32_pure(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf32_pure<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_pure(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32_pure<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_pure(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32_pure({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
			 * assume that the @c input string is all valid `UTF32`.
			 * @return {@c length_for_utf32(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				uc::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
			 * assume that the @c input string is all valid `UTF32`.
			 * @return {@c length_for_utf32(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32_correct(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = utf8::write_utf32_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32_correct(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf32_correct<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_correct(const uc::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32_correct<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_correct(const uc::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32_correct({input, std::char_traits<uc::char_type>::length(input)});
			}

			namespace u
			{
				using input_type = input_type_of<CharsType::UTF8>;
				using char_type = input_type::value_type;
				using size_type = input_type::size_type;
				using pointer_type = input_type::const_pointer;
			}

			/**
			 * @brief Checks if there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto validate(u::pointer_type current, u::pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Checks if there are all valid `UTF8` code point in the range of @c input.
			 * @return {@c ErrorCode::NONE, @c input.size()}
			 * @return {(error code), (the first invalid code point location)}
			 */
			[[nodiscard]] auto validate(u::input_type input) noexcept -> result_error_input_type;

			/**
			 * @brief Checks if there are all valid `UTF8` code point in the range of [@c input, @c input+std::char_traits<char_type>::length(input)].
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input)}
			 * @return {(error code), (the first invalid code point location)}
			 */
			[[nodiscard]] auto validate(u::pointer_type input) noexcept -> result_error_input_type;

			/**
			 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_latin(u::input_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `LATIN` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_latin(u::pointer_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf16(u::input_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `UTF16` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf16(u::pointer_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf32(u::input_type input) noexcept -> uc::size_type;

			/**
			 * @brief If @c input string is converted to a `UTF32` string, how many code points are output.
			 */
			[[nodiscard]] auto length_for_utf32(u::pointer_type input) noexcept -> uc::size_type;

			// =======================================================
			// UTF8 => LATIN

			/**
			 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				u::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_latin(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				u::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = utf8::write_latin(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_latin<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_latin_pure(
				output_type_of<CharsType::LATIN>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				u::input_type input
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
				u::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin_pure(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = utf8::write_latin_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin_pure(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_latin_pure<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_pure(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin_pure<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_pure(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin_pure({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				u::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string,
			 * assume that the @c input string is all valid `LATIN`.
			 * @return {@c length_for_latin(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				u::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin_correct(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::LATIN>::pointer output = string.data();
				std::ignore = utf8::write_latin_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::LATIN>::pointer>;
				}
			[[nodiscard]] auto write_latin_correct(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_latin_correct<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_correct(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin_correct<std::basic_string<output_type_of<CharsType::LATIN>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `LATIN` string.
			 */
			[[nodiscard]] inline auto write_latin_correct(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::LATIN>::value_type>
			{
				return utf8::write_latin_correct({input, std::char_traits<u::char_type>::length(input)});
			}

			// =======================================================
			// UTF8 => UTF16_LE

			/**
			 * @brief If there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				u::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				u::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_le(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_le<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf16_le_pure(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				u::input_type input
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
				u::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le_pure(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_le_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le_pure(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_le_pure<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_pure(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le_pure<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_pure(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le_pure({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				u::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				u::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le_correct(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_LE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_le_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_LE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_le_correct(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_le_correct<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_correct(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le_correct<std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_le_correct(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_LE>::value_type>
			{
				return utf8::write_utf16_le_correct({input, std::char_traits<u::char_type>::length(input)});
			}

			// =======================================================
			// UTF8 => UTF16_BE

			/**
			 * @brief If there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				u::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf16(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				u::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_be(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_be<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf16_be_pure(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				u::input_type input
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
				u::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be_pure(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_be_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be_pure(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_be_pure<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_pure(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be_pure<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_pure(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be_pure({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				u::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string,
			 * assume that the @c input string is all valid `UTF16`.
			 * @return {@c length_for_utf16(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				u::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be_correct(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF16_BE>::pointer output = string.data();
				std::ignore = utf8::write_utf16_be_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF16_BE>::pointer>;
				}
			[[nodiscard]] auto write_utf16_be_correct(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf16_be_correct<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_correct(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be_correct<std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF16` string.
			 */
			[[nodiscard]] inline auto write_utf16_be_correct(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16_BE>::value_type>
			{
				return utf8::write_utf16_be_correct({input, std::char_traits<u::char_type>::length(input)});
			}

			// =======================================================
			// UTF8 => UTF32

			/**
			 * @brief If there is at least one valid `UTF32` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				u::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf32(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				u::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = utf8::write_utf32(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf32<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf32_pure(
				output_type_of<CharsType::UTF32>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				u::input_type input
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
				u::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32_pure(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = utf8::write_utf32_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32_pure(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf32_pure<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_pure(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32_pure<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_pure(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32_pure({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
			 * assume that the @c input string is all valid `UTF32`.
			 * @return {@c length_for_utf32(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				u::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string,
			 * assume that the @c input string is all valid `UTF32`.
			 * @return {@c length_for_utf32(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				u::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32_correct(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF32>::pointer output = string.data();
				std::ignore = utf8::write_utf32_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF32>::pointer>;
				}
			[[nodiscard]] auto write_utf32_correct(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf32_correct<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_correct(const u::input_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32_correct<std::basic_string<output_type_of<CharsType::UTF32>::value_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF32` string.
			 */
			[[nodiscard]] inline auto write_utf32_correct(const u::pointer_type input) noexcept -> std::basic_string<output_type_of<CharsType::UTF32>::value_type>
			{
				return utf8::write_utf32_correct({input, std::char_traits<u::char_type>::length(input)});
			}

			// =======================================================
			// UTF8_CHAR => UTF8

			/**
			 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
				uc::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
				}
			[[nodiscard]] auto write_utf8(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = utf8::write_utf8(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
				}
			[[nodiscard]] auto write_utf8(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf8<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8(const uc::input_type input) noexcept -> std::basic_string<u::char_type>
			{
				return utf8::write_utf8<std::basic_string<u::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8(const uc::pointer_type input) noexcept -> std::basic_string<u::char_type>
			{
				return utf8::write_utf8({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf8_pure(
				output_type_of<CharsType::UTF8>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				uc::input_type input
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
				uc::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
				}
			[[nodiscard]] auto write_utf8_pure(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = utf8::write_utf8_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
				}
			[[nodiscard]] auto write_utf8_pure(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf8_pure<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_pure(const uc::input_type input) noexcept -> std::basic_string<u::char_type>
			{
				return utf8::write_utf8_pure<std::basic_string<u::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_pure(const uc::pointer_type input) noexcept -> std::basic_string<u::char_type>
			{
				return utf8::write_utf8_pure({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
				uc::pointer_type current,
				uc::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
				uc::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
				uc::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
				}
			[[nodiscard]] auto write_utf8_correct(const uc::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF8>::pointer output = string.data();
				std::ignore = utf8::write_utf8_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8>::pointer>;
				}
			[[nodiscard]] auto write_utf8_correct(const uc::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf8_correct<StringType>({input, std::char_traits<uc::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_correct(const uc::input_type input) noexcept -> std::basic_string<u::char_type>
			{
				return utf8::write_utf8_correct<std::basic_string<u::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_correct(const uc::pointer_type input) noexcept -> std::basic_string<u::char_type>
			{
				return utf8::write_utf8_correct({input, std::char_traits<uc::char_type>::length(input)});
			}

			// =======================================================
			// UTF8 => UTF8_CHAR

			/**
			 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c input.size(), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				u::input_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 * @return {@c ErrorCode::NONE, @c std::char_traits<char_type>::length(input), @c length_for_utf8(input)}
			 * @return {(error code), (the first invalid code point location), (how many code points are output)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				u::pointer_type input
			) noexcept -> result_error_input_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
				}
			[[nodiscard]] auto write_utf8(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = utf8::write_utf8(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<uc::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
				}
			[[nodiscard]] auto write_utf8(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf8<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8(const u::input_type input) noexcept -> std::basic_string<uc::char_type>
			{
				return utf8::write_utf8<std::basic_string<uc::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8(const u::pointer_type input) noexcept -> std::basic_string<uc::char_type>
			{
				return utf8::write_utf8({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf8_pure(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

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
				u::input_type input
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
				u::pointer_type input
			) noexcept -> result_error_input_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
				}
			[[nodiscard]] auto write_utf8_pure(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = utf8::write_utf8_pure(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
				}
			[[nodiscard]] auto write_utf8_pure(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf8_pure<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_pure(const u::input_type input) noexcept -> std::basic_string<uc::char_type>
			{
				return utf8::write_utf8_pure<std::basic_string<uc::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_pure(const u::pointer_type input) noexcept -> std::basic_string<uc::char_type>
			{
				return utf8::write_utf8_pure({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				u::pointer_type current,
				u::pointer_type end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				u::input_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string,
			 * assume that the @c input string is all valid `UTF8`.
			 * @return {@c length_for_utf8(input)}
			 * @note You can expect this function to always succeed (so the function is not marked `nodiscard`).
			 */
			auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				u::pointer_type input
			) noexcept -> result_output_type;

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
				}
			[[nodiscard]] auto write_utf8_correct(const u::input_type input) noexcept -> StringType
			{
				StringType string{};
				string.resize(input.size());

				output_type_of<CharsType::UTF8_CHAR>::pointer output = string.data();
				std::ignore = utf8::write_utf8_correct(output, input);

				return string;
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<u::size_type>());
					{
						string.data()
					} -> std::convertible_to<output_type_of<CharsType::UTF8_CHAR>::pointer>;
				}
			[[nodiscard]] auto write_utf8_correct(const u::pointer_type input) noexcept -> StringType
			{
				return utf8::write_utf8_correct<StringType>({input, std::char_traits<u::char_type>::length(input)});
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_correct(const u::input_type input) noexcept -> std::basic_string<uc::char_type>
			{
				return utf8::write_utf8_correct<std::basic_string<uc::char_type>>(input);
			}

			/**
			 * @brief Converts the @c input string `as far as possible` to a `UTF8` string.
			 */
			[[nodiscard]] inline auto write_utf8_correct(const u::pointer_type input) noexcept -> std::basic_string<uc::char_type>
			{
				return utf8::write_utf8_correct({input, std::char_traits<u::char_type>::length(input)});
			}
		}

		namespace utf16
		{
			/**
			 * @brief Checks if there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto validate_le(
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_latin_le(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_latin_pure_le(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_latin_correct_le(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf8_le(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf8_pure_le(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct_le(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf8_le(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf8_pure_le(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct_le(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF32` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf32_le(
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf32_pure_le(
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf32_correct_le(
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type_of<CharsType::UTF16_LE>::const_pointer current,
				input_type_of<CharsType::UTF16_LE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Checks if there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto validate_be(
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
		 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
		 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
		 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
		 */
			[[nodiscard]] auto write_latin_be(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_latin_pure_be(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_latin_correct_be(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf8_be(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf8_pure_be(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct_be(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf8_be(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf8_pure_be(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct_be(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF32` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf32_be(
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf32_pure_be(
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF32` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf32_correct_be(
				output_type_of<CharsType::UTF32>::pointer& output,
				input_type_of<CharsType::UTF16_BE>::const_pointer current,
				input_type_of<CharsType::UTF16_BE>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;
		}

		namespace utf32
		{
			/**
			 * @brief Checks if there is at least one valid `UTF32` code point in the range of [@c current, @c end].
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto validate(
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `LATIN` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_latin_pure(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `LATIN` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf8_pure(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf8_pure(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF8` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf16_le_pure(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF16 (little-endian)` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief If there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
			 * write that code point to @c output and iterate the @c output pointer (according to the number of code points actually written).
			 * @return {(how many iterations of the input pointer are required for the code point processed), (is the code point valid)}
			 */
			[[nodiscard]] auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end],
			 * and it is ASCII.
			 * @return You can assume that this function always returns {1, @c ErrorCode::NONE}
			 */
			[[nodiscard]] auto write_utf16_be_pure(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;

			/**
			 * @brief Assume that there is at least one valid `UTF16 (big-endian)` code point in the range of [@c current, @c end].
			 * @return You can assume that this function always returns {N, @c ErrorCode::NONE}, the value of N depends on the code point.
			 */
			[[nodiscard]] auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				input_type_of<CharsType::UTF32>::const_pointer current,
				input_type_of<CharsType::UTF32>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>;
		}
	}
}
