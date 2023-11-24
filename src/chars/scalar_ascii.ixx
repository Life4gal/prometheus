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
	using input_category = CharsCategory::ASCII;
	using input_type	 = chars::input_type<input_category>;
	using pointer_type	 = input_type::const_pointer;
	using size_type		 = input_type::size_type;

	template<>
	class Scalar<"ascii">
	{
	public:
		template<bool WithError, CharsCategory OutputCategory>
		[[nodiscard]] constexpr auto convert(const input_type input, typename output_type<OutputCategory>::pointer output) const noexcept -> auto
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
			GAL_PROMETHEUS_DEBUG_NOT_NULL(output);

			using output_char_type						= typename output_type<OutputCategory>::value_type;
			using output_pointer_type					= typename output_type<OutputCategory>::pointer;

			const auto				  input_length		= input.size();

			const pointer_type		  it_input_begin	= input.data();
			pointer_type			  it_input_current	= it_input_begin;
			const pointer_type		  it_input_end		= it_input_begin + input_length;

			const output_pointer_type it_output_begin	= output;
			output_pointer_type		  it_output_current = it_output_begin;

			if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				while (it_input_current < it_input_end)
				{
					// try to convert the next block of 16 ASCII bytes
					// if it is safe to read 16 more bytes, check that they are ascii
					if (it_input_current + 16 <= it_input_end)
					{
						const auto v1 = utility::unaligned_load<std::uint64_t>(it_input_current);
						const auto v2 = utility::unaligned_load<std::uint64_t>(it_input_current + sizeof(std::uint64_t));

						// We are only interested in these bits: 1000'1000'1000'1000, so it makes sense to concatenate everything
						// if NONE of these are set, e.g. all of them are zero, then everything is ASCII
						if (const auto v = v1 | v2;
							(v & 0x8080808080808080) == 0)
						{
							std::ranges::transform(
									std::ranges::subrange{it_input_current, it_input_current + 16},
									it_output_current,
									[](const auto c) noexcept -> void
									{ return utility::char_cast<output_char_type>(c); });

							it_input_current += 16;
							it_output_current += 16;
							continue;
						}
					}

					if (const auto byte = utility::char_cast<unsigned char>(*it_input_current);
						(byte & 0x80) == 0)
					{
						// ASCII
						*it_output_current = utility::char_cast<output_char_type>(byte);

						it_input_current += 1;
						it_output_current += 1;
					}
					else
					{
						*it_output_current = utility::char_cast<output_char_type>((byte >> 6) | 0b1100'0000);
						it_output_current += 1;

						*it_output_current = utility::char_cast<output_char_type>((byte & 0b11'1111) | 0b1000'0000);
						it_output_current += 1;

						it_input_current += 1;
					}
				}

				if constexpr (const auto count = static_cast<std::size_t>(it_output_current - it_output_begin);
							  WithError)
				{
					return result_type{.error = ErrorCode::NONE, .count = count};
				}
				else
				{
					return count;
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				std::ranges::transform(
						it_input_current,
						it_input_end,
						it_output_current,
						[](const auto c) noexcept -> void
						{
							if constexpr (OutputCategory == CharsCategory::UTF16_BE)
							{
								return std::byteswap(utility::char_cast<output_char_type>(c));
							}
							else
							{
								return utility::char_cast<output_char_type>(c);
							}
						});

				if constexpr (WithError)
				{
					return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(input_length)};
				}
				else
				{
					return static_cast<std::size_t>(input_length);
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				std::ranges::transform(
						it_input_current,
						it_input_end,
						it_output_current,
						[](const auto c) noexcept -> void
						{ return utility::char_cast<output_char_type>(c); });

				if constexpr (WithError)
				{
					return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(input_length)};
				}
				else
				{
					return static_cast<std::size_t>(input_length);
				}
			}
			else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
		}
	};

	export namespace instance::scalar
	{
		constexpr Scalar<"ascii"> ascii{};
	}
}// namespace gal::prometheus::chars
