// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:scalar.utf8;

import std;
import gal.prometheus.error;
import gal.prometheus.meta;
import gal.prometheus.memory;

import :encoding;

#else
#include <functional>
#include <numeric>

#include <prometheus/macro.hpp>
#include <chars/encoding.ixx>
#include <error/error.ixx>
#include <meta/meta.ixx>
#include <memory/memory.ixx>
#endif

namespace gal::prometheus::chars
{
	namespace scalar_utf8_detail
	{
		template<CharsCategory InputCategory>
		class ScalarUtf8Base
		{
		public:
			constexpr static auto input_category = InputCategory;
			using input_type = chars::input_type<input_category>;
			using char_type = typename input_type::value_type;
			using pointer_type = typename input_type::const_pointer;
			using size_type = typename input_type::size_type;

			template<bool ReturnResultType = false>
			[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				while (it_input_current != it_input_end)
				{
					for (; it_input_current + 16 <= it_input_end; it_input_current += 16)
					{
						const auto v1 = memory::unaligned_load<std::uint64_t>(it_input_current + 0);
						const auto v2 = memory::unaligned_load<std::uint64_t>(it_input_current + sizeof(std::uint64_t));

						if (const auto value = v1 | v2;
							(value & 0x8080'8080'8080'8080) != 0) { break; }
					}

					if (const auto it =
								std::ranges::find_if(
									it_input_current,
									it_input_end,
									[](const auto byte) noexcept { return static_cast<std::uint8_t>(byte) >= 0b1000'0000; });
						it == it_input_end)
					{
						if constexpr (ReturnResultType)
						{
							return result_type{.error = ErrorCode::NONE, .count = input_length};
						}
						else
						{
							return true;
						}
					}
					else { it_input_current = it; }

					const auto count_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if (const auto byte = static_cast<std::uint8_t>(*it_input_current);
						(byte & 0b1110'0000) == 0b1100'0000)
					{
						if (it_input_current + 2 > it_input_end)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}

						const auto next_byte = *(it_input_current + 1);

						if ((next_byte & 0b1100'0000) != 0b1000'0000)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}
						// range check
						if (const auto code_point = (byte & 0b0001'1111) << 6 | (next_byte & 0b0011'1111);
							(code_point < 0x80) || (0x7ff < code_point))
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::OVERLONG, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}

						it_input_current += 2;
					}
					else if ((byte & 0b1111'0000) == 0b1110'0000)
					{
						if (it_input_current + 3 > it_input_end)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}

						const auto next_byte_1 = *(it_input_current + 1);
						const auto next_byte_2 = *(it_input_current + 2);

						if (((next_byte_1 & 0b1100'0000) != 0b1000'0000) or ((next_byte_2 & 0b1100'0000) != 0b1000'0000))
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}
						// range check
						const auto code_point =
								(byte & 0b0000'1111) << 12 |
								(next_byte_1 & 0b0011'1111) << 6 |
								(next_byte_2 & 0b0011'1111);
						if ((code_point < 0x800) || (0xffff < code_point))
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::OVERLONG, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}
						if (0xd7ff < code_point && code_point < 0xe000)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::SURROGATE, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}

						it_input_current += 3;
					}
					else if ((byte & 0b1111'1000) == 0b1111'0000)
					{
						if (it_input_current + 4 > it_input_end)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}

						const auto next_byte_1 = *(it_input_current + 1);
						const auto next_byte_2 = *(it_input_current + 2);
						const auto next_byte_3 = *(it_input_current + 3);

						if (
							((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
							((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
							((next_byte_3 & 0b1100'0000) != 0b1000'0000)
						)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}

						// range check
						const auto code_point =
								(byte & 0b0000'0111) << 18 |
								(next_byte_1 & 0b0011'1111) << 12 |
								(next_byte_2 & 0b0011'1111) << 6 |
								(next_byte_3 & 0b0011'1111);
						if (code_point <= 0xffff)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::OVERLONG, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}
						if (0x10'ffff < code_point)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_LARGE, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}

						it_input_current += 4;
					}
					else
					{
						// we either have too many continuation bytes or an invalid leading byte
						if ((byte & 0b1100'0000) == 0b1000'0000)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_LONG, .count = count_if_error};
							}
							else
							{
								return false;
							}
						}

						if constexpr (ReturnResultType)
						{
							return {.error = ErrorCode::HEADER_BITS, .count = count_if_error};
						}
						else
						{
							return false;
						}
					}
				}

				if constexpr (ReturnResultType)
				{
					return {.error = ErrorCode::NONE, .count = input_length};
				}
				else
				{
					return false;
				}
			}

			template<bool ReturnResultType = false>
			[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
			{
				return validate<ReturnResultType>({input, std::char_traits<char_type>::length(input)});
			}

			// Finds the previous leading byte starting backward from `current` and validates with errors from there.
			// Used to pinpoint the location of an error when an invalid chunk is detected.
			// We assume that the stream starts with a leading byte, and to check that it is the case,
			// we ask that you pass a pointer to the start of the stream (begin).
			[[nodiscard]] constexpr static auto rewind_and_validate(
				const pointer_type begin,
				const pointer_type current,
				const pointer_type end
			) noexcept -> result_type
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(begin);
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current);
				GAL_PROMETHEUS_DEBUG_ASSUME(end >= current);
				GAL_PROMETHEUS_DEBUG_ASSUME(current >= begin);

				// First check that we start with a leading byte
				if ((begin[0] & 0b1100'0000) == 0b1000'0000) { return {.error = ErrorCode::TOO_LONG, .count = 0}; }

				size_type extra_count = 0;
				// A leading byte cannot be further than 4 bytes away
				for (std::ptrdiff_t i = 0; i < 5; ++i)
				{
					if (const auto byte = static_cast<std::uint8_t>(current[-i]);
						(byte & 0b1100'0000) == 0b1000'0000)
					{
						break;
					}
					extra_count += 1;
				}

				const pointer_type it_current = current - extra_count;

				auto result = validate<true>({it_current, static_cast<size_type>(end - begin + extra_count)});
				result.count -= extra_count;
				return result;
			}

			// note: we are not BOM aware
			template<CharsCategory OutputCategory>
			[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

				if constexpr (OutputCategory == CharsCategory::ASCII) { return code_points(input); } // NOLINT(bugprone-branch-clone)
				else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8) { return input.size(); }
				else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE or OutputCategory == CharsCategory::UTF16)
				{
					return std::transform_reduce(
						input.begin(),
						input.end(),
						static_cast<size_type>(0),
						std::plus<>{},
						[](const auto byte) noexcept
						{
							// -65 is 0b10111111
							return (static_cast<std::int8_t>(byte) > -65) //
							       +
							       (static_cast<std::uint8_t>(byte) >= 240) //
									;
						});
				}
				else if constexpr (OutputCategory == CharsCategory::UTF32)
				{
					return code_points(input);
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			// note: we are not BOM aware
			template<CharsCategory OutputCategory>
			[[nodiscard]] constexpr static auto length(const pointer_type input) noexcept -> size_type
			{
				return length<OutputCategory>({input, std::char_traits<char_type>::length(input)});
			}

			template<
				CharsCategory OutputCategory,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
				bool CheckNextBlock = true
			>
			[[nodiscard]] constexpr static auto convert(
				const input_type input,
				typename output_type<OutputCategory>::pointer output
			) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
				GAL_PROMETHEUS_DEBUG_NOT_NULL(output);
				if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
				{
					GAL_PROMETHEUS_DEBUG_ASSUME(validate(input));
				}

				using output_pointer_type = typename output_type<OutputCategory>::pointer;
				using output_char_type = typename output_type<OutputCategory>::value_type;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
				{
					std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
					it_input_current += input_length;
					it_output_current += input_length;
				}
				else
				{
					for (; it_input_current < it_input_end; ++it_input_current)
					{
						if constexpr (CheckNextBlock)
						{
							// fixme: step?
							constexpr auto step = 16;

							// try to convert the next block of `step` ASCII characters
							// if it is safe to read `step` more bytes, check that they are ascii
							if (it_input_current + 16 <= it_input_end)
							{
								static_assert(step % sizeof(std::uint64_t) == 0);

								if (const auto value = [it_input_current]() noexcept
									{
										if constexpr (step == sizeof(std::uint64_t))
										{
											return memory::unaligned_load<std::uint64_t>(it_input_current + 0);
										}
										else if constexpr (step == sizeof(std::uint64_t) * 2)
										{
											const auto v1 = memory::unaligned_load<std::uint64_t>(it_input_current + 0);
											const auto v2 = memory::unaligned_load<std::uint64_t>(it_input_current + sizeof(uint64_t));
											return v1 | v2;
										}
									}();
									(value & 0x8080'8080'8080'8080) == 0)
								{
									std::ranges::transform(
										it_input_current,
										it_input_current + step,
										it_output_current,
										[](const auto byte) noexcept { return static_cast<output_char_type>(byte); });

									// (`step` - 1) more step, see `for (; it_input_current < it_input_end; ++it_input_current)`
									it_input_current += step - 1;
									it_output_current += step;
									continue;
								}
							}
						}

						const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

						// suppose it is not an all ASCII byte sequence
						if (const auto leading_byte = static_cast<std::uint8_t>(*it_input_current);
							leading_byte < 0b1000'0000)
						{
							// converting one ASCII byte
							if constexpr (OutputCategory == CharsCategory::ASCII) { *it_output_current = static_cast<output_char_type>(leading_byte); } // NOLINT(bugprone-branch-clone)
							else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
							}
							else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
							{
								*it_output_current = [leading_byte]() noexcept -> auto
								{
									if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
									{
										return std::byteswap(static_cast<output_char_type>(leading_byte));
									}
									else { return static_cast<output_char_type>(leading_byte); }
								}();
							}
							else if constexpr (OutputCategory == CharsCategory::UTF32)
							{
								*it_output_current = static_cast<output_char_type>(leading_byte);
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }

							it_output_current += 1;
						}
						else if ((leading_byte & 0b1110'0000) == 0b1100'0000)
						{
							// we have a two-byte UTF-8
							// minimal bound checking
							if (it_input_current + 1 >= it_input_end)
							{
								if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
								}
								else if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT) { break; }
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}

							const auto next_byte = *(it_input_current + 1);
							// checks if the next byte is a valid continuation byte in UTF-8. A valid continuation byte starts with 10.
							if ((next_byte & 0b1100'0000) != 0b1000'0000)
							{
								if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
								}
								else if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT) { break; }
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}

							// assembles the Unicode code point from the two bytes. It does this by discarding the leading 110 and 10 bits from the two bytes, shifting the remaining bits of the first byte, and then combining the results with a bitwise OR operation.
							const auto code_point = (leading_byte & 0b0001'1111) << 6 | (next_byte & 0b0011'1111);

							if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
							{
								if (code_point < 0x80)
								{
									if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
									else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
									{
										return result_type{.error = ErrorCode::OVERLONG, .count = length_if_error};
									}
									else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
								}
								if (code_point >
								    []() noexcept -> auto
								    {
									    if constexpr (OutputCategory == CharsCategory::ASCII) { return 0xff; }
									    else { return 0x7ff; }
								    }())
								{
									if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
									else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
									{
										return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
									}
									else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
								}
							}

							*it_output_current = static_cast<output_char_type>(
								[code_point]() noexcept
								{
									if constexpr (
										OutputCategory != CharsCategory::UTF32 and
										(OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little)
									)
									{
										return std::byteswap(code_point);
									}
									else { return code_point; }
								}());

							// one more step, see `for (; it_input_current < it_input_end; ++it_input_current)`
							it_input_current += 1;
							it_output_current += 1;
						}
						else if ((leading_byte & 0b1111'0000) == 0b1110'0000)
						{
							// we have a three-byte UTF-8
							if constexpr (OutputCategory == CharsCategory::ASCII)
							{
								if constexpr (
									ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
									ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT) { return 0; }
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
								}
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}
							else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
							}
							else if constexpr (
								OutputCategory == CharsCategory::UTF16_LE or
								OutputCategory == CharsCategory::UTF16_BE or
								OutputCategory == CharsCategory::UTF32)
							{
								// minimal bound checking
								if (it_input_current + 2 >= it_input_end)
								{
									if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
									else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
									{
										return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
									}
									else if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT) { break; }
									else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
								}

								const auto next_byte_1 = *(it_input_current + 1);
								const auto next_byte_2 = *(it_input_current + 2);

								if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									if (
										((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
										((next_byte_2 & 0b1100'0000) != 0b1000'0000))
									{
										if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
										if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
										{
											return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
										}
										else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
									}
								}

								const auto code_point = static_cast<std::uint32_t>(
									(leading_byte & 0b0000'1111) << 12 |
									(next_byte_1 & 0b0011'1111) << 6 |
									(next_byte_2 & 0b0011'1111));

								if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									const auto do_return = [length_if_error](const auto error) noexcept
									{
										if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
										else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
										{
											return result_type{.error = error, .count = length_if_error};
										}
										else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
									};

									if (code_point < 0x800) { return do_return(ErrorCode::OVERLONG); }
									if (code_point > 0xffff) { return do_return(ErrorCode::TOO_LARGE); }
									if (code_point > 0xd7ff and code_point < 0xe000) { return do_return(ErrorCode::SURROGATE); }
								}

								*it_output_current = [cp = static_cast<output_char_type>(code_point)]() noexcept
								{
									if constexpr (
										OutputCategory != CharsCategory::UTF32 and
										(OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little)
									) { return std::byteswap(cp); }
									else { return cp; }
								}();

								// 2 more step, see `for (; it_input_current < it_input_end; ++it_input_current)`
								it_input_current += 2;
								it_output_current += 1;
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
						else if ((leading_byte & 0b1111'1000) == 0b1111'0000)
						{
							// we have a four-byte UTF-8 word.
							if constexpr (OutputCategory == CharsCategory::ASCII)
							{
								if constexpr (
									ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
									ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT) { return 0; }
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
								}
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}
							else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
							}
							// NOLINT(bugprone-branch-clone)
							else if constexpr (
								OutputCategory == CharsCategory::UTF16_LE or
								OutputCategory == CharsCategory::UTF16_BE or
								OutputCategory == CharsCategory::UTF32)
							{
								// minimal bound checking
								if (it_input_current + 3 >= it_input_end)
								{
									if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
									else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
									{
										return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
									}
									else if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT) { break; }
									else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
								}

								const auto next_byte_1 = *(it_input_current + 1);
								const auto next_byte_2 = *(it_input_current + 2);
								const auto next_byte_3 = *(it_input_current + 3);

								if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									if (
										((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
										((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
										((next_byte_3 & 0b1100'0000) != 0b1000'0000))
									{
										if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
										if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
										{
											return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
										}
										else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
									}
								}

								const auto code_point = static_cast<std::uint32_t>(
									(leading_byte & 0b0000'0111) << 18 |
									(next_byte_1 & 0b0011'1111) << 12 |
									(next_byte_2 & 0b0011'1111) << 6 |
									(next_byte_3 & 0b0011'1111));

								if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									const auto do_return = [length_if_error](const auto error) noexcept
									{
										if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
										else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
										{
											return result_type{.error = error, .count = length_if_error};
										}
										else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
									};

									if (code_point <= 0xffff) { return do_return(ErrorCode::TOO_LARGE); }
									if (code_point > 0x10'ffff) { return do_return(ErrorCode::TOO_LARGE); }
								}

								if constexpr (OutputCategory == CharsCategory::UTF32)
								{
									*it_output_current = static_cast<output_char_type>(code_point);
									it_output_current += 1;
								}
								else
								{
									const auto [high_surrogate, low_surrogate] = [cp = code_point - 0x1'0000]() noexcept -> auto
									{
										const auto high = static_cast<std::uint16_t>(0xd800 + (cp >> 10));
										const auto low = static_cast<std::uint16_t>(0xdc00 + (cp & 0x3ff));

										if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
										{
											return std::make_pair(std::byteswap(high), std::byteswap(low));
										}
										else { return std::make_pair(high, low); }
									}();

									*it_output_current = static_cast<output_char_type>(high_surrogate);
									it_output_current += 1;

									*it_output_current = static_cast<output_char_type>(low_surrogate);
									it_output_current += 1;
								}

								// 3 more step, see `for (; it_input_current < it_input_end; ++it_input_current)`
								it_input_current += 3;
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
						else
						{
							if constexpr (
								ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
								ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
							) { return 0; }
							else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
							{
								if ((leading_byte & 0b1100'0000) == 0b1000'0000)
								{
									// we have too many continuation bytes
									return result_type{.error = ErrorCode::TOO_LONG, .count = length_if_error};
								}
								// we have an invalid leading byte
								return result_type{.error = ErrorCode::HEADER_BITS, .count = length_if_error};
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
					}
				}

				if constexpr (
					ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
					ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
				) { return static_cast<std::size_t>(it_output_current - it_output_begin); }
				else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
				{
					return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			template<
				CharsCategory OutputCategory,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
				bool CheckNextBlock = true
			>
			[[nodiscard]] constexpr static auto convert(
				const pointer_type input,
				typename output_type<OutputCategory>::pointer output
			) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
			{
				return convert<OutputCategory, ProcessPolicy, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, output);
			}

			template<
				typename StringType,
				CharsCategory OutputCategory,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
				bool CheckNextBlock = true
			>
				requires requires(StringType& string)
				{
					string.resize(std::declval<size_type>());
					{
						string.data()
					} -> std::convertible_to<typename output_type<OutputCategory>::pointer>;
				}
			[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> StringType
			{
				StringType result{};
				result.resize(length<OutputCategory>(input));

				(void)convert<OutputCategory, ProcessPolicy, CheckNextBlock>(input, result.data());
				return result;
			}

			template<
				typename StringType,
				CharsCategory OutputCategory,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
				bool CheckNextBlock = true
			>
				requires requires(StringType& string)
				{
					string.resize(std::declval<size_type>());
					{
						string.data()
					} -> std::convertible_to<typename output_type<OutputCategory>::pointer>;
				}
			[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> StringType
			{
				StringType result{};
				result.resize(length<OutputCategory>(input));

				return convert<OutputCategory, ProcessPolicy, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, result.data());
			}

			template<
				CharsCategory OutputCategory,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
				bool CheckNextBlock = true
			>
			[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
			{
				std::basic_string<typename output_type<OutputCategory>::value_type> result{};
				result.resize(length<OutputCategory>(input));

				(void)convert<OutputCategory, ProcessPolicy, CheckNextBlock>(input, result.data());
				return result;
			}

			template<
				CharsCategory OutputCategory,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
				bool CheckNextBlock = true
			>
			[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
			{
				std::basic_string<typename output_type<OutputCategory>::value_type> result{};
				result.resize(length<OutputCategory>(input));

				return convert<OutputCategory, ProcessPolicy, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, result.data());
			}

			template<CharsCategory OutputCategory>
			[[nodiscard]] constexpr static auto rewind_and_convert(
				const pointer_type furthest_possible_begin,
				const input_type input,
				typename output_type<OutputCategory>::pointer output
			) noexcept -> result_type
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(furthest_possible_begin);
				GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
				GAL_PROMETHEUS_DEBUG_ASSUME(input.data() >= furthest_possible_begin);
				// fixme
				GAL_PROMETHEUS_DEBUG_ASSUME(furthest_possible_begin - input.data() <= 3);
				GAL_PROMETHEUS_DEBUG_NOT_NULL(output);

				// using output_pointer_type = typename output_type<OutputCategory>::pointer;
				// using output_char_type = typename output_type<OutputCategory>::value_type;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				// pointer_type it_input_current = it_input_begin;
				// const pointer_type it_input_end = it_input_begin + input_length;

				// const output_pointer_type it_output_begin = output;
				// output_pointer_type it_output_current = it_output_begin;

				// const auto range = std::ranges::subrange{std::make_reverse_iterator(it_input_current), std::make_reverse_iterator(furthest_possible_begin)};
				const auto range = std::ranges::subrange{furthest_possible_begin, it_input_begin + 1} | std::views::reverse;
				// fixme: no leading bytes?
				const auto extra_count = std::ranges::distance(
					range |
					std::views::take_while([](const auto c) noexcept -> bool { return (c & 0b1100'0000) != 0b1000'0000; })
				);

				const auto it_current = it_input_begin - extra_count;

				auto result = convert<OutputCategory, InputProcessPolicy::RETURN_RESULT_TYPE>({it_current, input_length + extra_count}, output);
				if (result.has_error()) { result.count -= extra_count; }

				return result;
			}

			[[nodiscard]] constexpr static auto code_points(const input_type input) noexcept -> std::size_t
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

				return std::ranges::count_if(
					input,
					[](const auto byte) noexcept -> bool
					{
						const auto b = static_cast<std::int8_t>(byte);

						// -65 is 0b10111111, anything larger in two-complement's should start a new code point.
						return b > -65;
					});
			}
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	template<>
	class Scalar<"utf8"> : public scalar_utf8_detail::ScalarUtf8Base<CharsCategory::UTF8> {};

	template<>
	class Scalar<"utf8_char"> : public scalar_utf8_detail::ScalarUtf8Base<CharsCategory::UTF8_CHAR> {};

	template<>
	struct scalar_processor_of<CharsCategory::UTF8>
	{
		using type = Scalar<"utf8">;
	};

	template<>
	struct scalar_processor_of<CharsCategory::UTF8_CHAR>
	{
		using type = Scalar<"utf8_char">;
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
} // namespace gal::prometheus::chars
