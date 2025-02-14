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

	namespace scalar::latin
	{
		constexpr auto chars_type = CharsType::LATIN;

		using data_type = latin_scalar_detail::data_type;

		[[nodiscard]] auto read_for_latin(input_type_of<chars_type>::const_pointer source) noexcept -> data_type;
		[[nodiscard]] auto read_for_utf8(input_type_of<chars_type>::const_pointer source) noexcept -> data_type;
		[[nodiscard]] auto read_for_utf16(input_type_of<chars_type>::const_pointer source) noexcept -> data_type;
		[[nodiscard]] auto read_for_utf32(input_type_of<chars_type>::const_pointer source) noexcept -> data_type;

		[[nodiscard]] auto validate_latin(input_type_of<chars_type>::const_pointer current, input_type_of<chars_type>::const_pointer end) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto validate_utf8(input_type_of<chars_type>::const_pointer current, input_type_of<chars_type>::const_pointer end) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto validate_utf16_le(input_type_of<chars_type>::const_pointer current, input_type_of<chars_type>::const_pointer end) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto validate_utf16_be(input_type_of<chars_type>::const_pointer current, input_type_of<chars_type>::const_pointer end) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto validate_utf32(input_type_of<chars_type>::const_pointer current, input_type_of<chars_type>::const_pointer end) noexcept -> std::pair<std::size_t, ErrorCode>;

		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;

		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;

		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;

		[[nodiscard]] auto write_utf16(
			output_type_of<CharsType::UTF16>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_utf16_pure(
			output_type_of<CharsType::UTF16>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_utf16_correct(
			output_type_of<CharsType::UTF16>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;

		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
		[[nodiscard]] auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer& output,
			input_type_of<chars_type>::const_pointer current,
			input_type_of<chars_type>::const_pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>;
	}
}
