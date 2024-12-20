// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <bit>
#include <functional>
#include <numeric>
#include <string>
#include <algorithm>

#include <prometheus/macro.hpp>

#include <chars/encoding.hpp>

#include <memory/rw.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::chars
{
	namespace scalar_utf8_detail
	{
		template<CharsType Type>
		class Scalar
		{
		public:
			constexpr static auto chars_type = Type;

			using input_type = input_type_of<chars_type>;
			using char_type = typename input_type::value_type;
			using pointer_type = typename input_type::const_pointer;
			using size_type = typename input_type::size_type;

		private:
			using data_type = std::uint64_t;

			constexpr static std::size_t size_per_char = sizeof(char_type);
			constexpr static std::size_t advance_per_step = sizeof(data_type) / size_per_char;

		public:
			template<bool ReturnResultType = false>
			[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				while (it_input_current != it_input_end)
				{
					// if it is safe to read 16 more bytes, check that they are latin
					for (constexpr auto step = 2 * advance_per_step; it_input_current + step <= it_input_end; it_input_current += step)
					{
						const auto v1 = memory::unaligned_load<data_type>(it_input_current + 0 * advance_per_step);
						const auto v2 = memory::unaligned_load<data_type>(it_input_current + 1 * advance_per_step);

						if (const auto value = v1 | v2;
							(value & 0x8080'8080'8080'8080) != 0)
						{
							break;
						}
					}

					if (it_input_current =
					    std::ranges::find_if(
						    it_input_current,
						    it_input_end,
						    [](const auto byte) noexcept
						    {
							    const auto b = static_cast<std::uint8_t>(byte);
							    // ASCII ONLY
							    return b >= 0b1000'0000;
						    }
					    );
						it_input_current == it_input_end
					)
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

					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if (const auto leading_byte = static_cast<std::uint8_t>(*it_input_current);
						(leading_byte & 0b1110'0000) == 0b1100'0000)
					{
						// we have a two-byte UTF-8

						// minimal bound checking
						if (it_input_current + 1 >= it_input_end)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = length_if_error};
							}
							else
							{
								return false;
							}
						}

						const auto next_byte = static_cast<std::uint8_t>(*(it_input_current + 1));

						if ((next_byte & 0b1100'0000) != 0b1000'0000)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = length_if_error};
							}
							else
							{
								return false;
							}
						}

						// range check
						const auto code_point = static_cast<std::uint32_t>(leading_byte & 0b0001'1111) << 6 | (next_byte & 0b0011'1111);

						if (code_point < 0x80)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::OVERLONG, .count = length_if_error};
							}
							else
							{
								return false;
							}
						}
						if (code_point > 0x7ff)
						{
							if constexpr (ReturnResultType)
							{
								return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
							}
							else
							{
								return false;
							}
						}

						it_input_current += 2;
					}
					else if ((leading_byte & 0b1111'0000) == 0b1110'0000)
					{
						// we have a three-byte UTF-8

						// minimal bound checking
						if (it_input_current + 2 >= it_input_end)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = length_if_error};
							}
							else
							{
								return false;
							}
						}

						const auto next_byte_1 = static_cast<std::uint8_t>(*(it_input_current + 1));
						const auto next_byte_2 = static_cast<std::uint8_t>(*(it_input_current + 2));

						if (
							((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
							((next_byte_2 & 0b1100'0000) != 0b1000'0000)
						)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = length_if_error};
							}
							else
							{
								return false;
							}
						}

						// range check
						const auto code_point = static_cast<std::uint32_t>(
							(leading_byte & 0b0000'1111) << 12 |
							(next_byte_1 & 0b0011'1111) << 6 |
							(next_byte_2 & 0b0011'1111)
						);

						const auto do_return = [length_if_error](const auto error) noexcept
						{
							if constexpr (ReturnResultType)
							{
								return result_type{.error = error, .count = length_if_error};
							}
							else
							{
								std::ignore = error;
								return false;
							}
						};

						if (code_point < 0x800)
						{
							return do_return(ErrorCode::OVERLONG);
						}
						if (code_point > 0xffff)
						{
							return do_return(ErrorCode::TOO_LARGE);
						}
						if (code_point > 0xd7ff and code_point < 0xe000)
						{
							return do_return(ErrorCode::SURROGATE);
						}

						it_input_current += 3;
					}
					else if ((leading_byte & 0b1111'1000) == 0b1111'0000)
					{
						// we have a four-byte UTF-8 word.

						// minimal bound checking
						if (it_input_current + 3 >= it_input_end)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = length_if_error};
							}
							else
							{
								return false;
							}
						}

						const auto next_byte_1 = static_cast<std::uint8_t>(*(it_input_current + 1));
						const auto next_byte_2 = static_cast<std::uint8_t>(*(it_input_current + 2));
						const auto next_byte_3 = static_cast<std::uint8_t>(*(it_input_current + 3));

						if (
							((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
							((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
							((next_byte_3 & 0b1100'0000) != 0b1000'0000)
						)
						{
							if constexpr (ReturnResultType)
							{
								return {.error = ErrorCode::TOO_SHORT, .count = length_if_error};
							}
							else
							{
								return false;
							}
						}

						// range check
						const auto code_point = static_cast<std::uint32_t>(
							(leading_byte & 0b0000'0111) << 18 |
							(next_byte_1 & 0b0011'1111) << 12 |
							(next_byte_2 & 0b0011'1111) << 6 |
							(next_byte_3 & 0b0011'1111)
						);

						const auto do_return = [length_if_error](const auto error) noexcept
						{
							if constexpr (ReturnResultType)
							{
								return result_type{.error = error, .count = length_if_error};
							}
							else
							{
								std::ignore = error;
								return false;
							}
						};

						if (code_point <= 0xffff)
						{
							return do_return(ErrorCode::OVERLONG);
						}
						if (code_point > 0x10'ffff)
						{
							return do_return(ErrorCode::TOO_LARGE);
						}

						it_input_current += 4;
					}
					else
					{
						// we either have too many continuation bytes or an invalid leading byte
						if ((leading_byte & 0b1100'0000) == 0b1000'0000)
						{
							if constexpr (ReturnResultType)
							{
								// we have too many continuation bytes
								return {.error = ErrorCode::TOO_LONG, .count = length_if_error};
							}
							else
							{
								return false;
							}
						}

						if constexpr (ReturnResultType)
						{
							// we have an invalid leading byte
							return {.error = ErrorCode::HEADER_BITS, .count = length_if_error};
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
					return true;
				}
			}

			template<bool ReturnResultType = false>
			[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
			{
				return Scalar::validate<ReturnResultType>({input, std::char_traits<char_type>::length(input)});
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
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(begin != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(end >= current);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current >= begin);

				// First check that we start with a leading byte
				if ((begin[0] & 0b1100'0000) == 0b1000'0000)
				{
					return {.error = ErrorCode::TOO_LONG, .count = 0};
				}

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

				auto result = Scalar::validate<true>({it_current, static_cast<size_type>(end - begin + extra_count)});
				result.count -= extra_count;
				return result;
			}

			// note: we are not BOM aware
			template<CharsType OutputType>
			[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				// ReSharper disable CppClangTidyBugproneBranchClone
				if constexpr (OutputType == CharsType::LATIN)
				{
					return Scalar::code_points(input);
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					return input.size();
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
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
						}
					);
				}
				// ReSharper disable CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return Scalar::code_points(input);
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			// note: we are not BOM aware
			template<CharsType OutputCategory>
			[[nodiscard]] constexpr static auto length(const pointer_type input) noexcept -> size_type
			{
				return Scalar::length<OutputCategory>({input, std::char_traits<char_type>::length(input)});
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
			>
			[[nodiscard]] constexpr static auto convert(
				const input_type input,
				typename output_type_of<OutputType>::pointer output
			) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);
				if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(Scalar::validate(input));
				}

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;
				using output_char_type = typename output_type::value_type;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
					it_input_current += input_length;
					it_output_current += input_length;
				}
				else
				{
					while (it_input_current < it_input_end)
					{
						// if it is safe to read 16 more bytes, check that they are ascii
						if (constexpr auto step = 2 * advance_per_step;
							it_input_current + step <= it_input_end)
						{
							const auto v1 = memory::unaligned_load<data_type>(it_input_current + 0 * advance_per_step);
							const auto v2 = memory::unaligned_load<data_type>(it_input_current + 1 * advance_per_step);

							if (const auto value = v1 | v2;
								(value & 0x8080'8080'8080'8080) == 0)
							{
								std::ranges::transform(
									it_input_current,
									it_input_current + step,
									it_output_current,
									[](const auto byte) noexcept
									{
										const auto b = static_cast<std::uint8_t>(byte);
										if constexpr (const auto data = static_cast<output_char_type>(b);
											(OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE) and
											(OutputType == CharsType::UTF16_LE) != (std::endian::native == std::endian::little)
										)
										{
											return std::byteswap(data);
										}
										else
										{
											return data;
										}
									}
								);

								it_input_current += step;
								it_output_current += step;
								continue;
							}
						}

						const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

						// suppose it is not an all ASCII byte sequence
						if (const auto leading_byte = static_cast<std::uint8_t>(*it_input_current);
							leading_byte < 0b1000'0000)
						{
							// converting one ASCII byte

							// ReSharper disable CppClangTidyBugproneBranchClone
							if constexpr (OutputType == CharsType::LATIN)
							{
								*it_output_current = static_cast<output_char_type>(leading_byte);
							}
							// ReSharper restore CppClangTidyBugproneBranchClone
							else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
							}
							else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE)
							{
								*it_output_current = [leading_byte]() noexcept -> auto
								{
									if constexpr ((OutputType == CharsType::UTF16_LE) != (std::endian::native == std::endian::little))
									{
										return std::byteswap(static_cast<output_char_type>(leading_byte));
									}
									else
									{
										return static_cast<output_char_type>(leading_byte);
									}
								}();
							}
							// ReSharper disable CppClangTidyBugproneBranchClone
							else if constexpr (OutputType == CharsType::UTF32)
							{
								*it_output_current = static_cast<output_char_type>(leading_byte);
							}
							// ReSharper restore CppClangTidyBugproneBranchClone
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }

							it_input_current += 1;
							it_output_current += 1;
						}
						else if ((leading_byte & 0b1110'0000) == 0b1100'0000)
						{
							// we have a two-byte UTF-8

							// minimal bound checking
							if (it_input_current + 1 >= it_input_end)
							{
								if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
								}
								else if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									break;
								}
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}

							const auto next_byte = static_cast<std::uint8_t>(*(it_input_current + 1));

							// checks if the next byte is a valid continuation byte in UTF-8.
							// A valid continuation byte starts with 10.
							if ((next_byte & 0b1100'0000) != 0b1000'0000)
							{
								if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
								}
								else if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									break;
								}
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}

							// assembles the Unicode code point from the two bytes.
							// It does this by discarding the leading 110 and 10 bits from the two bytes,
							// shifting the remaining bits of the first byte,
							// and then combining the results with a bitwise OR operation.
							const auto code_point = static_cast<std::uint32_t>(leading_byte & 0b0001'1111) << 6 | (next_byte & 0b0011'1111);

							if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
							{
								if (code_point < 0x80)
								{
									if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
									{
										return 0;
									}
									else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
									{
										return result_type{.error = ErrorCode::OVERLONG, .count = length_if_error};
									}
									else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
								}
								if (code_point >
								    []() noexcept -> std::uint32_t
								    {
									    if constexpr (OutputType == CharsType::LATIN) { return 0xff; }
									    else { return 0x7ff; }
								    }()
								)
								{
									if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
									{
										return 0;
									}
									else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
									{
										return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
									}
									else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
								}
							}

							// ReSharper disable CppClangTidyBugproneBranchClone
							if constexpr (OutputType == CharsType::LATIN)
							{
								*it_output_current = static_cast<output_char_type>(code_point);
							}
							// ReSharper restore CppClangTidyBugproneBranchClone
							else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
							}
							else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE)
							{
								*it_output_current = [code_point]() noexcept -> auto
								{
									if constexpr ((OutputType == CharsType::UTF16_LE) != (std::endian::native == std::endian::little))
									{
										return std::byteswap(static_cast<output_char_type>(code_point));
									}
									else { return static_cast<output_char_type>(code_point); }
								}();
							}
							// ReSharper disable CppClangTidyBugproneBranchClone
							else if constexpr (OutputType == CharsType::UTF32)
							{
								*it_output_current = static_cast<output_char_type>(code_point);
							}
							// ReSharper restore CppClangTidyBugproneBranchClone
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }

							it_input_current += 2;
							it_output_current += 1;
						}
						else if ((leading_byte & 0b1111'0000) == 0b1110'0000)
						{
							// we have a three-byte UTF-8

							if constexpr (OutputType == CharsType::LATIN)
							{
								if constexpr (
									ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
									ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
								)
								{
									return 0;
								}
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
								}
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}
							else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
							}
							else if constexpr (
								OutputType == CharsType::UTF16_LE or
								OutputType == CharsType::UTF16_BE or
								OutputType == CharsType::UTF32
							)
							{
								// minimal bound checking
								if (it_input_current + 2 >= it_input_end)
								{
									if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
									{
										return 0;
									}
									else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
									{
										return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
									}
									else if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
									{
										break;
									}
									else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
								}

								const auto next_byte_1 = static_cast<std::uint8_t>(*(it_input_current + 1));
								const auto next_byte_2 = static_cast<std::uint8_t>(*(it_input_current + 2));

								if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									if (
										((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
										((next_byte_2 & 0b1100'0000) != 0b1000'0000)
									)
									{
										if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
										{
											return 0;
										}
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
									(next_byte_2 & 0b0011'1111)
								);

								if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									const auto do_return = [length_if_error](const auto error) noexcept
									{
										if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
										{
											std::ignore = error;
											return 0;
										}
										else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
										{
											return result_type{.error = error, .count = length_if_error};
										}
										else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
									};

									if (code_point < 0x800)
									{
										return do_return(ErrorCode::OVERLONG);
									}
									if (code_point > 0xffff)
									{
										return do_return(ErrorCode::TOO_LARGE);
									}
									if (code_point > 0xd7ff and code_point < 0xe000)
									{
										return do_return(ErrorCode::SURROGATE);
									}
								}

								*it_output_current = [cp = static_cast<output_char_type>(code_point)]() noexcept
								{
									if constexpr (
										(OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE) and
										(OutputType == CharsType::UTF16_LE) != (std::endian::native == std::endian::little)
									)
									{
										return std::byteswap(cp);
									}
									else
									{
										return cp;
									}
								}();

								it_input_current += 3;
								it_output_current += 1;
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
						else if ((leading_byte & 0b1111'1000) == 0b1111'0000)
						{
							// we have a four-byte UTF-8 word.

							if constexpr (OutputType == CharsType::LATIN)
							{
								if constexpr (
									ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
									ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
								)
								{
									return 0;
								}
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
								}
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}
							else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
							}
							// NOLINT(bugprone-branch-clone)
							else if constexpr (
								OutputType == CharsType::UTF16_LE or
								OutputType == CharsType::UTF16_BE or
								OutputType == CharsType::UTF32
							)
							{
								// minimal bound checking
								if (it_input_current + 3 >= it_input_end)
								{
									if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
									{
										return 0;
									}
									else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
									{
										return result_type{.error = ErrorCode::TOO_SHORT, .count = length_if_error};
									}
									else if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
									{
										break;
									}
									else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
								}

								const auto next_byte_1 = static_cast<std::uint8_t>(*(it_input_current + 1));
								const auto next_byte_2 = static_cast<std::uint8_t>(*(it_input_current + 2));
								const auto next_byte_3 = static_cast<std::uint8_t>(*(it_input_current + 3));

								if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									if (
										((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
										((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
										((next_byte_3 & 0b1100'0000) != 0b1000'0000)
									)
									{
										if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
										{
											return 0;
										}
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
									(next_byte_3 & 0b0011'1111)
								);

								if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									const auto do_return = [length_if_error](const auto error) noexcept
									{
										if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
										{
											std::ignore = error;
											return 0;
										}
										else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
										{
											return result_type{.error = error, .count = length_if_error};
										}
										else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
									};

									if (code_point <= 0xffff)
									{
										return do_return(ErrorCode::OVERLONG);
									}
									if (code_point > 0x10'ffff)
									{
										return do_return(ErrorCode::TOO_LARGE);
									}
								}

								if constexpr (OutputType == CharsType::UTF32)
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

										if constexpr ((OutputType == CharsType::UTF16_LE) != (std::endian::native == std::endian::little))
										{
											return std::make_pair(std::byteswap(high), std::byteswap(low));
										}
										else
										{
											return std::make_pair(high, low);
										}
									}();

									*it_output_current = static_cast<output_char_type>(high_surrogate);
									it_output_current += 1;

									*it_output_current = static_cast<output_char_type>(low_surrogate);
									it_output_current += 1;
								}

								it_input_current += 4;
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
						else
						{
							if constexpr (
								ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
								ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
							)
							{
								return 0;
							}
							else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
							{
								// we either have too many continuation bytes or an invalid leading byte
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
				)
				{
					return static_cast<std::size_t>(it_output_current - it_output_begin);
				}
				else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
				{
					return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
			>
			[[nodiscard]] constexpr static auto convert(
				const pointer_type input,
				typename output_type_of<OutputType>::pointer output
			) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
			{
				return Scalar::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, output);
			}

			template<
				typename StringType,
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
			>
				requires requires(StringType& string)
				{
					string.resize(std::declval<size_type>());
					{
						string.data()
					} -> std::convertible_to<typename output_type_of<OutputType>::pointer>;
				}
			[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> StringType
			{
				StringType result{};
				result.resize(length<OutputType>(input));

				std::ignore = Scalar::convert<OutputType, ProcessPolicy>(input, result.data());
				return result;
			}

			template<
				typename StringType,
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
			>
				requires requires(StringType& string)
				{
					string.resize(std::declval<size_type>());
					{
						string.data()
					} -> std::convertible_to<typename output_type_of<OutputType>::pointer>;
				}
			[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> StringType
			{
				StringType result{};
				result.resize(length<OutputType>(input));

				return Scalar::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
			>
			[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
			{
				std::basic_string<typename output_type_of<OutputType>::value_type> result{};
				result.resize(length<OutputType>(input));

				std::ignore = Scalar::convert<OutputType, ProcessPolicy>(input, result.data());
				return result;
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
			>
			[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
			{
				std::basic_string<typename output_type_of<OutputType>::value_type> result{};
				result.resize(length<OutputType>(input));

				return Scalar::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
			}

			template<CharsType OutputType>
			[[nodiscard]] constexpr static auto rewind_and_convert(
				const pointer_type furthest_possible_begin,
				const input_type input,
				typename output_type_of<OutputType>::pointer output
			) noexcept -> result_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(furthest_possible_begin != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() >= furthest_possible_begin);
				// fixme
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(furthest_possible_begin - input.data() <= 3);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				// using output_pointer_type = typename output_type<OutputType>::pointer;
				// using output_char_type = typename output_type<OutputType>::value_type;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				// pointer_type it_input_current = it_input_begin;
				// const pointer_type it_input_end = it_input_begin + input_length;

				// const output_pointer_type it_output_begin = output;
				// output_pointer_type it_output_current = it_output_begin;

				// const auto range = std::ranges::subrange{std::make_reverse_iterator(it_input_current), std::make_reverse_iterator(furthest_possible_begin)};
				const auto range = std::ranges::subrange{furthest_possible_begin, it_input_begin} | std::views::reverse;
				// fixme: no leading bytes?
				const auto extra_count = std::ranges::distance(
					range |
					std::views::take_while([](const auto c) noexcept -> bool
					{
						return (c & 0b1100'0000) != 0b1000'0000;
					})
				);

				const auto it_current = it_input_begin - extra_count;

				auto result = Scalar::convert<OutputType, InputProcessPolicy::RETURN_RESULT_TYPE>({it_current, input_length + extra_count}, output);
				if (result.has_error())
				{
					result.count -= extra_count;
				}

				return result;
			}

			[[nodiscard]] constexpr static auto code_points(const input_type input) noexcept -> std::size_t
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				return std::ranges::count_if(
					input,
					[](const auto byte) noexcept -> bool
					{
						const auto b = static_cast<std::int8_t>(byte);

						// -65 is 0b10111111, anything larger in two-complement's should start a new code point.
						return b > -65;
					}
				);
			}
		};
	}

	template<>
	class Scalar<"utf8"> : public scalar_utf8_detail::Scalar<CharsType::UTF8> {};

	template<>
	class Scalar<"utf8.char"> : public scalar_utf8_detail::Scalar<CharsType::UTF8_CHAR> {};

	template<>
	class Scalar<"utf8_char"> : public scalar_utf8_detail::Scalar<CharsType::UTF8_CHAR> {};
}
