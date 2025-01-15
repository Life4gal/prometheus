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
#include <chars/scalar.common.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

// ReSharper disable once CppRedundantNamespaceDefinition
namespace gal::prometheus::chars
{
	template<>
	class Scalar<"utf32">
	{
	public:
		constexpr static auto chars_type = CharsType::UTF32;

		using input_type = input_type_of<chars_type>;
		using char_type = input_type::value_type;
		using pointer_type = input_type::const_pointer;
		using size_type = input_type::size_type;

		using data_type = scalar_block::data_type;

	private:
		[[nodiscard]] constexpr static auto validate(const char_type c) noexcept -> ErrorCode
		{
			if (c > 0x10'ffff)
			{
				return ErrorCode::TOO_LARGE;
			}

			if (c >= 0xd800 and c <= 0xdfff)
			{
				return ErrorCode::SURROGATE;
			}

			return ErrorCode::NONE;
		}

		/**
		 * 1-dword UTF-32:
		 *	=> 1 LATIN
		 *	=> 1/2/3/4 UTF-8
		 *	=> 1/2 UTF-16
		 */
		template<CharsType OutputType, bool PureAscii = false, bool Validate = true>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& dest,
			const pointer_type current,
			[[maybe_unused]] const pointer_type& end
		) noexcept -> std::pair<size_type, ErrorCode>
		{
			constexpr size_type length = 1;

			const auto value = static_cast<std::uint32_t>(*(current + 0));

			if constexpr (OutputType == CharsType::LATIN)
			{
				if constexpr (PureAscii)
				{
					*(dest + 0) = scalar_block::char_of<OutputType>(value);

					dest += 1;
					return {length, ErrorCode::NONE};
				}
				else
				{
					if constexpr (Validate)
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

						if constexpr (Validate)
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

					if constexpr (Validate)
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
						if constexpr (Validate)
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

					if constexpr (Validate)
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

	public:
		template<bool Detail = false>
		[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> auto
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			constexpr auto process_policy = Detail ? InputProcessPolicy::DEFAULT : InputProcessPolicy::RESULT;
			constexpr auto advance = scalar_block::advance_of<chars_type, chars_type>();

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			while (it_input_current < it_input_end)
			{
				const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
				constexpr auto current_output_length = static_cast<std::size_t>(0);

				if (const auto error = Scalar::validate(*(it_input_current + 0));
					error != ErrorCode::NONE)
				{
					return chars::make_result<process_policy>(
						error,
						current_input_length,
						current_output_length
					);
				}

				it_input_current += 1;
			}

			const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
			GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

			if (remaining != 0)
			{
				#if GAL_PROMETHEUS_COMPILER_DEBUG
				[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
				#endif

				const auto end = it_input_current + remaining;
				if (const auto it = std::ranges::find_if(
						it_input_current,
						end,
						[](const auto error) noexcept -> bool
						{
							return error != ErrorCode::NONE;
						},
						static_cast<auto(*)(char_type) noexcept -> ErrorCode>(&Scalar::validate)
					);
					it != end)
				{
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					constexpr auto current_output_length = static_cast<std::size_t>(0);

					return chars::make_result<process_policy>(
						Scalar::validate(*it),
						current_input_length,
						current_output_length
					);
				}

				it_input_current += remaining;
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
					[](const auto data) noexcept
					{
						const auto v = static_cast<std::uint32_t>(data);
						return
								1 // ascii
								+
								(v > 0x7f) // two-byte
								+
								(v > 0x7ff) // three-byte
								+
								(v > 0xffff) // four-byte
								;
					}
				);
			}
			else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
			{
				return std::transform_reduce(
					input.begin(),
					input.end(),
					static_cast<size_type>(0),
					std::plus<>{},
					[](const auto data) noexcept
					{
						const auto v = static_cast<std::uint32_t>(data);
						return
								1 // non-surrogate word
								+
								(v > 0xffff) // surrogate pair
								;
					}
				);
			}
			// ReSharper disable CppClangTidyBugproneBranchClone
			else if constexpr (OutputType == CharsType::UTF32)
			{
				return input.size();
			}
			// ReSharper restore CppClangTidyBugproneBranchClone
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		// note: we are not BOM aware
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto length(const pointer_type input) noexcept -> size_type
		{
			return Scalar::length<OutputType>({input, std::char_traits<char_type>::length(input)});
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

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const output_pointer_type it_output_begin = output;
			output_pointer_type it_output_current = it_output_begin;

			if constexpr (OutputType == CharsType::UTF32)
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

						return chars::make_result<ProcessPolicy>(
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
			else if constexpr (
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8 or
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE
			)
			{
				constexpr auto advance = scalar_block::advance_of<chars_type, OutputType>();

				const auto transform = [
							// Used to calculate the processed input length
							it_input_begin,
							&it_input_current,
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
							Pure,
							not assume_all_correct<ProcessPolicy>()
						>(
							it_output_current,
							it_input_current,
							it_input_end
						);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == 1);
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
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == end);
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
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

					if (const auto value = scalar_block::read<chars_type>(it_input_current);
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
			return Scalar::convert<string_type, OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)});
		}
	};
}
