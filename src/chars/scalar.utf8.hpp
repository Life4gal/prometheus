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

			// Finds the previous leading byte starting backward from `current` and validates with errors from there.
			// Used to pinpoint the location of an error when an invalid chunk is detected.
			// We assume that the stream starts with a leading byte, and to check that it is the case,
			// we ask that you pass a pointer to the start of the stream (begin).
			[[nodiscard]] constexpr static auto rewind_and_validate(
				const pointer_type begin,
				const pointer_type current,
				const pointer_type end
			) noexcept -> auto
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
				result.input -= extra_count;
				return result;
			}

			template<CharsType OutputType>
			[[nodiscard]] constexpr static auto rewind_and_convert(
				const pointer_type furthest_possible_begin,
				const input_type input,
				typename output_type_of<OutputType>::pointer output
			) noexcept -> auto
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
					std::views::take_while(
						[](const auto c) noexcept -> bool
						{
							return (c & 0b1100'0000) != 0b1000'0000;
						}
					)
				);

				const auto it_current = it_input_begin - extra_count;

				auto result = Scalar::convert<OutputType, InputProcessPolicy::DEFAULT>({it_current, input_length + extra_count}, output);
				if (result.has_error())
				{
					result.input -= extra_count;
				}

				return result;
			}

		public:
			template<bool Detail = false>
			[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> auto
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				constexpr auto process_policy = Detail ? InputProcessPolicy::DEFAULT : InputProcessPolicy::RESULT;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				// n is the minimum number of characters that need to be processed.
				// Actual processing may exceed this number (depending on the UTF8 character length)
				const auto check = [
							// Used to calculate the processed input length
							it_input_begin,
							&it_input_current,
							// Used to determine the length of the current character if it is a correct UTF8 character
							it_input_end
						](const size_type n) noexcept -> auto
				{
					// [error/input/output]
					constexpr auto process_policy_keep_all_result = InputProcessPolicy::WRITE_ALL_CORRECT_2;

					const auto end = it_input_current + n;

					while (it_input_current < end)
					{
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						constexpr auto current_output_length = static_cast<std::size_t>(0);

						if (const auto leading_byte = static_cast<std::uint8_t>(*it_input_current);
							(leading_byte & 0x80) == 0)
						{
							// ascii
							it_input_current += 1;
						}
						else if ((leading_byte & 0b1110'0000) == 0b1100'0000)
						{
							// we have a two-byte UTF-8

							// minimal bound checking
							if (it_input_current + 1 >= it_input_end)
							{
								return make_result<process_policy_keep_all_result>(
									ErrorCode::TOO_SHORT,
									current_input_length,
									current_output_length
								);
							}

							const auto next_byte = static_cast<std::uint8_t>(*(it_input_current + 1));

							if ((next_byte & 0b1100'0000) != 0b1000'0000)
							{
								return make_result<process_policy_keep_all_result>(
									ErrorCode::TOO_SHORT,
									current_input_length,
									current_output_length
								);
							}

							// range check
							const auto code_point = static_cast<std::uint32_t>(leading_byte & 0b0001'1111) << 6 | (next_byte & 0b0011'1111);

							if (code_point < 0x80)
							{
								return make_result<process_policy_keep_all_result>(
									ErrorCode::OVERLONG,
									current_input_length,
									current_output_length
								);
							}
							if (code_point > 0x7ff)
							{
								return make_result<process_policy_keep_all_result>(
									ErrorCode::TOO_LARGE,
									current_input_length,
									current_output_length
								);
							}

							it_input_current += 2;
						}
						else if ((leading_byte & 0b1111'0000) == 0b1110'0000)
						{
							// we have a three-byte UTF-8

							// minimal bound checking
							if (it_input_current + 2 >= it_input_end)
							{
								return make_result<process_policy_keep_all_result>(
									ErrorCode::TOO_SHORT,
									current_input_length,
									current_output_length
								);
							}

							const auto next_byte_1 = static_cast<std::uint8_t>(*(it_input_current + 1));
							const auto next_byte_2 = static_cast<std::uint8_t>(*(it_input_current + 2));

							if (
								((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
								((next_byte_2 & 0b1100'0000) != 0b1000'0000)
							)
							{
								return make_result<process_policy_keep_all_result>(
									ErrorCode::TOO_SHORT,
									current_input_length,
									current_output_length
								);
							}

							// range check
							const auto code_point = static_cast<std::uint32_t>(
								(leading_byte & 0b0000'1111) << 12 |
								(next_byte_1 & 0b0011'1111) << 6 |
								(next_byte_2 & 0b0011'1111)
							);

							const auto do_return = [current_input_length](const auto error) noexcept
							{
								return make_result<process_policy_keep_all_result>(
									error,
									current_input_length,
									current_output_length
								);
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
								return make_result<process_policy_keep_all_result>(
									ErrorCode::TOO_SHORT,
									current_input_length,
									current_output_length
								);
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
								return make_result<process_policy_keep_all_result>(
									ErrorCode::TOO_SHORT,
									current_input_length,
									current_output_length
								);
							}

							// range check
							const auto code_point = static_cast<std::uint32_t>(
								(leading_byte & 0b0000'0111) << 18 |
								(next_byte_1 & 0b0011'1111) << 12 |
								(next_byte_2 & 0b0011'1111) << 6 |
								(next_byte_3 & 0b0011'1111)
							);

							const auto do_return = [current_input_length](const auto error) noexcept
							{
								return make_result<process_policy_keep_all_result>(
									error,
									current_input_length,
									current_output_length
								);
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
								// we have too many continuation bytes
								return make_result<process_policy_keep_all_result>(
									ErrorCode::TOO_LONG,
									current_input_length,
									current_output_length
								);
							}

							// we have an invalid leading byte
							return make_result<process_policy_keep_all_result>(
								ErrorCode::HEADER_BITS,
								current_input_length,
								current_output_length
							);
						}
					}

					// ==================================================
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current >= end);
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					constexpr auto current_output_length = static_cast<std::size_t>(0);
					return make_result<process_policy_keep_all_result>(
						ErrorCode::NONE,
						current_input_length,
						current_output_length
					);
				};

				constexpr auto step = 1 * advance_per_step;
				// 8 bytes
				while (it_input_current + step <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, step};
					#endif

					if (const auto value = memory::unaligned_load<data_type>(it_input_current + 0 * advance_per_step);
						(value & 0x8080'8080'8080'8080) != 0)
					{
						// MSB => LSB
						const auto msb = (value >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);

						const auto packed = msb * static_cast<data_type>(0x01'02'04'08'10'20'40'80);

						const auto mask = static_cast<std::uint8_t>(packed >> 56);
						// [ascii] [non-ascii] [?] [?] [?] [?] [ascii] [ascii]
						//           ^ n_ascii
						//                                                 ^ n_next_possible_ascii_thunk_begin
						const auto n_ascii = std::countr_zero(mask);
						const auto n_next_possible_ascii_thunk_begin = step - std::countl_zero(mask) - n_ascii;

						it_input_current += n_ascii;
						if (const auto result = check(n_next_possible_ascii_thunk_begin);
							result.has_error())
						{
							return make_result<process_policy>(
								result.error,
								result.input,
								result.output
							);
						}
					}
					else
					{
						it_input_current += step;
					}
				}

				const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < step);

				#if GAL_PROMETHEUS_COMPILER_DEBUG
				[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
				#endif

				const auto result = check(remaining);
				return make_result<process_policy>(
					result.error,
					result.input,
					result.output
				);
			}

			template<bool Detail = false>
			[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> auto
			{
				return Scalar::validate<Detail>({input, std::char_traits<char_type>::length(input)});
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
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
			>
			[[nodiscard]] constexpr static auto convert(
				const input_type input,
				typename output_type_of<OutputType>::pointer output
			) noexcept -> auto
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);
				if constexpr (assume_all_correct<ProcessPolicy>())
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
					if constexpr (not assume_all_correct<ProcessPolicy>())
					{
						if (const auto result = Scalar::validate<true>(input);
							result.has_error())
						{
							if constexpr (write_all_correct<ProcessPolicy>())
							{
								std::memcpy(it_output_current, it_input_current, result.input * sizeof(char_type));
								it_input_current += result.input;
								it_output_current += result.input;
							}

							return make_result<ProcessPolicy>(
								result.error,
								result.input,
								result.input
							);
						}
					}

					std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
					it_input_current += input_length;
					it_output_current += input_length;
				}
				else
				{
					constexpr auto is_native_endian = (OutputType == CharsType::UTF16_LE) == (std::endian::native == std::endian::little);
					constexpr auto is_byte_swap_required =
							(OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE) and
							(not is_native_endian);

					const auto transform = [
								// Used to calculate the processed input length
								it_input_begin,
								&it_input_current,
								// Used to determine the length of the current character if it is a correct UTF8 character
								it_input_end,
								// Used to calculate the processed output length
								it_output_begin,
								&it_output_current
							]<bool Pure>(const size_type n) noexcept -> auto
					{
						// [error/input/output]
						constexpr auto process_policy_keep_all_result = InputProcessPolicy::WRITE_ALL_CORRECT_2;

						const auto do_write = [&it_output_current](const auto value) noexcept -> void
						{
							if constexpr (is_byte_swap_required)
							{
								*(it_output_current + 0) = std::byteswap(static_cast<output_char_type>(value));
							}
							else
							{
								*(it_output_current + 0) = static_cast<output_char_type>(value);
							}

							it_output_current += 1;
						};

						const auto end = it_input_current + n;

						while (it_input_current < end)
						{
							const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
							const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

							if constexpr (const auto leading_byte = static_cast<std::uint8_t>(*it_input_current);
								Pure)
							{
								GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((leading_byte & 0x80) == 0);

								do_write(leading_byte);

								it_input_current += 1;
							}
							else
							{
								if ((leading_byte & 0x80) == 0)
								{
									// ascii
									do_write(leading_byte);

									it_input_current += 1;
								}
								else if ((leading_byte & 0b1110'0000) == 0b1100'0000)
								{
									// we have a two-byte UTF-8

									// minimal bound checking
									if (it_input_current + 1 >= it_input_end)
									{
										return make_result<process_policy_keep_all_result>(
											ErrorCode::TOO_SHORT,
											current_input_length,
											current_output_length
										);
									}

									const auto next_byte = static_cast<std::uint8_t>(*(it_input_current + 1));

									// checks if the next byte is a valid continuation byte in UTF-8.
									// A valid continuation byte starts with 10.
									if ((next_byte & 0b1100'0000) != 0b1000'0000)
									{
										return make_result<process_policy_keep_all_result>(
											ErrorCode::TOO_SHORT,
											current_input_length,
											current_output_length
										);
									}

									// assembles the Unicode code point from the two bytes.
									// It does this by discarding the leading 110 and 10 bits from the two bytes,
									// shifting the remaining bits of the first byte,
									// and then combining the results with a bitwise OR operation.
									const auto code_point = static_cast<std::uint32_t>(leading_byte & 0b0001'1111) << 6 | (next_byte & 0b0011'1111);

									if constexpr (not assume_all_correct<ProcessPolicy>())
									{
										if (code_point < 0x80)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::OVERLONG,
												current_input_length,
												current_output_length
											);
										}
										if (code_point >
										    []() noexcept -> std::uint32_t
										    {
											    if constexpr (OutputType == CharsType::LATIN) { return 0xff; }
											    else { return 0x7ff; }
										    }()
										)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::TOO_LARGE,
												current_input_length,
												current_output_length
											);
										}
									}

									do_write(code_point);

									it_input_current += 2;
								}
								else if ((leading_byte & 0b1111'0000) == 0b1110'0000)
								{
									// we have a three-byte UTF-8

									if constexpr (
										OutputType == CharsType::UTF16_LE or
										OutputType == CharsType::UTF16_BE or
										OutputType == CharsType::UTF32
									)
									{
										// minimal bound checking
										if (it_input_current + 2 >= it_input_end)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::TOO_SHORT,
												current_input_length,
												current_output_length
											);
										}

										const auto next_byte_1 = static_cast<std::uint8_t>(*(it_input_current + 1));
										const auto next_byte_2 = static_cast<std::uint8_t>(*(it_input_current + 2));

										if constexpr (not assume_all_correct<ProcessPolicy>())
										{
											if (
												((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
												((next_byte_2 & 0b1100'0000) != 0b1000'0000)
											)
											{
												return make_result<process_policy_keep_all_result>(
													ErrorCode::TOO_SHORT,
													current_input_length,
													current_output_length
												);
											}
										}

										const auto code_point = static_cast<std::uint32_t>(
											(leading_byte & 0b0000'1111) << 12 |
											(next_byte_1 & 0b0011'1111) << 6 |
											(next_byte_2 & 0b0011'1111)
										);

										if constexpr (not assume_all_correct<ProcessPolicy>())
										{
											const auto do_return = [current_input_length, current_output_length](const auto error) noexcept
											{
												return make_result<process_policy_keep_all_result>(
													error,
													current_input_length,
													current_output_length
												);
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

										do_write(code_point);

										it_input_current += 3;
									}
									else
									{
										return make_result<process_policy_keep_all_result>(
											ErrorCode::TOO_LARGE,
											current_input_length,
											current_output_length
										);
									}
								}
								else if ((leading_byte & 0b1111'1000) == 0b1111'0000)
								{
									// we have a four-byte UTF-8 word.

									if constexpr (
										OutputType == CharsType::UTF16_LE or
										OutputType == CharsType::UTF16_BE or
										OutputType == CharsType::UTF32
									)
									{
										// minimal bound checking
										if (it_input_current + 3 >= it_input_end)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::TOO_SHORT,
												current_input_length,
												current_output_length
											);
										}

										const auto next_byte_1 = static_cast<std::uint8_t>(*(it_input_current + 1));
										const auto next_byte_2 = static_cast<std::uint8_t>(*(it_input_current + 2));
										const auto next_byte_3 = static_cast<std::uint8_t>(*(it_input_current + 3));

										if constexpr (not assume_all_correct<ProcessPolicy>())
										{
											if (
												((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
												((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
												((next_byte_3 & 0b1100'0000) != 0b1000'0000)
											)
											{
												return make_result<process_policy_keep_all_result>(
													ErrorCode::TOO_SHORT,
													current_input_length,
													current_output_length
												);
											}
										}

										const auto code_point = static_cast<std::uint32_t>(
											(leading_byte & 0b0000'0111) << 18 |
											(next_byte_1 & 0b0011'1111) << 12 |
											(next_byte_2 & 0b0011'1111) << 6 |
											(next_byte_3 & 0b0011'1111)
										);

										if constexpr (not assume_all_correct<ProcessPolicy>())
										{
											const auto do_return = [current_input_length, current_output_length](const auto error) noexcept
											{
												return make_result<process_policy_keep_all_result>(
													error,
													current_input_length,
													current_output_length
												);
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
											do_write(code_point);
										}
										else
										{
											const auto [high_surrogate, low_surrogate] = [cp = code_point - 0x1'0000]() noexcept -> auto
											{
												const auto high = static_cast<std::uint16_t>(0xd800 + (cp >> 10));
												const auto low = static_cast<std::uint16_t>(0xdc00 + (cp & 0x3ff));

												return std::make_pair(high, low);
											}();

											do_write(high_surrogate);
											do_write(low_surrogate);
										}

										it_input_current += 4;
									}
									else
									{
										return make_result<process_policy_keep_all_result>(
											ErrorCode::TOO_LARGE,
											current_input_length,
											current_output_length
										);
									}
								}
								else
								{
									// we either have too many continuation bytes or an invalid leading byte
									if ((leading_byte & 0b1100'0000) == 0b1000'0000)
									{
										// we have too many continuation bytes
										return make_result<process_policy_keep_all_result>(
											ErrorCode::TOO_LONG,
											current_input_length,
											current_output_length
										);
									}

									// we have an invalid leading byte
									return make_result<process_policy_keep_all_result>(
										ErrorCode::HEADER_BITS,
										current_input_length,
										current_output_length
									);
								}
							}
						}

						// ==================================================
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current >= end);
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						constexpr auto current_output_length = static_cast<std::size_t>(0);
						return make_result<process_policy_keep_all_result>(
							ErrorCode::NONE,
							current_input_length,
							current_output_length
						);
					};

					constexpr auto step = 1 * advance_per_step;
					// 8 bytes
					while (it_input_current + step <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, step};
						#endif

						if (const auto value = memory::unaligned_load<data_type>(it_input_current + 0 * advance_per_step);
							(value & 0x8080'8080'8080'8080) != 0)
						{
							// MSB => LSB
							const auto msb = (value >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);

							const auto packed = msb * static_cast<data_type>(0x01'02'04'08'10'20'40'80);

							const auto mask = static_cast<std::uint8_t>(packed >> 56);
							// [ascii] [non-ascii] [?] [?] [?] [?] [ascii] [ascii]
							//           ^ n_ascii
							//                                                 ^ n_next_possible_ascii_thunk_begin
							const auto n_ascii = std::countr_zero(mask);
							const auto n_next_possible_ascii_thunk_begin = step - std::countl_zero(mask) - n_ascii;

							const auto result1 = transform.template operator()<true>(n_ascii);
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result1.has_error());

							if (const auto result2 = transform.template operator()<false>(n_next_possible_ascii_thunk_begin);
								result2.has_error())
							{
								return make_result<ProcessPolicy>(
									result2.error,
									result2.input,
									result2.output
								);
							}
						}
						else
						{
							const auto result = transform.template operator()<true>(step);
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
						}
					}

					const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					GAL_PROMETHEUS_ERROR_ASSUME(remaining < step);

					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
					#endif

					if (const auto result = transform.template operator()<false>(remaining);
						result.has_error())
					{
						return make_result<ProcessPolicy>(
							result.error,
							result.input,
							result.output
						);
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return make_result<ProcessPolicy>(
					ErrorCode::NONE,
					current_input_length,
					current_output_length
				);
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
			>
			[[nodiscard]] constexpr static auto convert(
				const pointer_type input,
				typename output_type_of<OutputType>::pointer output
			) noexcept -> auto
			{
				return Scalar::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, output);
			}

			template<
				typename StringType,
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
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
				result.resize(Scalar::length<OutputType>(input));

				std::ignore = Scalar::convert<OutputType, ProcessPolicy>(input, result.data());
				return result;
			}

			template<
				typename StringType,
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
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
				return Scalar::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)});
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
			>
			[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
			{
				using string_type = std::basic_string<typename output_type_of<OutputType>::value_type>;
				return Scalar::convert<string_type, OutputType, ProcessPolicy>(input);
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
			>
			[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
			{
				using string_type = std::basic_string<typename output_type_of<OutputType>::value_type>;
				return Scalar::convert<string_type, OutputType, ProcessPolicy>(input);
			}

			[[nodiscard]] constexpr static auto code_points(const input_type input) noexcept -> size_type
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
