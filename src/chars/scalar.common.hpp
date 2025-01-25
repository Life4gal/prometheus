// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <chars/encoding.hpp>

#include <memory/rw.hpp>

namespace gal::prometheus::chars
{
	struct scalar_block
	{
		using data_type = std::uint64_t;
		using mask_type = std::uint8_t;

		// ===============================
		// READ

		/**
		 * @note When processing in scalar mode, data is not written in `block`,
		 * in other words, `block` is only used for reading,
		 * and the returned `advance` refers to the number of input characters to be processed.
		 */
		template<CharsType InputType, CharsType OutputType>
		[[nodiscard]] constexpr static auto advance_of() noexcept -> std::ptrdiff_t
		{
			std::ignore = OutputType;
			return sizeof(data_type) / sizeof(typename input_type_of<InputType>::value_type);
		}

		/**
		 * @note When processing in scalar mode, data is not written in `block`,
		 * in other words, `block` is only used for reading.
		 */
		template<CharsType InputType, CharsType OutputType>
		[[nodiscard]] constexpr static auto read(const typename input_type_of<InputType>::pointer source) noexcept -> data_type
		{
			std::ignore = InputType;
			std::ignore = OutputType;
			return memory::unaligned_load<data_type>(source);
		}

		// ===============================
		// CHECK

		/**
		 * @brief Checks if there is still at least one legal character at the current position.
		 * @tparam InputType Input Character Type
		 * @param current Current position of character input
		 * @param end End position of character input
		 * @return The number of input characters used for this check and the check result
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto validate(
			typename input_type_of<InputType>::pointer current,
			const typename input_type_of<InputType>::pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			if constexpr (
				InputType == CharsType::LATIN
			)
			{
				std::ignore = end;
				constexpr std::size_t length = 1;

				if (const auto value = static_cast<std::uint8_t>(*(current + 0));
					static_cast<std::uint8_t>(value) < 0x80)
				{
					return {length, ErrorCode::NONE};
				}

				return {length, ErrorCode::TOO_LARGE};
			}
			else if constexpr (
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				// 1-byte UTF-8
				// 2-bytes UTF-8 
				// 3-bytes UTF-8 
				// 4-bytes UTF-8

				const auto leading_byte = static_cast<std::uint8_t>(*(current + 0));

				if ((leading_byte & 0x80) == 0)
				{
					// ASCII
					constexpr std::size_t length = 1;

					return {length, ErrorCode::NONE};
				}

				if ((leading_byte & 0b1110'0000) == 0b1100'0000)
				{
					// we have a two-bytes UTF-8
					constexpr std::size_t length = 2;

					// minimal bound checking
					if (current + 1 >= end)
					{
						return {length, ErrorCode::TOO_SHORT};
					}

					const auto next_byte = static_cast<std::uint8_t>(*(current + 1));

					if ((next_byte & 0b1100'0000) != 0b1000'0000)
					{
						return {length, ErrorCode::TOO_SHORT};
					}

					// range check
					const auto code_point = static_cast<std::uint32_t>(
						(leading_byte & 0b0001'1111) << 6 |
						(next_byte & 0b0011'1111)
					);

					if (code_point < 0x80)
					{
						return {length, ErrorCode::OVERLONG};
					}
					if (code_point > 0x7ff)
					{
						return {length, ErrorCode::TOO_LARGE};
					}

					return {length, ErrorCode::NONE};
				}

				if ((leading_byte & 0b1111'0000) == 0b1110'0000)
				{
					// we have a three-byte UTF-8
					constexpr std::size_t length = 3;

					// minimal bound checking
					if (current + 2 >= end)
					{
						return {length, ErrorCode::TOO_SHORT};
					}

					const auto next_byte_1 = static_cast<std::uint8_t>(*(current + 1));
					const auto next_byte_2 = static_cast<std::uint8_t>(*(current + 2));

					if (
						((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
						((next_byte_2 & 0b1100'0000) != 0b1000'0000)
					)
					{
						return {length, ErrorCode::TOO_SHORT};
					}

					// range check
					const auto code_point = static_cast<std::uint32_t>(
						(leading_byte & 0b0000'1111) << 12 |
						(next_byte_1 & 0b0011'1111) << 6 |
						(next_byte_2 & 0b0011'1111)
					);

					if (code_point < 0x800)
					{
						return {length, ErrorCode::OVERLONG};
					}
					if (code_point > 0xffff)
					{
						return {length, ErrorCode::TOO_LARGE};
					}
					if (code_point > 0xd7ff and code_point < 0xe000)
					{
						return {length, ErrorCode::SURROGATE};
					}

					return {length, ErrorCode::NONE};
				}

				if ((leading_byte & 0b1111'1000) == 0b1111'0000)
				{
					// we have a four-byte UTF-8 word
					constexpr std::size_t length = 4;

					// minimal bound checking
					if (current + 3 >= end)
					{
						return {length, ErrorCode::TOO_SHORT};
					}

					const auto next_byte_1 = static_cast<std::uint8_t>(*(current + 1));
					const auto next_byte_2 = static_cast<std::uint8_t>(*(current + 2));
					const auto next_byte_3 = static_cast<std::uint8_t>(*(current + 3));

					if (
						((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
						((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
						((next_byte_3 & 0b1100'0000) != 0b1000'0000)
					)
					{
						return {length, ErrorCode::TOO_SHORT};
					}

					// range check
					const auto code_point = static_cast<std::uint32_t>(
						(leading_byte & 0b0000'0111) << 18 |
						(next_byte_1 & 0b0011'1111) << 12 |
						(next_byte_2 & 0b0011'1111) << 6 |
						(next_byte_3 & 0b0011'1111)
					);

					if (code_point <= 0xffff)
					{
						return {length, ErrorCode::OVERLONG};
					}
					if (code_point > 0x10'ffff)
					{
						return {length, ErrorCode::TOO_LARGE};
					}

					return {length, ErrorCode::NONE};
				}

				// we either have too many continuation bytes or an invalid leading byte
				constexpr std::size_t length = 0;

				if ((leading_byte & 0b1100'0000) == 0b1000'0000)
				{
					// we have too many continuation bytes
					return {length, ErrorCode::TOO_LONG};
				}

				// we have an invalid leading byte
				return {length, ErrorCode::HEADER_BITS};
			}
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE
			)
			{
				// 1-word UTF-16
				// 2-words UTF-16(surrogate pair)

				constexpr auto source_endian = InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big;

				if (const auto leading_word = scalar_block::utf16_to_native<source_endian>(*(current + 0));
					(leading_word & 0xf800) == 0xd800)
				{
					// we have a two-word UTF16
					// must be a surrogate pair
					constexpr std::size_t length = 2;

					// minimal bound checking
					if (current + 1 >= end)
					{
						return {length, ErrorCode::SURROGATE};
					}

					if (const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);
						diff > 0x3ff)
					{
						return {length, ErrorCode::SURROGATE};
					}

					const auto next_word = scalar_block::utf16_to_native<source_endian>(*(current + 1));
					if (const auto diff = static_cast<std::uint16_t>(next_word - 0xdc00);
						diff > 0x3ff)
					{
						return {length, ErrorCode::SURROGATE};
					}

					return {length, ErrorCode::NONE};
				}

				// we have a one-word UTF16
				constexpr std::size_t length = 1;

				return {length, ErrorCode::NONE};
			}
			else if constexpr (
				InputType == CharsType::UTF32
			)
			{
				std::ignore = end;
				constexpr std::size_t length = 1;

				const auto value = static_cast<std::uint32_t>(*(current + 0));

				if (value > 0x10'ffff)
				{
					return {length, ErrorCode::TOO_LARGE};
				}

				if (value >= 0xd800 and value <= 0xdfff)
				{
					return {length, ErrorCode::SURROGATE};
				}

				return {length, ErrorCode::NONE};
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		/**
		 * @brief Check whether the current block is pure ASCII or not.
		 * @code
		 * const auto value = scalar_block::read<chars_type, OutputType>(it_input_current);
		 * if (scalar_block::pure_ascii<chars_type>(value))
		 * { do_something(value); }
		 * else
		 * { do_something(value); }
		 * @endcode 
		 */
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto pure_ascii(const data_type value) noexcept -> bool
		{
			if constexpr (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				return (value & 0x8080'8080'8080'8080) == 0;
			}
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE or
				InputType == CharsType::UTF16
			)
			{
				return (value & 0xff80'ff80'ff80'ff80) == 0;
			}
			else if constexpr (
				InputType == CharsType::UTF32
			)
			{
				return (value & 0xffff'ff80'ffff'ff80) == 0;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		/**
		 * @note 8 bits characters only (LATIN/UTF8_CHAR/UTF8)
		 */
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto not_ascii_mask(const data_type value) noexcept -> std::uint8_t
		{
			if constexpr (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				// MSB => LSB
				const auto msb = (value >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);
				const auto packed = msb * static_cast<data_type>(0x01'02'04'08'10'20'40'80);
				const auto mask = static_cast<std::uint8_t>(packed >> 56);

				return mask;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("8 bits characters only (LATIN/UTF8_CHAR/UTF8)");
			}
		}

		/**
		 * @note 8 bits characters only (LATIN/UTF8_CHAR/UTF8)
		 */
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto not_ascii_count(const data_type value) noexcept -> std::size_t
		{
			if constexpr (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				// const auto mask = not_ascii_mask<InputType>(value);
				// const auto count = std::popcount(mask);
				// return count;

				// MSB => LSB
				const auto msb = (value >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);

				// const auto packed = msb * static_cast<data_type>(0x01'01'01'01'01'01'01'01);
				// const auto count = static_cast<std::uint8_t>(packed >> 56);

				const auto count = std::popcount(msb);
				return count;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("8 bits characters only (LATIN/UTF8_CHAR/UTF8)");
			}
		}

		// ===============================
		// WRITE

	private:
		/**
		 * @brief Convert input character to output character,
		 * if the output type is UTF16, it will be converted to the corresponding endian (assuming the input character is the native endian).
		 */
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto char_of(const auto value) noexcept -> typename output_type_of<OutputType>::value_type
		{
			using out_type = typename output_type_of<OutputType>::value_type;

			if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE)
			{
				if constexpr (const auto v16 = static_cast<std::uint16_t>(value);
					(OutputType == CharsType::UTF16_LE) != (std::endian::native == std::endian::little))
				{
					return static_cast<out_type>(std::byteswap(v16));
				}
				else
				{
					return static_cast<out_type>(v16);
				}
			}
			else
			{
				return static_cast<out_type>(value);
			}
		}

	public:
		/**
		 * @brief A character conversion may require more than one input character and may output more than one character.
		 * @tparam InputType Input Character Type
		 * @tparam OutputType Output Character Type
		 * @tparam PureAscii Whether the input for this conversion is pure ASCII or not
		 * @tparam AssumeAllCorrect Whether the input for this conversion does not contain illegal characters
		 * @param dest Destination of character output 
		 * @param current Current position of character input
		 * @param end End position of character input
		 * @return The number of input characters used for this conversion and the conversion result
		 * @note Conversion advances the output pointer (based on the actual number of output characters),
		 * the input pointer does not (it returns the number of input pointers to be advanced).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType InputType, CharsType OutputType, bool PureAscii = false, bool AssumeAllCorrect = false>
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<OutputType>::pointer& dest,
			typename input_type_of<InputType>::pointer current,
			const typename input_type_of<InputType>::pointer end
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			if constexpr (
				InputType == CharsType::LATIN
			)
			{
				// 1-byte LATIN:
				//	=> 1/2 UTF-8
				//	=> 1 UTF-16
				//	=> 1 UTF-32

				std::ignore = end;
				constexpr std::size_t length = 1;

				if constexpr (const auto value = static_cast<std::uint8_t>(*(current + 0));
					OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (PureAscii)
					{
						*(dest + 0) = scalar_block::char_of<OutputType>(value);

						dest += 1;
						return {length, ErrorCode::NONE};
					}
					else
					{
						if ((value & 0x80) == 0)
						{
							// ASCII
							*(dest + 0) = scalar_block::char_of<OutputType>(value);

							dest += 1;
							return {length, ErrorCode::NONE};
						}

						// 0b110?'???? 0b10??'????
						const auto c1 = static_cast<std::uint8_t>((value >> 6) | 0b1100'0000);
						const auto c2 = static_cast<std::uint8_t>((value & 0b0011'1111) | 0b1000'0000);

						*(dest + 0) = scalar_block::char_of<OutputType>(c1);
						*(dest + 1) = scalar_block::char_of<OutputType>(c2);

						dest += 2;
						return {length, ErrorCode::NONE};
					}
				}
				// ReSharper disable CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE)
				{
					*(dest + 0) = scalar_block::char_of<OutputType>(value);

					dest += 1;
					return {length, ErrorCode::NONE};
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					*(dest + 0) = scalar_block::char_of<OutputType>(value);

					dest += 1;
					return {length, ErrorCode::NONE};
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}
			else if constexpr (
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				// 1-byte UTF-8:
				//	=> 1 LATIN
				//	=> 1 UTF-16
				//	=> 1 UTF-32
				// 2-bytes UTF-8:
				//	=> 1 LATIN
				//	=> 1 UTF-16
				//	=> 1 UTF-32
				// 3-bytes UTF-8:
				//	=> 1 UTF-16
				//	=> 1 UTF-32
				// 4-bytes UTF-8:
				//	=> 2 UTF-16
				//	=> 1 UTF-32

				if constexpr (const auto leading_byte = static_cast<std::uint8_t>(*(current + 0));
					PureAscii)
				{
					constexpr std::size_t length = 1;

					*(dest + 0) = scalar_block::char_of<OutputType>(leading_byte);

					dest += 1;
					return {length, ErrorCode::NONE};
				}
				else
				{
					if ((leading_byte & 0x80) == 0)
					{
						// ASCII
						constexpr std::size_t length = 1;

						*(dest + 0) = scalar_block::char_of<OutputType>(leading_byte);

						dest += 1;
						return {length, ErrorCode::NONE};
					}

					if ((leading_byte & 0b1110'0000) == 0b1100'0000)
					{
						// we have a two-byte UTF-8
						constexpr std::size_t length = 2;

						// minimal bound checking
						if (current + 1 >= end)
						{
							return {length, ErrorCode::TOO_SHORT};
						}

						const auto next_byte = static_cast<std::uint8_t>(*(current + 1));

						// checks if the next byte is a valid continuation byte in UTF-8.
						// A valid continuation byte starts with 10.
						if ((next_byte & 0b1100'0000) != 0b1000'0000)
						{
							return {length, ErrorCode::TOO_SHORT};
						}

						// assembles the Unicode code point from the two bytes.
						// It does this by discarding the leading 110 and 10 bits from the two bytes,
						// shifting the remaining bits of the first byte,
						// and then combining the results with a bitwise OR operation.
						const auto code_point = static_cast<std::uint32_t>(
							(leading_byte & 0b0001'1111) << 6 |
							(next_byte & 0b0011'1111)
						);

						if constexpr (not AssumeAllCorrect)
						{
							if (code_point < 0x80)
							{
								return {length, ErrorCode::OVERLONG};
							}

							if (code_point >
							    []() noexcept -> std::uint32_t
							    {
								    if constexpr (OutputType == CharsType::LATIN) { return 0xff; }
								    else { return 0x7ff; }
							    }()
							)
							{
								return {length, ErrorCode::TOO_LARGE};
							}
						}

						*(dest + 0) = scalar_block::char_of<OutputType>(code_point);

						dest += 1;
						return {length, ErrorCode::NONE};
					}

					if ((leading_byte & 0b1111'0000) == 0b1110'0000)
					{
						// we have a three-byte UTF-8
						constexpr std::size_t length = 3;

						if constexpr (
							OutputType == CharsType::UTF16_LE or
							OutputType == CharsType::UTF16_BE or
							OutputType == CharsType::UTF32
						)
						{
							// minimal bound checking
							if (current + 2 >= end)
							{
								return {length, ErrorCode::TOO_SHORT};
							}

							const auto next_byte_1 = static_cast<std::uint8_t>(*(current + 1));
							const auto next_byte_2 = static_cast<std::uint8_t>(*(current + 2));

							if constexpr (not AssumeAllCorrect)
							{
								if (
									((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
									((next_byte_2 & 0b1100'0000) != 0b1000'0000)
								)
								{
									return {length, ErrorCode::TOO_SHORT};
								}
							}

							const auto code_point = static_cast<std::uint32_t>(
								(leading_byte & 0b0000'1111) << 12 |
								(next_byte_1 & 0b0011'1111) << 6 |
								(next_byte_2 & 0b0011'1111)
							);

							if constexpr (not AssumeAllCorrect)
							{
								if (code_point < 0x800)
								{
									return {length, ErrorCode::OVERLONG};
								}

								if (code_point > 0xffff)
								{
									return {length, ErrorCode::TOO_LARGE};
								}

								if (code_point > 0xd7ff and code_point < 0xe000)
								{
									return {length, ErrorCode::SURROGATE};
								}
							}

							*(dest + 0) = scalar_block::char_of<OutputType>(code_point);

							dest += 1;
							return {length, ErrorCode::NONE};
						}
						else
						{
							return {length, ErrorCode::TOO_LARGE};
						}
					}

					if ((leading_byte & 0b1111'1000) == 0b1111'0000)
					{
						// we have a four-byte UTF-8 word
						constexpr std::size_t length = 4;

						if constexpr (
							OutputType == CharsType::UTF16_LE or
							OutputType == CharsType::UTF16_BE or
							OutputType == CharsType::UTF32
						)
						{
							// minimal bound checking
							if (current + 3 >= end)
							{
								return {length, ErrorCode::TOO_SHORT};
							}

							const auto next_byte_1 = static_cast<std::uint8_t>(*(current + 1));
							const auto next_byte_2 = static_cast<std::uint8_t>(*(current + 2));
							const auto next_byte_3 = static_cast<std::uint8_t>(*(current + 3));

							if constexpr (not AssumeAllCorrect)
							{
								if (
									((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
									((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
									((next_byte_3 & 0b1100'0000) != 0b1000'0000)
								)
								{
									return {length, ErrorCode::TOO_SHORT};
								}
							}

							const auto code_point = static_cast<std::uint32_t>(
								(leading_byte & 0b0000'0111) << 18 |
								(next_byte_1 & 0b0011'1111) << 12 |
								(next_byte_2 & 0b0011'1111) << 6 |
								(next_byte_3 & 0b0011'1111)
							);

							if constexpr (not AssumeAllCorrect)
							{
								if (code_point <= 0xffff)
								{
									return {length, ErrorCode::OVERLONG};
								}

								if (code_point > 0x10'ffff)
								{
									return {length, ErrorCode::TOO_LARGE};
								}
							}

							if constexpr (OutputType == CharsType::UTF32)
							{
								*(dest + 0) = scalar_block::char_of<OutputType>(code_point);

								dest += 1;
							}
							else
							{
								const auto [high_surrogate, low_surrogate] = [cp = code_point - 0x1'0000]() noexcept -> auto
								{
									const auto high = static_cast<std::uint16_t>(0xd800 + (cp >> 10));
									const auto low = static_cast<std::uint16_t>(0xdc00 + (cp & 0x3ff));

									return std::make_pair(high, low);
								}();

								*(dest + 0) = scalar_block::char_of<OutputType>(high_surrogate);
								*(dest + 1) = scalar_block::char_of<OutputType>(low_surrogate);

								dest += 2;
							}

							return {length, ErrorCode::NONE};
						}
						else
						{
							return {length, ErrorCode::TOO_LARGE};
						}
					}

					// we either have too many continuation bytes or an invalid leading byte
					constexpr std::size_t length = 0;

					if ((leading_byte & 0b1100'0000) == 0b1000'0000)
					{
						// we have too many continuation bytes
						return {length, ErrorCode::TOO_LONG};
					}

					// we have an invalid leading byte
					return {length, ErrorCode::HEADER_BITS};
				}
			}
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE
			)
			{
				// 1-word UTF-16:
				//	=> 1 LATIN
				//	=> 1/2/3 UTF-8
				//	=> 1 UTF-32
				// 2-words UTF-16(surrogate pair):
				//	=> 4 UTF-8
				//	=> 1 UTF-32

				constexpr auto source_endian = InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big;

				if constexpr (const auto leading_word = scalar_block::utf16_to_native<source_endian>(*(current + 0));
					PureAscii)
				{
					constexpr std::size_t length = 1;

					*(dest + 0) = scalar_block::char_of<OutputType>(leading_word);

					dest += 1;
					return {length, ErrorCode::NONE};
				}
				else
				{
					if constexpr (OutputType == CharsType::LATIN)
					{
						constexpr std::size_t length = 1;

						if constexpr (not AssumeAllCorrect)
						{
							if ((leading_word & 0xff00) != 0)
							{
								return {length, ErrorCode::TOO_LARGE};
							}
						}

						*(dest + 0) = scalar_block::char_of<OutputType>(leading_word);

						dest += 1;
						return {length, ErrorCode::NONE};
					}
					else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
					{
						if ((leading_word & 0xff80) == 0)
						{
							// 1-word utf16 => 1-byte utf8
							constexpr std::size_t length = 1;

							*(dest + 0) = scalar_block::char_of<OutputType>(leading_word);

							dest += 1;
							return {length, ErrorCode::NONE};
						}

						if ((leading_word & 0xf800) == 0)
						{
							// 1-word utf16 => 2-bytes utf8
							constexpr std::size_t length = 1;

							// 0b110?'???? 0b10??'????
							const auto c1 = static_cast<std::uint16_t>((leading_word >> 6) | 0b1100'0000);
							const auto c2 = static_cast<std::uint16_t>((leading_word & 0b0011'1111) | 0b1000'0000);

							*(dest + 0) = scalar_block::char_of<OutputType>(c1);
							*(dest + 1) = scalar_block::char_of<OutputType>(c2);

							dest += 2;
							return {length, ErrorCode::NONE};
						}

						if ((leading_word & 0xf800) != 0xd800)
						{
							// 1-word utf16 => 3-bytes utf8
							constexpr std::size_t length = 1;

							// 0b1110'???? 0b10??'???? 0b10??'????
							const auto c1 = static_cast<std::uint16_t>((leading_word >> 12) | 0b1110'0000);
							const auto c2 = static_cast<std::uint16_t>(((leading_word >> 6) & 0b0011'1111) | 0b1000'0000);
							const auto c3 = static_cast<std::uint16_t>((leading_word & 0b0011'1111) | 0b1000'0000);

							*(dest + 0) = scalar_block::char_of<OutputType>(c1);
							*(dest + 1) = scalar_block::char_of<OutputType>(c2);
							*(dest + 2) = scalar_block::char_of<OutputType>(c3);

							dest += 3;
							return {length, ErrorCode::NONE};
						}

						// 2-word utf16 => 4-bytes utf8
						// must be a surrogate pair
						constexpr std::size_t length = 2;

						// minimal bound checking
						if (current + 1 >= end)
						{
							return {length, ErrorCode::SURROGATE};
						}

						const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);

						if constexpr (not AssumeAllCorrect)
						{
							if (diff > 0x3ff)
							{
								return {length, ErrorCode::SURROGATE};
							}
						}

						const auto next_word = scalar_block::utf16_to_native<source_endian>(*(current + 1));
						const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

						if constexpr (not AssumeAllCorrect)
						{
							if (next_diff > 0x3ff)
							{
								return {length, ErrorCode::SURROGATE};
							}
						}

						const auto value = static_cast<std::uint32_t>((diff << 10) + next_diff + 0x1'0000);

						// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
						const auto c1 = static_cast<std::uint16_t>((value >> 18) | 0b1111'0000);
						const auto c2 = static_cast<std::uint16_t>(((value >> 12) & 0b0011'1111) | 0b1000'0000);
						const auto c3 = static_cast<std::uint16_t>(((value >> 6) & 0b0011'1111) | 0b1000'0000);
						const auto c4 = static_cast<std::uint16_t>((value & 0b0011'1111) | 0b1000'0000);

						*(dest + 0) = scalar_block::char_of<OutputType>(c1);
						*(dest + 1) = scalar_block::char_of<OutputType>(c2);
						*(dest + 2) = scalar_block::char_of<OutputType>(c3);
						*(dest + 3) = scalar_block::char_of<OutputType>(c4);

						dest += 4;
						return {length, ErrorCode::NONE};
					}
					else if constexpr (OutputType == CharsType::UTF32)
					{
						if ((leading_word & 0xf800) == 0xd800)
						{
							// we have a two-word UTF16
							// must be a surrogate pair
							constexpr std::size_t length = 2;

							// minimal bound checking
							if (current + 1 >= end)
							{
								return {length, ErrorCode::SURROGATE};
							}

							const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);

							if constexpr (not AssumeAllCorrect)
							{
								if (diff > 0x3ff)
								{
									return {length, ErrorCode::SURROGATE};
								}
							}

							const auto next_word = scalar_block::utf16_to_native<source_endian>(*(current + 1));
							const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

							if constexpr (not AssumeAllCorrect)
							{
								if (next_diff > 0x3ff)
								{
									return {length, ErrorCode::SURROGATE};
								}
							}

							const auto value = static_cast<std::uint32_t>((diff << 10) + next_diff + 0x1'0000);

							*(dest + 0) = scalar_block::char_of<OutputType>(value);

							dest += 1;
							return {length, ErrorCode::NONE};
						}

						// we have a one-word UTF16
						constexpr std::size_t length = 1;

						*(dest + 0) = scalar_block::char_of<OutputType>(leading_word);

						dest += 1;
						return {length, ErrorCode::NONE};
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}
			}
			else if constexpr (
				InputType == CharsType::UTF32
			)
			{
				// 1-dword UTF-32:
				//	=> 1 LATIN
				//	=> 1/2/3/4 UTF-8
				//	=> 1/2 UTF-16

				std::ignore = end;
				constexpr std::size_t length = 1;

				if constexpr (const auto value = static_cast<std::uint32_t>(*(current + 0));
					OutputType == CharsType::LATIN)
				{
					if constexpr (PureAscii)
					{
						*(dest + 0) = scalar_block::char_of<OutputType>(value);

						dest += 1;
						return {length, ErrorCode::NONE};
					}
					else
					{
						if constexpr (not AssumeAllCorrect)
						{
							if ((value & 0xffff'ff00) != 0)
							{
								return {length, ErrorCode::TOO_LARGE};
							}
						}

						*(dest + 0) = scalar_block::char_of<OutputType>(value);

						dest += 1;
						return {length, ErrorCode::NONE};
					}
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					if constexpr (PureAscii)
					{
						*(dest + 0) = scalar_block::char_of<OutputType>(value);

						dest += 1;
						return {length, ErrorCode::NONE};
					}
					else
					{
						if ((value & 0xffff'ff80) == 0)
						{
							// 1-byte utf8

							*(dest + 0) = scalar_block::char_of<OutputType>(value);

							dest += 1;
							return {length, ErrorCode::NONE};
						}

						if ((value & 0xffff'f800) == 0)
						{
							// 2-bytes utf8

							// 0b110?'???? 0b10??'????
							const auto c1 = static_cast<std::uint32_t>((value >> 6) | 0b1100'0000);
							const auto c2 = static_cast<std::uint32_t>((value & 0b0011'1111) | 0b1000'0000);

							*(dest + 0) = scalar_block::char_of<OutputType>(c1);
							*(dest + 1) = scalar_block::char_of<OutputType>(c2);

							dest += 2;
							return {length, ErrorCode::NONE};
						}

						if ((value & 0xffff'0000) == 0)
						{
							// 3-bytes utf8

							if constexpr (not AssumeAllCorrect)
							{
								if (value >= 0xd800 and value <= 0xdfff)
								{
									return {length, ErrorCode::SURROGATE};
								}
							}

							// 0b1110'???? 0b10??'???? 0b10??'????
							const auto c1 = static_cast<std::uint32_t>((value >> 12) | 0b1110'0000);
							const auto c2 = static_cast<std::uint32_t>(((value >> 6) & 0b0011'1111) | 0b1000'0000);
							const auto c3 = static_cast<std::uint32_t>((value & 0b0011'1111) | 0b1000'0000);

							*(dest + 0) = scalar_block::char_of<OutputType>(c1);
							*(dest + 1) = scalar_block::char_of<OutputType>(c2);
							*(dest + 2) = scalar_block::char_of<OutputType>(c3);

							dest += 3;
							return {length, ErrorCode::NONE};
						}

						// 4-bytes utf8

						if constexpr (not AssumeAllCorrect)
						{
							if (value > 0x0010'ffff)
							{
								return {length, ErrorCode::TOO_LARGE};
							}
						}

						// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
						const auto c1 = static_cast<std::uint32_t>((value >> 18) | 0b1111'0000);
						const auto c2 = static_cast<std::uint32_t>(((value >> 12) & 0b0011'1111) | 0b1000'0000);
						const auto c3 = static_cast<std::uint32_t>(((value >> 6) & 0b0011'1111) | 0b1000'0000);
						const auto c4 = static_cast<std::uint32_t>((value & 0b0011'1111) | 0b1000'0000);

						*(dest + 0) = scalar_block::char_of<OutputType>(c1);
						*(dest + 1) = scalar_block::char_of<OutputType>(c2);
						*(dest + 2) = scalar_block::char_of<OutputType>(c3);
						*(dest + 3) = scalar_block::char_of<OutputType>(c4);

						dest += 4;
						return {length, ErrorCode::NONE};
					}
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE)
				{
					if constexpr (PureAscii)
					{
						*(dest + 0) = scalar_block::char_of<OutputType>(value);

						dest += 1;
						return {length, ErrorCode::NONE};
					}
					else
					{
						if ((value & 0xffff'0000) == 0)
						{
							if constexpr (not AssumeAllCorrect)
							{
								if (value >= 0xd800 and value <= 0xdfff)
								{
									return {length, ErrorCode::SURROGATE};
								}
							}

							*(dest + 0) = scalar_block::char_of<OutputType>(value);

							dest += 1;
							return {length, ErrorCode::NONE};
						}

						// will generate a surrogate pair

						if constexpr (not AssumeAllCorrect)
						{
							if (value > 0x0010'ffff)
							{
								return {length, ErrorCode::TOO_LARGE};
							}
						}

						const auto [high_surrogate, low_surrogate] = [v = value - 0x0001'0000]() noexcept
						{
							const auto high = static_cast<std::uint16_t>(0xd800 + (v >> 10));
							const auto low = static_cast<std::uint16_t>(0xdc00 + (v & 0x3ff));

							return std::make_pair(high, low);
						}();

						*(dest + 0) = scalar_block::char_of<OutputType>(high_surrogate);
						*(dest + 1) = scalar_block::char_of<OutputType>(low_surrogate);

						dest += 2;
						return {length, ErrorCode::NONE};
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

		// ===============================
		// UTF16

		template<std::endian SourceEndian, typename T>
		[[nodiscard]] constexpr static auto utf16_to_native(const T value) noexcept -> std::uint16_t //
			requires (std::is_same_v<T, char16_t> or std::is_same_v<T, std::uint16_t>)
		{
			if constexpr (const auto v16 = static_cast<std::uint16_t>(value);
				SourceEndian != std::endian::native)
			{
				return std::byteswap(v16);
			}
			else
			{
				return v16;
			}
		}
	};
}
