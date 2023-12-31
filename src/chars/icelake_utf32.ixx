// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <intrin.h>

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:icelake.utf32;

import std;
import gal.prometheus.string;
import gal.prometheus.error;
import gal.prometheus.utility;

import :encoding;
import :converter;
import :table;
import :scalar;

namespace gal::prometheus::chars
{
	template<>
	class Simd<"utf32">
	{
	public:
		using scalar_type					 = Scalar<"utf32">;

		constexpr static auto input_category = scalar_type::input_category;
		using input_type					 = scalar_type::input_type;
		using char_type						 = scalar_type::char_type;
		using pointer_type					 = scalar_type::pointer_type;
		using size_type						 = scalar_type::size_type;

		template<bool ReturnResultType>
		[[nodiscard]] constexpr auto validate(const input_type input) const noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			const auto		   input_length		= input.size();

			const pointer_type it_input_begin	= input.data();
			pointer_type	   it_input_current = it_input_begin;
			const pointer_type it_input_end		= it_input_begin + input_length;

			if constexpr (ReturnResultType)
			{
				for (; it_input_current + 16 <= it_input_end; it_input_current += 16)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					const auto utf32		   = _mm512_loadu_si512(it_input_current);
					if (const auto outside_range = _mm512_cmp_epu32_mask(utf32, _mm512_set1_epi32(0x10'ffff), _MM_CMPINT_GT);
						outside_range) { return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + std::countr_zero(outside_range)}; }

					const auto utf32_offset = _mm512_add_epi32(utf32, _mm512_set1_epi32(0xffff'2000));
					if (const auto surrogate_range = _mm512_cmp_epu32_mask(utf32_offset, _mm512_set1_epi32(0xffff'f7ff), _MM_CMPINT_GT);
						surrogate_range) { return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error + std::countr_zero(surrogate_range)}; }
				}

				if (it_input_current < it_input_end)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					const auto utf32		   = _mm512_maskz_loadu_epi32((__mmask16{1} << (it_input_end - it_input_current)) - 1, it_input_current);
					if (const auto outside_range = _mm512_cmp_epu32_mask(utf32, _mm512_set1_epi32(0x10'ffff), _MM_CMPINT_GT);
						outside_range) { return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + std::countr_zero(outside_range)}; }

					const auto utf32_offset = _mm512_add_epi32(utf32, _mm512_set1_epi32(0xffff'2000));
					if (const auto surrogate_range = _mm512_cmp_epu32_mask(utf32_offset, _mm512_set1_epi32(0xffff'f7ff), _MM_CMPINT_GT);
						surrogate_range) { return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error + std::countr_zero(surrogate_range)}; }
				}

				return result_type{.error = ErrorCode::NONE, .count = input_length};
			}
			else
			{
				const auto offset			   = _mm512_set1_epi32(0xffff'2000);
				const auto standard_max		   = _mm512_set1_epi32(0x0010'ffff);
				const auto standard_offset_max = _mm512_set1_epi32(0xffff'f7ff);

				auto	   current_max		   = _mm512_setzero_si512();
				auto	   current_offset_max  = _mm512_setzero_si512();

				for (; it_input_current + 16 <= it_input_end; it_input_current += 16)
				{
					const auto utf32   = _mm512_loadu_si512(it_input_current);

					current_max		   = _mm512_max_epu32(utf32, current_max);
					current_offset_max = _mm512_max_epu32(_mm512_add_epi32(utf32, offset), current_offset_max);
				}

				if (const auto is_zero = _mm512_xor_si512(_mm512_max_epu32(current_max, standard_max), standard_max);
					_mm512_test_epi8_mask(is_zero, is_zero) != 0) { return false; }

				if (const auto is_zero = _mm512_xor_si512(_mm512_max_epu32(current_offset_max, standard_offset_max), standard_offset_max);
					_mm512_test_epi8_mask(is_zero, is_zero) != 0) { return false; }

				return static_cast<bool>(instance::scalar::utf32.validate({it_input_current, input_length - (it_input_current - it_input_begin)}));
			}
		}

		template<bool ReturnResultType>
		[[nodiscard]] constexpr auto validate(const pointer_type input) const noexcept -> result_type
		{
			return this->validate<ReturnResultType>({input, std::char_traits<char_type>::length(input)});
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory>
		[[nodiscard]] constexpr auto length(const input_type input) const noexcept -> size_type
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			const auto		   input_length		= input.size();

			const pointer_type it_input_begin	= input.data();
			pointer_type	   it_input_current = it_input_begin;
			const pointer_type it_input_end		= it_input_begin + input_length;

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				return instance::scalar::utf32.length<OutputCategory>(input);
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				const auto v_0000_007f	 = _mm512_set1_epi32(0x0000'0007f);
				const auto v_0000_07ff	 = _mm512_set1_epi32(0x0000'07ff);
				const auto v_0000_ffff	 = _mm512_set1_epi32(0x0000'ffff);

				auto	   result_length = static_cast<size_type>(0);

				for (; it_input_current + 16 <= it_input_end; it_input_current += 16)
				{
					const auto utf32			   = _mm512_loadu_si512(it_input_current);

					const auto ascii_bitmask	   = _mm512_cmple_epu32_mask(utf32, v_0000_007f);
					const auto two_bytes_bitmask   = _mm512_mask_cmple_epu32_mask(_knot_mask16(ascii_bitmask), utf32, v_0000_07ff);
					const auto three_bytes_bitmask = _mm512_mask_cmple_epu32_mask(_knot_mask16(_mm512_kor(ascii_bitmask, two_bytes_bitmask)), utf32, v_0000_ffff);

					const auto ascii_count		   = std::popcount(ascii_bitmask);
					const auto two_bytes_count	   = std::popcount(two_bytes_bitmask);
					const auto three_bytes_count   = std::popcount(three_bytes_bitmask);
					const auto four_bytes_count	   = 16 - ascii_count - two_bytes_count - three_bytes_count;

					result_length += ascii_count + 2 * two_bytes_count + 3 * three_bytes_count + 4 * four_bytes_count;
				}

				return result_length + instance::scalar::utf32.length<OutputCategory>({it_input_current, input_length - (it_input_current - it_input_begin)});
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				return instance::scalar::utf32.length<OutputCategory>(input);
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				return instance::scalar::utf32.length<OutputCategory>(input);
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

		// fixme: haswell implementation for now
		template<CharsCategory OutputCategory, InputProcessCriterion Criterion = InputProcessCriterion::RETURN_RESULT_TYPE, bool CheckNextBlock = true>
		[[nodiscard]] constexpr auto convert(const input_type input, typename output_type<OutputCategory>::pointer output) const noexcept -> std::conditional_t<Criterion == InputProcessCriterion::RETURN_RESULT_TYPE, result_type, std::size_t>
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
			GAL_PROMETHEUS_DEBUG_NOT_NULL(output);

			using output_pointer_type					= typename output_type<OutputCategory>::pointer;

			const auto				  input_length		= input.size();

			const pointer_type		  it_input_begin	= input.data();
			pointer_type			  it_input_current	= it_input_begin;
			const pointer_type		  it_input_end		= it_input_begin + input_length;

			const output_pointer_type it_output_begin	= output;
			output_pointer_type		  it_output_current = it_output_begin;

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				const auto v_0000_00ff	= _mm512_set1_epi32(0x0000'00ff);
				const auto shuffle_mask = _mm512_set_epi8(
						// clang-format off
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						0, 0, 0, 0, 0, 0, 0, 0,
						60, 56, 52, 48, 44, 40, 36, 32,
						28, 24, 20, 16, 12, 8, 4, 0
						// clang-format on
				);

				for (; it_input_current + 16 <= it_input_end; it_input_current += 16, it_output_current += 16)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					const auto in			   = _mm512_loadu_si512(it_input_current);

					if (_mm512_cmpgt_epu32_mask(in, v_0000_00ff))
					{
						if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							return 0;
						}
						else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
						{
							const auto it = std::ranges::find_if(
									it_input_current,
									it_input_current + 16,
									[](const auto data) noexcept
									{ return data > 0xff; });
							GAL_PROMETHEUS_DEBUG_ASSUME(it != it_input_current + 16);
							return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + std::ranges::distance(it_input_current, it)};
						}
						else
						{
							GAL_PROMETHEUS_STATIC_UNREACHABLE();
						}
					}

					_mm_storeu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current), _mm512_castsi512_si128(_mm512_permutexvar_epi8(shuffle_mask, in)));
				}

				if (const auto remaining = it_input_end - it_input_current;
					remaining != 0)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					const auto mask			   = static_cast<__mmask16>((1 << remaining) - 1);
					const auto in			   = _mm512_maskz_loadu_epi32(mask, it_input_current);

					if (_mm512_cmpgt_epu32_mask(in, v_0000_00ff))
					{
						if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							return 0;
						}
						else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
						{
							const auto it = std::ranges::find_if(
									it_input_current,
									it_input_current + 16,
									[](const auto data) noexcept
									{ return data > 0xff; });
							GAL_PROMETHEUS_DEBUG_ASSUME(it != it_input_current + 16);
							return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + std::ranges::distance(it_input_current, it)};
						}
						else
						{
							GAL_PROMETHEUS_STATIC_UNREACHABLE();
						}
					}

					_mm_mask_storeu_epi8(it_output_current, mask, _mm512_castsi512_si128(_mm512_permutexvar_epi8(shuffle_mask, in)));
					it_input_current += remaining;
					it_output_current += remaining;
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				const auto process = [&]() noexcept -> std::conditional_t<Criterion == InputProcessCriterion::RETURN_RESULT_TYPE, result_type, bool>
				{
					const auto	   v_0010_ffff		  = _mm256_set1_epi32(0x0010'ffff);
					const auto	   v_7fff_ffff		  = _mm256_set1_epi32(0x7fff'ffff);
					const auto	   v_0000_ff80		  = _mm256_set1_epi16(0x0000'ff80);
					const auto	   v_0000_0000		  = _mm256_setzero_si256();
					const auto	   v_0000_f800		  = _mm256_set1_epi16(0x0000'f800);
					const auto	   v_0000_1f00		  = _mm256_set1_epi16(0x0000'1f00);
					const auto	   v_0000_003f		  = _mm256_set1_epi16(0x0000'003f);
					const auto	   v_0000_c080		  = _mm256_set1_epi16(0x0000'c080);
					const auto	   v_ffff_0000		  = _mm256_set1_epi32(0xffff'0000);
					const auto	   v_0000_d800		  = _mm256_set1_epi16(0x0000'd800);

					// to avoid overruns
					constexpr auto safety_margin	  = 12;

					// used iff Criterion != InputProcessCriterion::RETURN_RESULT_TYPE
					auto		   running_max		  = _mm256_setzero_si256();
					auto		   forbidden_bytemask = _mm256_setzero_si256();

					while (it_input_current + 16 + safety_margin <= it_input_end)
					{
						const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

						const auto in			   = _mm256_loadu_si256(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m256i*, it_input_current + 0 * sizeof(__m256i) / sizeof(char_type)));
						const auto next_in		   = _mm256_loadu_si256(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m256i*, it_input_current + 1 * sizeof(__m256i) / sizeof(char_type)));

						if constexpr (Criterion != InputProcessCriterion::RETURN_RESULT_TYPE or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							running_max = _mm256_max_epu32(_mm256_max_epu32(in, running_max), next_in);
						}
						else
						{
							// Check for too large input
							if (const auto max_input = _mm256_max_epu32(_mm256_max_epu32(in, next_in), v_0010_ffff);
								static_cast<std::uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi32(max_input, v_0010_ffff))) != 0xffff'ffff)
							{
								return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
							}
						}

						// Pack 32-bit UTF-32 code units to 16-bit UTF-16 code units with unsigned saturation
						const auto in_16 = _mm256_permute4x64_epi64(_mm256_packus_epi32(_mm256_and_si256(in, v_7fff_ffff), _mm256_and_si256(next_in, v_7fff_ffff)), 0b1101'1000);

						// Try to apply UTF-16 => UTF-8 routine on 256 bits
						if (_mm256_testz_si256(in_16, v_0000_ff80))
						{
							// ascii
							// pack the bytes
							const auto utf8_packed = _mm_packus_epi16(_mm256_castsi256_si128(in_16), _mm256_extractf128_si256(in_16, 1));
							// store 16 bytes
							_mm_storeu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current), utf8_packed);
							// adjust iterators
							it_input_current += 16;
							it_output_current += 16;
							// we are done for this round
							continue;
						}

						// no bits set above 7th bit
						const auto one_byte_bytemask		 = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_0000_ff80), v_0000_0000);
						const auto one_byte_bitmask			 = static_cast<std::uint32_t>(_mm256_movemask_epi8(one_byte_bytemask));

						// no bits set above 11th bit
						const auto one_or_two_bytes_bytemask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_0000_f800), v_0000_0000);
						const auto one_or_two_bytes_bitmask	 = static_cast<std::uint32_t>(_mm256_movemask_epi8(one_or_two_bytes_bytemask));

						if (one_or_two_bytes_bitmask == 0xffff'ffff)
						{
							// 1.prepare 2-bytes values
							// input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
							// expected output   : [110a|aaaa|10bb|bbbb] x 8

							// t0 = [000a|aaaa|bbbb|bb00]
							const auto	t0			  = _mm256_slli_epi16(in_16, 2);
							// t1 = [000a|aaaa|0000|0000]
							const auto	t1			  = _mm256_and_si256(t0, v_0000_1f00);
							// t2 = [0000|0000|00bb|bbbb]
							const auto	t2			  = _mm256_and_si256(in_16, v_0000_003f);
							// t3 = [000a|aaaa|00bb|bbbb]
							const auto	t3			  = _mm256_or_si256(t1, t2);
							// t4 = [110a|aaaa|10bb|bbbb]
							const auto	t4			  = _mm256_or_si256(t3, v_0000_c080);

							// 2.merge ascii and 2-bytes codewords
							const auto	utf8_unpacked = _mm256_blendv_epi8(t4, in_16, one_byte_bytemask);

							// 3.prepare bitmask for 8-bits lookup
							const auto	mask_0		  = static_cast<std::uint32_t>(one_byte_bitmask & 0x5555'5555);
							const auto	mask_1		  = static_cast<std::uint32_t>(mask_0 >> 7);
							const auto	mask		  = static_cast<std::uint32_t>((mask_0 | mask_1) & 0x00ff'00ff);

							// 4.pack the bytes
							const auto	length_0	  = table_utf16_to_utf8::_1_2[utility::truncate<std::uint8_t>(mask)].front();
							const auto* row_0		  = table_utf16_to_utf8::_1_2[utility::truncate<std::uint8_t>(mask)].data() + 1;
							const auto	length_1	  = table_utf16_to_utf8::_1_2[utility::truncate<std::uint8_t>(mask >> 16)].front();
							const auto* row_1		  = table_utf16_to_utf8::_1_2[utility::truncate<std::uint8_t>(mask >> 16)].data() + 1;

							const auto	shuffle_0	  = _mm_loadu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_0));
							const auto	shuffle_1	  = _mm_loadu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_1));

							const auto	utf8_packed	  = _mm256_shuffle_epi8(utf8_unpacked, _mm256_setr_m128i(shuffle_0, shuffle_1));

							// 5.store the bytes
							_mm_storeu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current), _mm256_castsi256_si128(utf8_packed));
							it_output_current += length_0;
							_mm_storeu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current), _mm256_extractf128_si256(utf8_packed, 1));
							it_output_current += length_1;

							// 6.adjust iterators
							it_input_current += 16;
						}

						// check for overflow in packing
						const auto saturation_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(_mm256_or_si256(in, next_in), v_ffff_0000), v_0000_0000);
						const auto saturation_bitmask  = static_cast<std::uint32_t>(_mm256_movemask_epi8(saturation_bytemask));

						if (saturation_bitmask == 0xffff'ffff)
						{
							// code units from register produce either 1, 2 or 3 UTF-8 bytes

							if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
							{
								forbidden_bytemask = _mm256_or_si256(forbidden_bytemask, _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_0000_f800), v_0000_d800));
							}
							else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
							{
								forbidden_bytemask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_0000_f800), v_0000_d800);
								if (static_cast<std::uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) != 0x0)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
							}

							// In this branch we handle three cases:
							// 1. [0000|0000|0ccc|cccc] => [0ccc|cccc]                           - single UFT-8 byte
							// 2. [0000|0bbb|bbcc|cccc] => [110b|bbbb], [10cc|cccc]              - two UTF-8 bytes
							// 3. [aaaa|bbbb|bbcc|cccc] => [1110|aaaa], [10bb|bbbb], [10cc|cccc] - three UTF-8 bytes

							// We expand the input word (16-bit) into two code units (32-bit), thus we have room for four bytes.
							// However, we need five distinct bit layouts. Note that the last byte in cases #2 and #3 is the same.

							// We precompute byte 1 for case #1 and the common byte for cases #2 and #3 in register t2.
							// We precompute byte 1 for case #3 and -- **conditionally** -- precompute either byte 1 for case #2 or byte 2 for case #3. Note that they differ by exactly one bit.
							// Finally from these two code units we build proper UTF-8 sequence, taking into account the case (i.e, the number of bytes to write).

							// input  : [aaaa|bbbb|bbcc|cccc]
							// output :
							//         t2 => [0ccc|cccc] [10cc|cccc]
							//         s4 => [1110|aaaa] ([110b|bbbb] or [10bb|bbbb])

							// clang-format off
							const auto dup_even	  = _mm256_setr_epi16(
								0x0000, 0x0202, 0x0404, 0x0606,
								0x0808, 0x0a0a, 0x0c0c, 0x0e0e,
								0x0000, 0x0202, 0x0404, 0x0606,
								0x0808, 0x0a0a, 0x0c0c, 0x0e0e
								);
							// clang-format on

							// [aaaa|bbbb|bbcc|cccc] => [bbcc|cccc|bbcc|cccc]
							const auto t0	 = _mm256_shuffle_epi8(in_16, dup_even);
							// [bbcc|cccc|bbcc|cccc] => [00cc|cccc|0bcc|cccc]
							const auto t1	 = _mm256_and_si256(t0, _mm256_set1_epi16(0b0011'1111'0111'1111));
							// [00cc|cccc|0bcc|cccc] => [10cc|cccc|0bcc|cccc]
							const auto t2	 = _mm256_or_si256(t1, _mm256_set1_epi16(0b1000'0000'0000'0000));

							// [aaaa|bbbb|bbcc|cccc] =>  [0000|aaaa|bbbb|bbcc]
							const auto s0	 = _mm256_srli_epi16(in_16, 4);
							// [0000|aaaa|bbbb|bbcc] => [0000|aaaa|bbbb|bb00]
							const auto s1	 = _mm256_and_si256(s0, _mm256_set1_epi16(0b0000'1111'1111'1100));
							// [0000|aaaa|bbbb|bb00] => [00bb|bbbb|0000|aaaa]
							const auto s2	 = _mm256_maddubs_epi16(s1, _mm256_set1_epi16(0x0140));
							// [00bb|bbbb|0000|aaaa] => [11bb|bbbb|1110|aaaa]
							const auto s3	 = _mm256_or_si256(s2, _mm256_set1_epi16(0b1100'0000'1110'0000));
							const auto s4	 = _mm256_xor_si256(s3, _mm256_andnot_si256(one_or_two_bytes_bytemask, _mm256_set1_epi16(0b0100'0000'0000'0000)));

							// expand code units 16-bit => 32-bit
							const auto out_0 = _mm256_unpacklo_epi16(t2, s4);
							const auto out_1 = _mm256_unpackhi_epi16(t2, s4);
							// compress 32-bit code units into 1, 2 or 3 bytes -- 2 x shuffle
							const auto mask =
									(one_byte_bitmask & 0x5555'5555) |
									(one_or_two_bytes_bitmask & 0xaaaa'aaaa);

							const auto	length_0  = table_utf16_to_utf8::_1_2_3[utility::truncate<std::uint8_t>(mask)].front();
							const auto* row_0	  = table_utf16_to_utf8::_1_2_3[utility::truncate<std::uint8_t>(mask)].data() + 1;
							const auto	length_1  = table_utf16_to_utf8::_1_2_3[utility::truncate<std::uint8_t>(mask >> 8)].front();
							const auto* row_1	  = table_utf16_to_utf8::_1_2_3[utility::truncate<std::uint8_t>(mask >> 8)].data() + 1;
							const auto	length_2  = table_utf16_to_utf8::_1_2_3[utility::truncate<std::uint8_t>(mask >> 16)].front();
							const auto* row_2	  = table_utf16_to_utf8::_1_2_3[utility::truncate<std::uint8_t>(mask >> 16)].data() + 1;
							const auto	length_3  = table_utf16_to_utf8::_1_2_3[utility::truncate<std::uint8_t>(mask >> 24)].front();
							const auto* row_3	  = table_utf16_to_utf8::_1_2_3[utility::truncate<std::uint8_t>(mask >> 24)].data() + 1;

							const auto	shuffle_0 = _mm_loadu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_0));
							const auto	shuffle_1 = _mm_loadu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_1));
							const auto	shuffle_2 = _mm_loadu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_2));
							const auto	shuffle_3 = _mm_loadu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_3));

							const auto	utf8_0	  = _mm_shuffle_epi8(_mm256_castsi256_si128(out_0), shuffle_0);
							const auto	utf8_1	  = _mm_shuffle_epi8(_mm256_castsi256_si128(out_1), shuffle_1);
							const auto	utf8_2	  = _mm_shuffle_epi8(_mm256_extractf128_si256(out_0, 1), shuffle_2);
							const auto	utf8_3	  = _mm_shuffle_epi8(_mm256_extractf128_si256(out_1, 1), shuffle_3);

							_mm_storeu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current), utf8_0);
							it_output_current += length_0;
							_mm_storeu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current), utf8_1);
							it_output_current += length_1;
							_mm_storeu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current), utf8_2);
							it_output_current += length_2;
							_mm_storeu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current), utf8_3);
							it_output_current += length_3;

							it_input_current += 16;
						}
						else
						{
							// at least one 32-bit word is larger than 0xffff <=> it will produce four UTF-8 bytes
							// scalar fallback
							const auto step = static_cast<size_type>(it_input_end - it_input_current < 16 ? it_input_end - it_input_current : 16);
							if constexpr (auto result = instance::scalar::utf32.convert<OutputCategory, Criterion, CheckNextBlock>({it_input_current, step}, it_output_current);
										  Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
							{
								static_assert(std::is_same_v<decltype(result), std::size_t>);
								it_output_current += result;
								if (result == 0)
								{
									return false;
								}
							}
							else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
							{
								static_assert(std::is_same_v<decltype(result), result_type>);
								it_output_current += result.count;
								if (not result)
								{
									result.count += length_if_error;
								}
								return result;
							}
							else
							{
								GAL_PROMETHEUS_STATIC_UNREACHABLE();
							}
						}
					}

					if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
					{
						// check for invalid input
						if (static_cast<std::uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi32(_mm256_max_epu32(running_max, v_0010_ffff), v_0010_ffff))) != 0xffff'ffff)
						{
							return false;
						}
						if (static_cast<std::uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) != 0x0)
						{
							return false;
						}

						return true;
					}
					else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
					{
						return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
					}
					else
					{
						GAL_PROMETHEUS_STATIC_UNREACHABLE();
					}
				};

				const auto result = process();
				if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
				{
					if (not result)
					{
						return 0;
					}
				}
				else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
				{
					if (not result)
					{
						return result;
					}
				}
				else
				{
					GAL_PROMETHEUS_STATIC_UNREACHABLE();
				}

				if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					remaining != 0)
				{
					if (const auto scalar_result = instance::scalar::utf32.convert<OutputCategory, Criterion, CheckNextBlock>({it_input_current, remaining}, it_output_current);
						not scalar_result)
					{
						if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							return 0;
						}
						else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
						{
							return result_type{.error = scalar_result.error, .count = result.count + scalar_result.count};
						}
						else
						{
							GAL_PROMETHEUS_STATIC_UNREACHABLE();
						}
					}
					else
					{
						if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							it_output_current += scalar_result;
						}
						else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
						{
							it_output_current += scalar_result.count;
						}
						else
						{
							GAL_PROMETHEUS_STATIC_UNREACHABLE();
						}
					}
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				const auto process = [&]() noexcept -> std::conditional_t<Criterion == InputProcessCriterion::RETURN_RESULT_TYPE, result_type, bool>
				{
					const auto	   v_ffff_0000		  = _mm256_set1_epi32(0xffff'0000);
					const auto	   v_0000_0000		  = _mm256_setzero_si256();
					const auto	   v_0000_f800		  = _mm256_set1_epi16(0x0000'f800);
					const auto	   v_0000_d800		  = _mm256_set1_epi16(0x0000'd800);

					// to avoid overruns
					constexpr auto safety_margin	  = 12;

					// used iff Criterion != InputProcessCriterion::RETURN_RESULT_TYPE
					auto		   forbidden_bytemask = _mm256_setzero_si256();

					while (it_input_current + 8 + safety_margin <= it_input_end)
					{
						const auto length_if_error	   = static_cast<std::size_t>(it_input_current - it_input_begin);

						const auto in				   = _mm256_loadu_si256(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m256i*, it_input_current + 0));

						// no bits set above 16th bit <=> can pack to UTF16 without surrogate pairs
						const auto saturation_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffff_0000), v_0000_0000);
						const auto saturation_bitmask  = static_cast<std::uint32_t>(_mm256_movemask_epi8(saturation_bytemask));

						if (saturation_bitmask == 0xffff'ffff)
						{
							if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
							{
								forbidden_bytemask = _mm256_or_si256(forbidden_bytemask, _mm256_cmpeq_epi32(_mm256_and_si256(in, v_0000_f800), v_0000_d800));
							}
							else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
							{
								forbidden_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_0000_f800), v_0000_d800);
								if (static_cast<std::uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) != 0x0)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
							}

							const auto utf16_packed = [packed = _mm_packus_epi32(_mm256_castsi256_si128(in), _mm256_extractf128_si256(in, 1))]() noexcept
							{
								if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
								{
									return _mm_shuffle_epi8(packed, _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14));
								}
								else
								{
									return packed;
								}
							}();

							_mm_storeu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current), utf16_packed);

							it_input_current += 8;
							it_output_current += 8;
						}
						else
						{
							const auto step = static_cast<size_type>(it_input_end - it_input_current < 8 ? it_input_end - it_input_current : 8);
							if constexpr (auto result = instance::scalar::utf32.convert<OutputCategory, Criterion, CheckNextBlock>({it_input_current, step}, it_output_current);
										  Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
							{
								static_assert(std::is_same_v<decltype(result), std::size_t>);
								it_output_current += result;
								if (result == 0)
								{
									return false;
								}
							}
							else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
							{
								static_assert(std::is_same_v<decltype(result), result_type>);
								it_output_current += result.count;
								if (not result)
								{
									result.count += length_if_error;
								}
								return result;
							}
							else
							{
								GAL_PROMETHEUS_STATIC_UNREACHABLE();
							}
						}
					}

					if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
					{
						// check for invalid input
						if (static_cast<std::uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) != 0x0)
						{
							return false;
						}

						return true;
					}
					else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
					{
						return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
					}
					else
					{
						GAL_PROMETHEUS_STATIC_UNREACHABLE();
					}
				};

				const auto result = process();
				if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
				{
					if (not result)
					{
						return 0;
					}
				}
				else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
				{
					if (not result)
					{
						return result;
					}
				}
				else
				{
					GAL_PROMETHEUS_STATIC_UNREACHABLE();
				}

				if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					remaining != 0)
				{
					if (const auto scalar_result = instance::scalar::utf32.convert<OutputCategory, Criterion, CheckNextBlock>({it_input_current, remaining}, it_output_current);
						not scalar_result)
					{
						if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							return 0;
						}
						else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
						{
							return result_type{.error = scalar_result.error, .count = result.count + scalar_result.count};
						}
						else
						{
							GAL_PROMETHEUS_STATIC_UNREACHABLE();
						}
					}
					else
					{
						if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							it_output_current += scalar_result;
						}
						else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
						{
							it_output_current += scalar_result.count;
						}
						else
						{
							GAL_PROMETHEUS_STATIC_UNREACHABLE();
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

	export namespace instance::simd
	{
		constexpr Simd<"utf32"> utf32{};
	}
}// namespace gal::prometheus::chars
