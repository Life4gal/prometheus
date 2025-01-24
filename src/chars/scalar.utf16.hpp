// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
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
#include <chars/scalar.common.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::chars
{
	namespace scalar_utf16_detail
	{
		template<CharsType T>
		class Scalar
		{
		public:
			constexpr static auto chars_type = T;

			using input_type = input_type_of<chars_type>;
			using char_type = typename input_type::value_type;
			using pointer_type = typename input_type::const_pointer;
			using size_type = typename input_type::size_type;

			using data_type = scalar_block::data_type;

		private:
			constexpr static auto know_endian = chars_type == CharsType::UTF16_LE or chars_type == CharsType::UTF16_BE;
			constexpr static auto default_endian =
					(not know_endian)
						? std::endian::native
						: (chars_type == CharsType::UTF16_LE)
						? std::endian::little
						: std::endian::big;

			template<std::endian SourceEndian>
			[[nodiscard]] constexpr static auto to_native_word(const auto value) noexcept -> std::uint16_t
			{
				if constexpr (const auto data = static_cast<std::uint16_t>(value);
					SourceEndian != std::endian::native)
				{
					return std::byteswap(data);
				}
				else
				{
					return data;
				}
			}

			// 1-word UTF-16
			// 2-words UTF-16(surrogate pair)
			template<std::endian SourceEndian>
			[[nodiscard]] constexpr static auto validate(const pointer_type current, const pointer_type end) noexcept -> std::pair<size_type, ErrorCode>
			{
				const auto leading_word = Scalar::to_native_word<SourceEndian>(*(current + 0));

				if ((leading_word & 0xf800) == 0xd800)
				{
					// we have a two-word UTF16
					// must be a surrogate pair
					constexpr size_type length = 2;

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

					const auto next_word = to_native_word<SourceEndian>(*(current + 1));
					if (const auto diff = static_cast<std::uint16_t>(next_word - 0xdc00);
						diff > 0x3ff)
					{
						return {length, ErrorCode::SURROGATE};
					}

					return {length, ErrorCode::NONE};
				}

				// we have a one-word UTF16
				constexpr size_type length = 1;

				return {length, ErrorCode::NONE};
			}

			/**
			 * 1-word UTF-16:
			 *	=> 1 LATIN
			 *	=> 1/2/3 UTF-8
			 *	=> 1 UTF-32
			 * 2-words UTF-16(surrogate pair):
			 *	=> 4 UTF-8
			 *	=> 1 UTF-32
			 */
			template<CharsType OutputType, std::endian SourceEndian, bool PureAscii = false, bool Validate = true>
			constexpr static auto write(
				typename output_type_of<OutputType>::pointer& dest,
				const pointer_type current,
				const pointer_type& end
			) noexcept -> std::pair<size_type, ErrorCode>
			{
				const auto leading_word = Scalar::to_native_word<SourceEndian>(*(current + 0));

				if constexpr (PureAscii)
				{
					constexpr size_type length = 1;

					*(dest + 0) = scalar_block::char_of<OutputType>(leading_word);

					dest += 1;
					return {length, ErrorCode::NONE};
				}
				else
				{
					if constexpr (OutputType == CharsType::LATIN)
					{
						constexpr size_type length = 1;

						if constexpr (Validate)
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
							constexpr size_type length = 1;

							*(dest + 0) = scalar_block::char_of<OutputType>(leading_word);

							dest += 1;
							return {length, ErrorCode::NONE};
						}

						if ((leading_word & 0xf800) == 0)
						{
							// 1-word utf16 => 2-bytes utf8
							constexpr size_type length = 1;

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
							constexpr size_type length = 1;

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
						constexpr size_type length = 2;

						// minimal bound checking
						if (current + 1 >= end)
						{
							return {length, ErrorCode::SURROGATE};
						}

						const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);

						if constexpr (Validate)
						{
							if (diff > 0x3ff)
							{
								return {length, ErrorCode::SURROGATE};
							}
						}

						const auto next_word = Scalar::to_native_word<SourceEndian>(*(current + 1));
						const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

						if constexpr (Validate)
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
							constexpr size_type length = 2;

							// minimal bound checking
							if (current + 1 >= end)
							{
								return {length, ErrorCode::SURROGATE};
							}

							const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);

							if constexpr (Validate)
							{
								if (diff > 0x3ff)
								{
									return {length, ErrorCode::SURROGATE};
								}
							}

							const auto next_word = Scalar::to_native_word<SourceEndian>(*(current + 1));
							const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

							if constexpr (Validate)
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
						constexpr size_type length = 1;

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

				while (it_input_current < it_input_end)
				{
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					constexpr auto current_output_length = static_cast<std::size_t>(0);

					const auto [length, error] = Scalar::validate<SourceEndian>(it_input_current, it_input_end);
					if (error != ErrorCode::NONE)
					{
						return chars::make_result<process_policy>(
							error,
							current_input_length,
							current_output_length
						);
					}

					it_input_current += length;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				constexpr auto current_output_length = static_cast<std::size_t>(0);
				return chars::make_result<process_policy>(
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
							const auto native_word = Scalar::to_native_word<SourceEndian>(word);

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
							const auto native_word = Scalar::to_native_word<SourceEndian>(word);

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
				else if constexpr (
					OutputType == CharsType::LATIN or
					OutputType == CharsType::UTF8_CHAR or
					OutputType == CharsType::UTF8 or
					OutputType == CharsType::UTF32
				)
				{
					constexpr auto advance = scalar_block::advance_of<chars_type, OutputType>();

					const auto transform = [
								// Used to calculate the processed input length
								it_input_begin,
								&it_input_current,
								// Used to determine the length of the current character if it is a correct UTF8 character
								it_input_end,
								// Used to calculate the processed output length
								it_output_begin,
								&it_output_current
							]<bool Pure>(const decltype(advance) n) noexcept -> auto
					{
						// [error/input/output]
						constexpr auto process_policy_keep_all_result = InputProcessPolicy::WRITE_ALL_CORRECT_2;

						const auto end = it_input_current + n;
						while (it_input_current < end)
						{
							const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
							const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

							const auto [length, error] = Scalar::write<
								OutputType,
								SourceEndian,
								Pure,
								not assume_all_correct<ProcessPolicy>()
							>(
								it_output_current,
								it_input_current,
								it_input_end
							);
							if (error != ErrorCode::NONE)
							{
								return chars::make_result<process_policy_keep_all_result>(
									error,
									current_input_length,
									current_output_length
								);
							}

							it_input_current += length;
						}

						// ==================================================
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current >= end);
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						constexpr auto current_output_length = static_cast<std::size_t>(0);
						return chars::make_result<process_policy_keep_all_result>(
							ErrorCode::NONE,
							current_input_length,
							current_output_length
						);
					};

					while (it_input_current + advance <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
						#endif

						if (const auto value = [it_input_current]() noexcept -> auto
							{
								if constexpr (const auto data = scalar_block::read<chars_type>(it_input_current);
									SourceEndian == std::endian::native)
								{
									return data;
								}
								else
								{
									return (data >> 8) | (data << 56);
								}
							}();
							not scalar_block::pure_ascii<chars_type>(value))
						{
							if (const auto result = transform.template operator()<false>(advance);
								result.has_error())
							{
								return chars::make_result<ProcessPolicy>(
									result.error,
									result.input,
									result.output
								);
							}
						}
						else
						{
							const auto result = transform.template operator()<true>(advance);
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
						}
					}

					const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

					if (remaining != 0)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
						#endif

						if (const auto result = transform.template operator()<false>(remaining);
							result.has_error())
						{
							return chars::make_result<ProcessPolicy>(
								result.error,
								result.input,
								result.output
							);
						}
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return chars::make_result<ProcessPolicy>(
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
						const auto native_word = Scalar::to_native_word<SourceEndian>(word);

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
						return std::byteswap(word);
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
	class Scalar<"utf16"> : public scalar_utf16_detail::Scalar<CharsType::UTF16> {};

	template<>
	class Scalar<"utf16.le"> : public scalar_utf16_detail::Scalar<CharsType::UTF16_LE> {};

	template<>
	class Scalar<"utf16_le"> : public scalar_utf16_detail::Scalar<CharsType::UTF16_LE> {};

	template<>
	class Scalar<"utf16.be"> : public scalar_utf16_detail::Scalar<CharsType::UTF16_BE> {};

	template<>
	class Scalar<"utf16_be"> : public scalar_utf16_detail::Scalar<CharsType::UTF16_BE> {};
}
