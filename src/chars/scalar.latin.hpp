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

template<>
class gal::prometheus::chars::Scalar<"latin">
{
public:
	constexpr static auto chars_type = CharsType::LATIN;

	using input_type = input_type_of<chars_type>;
	using char_type = input_type::value_type;
	using pointer_type = input_type::const_pointer;
	using size_type = input_type::size_type;

	constexpr static std::size_t char_size = sizeof(char_type);

	// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
	template<bool ReturnResultType = false>
	[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		for (constexpr auto step = 2 * sizeof(std::uint64_t) / char_size; it_input_current + step <= it_input_end; it_input_current += step)
		{
			constexpr auto offset = sizeof(std::uint64_t) / char_size;

			const auto v1 = memory::unaligned_load<std::uint64_t>(it_input_current + 0 * offset);
			const auto v2 = memory::unaligned_load<std::uint64_t>(it_input_current + 1 * offset);

			if (const auto value = v1 | v2;
				(value & 0x8080'8080'8080'8080) != 0)
			{
				break;
			}
		}

		if (const auto it =
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

	// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
	template<bool ReturnResultType = false>
	[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		return Scalar::validate<ReturnResultType>({input, std::char_traits<char_type>::length(input)});
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
			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const auto pop = [](const std::uint64_t value) noexcept -> size_type
			{
				return ((value >> 7) & 0x0101010101010101ull) * 0x0101010101010101ull >> 56;
			};
			size_type output_length = input_length;

			for (constexpr auto step = 4 * sizeof(std::uint64_t) / char_size; it_input_current + step <= it_input_end; it_input_current += step)
			{
				constexpr auto offset = sizeof(std::uint64_t) / char_size;

				const auto v1 = memory::unaligned_load<std::uint64_t>(it_input_current + 0 * offset);
				const auto v2 = memory::unaligned_load<std::uint64_t>(it_input_current + 1 * offset);
				const auto v3 = memory::unaligned_load<std::uint64_t>(it_input_current + 2 * offset);
				const auto v4 = memory::unaligned_load<std::uint64_t>(it_input_current + 3 * offset);

				output_length += pop(v1);
				output_length += pop(v2);
				output_length += pop(v3);
				output_length += pop(v4);
			}

			for (constexpr auto step = 1 * sizeof(std::uint64_t) / char_size; it_input_current + step <= it_input_end; it_input_current += step)
			{
				constexpr auto offset = sizeof(std::uint64_t) / char_size;

				const auto v1 = memory::unaligned_load<std::uint64_t>(it_input_current + 0 * offset);

				output_length += pop(v1);
			}

			return std::transform_reduce(
				it_input_current,
				it_input_end,
				output_length,
				std::plus<>{},
				[](const auto byte) noexcept
				{
					const auto b = static_cast<std::uint8_t>(byte);
					return b >> 7;
				}
			);
		}
		// ReSharper disable CppClangTidyBugproneBranchClone
		else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
		{
			return input.size();
		}
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
			// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(Scalar::validate(input));
		}

		using output_pointer_type = typename output_type_of<OutputType>::pointer;
		using output_char_type = typename output_type_of<OutputType>::value_type;

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		const output_pointer_type it_output_begin = output;
		output_pointer_type it_output_current = it_output_begin;

		if constexpr (OutputType == CharsType::LATIN)
		{
			std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
			it_input_current += input_length;
			it_output_current += input_length;
		}
		else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
		{
			while (it_input_current < it_input_end)
			{
				// if it is safe to read 16 more bytes, check that they are latin
				if (constexpr auto step = 2 * sizeof(std::uint64_t) / char_size;
					it_input_current + step <= it_input_end)
				{
					constexpr auto offset = sizeof(std::uint64_t) / char_size;

					const auto v1 = memory::unaligned_load<std::uint64_t>(it_input_current + 0 * offset);
					const auto v2 = memory::unaligned_load<std::uint64_t>(it_input_current + 1 * offset);

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
								return static_cast<output_char_type>(b);
							}
						);

						it_input_current += step;
						it_output_current += step;
						continue;
					}
				}

				if (const auto byte = static_cast<std::uint8_t>(*it_input_current);
					(byte & 0x80) == 0)
				{
					*(it_output_current + 0) = static_cast<output_char_type>(byte);

					it_input_current += 1;
					it_output_current += 1;
				}
				else
				{
					*(it_output_current + 0) = static_cast<output_char_type>((byte >> 6) | 0b1100'0000);
					*(it_output_current + 1) = static_cast<output_char_type>((byte & 0b0011'1111) | 0b1000'0000);

					it_input_current += 1;
					it_output_current += 2;
				}
			}
		}
		else if constexpr (
			OutputType == CharsType::UTF16_LE or
			OutputType == CharsType::UTF16_BE
		)
		{
			std::ranges::transform(
				it_input_current,
				it_input_end,
				it_output_current,
				[](const auto byte) noexcept
				{
					const auto b = static_cast<std::uint8_t>(byte);
					if constexpr (
						// extend Latin-1 char to 16-bit Unicode code point
						const auto data = static_cast<output_char_type>(b);
						(OutputType == CharsType::UTF16_LE) == (std::endian::native == std::endian::little)
					)
					{
						return data;
					}
					else
					{
						return std::byteswap(data);
					}
				}
			);
			it_input_current += input_length;
			it_output_current += input_length;
		}
		else if constexpr (OutputType == CharsType::UTF32)
		{
			std::ranges::transform(
				it_input_current,
				it_input_end,
				it_output_current,
				[](const auto byte) noexcept
				{
					const auto b = static_cast<std::uint8_t>(byte);
					return static_cast<output_char_type>(b);
				}
			);
			it_input_current += input_length;
			it_output_current += input_length;
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("Unknown or unsupported `OutputType` (we don't know the `endian` by UTF16, so it's not allowed to use it here).");
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
};
