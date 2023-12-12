// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:scalar.ascii;

import std;
import gal.prometheus.string;
import gal.prometheus.error;
import gal.prometheus.utility;

import :encoding;
import :converter;

namespace gal::prometheus::chars
{
	template<>
	class Scalar<"ascii">
	{
	public:
		constexpr static auto input_category = CharsCategory::ASCII;
		using input_type					 = chars::input_type<input_category>;
		using char_type						 = input_type::value_type;
		using pointer_type					 = input_type::const_pointer;
		using size_type						 = input_type::size_type;

		[[nodiscard]] constexpr auto validate(const input_type input) const noexcept -> result_type
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			const auto		   input_length		= input.size();

			const pointer_type it_input_begin	= input.data();
			pointer_type	   it_input_current = it_input_begin;
			const pointer_type it_input_end		= it_input_begin + input_length;

			for (; it_input_current + 16 <= it_input_end; it_input_current += 16)
			{
				const auto v1 = utility::unaligned_load<std::uint64_t>(it_input_current + 0);
				const auto v2 = utility::unaligned_load<std::uint64_t>(it_input_current + sizeof(std::uint64_t));

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
								{ return byte >= 0b1000'0000; });
				it != it_input_end)
			{
				return result_type{.error = ErrorCode::TOO_LARGE, .count = static_cast<std::size_t>(std::ranges::distance(it, it_input_begin))};
			}

			return result_type{.error = ErrorCode::NONE, .count = input_length};
		}

		[[nodiscard]] constexpr auto validate(const pointer_type input) const noexcept -> result_type
		{
			return this->validate({input, std::char_traits<char_type>::length(input)});
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory>
		[[nodiscard]] constexpr auto length(const input_type input) const noexcept -> size_type
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				return input.size();
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				return std::transform_reduce(
						input.begin(),
						input.end(),
						static_cast<size_type>(0),
						std::plus<>{},
						[](const auto byte) noexcept
						{
							return +(byte >> 7);
						});
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				return input.size();
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				return input.size();
			}
			else
			{
				GAL_PROMETHEUS_STATIC_UNREACHABLE();
			}
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory>
		[[nodiscard]] constexpr auto length(const pointer_type input) const noexcept -> size_type
		{
			return this->length<OutputCategory>({input, std::char_traits<char_type>::length(input)});
		}

		template<CharsCategory OutputCategory, InputProcessCriterion Criterion = InputProcessCriterion::RETURN_RESULT_TYPE, bool CheckNextBlock = true>
		[[nodiscard]] constexpr auto convert(const input_type input, typename output_type<OutputCategory>::pointer output) const noexcept -> std::conditional_t<Criterion == InputProcessCriterion::RETURN_RESULT_TYPE, result_type, std::size_t>
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
			GAL_PROMETHEUS_DEBUG_NOT_NULL(output);

			using output_pointer_type					= typename output_type<OutputCategory>::pointer;
			using output_char_type						= typename output_type<OutputCategory>::value_type;

			const auto				  input_length		= input.size();

			const pointer_type		  it_input_begin	= input.data();
			pointer_type			  it_input_current	= it_input_begin;
			const pointer_type		  it_input_end		= it_input_begin + input_length;

			const output_pointer_type it_output_begin	= output;
			output_pointer_type		  it_output_current = it_output_begin;

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				it_input_current += input_length;
				it_output_current += input_length;
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					if constexpr (CheckNextBlock)
					{
						// if it is safe to read 16 more bytes, check that they are ascii
						if (it_input_current + 16 <= it_input_end)
						{
							const auto v1 = utility::unaligned_load<std::uint64_t>(it_input_current + 0);
							const auto v2 = utility::unaligned_load<std::uint64_t>(it_input_current + sizeof(std::uint64_t));

							if (const auto value = v1 | v2;
								(value & 0x8080'8080'8080'8080) == 0)
							{
								std::ranges::transform(
										it_input_current,
										it_input_current + 16,
										it_output_current,
										[](const auto byte) noexcept
										{
											return utility::char_cast<output_char_type>(byte);
										});

								// 15 more step
								it_input_current += 15;
								it_output_current += 16;
								continue;
							}
						}
					}

					if (const auto byte = *it_input_current;
						(byte & 0x80) == 0)
					{
						*(it_output_current + 0) = utility::char_cast<output_char_type>(byte);
						it_output_current += 1;
					}
					else
					{
						*(it_output_current + 0) = utility::char_cast<output_char_type>((byte >> 6) | 0b1100'0000);
						*(it_output_current + 1) = utility::char_cast<output_char_type>((byte & 0b0011'1111) | 0b1000'0000);
						it_output_current += 2;
					}
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE or OutputCategory == CharsCategory::UTF32)
			{
				std::ranges::transform(
						it_input_current,
						it_input_end,
						it_output_current,
						[](const auto byte) noexcept
						{
							if constexpr (OutputCategory == CharsCategory::UTF32 or ((OutputCategory == CharsCategory::UTF16_LE) == (std::endian::native == std::endian::little)))
							{
								return utility::char_cast<output_char_type>(byte);
							}
							else
							{
								return utility::char_cast<output_char_type>(std::byteswap(utility::char_cast<output_char_type>(byte)));
							}
						});
			}
			else
			{
				GAL_PROMETHEUS_UNREACHABLE();
			}

			if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
			{
				return static_cast<std::size_t>(it_output_current - it_output_begin);
			}
			if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
			{
				return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
			}
			else
			{
				GAL_PROMETHEUS_STATIC_UNREACHABLE();
			}
		}

		template<CharsCategory OutputCategory, InputProcessCriterion Criterion = InputProcessCriterion::RETURN_RESULT_TYPE, bool CheckNextBlock = true>
		[[nodiscard]] constexpr auto convert(const pointer_type input, typename output_type<OutputCategory>::pointer output) const noexcept -> std::conditional_t<Criterion == InputProcessCriterion::RETURN_RESULT_TYPE, result_type, std::size_t>
		{
			return this->convert<OutputCategory, Criterion, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, output);
		}

		template<typename StringType, CharsCategory OutputCategory, InputProcessCriterion Criterion = InputProcessCriterion::RETURN_RESULT_TYPE, bool CheckNextBlock = true>
			requires requires(StringType& string) {
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<typename output_type<OutputCategory>::pointer>;
			}
		[[nodiscard]] constexpr auto convert(const input_type input) const noexcept -> StringType
		{
			StringType result{};
			result.resize(this->length<OutputCategory>(input));

			(void)this->convert<OutputCategory, Criterion, CheckNextBlock>(input, result.data());
			return result;
		}

		template<typename StringType, CharsCategory OutputCategory, InputProcessCriterion Criterion = InputProcessCriterion::RETURN_RESULT_TYPE, bool CheckNextBlock = true>
			requires requires(StringType& string) {
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<typename output_type<OutputCategory>::pointer>;
			}
		[[nodiscard]] constexpr auto convert(const pointer_type input) const noexcept -> StringType
		{
			StringType result{};
			result.resize(this->length<OutputCategory>(input));

			return this->convert<OutputCategory, Criterion, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, result.data());
		}
	};

	export namespace instance::scalar
	{
		constexpr Scalar<"ascii"> ascii{};
	}
}// namespace gal::prometheus::chars
