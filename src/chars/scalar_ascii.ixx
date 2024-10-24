// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:chars.scalar.ascii;

import std;

import :meta;
import :memory;
#if GAL_PROMETHEUS_COMPILER_DEBUG
import :error;
#endif

import :chars.encoding;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <bit>
#include <functional>
#include <numeric>
#include <string>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <chars/encoding.ixx>
#include <meta/meta.ixx>
#include <memory/memory.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

// ReSharper disable once CppRedundantNamespaceDefinition
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(chars)
{
	template<>
	class Scalar<"ascii">
	{
	public:
		constexpr static auto input_category = CharsCategory::ASCII;
		using input_type = chars::input_type<input_category>;
		using char_type = input_type::value_type;
		using pointer_type = input_type::const_pointer;
		using size_type = input_type::size_type;

		template<bool ReturnResultType = false>
		[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

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
							[](const auto byte) noexcept { return byte >= 0b1000'0000; });
				it != it_input_end)
			{
				if constexpr (ReturnResultType)
				{
					return result_type{.error = ErrorCode::TOO_LARGE, .count = static_cast<std::size_t>(std::ranges::distance(it, it_input_begin))};
				}
				else
				{
					return false;
				}
			}

			if constexpr (ReturnResultType)
			{
				return result_type{.error = ErrorCode::NONE, .count = input_length};
			}
			else
			{
				return true;
			}
		}

		template<bool ReturnResultType = false>
		[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
		{
			return validate<ReturnResultType>({input, std::char_traits<char_type>::length(input)});
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory>
		[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			if constexpr (OutputCategory == CharsCategory::ASCII) { return input.size(); } // NOLINT(bugprone-branch-clone)
			else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
			{
				return std::transform_reduce(
					input.begin(),
					input.end(),
					input.size(),
					std::plus<>{},
					[](const auto byte) noexcept { return +(byte >> 7); });
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE or OutputCategory == CharsCategory::UTF16) { return input.size(); }
			else if constexpr (OutputCategory == CharsCategory::UTF32) { return input.size(); }
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
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);
			if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(validate(input));
			}

			using output_pointer_type = typename output_type<OutputCategory>::pointer;
			using output_char_type = typename output_type<OutputCategory>::value_type;

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const output_pointer_type it_output_begin = output;
			output_pointer_type it_output_current = it_output_begin;

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				it_input_current += input_length;
				it_output_current += input_length;
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					if constexpr (CheckNextBlock)
					{
						// if it is safe to read 16 more bytes, check that they are ascii
						if (it_input_current + 16 <= it_input_end)
						{
							const auto v1 = memory::unaligned_load<std::uint64_t>(it_input_current + 0);
							const auto v2 = memory::unaligned_load<std::uint64_t>(it_input_current + sizeof(std::uint64_t));

							if (const auto value = v1 | v2;
								(value & 0x8080'8080'8080'8080) == 0)
							{
								std::ranges::transform(
									it_input_current,
									it_input_current + 16,
									it_output_current,
									[](const auto byte) noexcept { return static_cast<output_char_type>(byte); });

								// 15 more step, see `for (; it_input_current < it_input_end; ++it_input_current)`
								it_input_current += 15;
								it_output_current += 16;
								continue;
							}
						}
					}

					if (const auto byte = *it_input_current;
						(byte & 0x80) == 0)
					{
						*(it_output_current + 0) = static_cast<output_char_type>(byte);
						it_output_current += 1;
					}
					else
					{
						*(it_output_current + 0) = static_cast<output_char_type>((byte >> 6) | 0b1100'0000);
						*(it_output_current + 1) = static_cast<output_char_type>((byte & 0b0011'1111) | 0b1000'0000);
						it_output_current += 2;
					}
				}
			}
			else if constexpr (
				OutputCategory == CharsCategory::UTF16_LE or
				OutputCategory == CharsCategory::UTF16_BE or
				// OutputCategory == CharsCategory::UTF16 or
				OutputCategory == CharsCategory::UTF32
			)
			{
				std::ranges::transform(
					it_input_current,
					it_input_end,
					it_output_current,
					[](const auto byte) noexcept
					{
						if constexpr (
							OutputCategory == CharsCategory::UTF32 or
							((OutputCategory == CharsCategory::UTF16_LE) == (std::endian::native == std::endian::little))
						) { return static_cast<output_char_type>(byte); }
						else { return static_cast<output_char_type>(std::byteswap(static_cast<output_char_type>(byte))); }
					});
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("Unknown or unsupported `OutputCategory` (we don't know the `endian` by UTF16, so it's not allowed to use it here)."); }

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
	};

	template<>
	struct scalar_processor_of<CharsCategory::ASCII>
	{
		using type = Scalar<"ascii">;
	};
}
