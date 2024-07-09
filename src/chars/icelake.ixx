// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <intrin.h>
#include <prometheus/macro.hpp>

export module gal.prometheus.chars:icelake;

import :encoding;
export import :icelake.ascii;
export import :icelake.utf8;
export import :icelake.utf16;
export import :icelake.utf32;

#else
#include <intrin.h>

#include <chars/encoding.ixx>
#include <chars/icelake_ascii.ixx>
#include <chars/icelake_utf8.ixx>
#include <chars/icelake_utf16.ixx>
#include <chars/icelake_utf32.ixx>
#endif

// ReSharper disable once CppRedundantNamespaceDefinition
GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::chars)
{
	template<>
	class Encoding<"icelake">
	{
	public:
		[[nodiscard]] constexpr static auto encoding_of(const std::span<const char8_t> input) noexcept -> EncodingType
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			if (const auto bom = bom_of(input); bom != EncodingType::UNKNOWN)
			{
				return bom;
			}

			if (const auto input_length = input.size();
				(input_length % 2) == 0)
			{
				const auto it_input_begin = input.data();
				auto it_input_current = it_input_begin;
				const auto it_input_end = it_input_begin + input_length;

				avx512_utf8_checker checker{};
				auto current_max = _mm512_setzero_si512();

				for (; it_input_current + 64 <= it_input_end; it_input_current += 64)
				{
					const auto in = _mm512_loadu_si512(it_input_current);
					const auto diff = _mm512_sub_epi16(in, _mm512_set1_epi16(static_cast<short>(0xd800)));
					if (const auto surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(0x0800));
						surrogates)
					{
						// Can still be either UTF-16LE or UTF-32 depending on the positions of the surrogates
						// To be valid UTF-32, a surrogate cannot be in the two most significant bytes of any 32-bit word.
						// On the other hand, to be valid UTF-16LE, at least one surrogate must be in the two most significant bytes of a 32-bit word since they always come in pairs in UTF-16LE.
						// Note that we always proceed in multiple of 4 before this point so there is no offset in 32-bit code units.

						if ((surrogates & 0xaaaa'aaaa) != 0)
						{
							const auto high_surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(0x0400));
							const auto low_surrogates = surrogates ^ high_surrogates;
							// high must be followed by low
							if ((high_surrogates << 1) != low_surrogates)
							{
								return EncodingType::UNKNOWN;
							}

							if ((high_surrogates & 0x8000'0000) != 0)
							{
								// advance only by 31 code units so that we start with the high surrogate on the next round.
								it_input_current += 31 * sizeof(char16_t);
							}
							else
							{
								it_input_current += 32 * sizeof(char16_t);
							}

							if (Simd<"icelake.utf16">::validate<std::endian::little>(
								{
										GAL_PROMETHEUS_SEMANTIC_UNRESTRICTED_CHAR_POINTER_CAST(Scalar<"utf16">::char_type, it_input_current),
										static_cast<Simd<"icelake.utf16">::size_type>((it_input_end - it_input_current) / sizeof(Scalar<"utf16">::char_type))
								}
							))
							{
								return EncodingType::UTF16_LE;
							}

							return EncodingType::UNKNOWN;
						}

						if ((input_length % 4) == 0 and
						    Simd<"icelake.utf32">::validate(
							    {
									    GAL_PROMETHEUS_SEMANTIC_UNRESTRICTED_CHAR_POINTER_CAST(Scalar<"utf32">::char_type, it_input_current),
									    static_cast<Simd<"icelake.utf32">::size_type>((it_input_end - it_input_current) / sizeof(Scalar<"utf32">::char_type))
							    }
						    ))
						{
							return EncodingType::UTF32_LE;
						}

						return EncodingType::UNKNOWN;
					}

					// If no surrogate, validate under other encodings as well

					// UTF-32 validation
					current_max = _mm512_max_epu32(in, current_max);
					// UTF-8 validation
					checker.check_input(in);
				}

				// Check which encodings are possible
				auto all_possible = std::to_underlying(EncodingType::UNKNOWN);
				const auto remaining = it_input_end - it_input_current;

				// utf8
				{
					if (remaining != 0)
					{
						const auto in = _mm512_maskz_loadu_epi8((__mmask64{1} << remaining) - 1, it_input_current);
						checker.check_input(in);
					}
					checker.check_eof();
					if (not checker.has_error())
					{
						all_possible |= std::to_underlying(EncodingType::UTF8);
					}
				}
				// utf16
				{
					if (Scalar<"utf16">::validate<std::endian::little>(
						{
								GAL_PROMETHEUS_SEMANTIC_UNRESTRICTED_CHAR_POINTER_CAST(Scalar<"utf16">::char_type, it_input_current),
								static_cast<Simd<"icelake.utf16">::size_type>(remaining / sizeof(Scalar<"utf16">::char_type))
						}
					))
					{
						all_possible |= std::to_underlying(EncodingType::UTF16_LE);
					}
				}
				// utf32
				{
					if ((input_length % 4) == 0)
					{
						current_max = _mm512_max_epu32(_mm512_maskz_loadu_epi8((__mmask64{1} << remaining) - 1, it_input_current), current_max);
						if (const auto outside_range = _mm512_cmp_epu32_mask(current_max, _mm512_set1_epi32(0x0010'ffff), _MM_CMPINT_GT);
							outside_range == 0)
						{
							all_possible |= std::to_underlying(EncodingType::UTF32_LE);
						}
					}
				}

				return static_cast<EncodingType>(all_possible);
			}

			if (Simd<"icelake.utf8">::validate(input))
			{
				return EncodingType::UTF8;
			}

			return EncodingType::UNKNOWN;
		}

		[[nodiscard]] constexpr static auto encoding_of(const std::span<const char> input) noexcept -> EncodingType
		{
			static_assert(sizeof(char) == sizeof(char8_t));

			const auto* char8_string = GAL_PROMETHEUS_SEMANTIC_UNRESTRICTED_CHAR_POINTER_CAST(char8_t, input.data());
			return encoding_of({char8_string, input.size()});
		}
	};
}
