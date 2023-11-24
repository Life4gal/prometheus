// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:scalar.utf8;

import std;
import gal.prometheus.string;
import gal.prometheus.error;
import gal.prometheus.utility;

import :encoding;
import :converter;

namespace gal::prometheus::chars
{
	using input_category = CharsCategory::UTF8;
	using input_type	 = chars::input_type<input_category>;
	using pointer_type	 = input_type::const_pointer;
	using size_type		 = input_type::size_type;

	template<>
	class Scalar<"utf8">
	{
	public:
		[[nodiscard]] constexpr auto validate(const input_type input) const noexcept -> result_type
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			const auto		   input_length		= input.size();

			const pointer_type it_input_begin	= input.data();
			pointer_type	   it_input_current = it_input_begin;
			const pointer_type it_input_end		= it_input_begin + input_length;

			while (it_input_current != it_input_end)
			{
				// check of the next 16 bytes are ascii.
				auto next_it = it_input_current + 16;
				// if it is safe to read 16 more bytes, check that they are ascii.
				if (next_it <= it_input_end)
				{
					const auto v1 = utility::unaligned_load<std::uint64_t>(it_input_current);
					const auto v2 = utility::unaligned_load<std::uint64_t>(it_input_current + sizeof(std::uint64_t));

					if (const auto v = v1 | v2;
						(v & 0x8080808080808080) == 0)
					{
						it_input_current = next_it;
						continue;
					}
				}

				const auto byte = *it_input_current;
				while (byte < 0b10000000)
				{
					it_input_current += 1;

					if (it_input_current == it_input_end) { return {.error = ErrorCode::NONE, .count = input_length}; }
					byte = *it_input_current;
				}

				const auto count_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				if ((byte & 0b11100000) == 0b11000000)
				{
					next_it = it_input_current + 2;

					if (it_input_current > it_input_end) { return {.error = ErrorCode::TOO_SHORT, .count = count_if_error}; }

					const auto next_byte = *(it_input_current + 1);

					if ((next_byte & 0b11000000) != 0b10000000) { return {.error = ErrorCode::TOO_SHORT, .count = count_if_error}; }
					// range check
					if (const auto code_point = (byte & 0b00011111) << 6 | (next_byte & 0b00111111);
						(code_point < 0x80) || (0x7ff < code_point)) { return {.error = ErrorCode::OVERLONG, .count = count_if_error}; }
				}
				else if ((byte & 0b11110000) == 0b11100000)
				{
					next_it = it_input_current + 3;

					if (it_input_current > it_input_end) { return {.error = ErrorCode::TOO_SHORT, .count = count_if_error}; }

					const auto next_byte_1 = *(it_input_current + 1);
					const auto next_byte_2 = *(it_input_current + 2);

					if ((next_byte_1 & 0b11000000) != 0b10000000) { return {.error = ErrorCode::TOO_SHORT, .count = count_if_error}; }
					if ((next_byte_2 & 0b11000000) != 0b10000000) { return {.error = ErrorCode::TOO_SHORT, .count = count_if_error}; }
					// range check
					const auto code_point = (byte & 0b00001111) << 12 |
											(next_byte_1 & 0b00111111) << 6 |
											(next_byte_2 & 0b00111111);
					if ((code_point < 0x800) || (0xffff < code_point)) { return {.error = ErrorCode::OVERLONG, .count = count_if_error}; }
					if (0xd7ff < code_point && code_point < 0xe000) { return {.error = ErrorCode::SURROGATE, .count = count_if_error}; }
				}
				else if ((byte & 0b11111000) == 0b11110000)
				{
					next_it = it_input_current + 4;

					if (it_input_current > it_input_end) { return {.error = ErrorCode::TOO_SHORT, .count = count_if_error}; }

					const auto next_byte_1 = *(it_input_current + 1);
					const auto next_byte_2 = *(it_input_current + 2);
					const auto next_byte_3 = *(it_input_current + 3);

					if ((next_byte_1 & 0b11000000) != 0b10000000) { return {.error = ErrorCode::TOO_SHORT, .count = count_if_error}; }
					if ((next_byte_2 & 0b11000000) != 0b10000000) { return {.error = ErrorCode::TOO_SHORT, .count = count_if_error}; }
					if ((next_byte_3 & 0b11000000) != 0b10000000) { return {.error = ErrorCode::TOO_SHORT, .count = count_if_error}; }
					// range check
					const auto code_point =
							(byte & 0b00000111) << 18 | (next_byte_1 & 0b00111111) << 12 |
							(next_byte_2 & 0b00111111) << 6 | (next_byte_3 & 0b00111111);
					if (code_point <= 0xffff) { return {.error = ErrorCode::OVERLONG, .count = count_if_error}; }
					if (0x10ffff < code_point) { return {.error = ErrorCode::TOO_LARGE, .count = count_if_error}; }
				}
				else
				{
					// we either have too many continuation bytes or an invalid leading byte
					if ((byte & 0b11000000) == 0b10000000) { return {.error = ErrorCode::TOO_LONG, .count = count_if_error}; }
					return {.error = ErrorCode::HEADER_BITS, .count = count_if_error};
				}
			}

			return {.error = ErrorCode::NONE, .count = input_length};
		}

		// Finds the previous leading byte starting backward from input.data() and validates with errors from there.
		// Used to pinpoint the location of an error when an invalid chunk is detected.
		[[nodiscard]] constexpr auto rewind_and_validate(const pointer_type begin, const pointer_type current, const pointer_type end) const noexcept -> result_type
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(begin);
			GAL_PROMETHEUS_DEBUG_NOT_NULL(current);
			GAL_PROMETHEUS_DEBUG_ASSUME(end >= current);
			// A leading byte cannot be further than 4 bytes away
			GAL_PROMETHEUS_DEBUG_ASSUME(current - begin <= 4);

			// First check that we start with a leading byte
			if ((begin[0] & 0b11000000) == 0b10000000) { return {.error = ErrorCode::TOO_LONG, .count = 0}; }

			const auto		   range	   = std::span{begin, current};
			const auto		   extra_count = std::ranges::distance(range | std::views::reverse | std::views::take_while([](const auto byte) noexcept -> bool
																												{ return (byte & 0b11000000) == 0b10000000; }));

			const pointer_type it_current  = current - extra_count;

			auto			   result	   = this->validate({it_current, static_cast<size_type>(end - begin + extra_count)});
			result.count -= extra_count;
			return result;
		}

		template<bool WithError, CharsCategory OutputCategory, bool AssumeValidInput = false>
		[[nodiscard]] constexpr auto convert(const input_type input, typename output_type<OutputCategory>::pointer output) const noexcept -> auto
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
			GAL_PROMETHEUS_DEBUG_NOT_NULL(output);

			using output_char_type						= typename output_type<OutputCategory>::value_type;
			using output_pointer_type					= typename output_type<OutputCategory>::pointer;

			const auto				  input_length		= input.size();

			const pointer_type		  it_input_begin	= input.data();
			pointer_type			  it_input_current	= it_input_begin;
			const pointer_type		  it_input_end		= it_input_begin + input_length;

			const output_pointer_type it_output_begin	= output;
			output_pointer_type		  it_output_current = it_output_begin;

			if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				std::memcpy(it_output_current, it_input_current, input_length);

				if constexpr (
						const auto count = static_cast<std::size_t>(input_length);
						WithError)
				{
					return result_type{.error = ErrorCode::NONE, .count = input_length};
				}
				else
				{
					return input_length;
				}
			}

			while (it_input_current < it_input_end)
			{
				constexpr auto loop_step = []() noexcept -> auto
				{
					if constexpr (AssumeValidInput and OutputCategory != CharsCategory::ASCII)
					{
						return 16;
					}
					else
					{
						return 8;
					}
				}();

				// try to convert the next block of loop_step ASCII bytes
				// if it is safe to read loop_step more bytes, check that they are ascii
				if (it_input_current + loop_step <= it_input_end)
				{
					const auto v = [it_input_current]() noexcept -> auto
					{
						const auto v1 = utility::unaligned_load<std::uint64_t>(it_input_current);
						if constexpr (loop_step == 16)
						{
							const auto v2 = utility::unaligned_load<std::uint64_t>(it_input_current + sizeof(std::uint64_t));
							return v1 | v2;
						}
						else
						{
							return v1;
						}
					}();

					// We are only interested in these bits: 1000'1000'1000'1000...etc
					// if NONE of these are set, e.g. all of them are zero, then everything is ASCII
					if ((v & 0x8080808080808080) == 0)
					{
						std::ranges::transform(
								std::ranges::subrange{it_input_current, it_input_current + loop_step},
								it_output_current,
								[](const auto c) noexcept -> void
								{
									if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
									{
										if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
										{
											return std::byteswap(utility::char_cast<output_char_type>(c));
										}
									}
									return utility::char_cast<output_char_type>(c);
								});

						it_input_current += loop_step;
						it_output_current += loop_step;
						continue;
					}
				}

				const auto count_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				if (const auto leading_byte = *it_input_current;
					leading_byte < 0b1000'0000)
				{
					// converting one ASCII byte
					if constexpr (OutputCategory == CharsCategory::ASCII)
					{
						*it_output_current = utility::char_cast<output_char_type>(leading_byte);
					}
					else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
					{
						*it_output_current = [leading_byte]() noexcept -> auto
						{
							if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
							{
								return std::byteswap(utility::char_cast<output_char_type>(leading_byte));
							}
							else
							{
								return utility::char_cast<output_char_type>(leading_byte);
							}
						}();
					}
					else if constexpr (OutputCategory == CharsCategory::UTF32)
					{
						*it_output_current = utility::char_cast<output_char_type>(leading_byte);
					}
					else
					{
						GAL_PROMETHEUS_STATIC_UNREACHABLE();
					}

					it_input_current += 1;
					it_output_current += 1;
				}
				else if ((leading_byte & 0b1110'0000) == 0b1100'0000)
				{
					// we have a two-byte UTF-8
					if (it_input_current + 1 >= it_input_end)
					{
						// minimal bound checking
						if constexpr (AssumeValidInput)
						{
							break;
						}
						else
						{
							if constexpr (WithError)
							{
								return result_type{.error = ErrorCode::TOO_SHORT, .count = count_if_error};
							}
							else
							{
								return count_if_error;
							}
						}
					}

					const auto next_byte = *(it_input_current + 1);

					if ((next_byte & 0b1100'0000) != 0b1000'0000)
					{
						// checks if the next byte is a valid continuation byte in UTF-8. A valid continuation byte starts with 10.
						if constexpr (AssumeValidInput)
						{
							return static_cast<std::size_t>(0);
						}
						else
						{
							if constexpr (WithError)
							{
								return result_type{.error = ErrorCode::TOO_SHORT, .count = count_if_error};
							}
							else
							{
								return count_if_error;
							}
						}
					}

					// assembles the Unicode code point from the two bytes. It does this by discarding the leading 110 and 10 bits from the two bytes, shifting the remaining bits of the first byte, and then combining the results with a bitwise OR operation.
					const auto code_point = [leading_byte, next_byte]() noexcept -> auto
					{
						const auto cp = static_cast<std::uint32_t>((leading_byte & 0b0001'1111) << 6 | (next_byte & 0b0011'1111));

						if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
						{
							return std::byteswap(cp);
						}
						else
						{
							return cp;
						}
					}();

					if constexpr (not AssumeValidInput)
					{
						if (code_point < 0x80)
						{
							if constexpr (WithError)
							{
								return result_type{.error = ErrorCode::OVERLONG, .count = count_if_error};
							}
							else
							{
								return count_if_error;
							}
						}
						if (code_point >
									[]() noexcept -> auto
							{
								if constexpr (OutputCategory == CharsCategory::ASCII)
								{
									return 0xff;
								}
								else
								{
									return 0x7ff;
								}
							}())
						{
							if constexpr (WithError)
							{
								return result_type{.error = ErrorCode::TOO_LARGE, .count = count_if_error};
							}
							else
							{
								return count_if_error;
							}
						}
					}

					// We only care about the range 129-255 which is Non-ASCII latin1 characters
					*it_output_current = utility::char_cast<output_char_type>(code_point);

					it_input_current += 2;
					it_output_current += 1;
				}
				else if ((leading_byte & 0b1111'0000) == 0b1110'0000)
				{
					// we have a three-byte UTF-8
					if constexpr (OutputCategory == CharsCategory::ASCII)
					{
						if constexpr (AssumeValidInput)
						{
							return static_cast<std::size_t>(0);
						}
						else
						{
							if constexpr (WithError)
							{
								return result_type{.error = ErrorCode::TOO_LARGE, .count = count_if_error};
							}
							else
							{
								return count_if_error;
							}
						}
					}
					else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE or OutputCategory == CharsCategory::UTF32)
					{
						if (it_input_current + 2 >= it_input_end)
						{
							// minimal bound checking
							if constexpr (AssumeValidInput)
							{
								break;
							}
							else
							{
								if constexpr (WithError)
								{
									return result_type{.error = ErrorCode::TOO_SHORT, .count = count_if_error};
								}
								else
								{
									return count_if_error;
								}
							}
						}

						const auto next_byte_1 = *(it_input_current + 1);
						const auto next_byte_2 = *(it_input_current + 2);

						if constexpr (not AssumeValidInput)
						{
							if (
									((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
									((next_byte_2 & 0b1100'0000) != 0b1000'0000))
							{
								if constexpr (WithError)
								{
									return result_type{.error = ErrorCode::TOO_SHORT, .count = count_if_error};
								}
								else
								{
									return count_if_error;
								}
							}
						}

						auto code_point = static_cast<std::uint32_t>(
								(leading_byte & 0b0000'1111) << 12 |
								(next_byte_1 & 0b0011'1111) << 6 |
								(next_byte_2 & 0b0011'1111));

						if constexpr (not AssumeValidInput)
						{
							if (code_point < 0x800)
							{
								if constexpr (WithError)
								{
									return result_type{.error = ErrorCode::OVERLONG, .count = count_if_error};
								}
								else
								{
									return count_if_error;
								}
							}
							if (code_point > 0xffff)
							{
								if constexpr (WithError)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = count_if_error};
								}
								else
								{
									return count_if_error;
								}
							}
							if (code_point > 0xd7ff and code_point < 0xe000)
							{
								if constexpr (WithError)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = count_if_error};
								}
								else
								{
									return count_if_error;
								}
							}
						}

						if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
						{
							if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native != std::endian::little))
							{
								code_point = std::byteswap(code_point);
							}
						}

						*it_output_current = utility::char_cast<output_char_type>(code_point);

						it_input_current += 3;
						it_output_current += 1;
					}
					else
					{
						GAL_PROMETHEUS_STATIC_UNREACHABLE();
					}
				}
				else if ((leading_byte & 0b1111'1000) == 0b1111'0000)
				{
					// we have a 4-byte UTF-8 word.
					if constexpr (OutputCategory == CharsCategory::ASCII)
					{
						if constexpr (AssumeValidInput)
						{
							return static_cast<std::size_t>(0);
						}
						else
						{
							if constexpr (WithError)
							{
								return result_type{.error = ErrorCode::TOO_LARGE, .count = count_if_error};
							}
							else
							{
								return count_if_error;
							}
						}
					}
					else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE or OutputCategory == CharsCategory::UTF32)
					{
						if (it_input_current + 3 >= it_input_end)
						{
							// minimal bound checking
							if constexpr (AssumeValidInput)
							{
								break;
							}
							else
							{
								if constexpr (WithError)
								{
									return result_type{.error = ErrorCode::TOO_SHORT, .count = count_if_error};
								}
								else
								{
									return count_if_error;
								}
							}
						}

						const auto next_byte_1 = *(it_input_current + 1);
						const auto next_byte_2 = *(it_input_current + 2);
						const auto next_byte_3 = *(it_input_current + 3);

						if constexpr (not AssumeValidInput)
						{
							if (
									((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
									((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
									((next_byte_3 & 0b1100'0000) != 0b1000'0000))
							{
								if constexpr (WithError)
								{
									return result_type{.error = ErrorCode::TOO_SHORT, .count = count_if_error};
								}
								else
								{
									return count_if_error;
								}
							}
						}

						auto code_point = static_cast<std::uint32_t>(
								(leading_byte & 0b0000'0111) << 18 |
								(next_byte_1 & 0b0011'1111) << 12 |
								(next_byte_2 & 0b0011'1111) << 6 |
								(next_byte_3 & 0b0011'1111));

						if constexpr (not AssumeValidInput)
						{
							if (code_point <= 0xffff)
							{
								if constexpr (WithError)
								{
									return result_type{.error = ErrorCode::OVERLONG, .count = count_if_error};
								}
								else
								{
									return count_if_error;
								}
							}
							if (code_point > 0x10'ffff)
							{
								if constexpr (WithError)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = count_if_error};
								}
								else
								{
									return count_if_error;
								}
							}
						}

						if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
						{
							code_point -= 0x1'0000;
							const auto [high_surrogate, low_surrogate] = [code_point]() noexcept -> auto
							{
								const auto high = static_cast<std::uint16_t>(0xD800 + (code_point >> 10));
								const auto low	= static_cast<std::uint16_t>(0xDC00 + (code_point & 0x3FF));

								if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
								{
									return std::make_pair(std::byteswap(high), std::byteswap(low));
								}
								else
								{
									return std::make_pair(high, low);
								}
							}();

							*it_output_current = utility::char_cast<output_char_type>(high_surrogate);
							it_output_current += 1;

							*it_output_current = utility::char_cast<output_char_type>(low_surrogate);
							it_output_current += 1;
						}
						else
						{
							*it_output_current = utility::char_cast<output_char_type>(code_point);
						}

						it_input_current += 4;
					}
					else
					{
						GAL_PROMETHEUS_STATIC_UNREACHABLE();
					}
				}
				else
				{
					if constexpr (not AssumeValidInput)
					{
						if ((leading_byte & 0b1100'0000) == 0b1000'0000)
						{
							// we have too many continuation bytes
							if constexpr (WithError)
							{
								return result_type{.error = ErrorCode::TOO_LONG, .count = count_if_error};
							}
							else
							{
								return count_if_error;
							}
						}

						// we have an invalid leading byte
						if constexpr (WithError)
						{
							return result_type{.error = ErrorCode::HEADER_BITS, .count = count_if_error};
						}
						else
						{
							return count_if_error;
						}
					}
					else
					{
						return static_cast<std::size_t>(0);
					}
				}
			}

			if constexpr (const auto count = static_cast<std::size_t>(it_output_current - it_output_begin);
						  WithError)
			{
				return result_type{.error = ErrorCode::NONE, .count = count};
			}
			else
			{
				return count;
			}
		}

		template<CharsCategory OutputCategory>
		[[nodiscard]] constexpr auto rewind_and_convert(const pointer_type furthest_possible_begin, const input_type input, typename output_type<OutputCategory>::pointer output) const noexcept -> result_type
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(furthest_possible_begin);
			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
			GAL_PROMETHEUS_DEBUG_ASSUME(input.data() > furthest_possible_begin);
			GAL_PROMETHEUS_DEBUG_NOT_NULL(output);

			using output_pointer_type					= typename output_type<OutputCategory>::pointer;

			const auto				  input_length		= input.size();

			const pointer_type		  it_input_begin	= input.data();
			pointer_type			  it_input_current	= it_input_begin;
			// const pointer_type		  it_input_end		= it_input_begin + input_length;

			const output_pointer_type it_output_begin	= output;
			output_pointer_type		  it_output_current = it_output_begin;

			const auto				  range				= std::ranges::subrange{std::make_reverse_iterator(it_input_current), std::make_reverse_iterator(furthest_possible_begin)};
			// fixme: no leading bytes?
			const auto				  extra_count		= std::ranges::distance(
					 range |
					 std::views::take_while(
							 [](const auto c) noexcept -> bool
							 {
								 if constexpr (OutputCategory == CharsCategory::ASCII)
								 {
									 return (c & 0b1100'0000) != 0b1000'0000;
								 }
								 else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
								 {
									 return (c & 0b1100'0000) != 0b1000'0000;
								 }
								 else
								 {
									 GAL_PROMETHEUS_STATIC_UNREACHABLE();
								 }
							 }));

			const auto it_current = it_input_begin - extra_count;

			auto	   result	  = this->convert<true, OutputCategory>({it_current, input_length + extra_count}, output);
			if (result.has_error()) { result.count -= extra_count; }

			return result;
		}
	};

	export namespace instance::scalar
	{
		constexpr Scalar<"utf8"> utf8{};
	}
}// namespace gal::prometheus::chars
