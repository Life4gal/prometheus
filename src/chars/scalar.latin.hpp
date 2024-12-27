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

private:
	using data_type = std::uint64_t;

	constexpr static std::size_t size_per_char = sizeof(char_type);
	constexpr static std::size_t advance_per_step = sizeof(data_type) / size_per_char;

public:
	// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
	template<bool Detail = false>
	[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> auto
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

		constexpr auto process_policy = Detail ? InputProcessPolicy::DEFAULT : InputProcessPolicy::RESULT;

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		constexpr auto step = 1 * advance_per_step;
		// 8 bytes
		while (it_input_current + 1 * step <= it_input_end)
		{
			const auto v = memory::unaligned_load<data_type>(it_input_current + 0 * advance_per_step);

			if (const auto value = v;
				(value & 0x8080'8080'8080'8080) != 0)
			{
				// MSB => LSB
				const auto msb = (v >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);

				const auto packed = msb * static_cast<data_type>(0x01'02'04'08'10'20'40'80);

				const auto mask = static_cast<std::uint8_t>(packed >> 56);
				const auto pos = std::countr_zero(mask);

				it_input_current += pos;

				const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
				constexpr auto current_output_length = static_cast<std::size_t>(0);

				return make_result<process_policy>(
					ErrorCode::TOO_LARGE,
					current_input_length,
					current_output_length
				);
			}

			it_input_current += 1 * step;
		}
		// any bytes
		if (std::ranges::any_of(
				it_input_current,
				it_input_end,
				[](const auto byte) noexcept -> bool
				{
					const auto b = static_cast<std::uint8_t>(byte);
					// ASCII ONLY
					return b >= 0b1000'0000;
				}
			)
		)
		{
			const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
			constexpr auto current_output_length = static_cast<std::size_t>(0);

			return make_result<process_policy>(
				ErrorCode::TOO_LARGE,
				current_input_length,
				current_output_length
			);
		}
		it_input_current = it_input_end;

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

	// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
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
			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			size_type output_length = input_length;

			constexpr auto step = 1 * advance_per_step;
			// 8 bytes
			while (it_input_current + 1 * step <= it_input_end)
			{
				const auto v = memory::unaligned_load<data_type>(it_input_current + 0 * advance_per_step);

				if (const auto value = v;
					(value & 0x8080'8080'8080'8080) != 0)
				{
					// MSB => LSB
					const auto msb = (v >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);

					// const auto packed = msb * static_cast<data_type>(0x01'01'01'01'01'01'01'01);
					//
					// const auto count = static_cast<std::uint8_t>(packed >> 56);

					const auto count = std::popcount(msb);

					output_length += count;
				}

				it_input_current += 1 * step;
			}
			// any bytes
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
				// at least one character is processed in each round
				size_type n = 1;

				constexpr auto step = 1 * advance_per_step;
				// 8 bytes
				while (it_input_current + 1 * step <= it_input_end)
				{
					const auto v = memory::unaligned_load<data_type>(it_input_current + 0 * advance_per_step);

					if (const auto value = v;
						(value & 0x8080'8080'8080'8080) != 0)
					{
						// MSB => LSB
						const auto msb = (v >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);

						const auto packed = msb * static_cast<data_type>(0x01'02'04'08'10'20'40'80);

						const auto mask = static_cast<std::uint8_t>(packed >> 56);
						const auto pos_ascii = std::countr_zero(mask);

						std::ranges::transform(
							it_input_current,
							it_input_current + pos_ascii,
							it_output_current,
							[](const auto byte) noexcept
							{
								const auto b = static_cast<std::uint8_t>(byte);
								return static_cast<output_char_type>(b);
							}
						);

						// skip n characters until next possible block of ascii characters
						n = step - std::countl_zero(mask) - pos_ascii;
						it_input_current += pos_ascii;
						it_output_current += pos_ascii;
						break;
					}

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

					// pure ascii block, no extra characters need to be processed separately
					n = 0;
					it_input_current += 1 * step;
					it_output_current += 1 * step;
				}

				// any bytes
				std::ranges::for_each(
					it_input_current,
					it_input_current + n,
					[&it_output_current](const auto byte) noexcept -> void
					{
						if (const auto data = static_cast<std::uint8_t>(byte);
							(data & 0x80) == 0)
						{
							*(it_output_current + 0) = static_cast<output_char_type>(data);

							it_output_current += 1;
						}
						else
						{
							*(it_output_current + 0) = static_cast<output_char_type>((data >> 6) | 0b1100'0000);
							*(it_output_current + 1) = static_cast<output_char_type>((data & 0b0011'1111) | 0b1000'0000);

							it_output_current += 2;
						}
					}
				);
				it_input_current += n;
			}
		}
		else if constexpr (
			OutputType == CharsType::UTF16_LE or
			OutputType == CharsType::UTF16_BE
		)
		{
			constexpr auto is_native_endian = (OutputType == CharsType::UTF16_LE) == (std::endian::native == std::endian::little);

			// zero extend each set of 8 bit latin1 characters to 16-bit integers
			std::ranges::transform(
				it_input_current,
				it_input_end,
				it_output_current,
				[](const auto byte) noexcept
				{
					const auto b = static_cast<std::uint8_t>(byte);
					if constexpr (
						const auto data = static_cast<output_char_type>(b);
						is_native_endian
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
			// zero extend each set of 8 bit latin1 characters to 32-bit integers
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
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
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
};
