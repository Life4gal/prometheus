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
	template<>
	class Scalar<"utf16">;

	template<>
	class Scalar<"utf16.le">;

	template<>
	class Scalar<"utf16_le">;

	template<>
	class Scalar<"utf16.be">;

	template<>
	class Scalar<"utf16_be">;

	template<>
	struct descriptor_type<Scalar<"utf16">>
	{
		constexpr static auto chars_type = CharsType::UTF16;

		using input_type = input_type_of<chars_type>;
		using char_type = input_type::value_type;
		using pointer_type = input_type::const_pointer;
		using size_type = input_type::size_type;

		using data_type = std::uint64_t;

		constexpr static std::ptrdiff_t advance_per_step = sizeof(data_type) / sizeof(char_type);
		// zero extend ==> 1 char -> 1 out_char
		template<CharsType Type>
		constexpr static std::ptrdiff_t advance_per_step_with = advance_per_step;
	};

	template<>
	struct descriptor_type<Scalar<"utf16.le">> : descriptor_type<Scalar<"utf16">>
	{
		constexpr static auto chars_type = CharsType::UTF16_LE;
	};

	template<>
	struct descriptor_type<Scalar<"utf16_le">> : descriptor_type<Scalar<"utf16.le">> {};

	template<>
	struct descriptor_type<Scalar<"utf16.be">> : descriptor_type<Scalar<"utf16">>
	{
		constexpr static auto chars_type = CharsType::UTF16_BE;
	};

	template<>
	struct descriptor_type<Scalar<"utf16_be">> : descriptor_type<Scalar<"utf16.be">> {};

	namespace scalar_utf16_detail
	{
		template<typename Identity>
		class Scalar
		{
		public:
			using descriptor_type = descriptor_type<Identity>;

			constexpr static auto chars_type = descriptor_type::chars_type;

			using input_type = typename descriptor_type::input_type;
			using char_type = typename descriptor_type::char_type;
			using pointer_type = typename descriptor_type::pointer_type;
			using size_type = typename descriptor_type::size_type;

			using data_type = typename descriptor_type::data_type;

		private:
			constexpr static auto know_endian = chars_type == CharsType::UTF16_LE or chars_type == CharsType::UTF16_BE;
			constexpr static auto default_endian =
					(not know_endian)
						? std::endian::native
						: (chars_type == CharsType::UTF16_LE)
						? std::endian::little
						: std::endian::big;

		public:
			template<bool Detail = false, std::endian SourceEndian = default_endian>
			[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> auto
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				constexpr auto process_policy = Detail ? InputProcessPolicy::DEFAULT : InputProcessPolicy::RESULT;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				while (it_input_current != it_input_end)
				{
					if (const auto word = to_native_word<SourceEndian>(*(it_input_current + 0));
						(word & 0xf800) == 0xd800)
					{
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						constexpr auto current_output_length = static_cast<std::size_t>(0);

						// minimal bound checking
						if (it_input_current + 1 >= it_input_end)
						{
							return make_result<process_policy>(
								ErrorCode::SURROGATE,
								current_input_length,
								current_output_length
							);
						}

						if (const auto diff = static_cast<std::uint16_t>(word - 0xd800);
							diff > 0x3ff)
						{
							return make_result<process_policy>(
								ErrorCode::SURROGATE,
								current_input_length,
								current_output_length
							);
						}

						const auto next_word = to_native_word<SourceEndian>(*(it_input_current + 1));
						if (const auto diff = static_cast<std::uint16_t>(next_word - 0xdc00);
							diff > 0x3ff)
						{
							return make_result<process_policy>(
								ErrorCode::SURROGATE,
								current_input_length,
								current_output_length
							);
						}

						it_input_current += 2;
					}
					else
					{
						it_input_current += 1;
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				constexpr auto current_output_length = static_cast<std::size_t>(0);
				return make_result<process_policy>(
					ErrorCode::NONE,
					current_input_length,
					current_output_length
				);
			}

			template<bool Detail = false, std::endian SourceEndian = default_endian>
			[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> auto
			{
				return Scalar::validate<Detail, SourceEndian>({input, std::char_traits<char_type>::length(input)});
			}

			// note: we are not BOM aware
			template<CharsType OutputType, std::endian SourceEndian = default_endian>
			[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				// ReSharper disable CppClangTidyBugproneBranchClone
				if constexpr (OutputType == CharsType::LATIN)
				{
					return input.size();
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					return std::transform_reduce(
						input.begin(),
						input.end(),
						static_cast<size_type>(0),
						std::plus<>{},
						[](const auto word) noexcept
						{
							const auto native_word = to_native_word<SourceEndian>(word);

							return
									// ASCII
									1
									+
									// non-ASCII is at least 2 bytes, surrogates are 2*2 == 4 bytes
									(native_word > 0x7f)
									+
									(native_word > 0x7ff && native_word <= 0xd7ff)
									+
									(native_word >= 0xe000);
						}
					);
				}
				// ReSharper disable CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					return input.size();
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return std::transform_reduce(
						input.begin(),
						input.end(),
						static_cast<size_type>(0),
						std::plus<>{},
						[](const auto word) noexcept
						{
							const auto native_word = to_native_word<SourceEndian>(word);

							return +((native_word & 0xfc00) != 0xdc00);
						}
					);
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			// note: we are not BOM aware
			template<CharsType OutputType, std::endian SourceEndian = default_endian>
			[[nodiscard]] constexpr static auto length(const pointer_type input) noexcept -> size_type
			{
				return Scalar::length<OutputType, SourceEndian>({input, std::char_traits<char_type>::length(input)});
			}

			template<
				CharsType OutputType,
				std::endian SourceEndian = default_endian,
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
					// fixme: error C2187
					// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(Scalar::validate<false, SourceEndian>(input));
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((Scalar::validate<false, SourceEndian>(input)));
				}

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				if constexpr (
					OutputType == CharsType::UTF16_LE or
					OutputType == CharsType::UTF16_BE or
					OutputType == CharsType::UTF16
				)
				{
					constexpr auto flip =
							(OutputType != CharsType::UTF16) and
							((SourceEndian == std::endian::little) != (OutputType == CharsType::UTF16_LE));

					if constexpr (not assume_all_correct<ProcessPolicy>())
					{
						if (const auto result = Scalar::validate<true>(input);
							result.has_error())
						{
							if constexpr (write_all_correct<ProcessPolicy>())
							{
								if constexpr (flip)
								{
									Scalar::flip_endian({it_input_current, result.input}, it_output_current);
								}
								else
								{
									std::memcpy(it_output_current, it_input_current, result.input * sizeof(char_type));
								}

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

					if constexpr (flip)
					{
						Scalar::flip_endian(input, it_output_current);
					}
					else
					{
						std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
					}

					it_input_current += input_length;
					it_output_current += input_length;
				}
				else
				{
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

						const auto end = it_input_current + n;

						while (it_input_current < end)
						{
							const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
							const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

							if constexpr (const auto leading_word = to_native_word<SourceEndian>(*(it_input_current + 0));
								Pure)
							{
								*(it_output_current + 0) = to_output_type<OutputType>(leading_word);

								it_output_current += 1;
								it_input_current += 1;
							}
							else
							{
								if constexpr (OutputType == CharsType::LATIN)
								{
									if constexpr (not assume_all_correct<ProcessPolicy>())
									{
										if ((leading_word & 0xff00) != 0)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::TOO_LARGE,
												current_input_length,
												current_output_length
											);
										}
									}

									*(it_output_current + 0) = to_output_type<OutputType>(leading_word & 0xff);

									it_output_current += 1;
									it_input_current += 1;
								}
								else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
								{
									if ((leading_word & 0xff80) == 0)
									{
										// 1-byte utf8

										*(it_output_current + 0) = to_output_type<OutputType>(leading_word);

										it_input_current += 1;
										it_output_current += 1;
									}
									else if ((leading_word & 0xf800) == 0)
									{
										// 2-bytes utf8

										// 0b110?'???? 0b10??'????
										*(it_output_current + 0) = to_output_type<OutputType>((leading_word >> 6) | 0b1100'0000);
										*(it_output_current + 1) = to_output_type<OutputType>((leading_word & 0b0011'1111) | 0b1000'0000);

										it_input_current += 1;
										it_output_current += 2;
									}
									else if ((leading_word & 0xf800) != 0xd800)
									{
										// 3-bytes utf8

										// 0b1110'???? 0b10??'???? 0b10??'????
										*(it_output_current + 0) = to_output_type<OutputType>((leading_word >> 12) | 0b1110'0000);
										*(it_output_current + 1) = to_output_type<OutputType>(((leading_word >> 6) & 0b0011'1111) | 0b1000'0000);
										*(it_output_current + 2) = to_output_type<OutputType>((leading_word & 0b0011'1111) | 0b1000'0000);

										it_input_current += 1;
										it_output_current += 3;
									}
									else
									{
										// 4-bytes utf8
										// must be a surrogate pair

										// minimal bound checking
										if (it_input_current + 1 >= it_input_end)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::SURROGATE,
												current_input_length,
												current_output_length
											);
										}

										const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);

										if constexpr (not assume_all_correct<ProcessPolicy>())
										{
											if (diff > 0x3ff)
											{
												return make_result<process_policy_keep_all_result>(
													ErrorCode::SURROGATE,
													current_input_length,
													current_output_length
												);
											}
										}

										const auto next_word = to_native_word<SourceEndian>(*(it_input_current + 1));
										const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

										if constexpr (not assume_all_correct<ProcessPolicy>())
										{
											if (next_diff > 0x3ff)
											{
												return make_result<process_policy_keep_all_result>(
													ErrorCode::SURROGATE,
													current_input_length,
													current_output_length
												);
											}
										}

										const auto value = static_cast<std::uint32_t>((diff << 10) + next_diff + 0x1'0000);

										// will generate four UTF-8 bytes
										// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
										*(it_output_current + 0) = to_output_type<OutputType>((value >> 18) | 0b1111'0000);
										*(it_output_current + 1) = to_output_type<OutputType>(((value >> 12) & 0b0011'1111) | 0b1000'0000);
										*(it_output_current + 2) = to_output_type<OutputType>(((value >> 6) & 0b0011'1111) | 0b1000'0000);
										*(it_output_current + 3) = to_output_type<OutputType>((value & 0b0011'1111) | 0b1000'0000);

										it_input_current += 2;
										it_output_current += 4;
									}
								}
								else if constexpr (OutputType == CharsType::UTF32)
								{
									if ((leading_word & 0xf800) != 0xd800)
									{
										// no surrogate pair, extend 16-bit word to 32-bit word
										*(it_output_current + 0) = to_output_type<OutputType>(leading_word);

										it_input_current += 1;
										it_output_current += 1;
									}
									else
									{
										// must be a surrogate pair

										// minimal bound checking
										if (it_input_current + 1 >= it_input_end)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::SURROGATE,
												current_input_length,
												current_output_length
											);
										}

										const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);

										if constexpr (not assume_all_correct<ProcessPolicy>())
										{
											if (diff > 0x3ff)
											{
												return make_result<process_policy_keep_all_result>(
													ErrorCode::SURROGATE,
													current_input_length,
													current_output_length
												);
											}
										}

										const auto next_word = to_native_word<SourceEndian>(*(it_input_current + 1));
										const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

										if constexpr (not assume_all_correct<ProcessPolicy>())
										{
											if (next_diff > 0x3ff)
											{
												return make_result<process_policy_keep_all_result>(
													ErrorCode::SURROGATE,
													current_input_length,
													current_output_length
												);
											}
										}

										const auto value = static_cast<std::uint32_t>((diff << 10) + next_diff + 0x1'0000);

										*(it_output_current + 0) = to_output_type<OutputType>(value);

										it_input_current += 2;
										it_output_current += 1;
									}
								}
								else
								{
									GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
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

					while (it_input_current + descriptor_type::advance_per_step <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, descriptor_type::advance_per_step};
						#endif

						const auto value = [it_input_current]() noexcept -> auto
						{
							if constexpr (const auto data = memory::unaligned_load<data_type>(it_input_current);
								SourceEndian == std::endian::native)
							{
								return data;
							}
							else
							{
								return (data >> 8) | (data << 56);
							}
						}();
						const auto pure = [value]() noexcept -> bool
						{
							if constexpr (OutputType == CharsType::LATIN)
							{
								return (value & static_cast<data_type>(0xff00'ff00'ff00'ff00)) == 0;
							}
							else
							{
								return (value & static_cast<data_type>(0xff80'ff80'ff80'ff80)) == 0;
							}
						}();

						if (not pure)
						{
							// const auto to_bit = [value](const auto offset) noexcept -> auto
							// {
							// 	const auto v = static_cast<std::uint16_t>(value >> offset);
							// 	return +(v < 0x0080);
							// };
							//
							// const auto ascii_mask = static_cast<std::uint8_t>(
							// 	to_bit(48) << 3 |
							// 	to_bit(32) << 2 |
							// 	to_bit(16) << 1 |
							// 	to_bit(0) << 0
							// );
							//
							// const auto non_ascii_mask = static_cast<std::uint8_t>((~ascii_mask) & 0xf);
							//
							// // [ascii] [non-ascii] [?] [ascii]
							// //           ^ n_ascii
							// //                                  ^ n_next_possible_ascii_chunk_begin
							// const auto n_ascii = std::countr_zero(non_ascii_mask);
							// const auto n_next_possible_ascii_chunk_begin =
							// 		descriptor_type::advance_per_step -
							// 		(
							// 			// 0000'????
							// 			std::countl_zero(non_ascii_mask) - 4
							// 		) -
							// 		n_ascii;
							//
							// const auto result1 = transform.template operator()<true>(n_ascii);
							// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result1.has_error());
							//
							// if (const auto result2 = transform.template operator()<false>(n_next_possible_ascii_chunk_begin);
							// 	result2.has_error())
							// {
							// 	return make_result<ProcessPolicy>(
							// 		result2.error,
							// 		result2.input,
							// 		result2.output
							// 	);
							// }
							if (const auto result = transform.template operator()<false>(descriptor_type::advance_per_step);
								result.has_error())
							{
								return make_result<ProcessPolicy>(
									result.error,
									result.input,
									result.output
								);
							}
						}
						else
						{
							const auto result = transform.template operator()<true>(descriptor_type::advance_per_step);
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
						}
					}

					const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					GAL_PROMETHEUS_ERROR_ASSUME(remaining < descriptor_type::advance_per_step);

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
				std::endian SourceEndian = default_endian,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
			>
			[[nodiscard]] constexpr static auto convert(
				const pointer_type input,
				typename output_type_of<OutputType>::pointer output
			) noexcept -> auto
			{
				return Scalar::convert<OutputType, SourceEndian, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, output);
			}

			template<
				typename StringType,
				CharsType OutputType,
				std::endian SourceEndian = default_endian,
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
				result.resize(Scalar::length<OutputType, SourceEndian>(input));

				std::ignore = Scalar::convert<OutputType, SourceEndian, ProcessPolicy>(input, result.data());
				return result;
			}

			template<
				typename StringType,
				CharsType OutputType,
				std::endian SourceEndian = default_endian,
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
				return Scalar::convert<OutputType, SourceEndian, ProcessPolicy>({input, std::char_traits<char_type>::length(input)});
			}

			template<
				CharsType OutputType,
				std::endian SourceEndian = default_endian,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
			>
			[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
			{
				using string_type = std::basic_string<typename output_type_of<OutputType>::value_type>;
				return Scalar::convert<string_type, OutputType, SourceEndian, ProcessPolicy>(input);
			}

			template<
				CharsType OutputType,
				std::endian SourceEndian = default_endian,
				InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
			>
			[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
			{
				using string_type = std::basic_string<typename output_type_of<OutputType>::value_type>;
				return Scalar::convert<string_type, OutputType, SourceEndian, ProcessPolicy>(input);
			}

			template<std::endian SourceEndian = default_endian>
			[[nodiscard]] constexpr static auto code_points(const input_type input) noexcept -> size_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				return std::ranges::count_if(
					input,
					[](const auto word) noexcept -> bool
					{
						const auto native_word = to_native_word<SourceEndian>(word);

						return (native_word & 0xfc00) != 0xdc00;
					}
				);
			}

			constexpr static auto flip_endian(const input_type input, const typename output_type_of<chars_type>::pointer output) noexcept -> void
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				std::ranges::transform(
					input,
					output,
					[](const auto word) noexcept
					{
						// force byte swap
						return to_output_type<std::endian::native == std::endian::little ? CharsType::UTF16_BE : CharsType::UTF16_LE>(word);
					}
				);
			}

			template<typename StringType>
				requires requires(StringType& string)
				{
					string.resize(std::declval<size_type>());
					{
						string.data()
					} -> std::convertible_to<typename output_type_of<chars_type>::pointer>;
				}
			[[nodiscard]] constexpr static auto flip_endian(const input_type input) noexcept -> StringType
			{
				StringType result{};
				result.resize(Scalar::length<chars_type>(input));

				Scalar::flip_endian(input, result.data());
				return result;
			}

			[[nodiscard]] constexpr static auto flip_endian(const input_type input) noexcept -> std::basic_string<typename output_type_of<chars_type>::value_type>
			{
				std::basic_string<typename output_type_of<chars_type>::value_type> result{};
				result.resize(Scalar::length<chars_type>(input));

				Scalar::flip_endian(input, result.data());
				return result;
			}

			// ===================================================
			// Scalar<"utf16.le"> / Scalar<"utf16.be">

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy
			>
				requires (know_endian)
			[[nodiscard]] constexpr static auto convert(
				const input_type input,
				typename output_type_of<OutputType>::pointer output
			) noexcept -> auto
			{
				return Scalar::convert<OutputType, default_endian, ProcessPolicy>(input, output);
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy
			>
				requires (know_endian)
			[[nodiscard]] constexpr static auto convert(
				const pointer_type input,
				typename output_type_of<OutputType>::pointer output
			) noexcept -> auto
			{
				return Scalar::convert<OutputType, default_endian, ProcessPolicy>(input, output);
			}

			template<
				typename StringType,
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy
			>
				requires (know_endian) and requires(StringType& string)
				{
					string.resize(std::declval<size_type>());
					{
						string.data()
					} -> std::convertible_to<typename output_type_of<OutputType>::pointer>;
				}
			[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> StringType
			{
				return Scalar::convert<StringType, OutputType, default_endian, ProcessPolicy>(input);
			}

			template<
				typename StringType,
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy
			>
				requires (know_endian) and requires(StringType& string)
				{
					string.resize(std::declval<size_type>());
					{
						string.data()
					} -> std::convertible_to<typename output_type_of<OutputType>::pointer>;
				}
			[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> StringType
			{
				return Scalar::convert<StringType, OutputType, default_endian, ProcessPolicy>(input);
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy
			>
				requires (know_endian)
			[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
			{
				return Scalar::convert<OutputType, default_endian, ProcessPolicy>(input);
			}

			template<
				CharsType OutputType,
				InputProcessPolicy ProcessPolicy
			>
				requires (know_endian)
			[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
			{
				return Scalar::convert<OutputType, default_endian, ProcessPolicy>(input);
			}
		};
	}

	template<>
	class Scalar<"utf16"> : public scalar_utf16_detail::Scalar<Scalar<"utf16">> {};

	template<>
	class Scalar<"utf16.le"> : public scalar_utf16_detail::Scalar<Scalar<"utf16.le">> {};

	template<>
	class Scalar<"utf16.be"> : public scalar_utf16_detail::Scalar<Scalar<"utf16.be">> {};

	template<>
	class Scalar<"utf16_be"> : public scalar_utf16_detail::Scalar<Scalar<"utf16_be">> {};
}
