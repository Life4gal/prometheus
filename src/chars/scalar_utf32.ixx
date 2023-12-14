// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:scalar.utf32;

import std;
import gal.prometheus.string;
import gal.prometheus.error;
import gal.prometheus.utility;

import :encoding;
import :converter;

namespace gal::prometheus::chars
{
	template<>
	class Scalar<"utf32">
	{
	public:
		constexpr static auto input_category = CharsCategory::UTF32;
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

			while (it_input_current != it_input_end)
			{
				const auto counf_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				if (const auto word = *it_input_current;
					word > 0x10'ffff) { return result_type{.error = ErrorCode::TOO_LARGE, .count = counf_if_error}; }
				else if (word >= 0xd800 and word <= 0xdfff) { return result_type{.error = ErrorCode::SURROGATE, .count = counf_if_error}; }

				it_input_current += 1;
			}

			return {.error = ErrorCode::NONE, .count = input_length};
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
						[](const auto data) noexcept
						{
							return 1// ascii
								   +
								   (data > 0x7f)// two-byte
								   +
								   (data > 0x7ff)// three-byte
								   +
								   (data > 0xffff)// four-byte
									;
						});
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				return std::transform_reduce(
						input.begin(),
						input.end(),
						static_cast<size_type>(0),
						std::plus<>{},
						[](const auto data) noexcept
						{
							return 1// non-surrogate word
								   +
								   (data > 0xffff)// surrogate pair
									;
						});
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
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if constexpr (CheckNextBlock)
					{
						// try to convert the next block of 2 ASCII characters
						// if it is safe to read 8 more bytes, check that they are ascii
						if (it_input_current + 2 <= it_input_end)
						{
							if (const auto value = utility::unaligned_load<std::uint64_t>(it_input_current);
								(value & 0xffff'ff80'ffff'ff80) == 0)
							{
								*(it_output_current + 0) = utility::char_cast<output_char_type>(*(it_input_current + 0));
								*(it_output_current + 1) = utility::char_cast<output_char_type>(*(it_input_current + 1));

								// one more step
								it_input_current += 1;
								it_output_current += 2;
								continue;
							}
						}
					}

					const auto word = *it_input_current;
					if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
					{
						if ((word & 0xffff'ff00) != 0)
						{
							if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
							{
								return 0;
							}
							else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
							{
								return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
							}
							else
							{
								GAL_PROMETHEUS_STATIC_UNREACHABLE();
							}
						}
					}

					*(it_output_current + 0) = utility::char_cast<output_char_type>(word & 0xff);
					it_output_current += 1;
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if constexpr (CheckNextBlock)
					{
						// try to convert the next block of 2 ASCII characters
						// if it is safe to read 8 more bytes, check that they are ascii
						if (it_input_current + 2 <= it_input_end)
						{
							if (const auto value = utility::unaligned_load<std::uint64_t>(it_input_current);
								(value & 0xffff'ff80'ffff'ff80) == 0)
							{
								*(it_output_current + 0) = utility::char_cast<output_char_type>(*(it_input_current + 0));
								*(it_output_current + 1) = utility::char_cast<output_char_type>(*(it_input_current + 1));

								// one more step
								it_input_current += 1;
								it_output_current += 2;
								continue;
							}
						}
					}

					if (const auto word = *it_input_current;
						(word & 0xffff'ff80) == 0)
					{
						// 1-byte ascii
						*(it_output_current + 0) = utility::char_cast<output_char_type>(word);
						it_output_current += 1;
					}
					else if ((word & 0xffff'f800) == 0)
					{
						// 2-bytes utf8
						// 0b110?'???? 0b10??'????
						*(it_output_current + 0) = utility::char_cast<output_char_type>((word >> 6) | 0b1100'0000);
						*(it_output_current + 1) = utility::char_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);
						it_output_current += 2;
					}
					else if ((word & 0xffff'0000) == 0)
					{
						// 3-bytes utf8
						if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							if (word >= 0xd800 and word <= 0xdfff)
							{
								if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
								else
								{
									GAL_PROMETHEUS_STATIC_UNREACHABLE();
								}
							}
						}
						// 0b1110'???? 0b10??'???? 0b10??'????
						*(it_output_current + 0) = utility::char_cast<output_char_type>((word >> 12) | 0b1110'0000);
						*(it_output_current + 1) = utility::char_cast<output_char_type>(((word >> 6) & 0b0011'1111) | 0b1000'0000);
						*(it_output_current + 2) = utility::char_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);
						it_output_current += 3;
					}
					else
					{
						// 4-bytes utf8
						if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							if (word > 0x0010'ffff)
							{
								if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
								}
								else
								{
									GAL_PROMETHEUS_STATIC_UNREACHABLE();
								}
							}
						}
						// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
						*(it_output_current + 0) = utility::char_cast<output_char_type>((word >> 18) | 0b1111'0000);
						*(it_output_current + 1) = utility::char_cast<output_char_type>(((word >> 12) & 0b0011'1111) | 0b1000'0000);
						*(it_output_current + 2) = utility::char_cast<output_char_type>(((word >> 6) & 0b0011'1111) | 0b1000'0000);
						*(it_output_current + 3) = utility::char_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);
						it_output_current += 4;
					}
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if (const auto word = *it_input_current;
						(word & 0xffff'0000) == 0)
					{
						if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							if (word >= 0xd800 and word <= 0xdfff)
							{
								if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
							}
						}
						const auto real_word = [w = utility::char_cast<output_char_type>(word)]() noexcept
						{
							if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
							{
								return std::byteswap(w);
							}
							else
							{
								return w;
							}
						}();

						*(it_output_current + 0) = utility::char_cast<output_char_type>(real_word);
						it_output_current += 1;
					}
					else
					{
						if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							if (word > 0x0010'ffff)
							{
								if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
								}
							}
							const auto [high_surrogate, low_surrogate] = [real_word = word - 0x0001'0000]() noexcept
							{
								const auto high = 0xd800 + (real_word >> 10);
								const auto low	= 0xdc00 + (real_word & 0x3ff);
								if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
								{
									return std::make_pair(std::byteswap(high), std::byteswap(low));
								}
								else
								{
									return std::make_pair(high, low);
								}
							}();

							*(it_output_current + 0) = utility::char_cast<output_char_type>(high_surrogate);
							*(it_output_current + 1) = utility::char_cast<output_char_type>(low_surrogate);
							it_output_current += 2;
						}
					}
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				it_input_current += input_length;
				it_output_current += input_length;
			}
			else
			{
				GAL_PROMETHEUS_UNREACHABLE();
			}

			if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
			{
				return static_cast<std::size_t>(it_output_current - it_output_begin);
			}
			else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
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
		constexpr Scalar<"utf32"> utf32{};
	}
}// namespace gal::prometheus::chars
