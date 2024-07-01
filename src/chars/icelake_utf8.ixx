// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <intrin.h>
#include <prometheus/macro.hpp>

export module gal.prometheus.chars:icelake.utf8;

import std;
import gal.prometheus.error;
import gal.prometheus.meta;
import gal.prometheus.memory;

import :encoding;
import :scalar.utf8;

#else
#include <intrin.h>

#include <prometheus/macro.hpp>
#include <chars/encoding.ixx>
#include <error/error.ixx>
#include <meta/meta.ixx>
#endif

namespace gal::prometheus::chars
{
	namespace icelake_utf8_detail
	{
		using data_type = __m512i;

		template<unsigned I0, unsigned I1, unsigned I2, unsigned I3>
			requires
			(I0 <= 3) and
			(I1 <= 3) and
			(I2 <= 3) and
			(I3 <= 3)
		[[nodiscard]] auto shuffle(const data_type value) noexcept -> data_type
		{
			constexpr auto s = static_cast<int>(I0 | (I1 << 2) | (I2 << 4) | (I3 << 6));
			return _mm512_shuffle_i32x4(value, value, s);
		}

		template<unsigned I>
			requires(I <= 3)
		[[nodiscard]] auto broadcast(const data_type value) noexcept -> data_type //
		{
			return shuffle<I, I, I, I>(value);
		}

		[[nodiscard]] inline auto expand_and_identify(const data_type lane_0, const data_type lane_1) noexcept -> std::pair<data_type, int>
		{
			const auto expand_ver2 = _mm512_setr_epi64(
				0x0403'0201'0302'0100,
				0x0605'0403'0504'0302,
				0x0807'0605'0706'0504,
				0x0a09'0807'0908'0706,
				0x0c0b'0a09'0b0a'0908,
				0x0e0d'0c0b'0d0c'0b0a,
				0x000f'0e0d'0f0e'0d0c,
				0x0201'000f'0100'0f0e
			);
			const auto v_00c0 = _mm512_set1_epi32(0xc0);
			const auto v_0080 = _mm512_set1_epi32(0x80);

			const auto merged = _mm512_mask_mov_epi32(lane_0, 0x1000, lane_1);
			const auto input = _mm512_shuffle_epi8(merged, expand_ver2);
			const auto t0 = _mm512_and_si512(input, v_00c0);
			const auto leading_bytes = _mm512_cmpneq_epu32_mask(t0, v_0080);

			return {
					_mm512_mask_compress_epi32(_mm512_setzero_si512(), leading_bytes, input),
					static_cast<int>(std::popcount(leading_bytes))
			};
		}

		[[nodiscard]] inline auto expand_utf8_to_utf32(const data_type input, const data_type char_class) noexcept -> data_type
		{
			//  Input:
			//  - utf8: bytes stored at separate 32-bit code units
			//  - valid: which code units have valid UTF-8 characters
			// 
			//  Bit layout of single word. We show 4 cases for each possible UTF-8 character encoding.
			//  The `?` denotes bits we must not assume their value.
			// 
			//  |10dd.dddd|10cc.cccc|10bb.bbbb|1111.0aaa| 4-byte char
			//  |????.????|10cc.cccc|10bb.bbbb|1110.aaaa| 3-byte char
			//  |????.????|????.????|10bb.bbbb|110a.aaaa| 2-byte char
			//  |????.????|????.????|????.????|0aaa.aaaa| ASCII char
			//    byte 3    byte 2    byte 1     byte 0
			// ReSharper disable once CppJoinDeclarationAndAssignment
			data_type result;

			// 1. Reset control bits of continuation bytes and the MSB of the leading byte,
			// this makes all bytes unsigned (and does not alter ASCII char).
			// 
			// |00dd.dddd|00cc.cccc|00bb.bbbb|0111.0aaa| 4-byte char
			// |00??.????|00cc.cccc|00bb.bbbb|0110.aaaa| 3-byte char
			// |00??.????|00??.????|00bb.bbbb|010a.aaaa| 2-byte char
			// |00??.????|00??.????|00??.????|0aaa.aaaa| ASCII char
			//  ^^        ^^        ^^        ^
			result = _mm512_and_si512(input, _mm512_set1_epi32(0x3f3f'3f7f));

			// 2. Swap and join fields A-B and C-D
			// 
			// |0000.cccc|ccdd.dddd|0001.110a|aabb.bbbb| 4-byte char
			// |0000.cccc|cc??.????|0001.10aa|aabb.bbbb| 3-byte char
			// |0000.????|????.????|0001.0aaa|aabb.bbbb| 2-byte char
			// |0000.????|????.????|000a.aaaa|aa??.????| ASCII char
			result = _mm512_maddubs_epi16(result, _mm512_set1_epi32(0x0140'0140));

			// 3. Swap and join fields AB & CD
			// 
			// |0000.0001|110a.aabb|bbbb.cccc|ccdd.dddd| 4-byte char
			// |0000.0001|10aa.aabb|bbbb.cccc|cc??.????| 3-byte char
			// |0000.0001|0aaa.aabb|bbbb.????|????.????| 2-byte char
			// |0000.000a|aaaa.aa??|????.????|????.????| ASCII char
			result = _mm512_madd_epi16(result, _mm512_set1_epi32(0x0001'1000));

			// 4. Shift left the values by variable amounts to reset highest UTF-8 bits
			// |aaab.bbbb|bccc.cccd|dddd.d000|0000.0000| 4-byte char -- by 11
			// |aaaa.bbbb|bbcc.cccc|????.??00|0000.0000| 3-byte char -- by 10
			// |aaaa.abbb|bbb?.????|????.???0|0000.0000| 2-byte char -- by 9
			// |aaaa.aaa?|????.????|????.????|?000.0000| ASCII char -- by 7
			//
			//  continuation = 0
			//  ascii    = 7
			//  2_bytes = 9
			//  3_bytes = 10
			//  4_bytes = 11
			//
			//  shift_left_v3 = 4 * [
			//      ascii, # 0000
			//      ascii, # 0001
			//      ascii, # 0010
			//      ascii, # 0011
			//      ascii, # 0100
			//      ascii, # 0101
			//      ascii, # 0110
			//      ascii, # 0111
			//      continuation, # 1000
			//      continuation, # 1001
			//      continuation, # 1010
			//      continuation, # 1011
			//      2_bytes, # 1100
			//      2_bytes, # 1101
			//      3_bytes, # 1110
			//      4_bytes, # 1111
			// ]
			result = _mm512_sllv_epi32(
				result,
				_mm512_shuffle_epi8(
					// shift_left_v3
					_mm512_setr_epi64(
						0x0707'0707'0707'0707,
						0x0b0a'0909'0000'0000,
						0x0707'0707'0707'0707,
						0x0b0a'0909'0000'0000,
						0x0707'0707'0707'0707,
						0x0b0a'0909'0000'0000,
						0x0707'0707'0707'0707,
						0x0b0a'0909'0000'0000
					),
					char_class
				)
			);

			// 5. Shift right the values by variable amounts to reset lowest bits
			// |0000.0000|000a.aabb|bbbb.cccc|ccdd.dddd| 4-byte char -- by 11
			// |0000.0000|0000.0000|aaaa.bbbb|bbcc.cccc| 3-byte char -- by 16
			// |0000.0000|0000.0000|0000.0aaa|aabb.bbbb| 2-byte char -- by 21
			// |0000.0000|0000.0000|0000.0000|0aaa.aaaa| ASCII char -- by 25
			result = _mm512_srlv_epi32(
				result,
				_mm512_shuffle_epi8(
					// shift_right
					// 4 * [25, 25, 25, 25, 25, 25, 25, 25, 0, 0, 0, 0, 21, 21, 16, 11]
					_mm512_setr_epi64(
						0x1919'1919'1919'1919,
						0x0b10'1515'0000'0000,
						0x1919'1919'1919'1919,
						0x0b10'1515'0000'0000,
						0x1919'1919'1919'1919,
						0x0b10'1515'0000'0000,
						0x1919'1919'1919'1919,
						0x0b10'1515'0000'0000),
					char_class
				)
			);

			return result;
		}

		[[nodiscard]] inline auto expand_utf8_to_utf32(const data_type input) noexcept -> data_type
		{
			const auto v_0000_000f = _mm512_set1_epi32(0x0000'000f);
			const auto v_8080_8000 = _mm512_set1_epi32(static_cast<int>(0x8080'8000));

			const auto char_class = _mm512_ternarylogic_epi32(_mm512_srli_epi32(input, 4), v_0000_000f, v_8080_8000, 0xea);
			return expand_utf8_to_utf32(input, char_class);
		}

		template<CharsCategory Category>
		[[nodiscard]] auto store_ascii(const data_type in, const data_type byte_flip, auto it_output_current) noexcept -> std::size_t
		{
			if constexpr (Category == CharsCategory::UTF32)
			{
				const auto t0 = _mm512_castsi512_si128(in);
				const auto t1 = _mm512_extracti32x4_epi32(in, 1);
				const auto t2 = _mm512_extracti32x4_epi32(in, 2);
				const auto t3 = _mm512_extracti32x4_epi32(in, 3);

				_mm512_storeu_si512(it_output_current + 0 * 16, _mm512_cvtepu8_epi32(t0));
				_mm512_storeu_si512(it_output_current + 1 * 16, _mm512_cvtepu8_epi32(t1));
				_mm512_storeu_si512(it_output_current + 2 * 16, _mm512_cvtepu8_epi32(t2));
				_mm512_storeu_si512(it_output_current + 3 * 16, _mm512_cvtepu8_epi32(t3));
			}
			else if constexpr (Category == CharsCategory::UTF16_LE or Category == CharsCategory::UTF16_BE)
			{
				const auto h0 = _mm512_castsi512_si256(in);
				const auto h1 = _mm512_extracti64x4_epi64(in, 1);
				if constexpr (Category == CharsCategory::UTF16_BE)
				{
					_mm512_storeu_si512(it_output_current + 0 * 16, _mm512_shuffle_epi8(_mm512_cvtepu8_epi16(h0), byte_flip));
					_mm512_storeu_si512(it_output_current + 2 * 16, _mm512_shuffle_epi8(_mm512_cvtepu8_epi16(h1), byte_flip));
				}
				else
				{
					_mm512_storeu_si512(it_output_current + 0 * 16, _mm512_cvtepu8_epi16(h0));
					_mm512_storeu_si512(it_output_current + 2 * 16, _mm512_cvtepu8_epi16(h1));
				}
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }

			return 64;
		}

		// utf32_to_utf16 converts `count` lower UTF-32 code units from input `utf32` into UTF-16. It may overflow.
		// return how many 16-bit code units were stored.
		template<bool IsBigEndian, bool Masked>
		[[nodiscard]] auto utf32_to_utf16(const data_type input, const data_type byte_flip, const int count, auto it_output_current) noexcept -> std::size_t
		{
			GAL_PROMETHEUS_DEBUG_AXIOM(count > 0);

			const auto v_0000_ffff = _mm512_set1_epi32(0x0000'ffff);
			const auto v_0001_0000 = _mm512_set1_epi32(0x0001'0000);
			const auto v_ffff_0000 = _mm512_set1_epi32(static_cast<int>(0xffff'0000));
			const auto v_fc00_fc00 = _mm512_set1_epi32(static_cast<int>(0xfc00'fc00));
			const auto v_d800_dc00 = _mm512_set1_epi32(static_cast<int>(0xd800'dc00));

			// used only if Masked
			const auto count_mask = static_cast<__mmask16>((1 << count) - 1);

			// check if we have any surrogate pairs
			const auto surrogate_pair_mask = [input, v_0000_ffff, count_mask]
			{
				if constexpr (Masked) { return _mm512_mask_cmpgt_epu32_mask(count_mask, input, v_0000_ffff); }
				else { return _mm512_cmpgt_epu32_mask(input, v_0000_ffff); }
			}();
			if (surrogate_pair_mask == 0)
			{
				if constexpr (IsBigEndian)
				{
					if constexpr (Masked)
					{
						_mm256_mask_storeu_epi16(
							it_output_current,
							count_mask,
							_mm256_shuffle_epi8(_mm512_cvtepi32_epi16(input), _mm512_castsi512_si256(byte_flip))
						);
					}
					else
					{
						_mm256_storeu_si256(
							GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m256i*, it_output_current),
							_mm256_shuffle_epi8(
								_mm512_cvtepi32_epi16(input),
								_mm512_castsi512_si256(byte_flip)
							)
						);
					}
				}
				else
				{
					if constexpr (Masked) { _mm256_mask_storeu_epi16(it_output_current, count_mask, _mm512_cvtepi32_epi16(input)); }
					else
					{
						_mm256_storeu_si256(
							GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m256i*, it_output_current),
							_mm512_cvtepi32_epi16(input)
						);
					}
				}

				return count;
			}

			const auto total_count = count + std::popcount(surrogate_pair_mask);

			// build surrogate pair code units in 32-bit lanes

			// t0 = 8 x [000000000000aaaa|aaaaaabbbbbbbbbb]
			const auto t0 = _mm512_sub_epi32(input, v_0001_0000);
			// t1 = 8 x [000000aaaaaaaaaa|bbbbbbbbbb000000]
			const auto t1 = _mm512_slli_epi32(t0, 6);
			// t2 = 8 x [000000aaaaaaaaaa|aaaaaabbbbbbbbbb] -- copy hi word from t1 to t0
			// 0xe4 = (t1 and v_ffff_0000) or (t0 and not v_ffff_0000)
			const auto t2 = _mm512_ternarylogic_epi32(t1, t0, v_ffff_0000, 0xe4);
			// t2 = 8 x [110110aaaaaaaaaa|110111bbbbbbbbbb] -- copy hi word from t1 to t0
			// 0xba = (t2 and not v_fc00_fc000) or v_d800_dc00
			const auto t3 = _mm512_ternarylogic_epi32(t2, v_fc00_fc00, v_d800_dc00, 0xba);
			const auto t4 = _mm512_mask_blend_epi32(surrogate_pair_mask, input, t3);
			const auto t5 = [t4, byte_flip]
			{
				if constexpr (IsBigEndian) { return _mm512_shuffle_epi8(_mm512_ror_epi32(t4, 16), byte_flip); }
				else { return _mm512_ror_epi32(t4, 16); }
			}();

			const auto non_zero = _kor_mask32(0xaaaa'aaaa, _mm512_cmpneq_epi16_mask(t5, _mm512_setzero_si512()));
			// _mm512_mask_compressstoreu_epi16(it_output_current, non_zero, t5);
			_mm512_mask_storeu_epi16(
				it_output_current,
				static_cast<__mmask32>((1 << total_count) - 1),
				_mm512_maskz_compress_epi16(non_zero, t5)
			);

			return total_count;
		}

		template<CharsCategory Category, bool Masked>
		[[nodiscard]] auto store_utf16_or_utf32(const data_type in, const data_type byte_flip, const int count, auto it_output_current) noexcept -> std::size_t
		{
			if constexpr (Category == CharsCategory::UTF32)
			{
				if constexpr (Masked)
				{
					const auto mask = static_cast<__mmask16>((1 << count) - 1);
					_mm512_mask_storeu_epi32(it_output_current, mask, in);
				}
				else { _mm512_storeu_si512(it_output_current, in); }

				return count;
			}
			else if constexpr (Category == CharsCategory::UTF16_LE or Category == CharsCategory::UTF16_BE)
			{
				return icelake_utf8_detail::utf32_to_utf16<Category == CharsCategory::UTF16_BE, Masked>(
					in,
					byte_flip,
					count,
					it_output_current
				);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<CharsCategory Category, bool Masked>
		[[nodiscard]] auto transcode_16(const data_type lane_0, const data_type lane_1, const data_type byte_flip, auto it_output_current) noexcept -> std::size_t
		{
			// # lane{0,1,2} have got bytes:
			// [  0,  1,  2,  3,  4,  5,  6,  8,  9, 10, 11, 12, 13, 14, 15]
			// # lane3 has got bytes:
			// [ 16, 17, 18, 19,  4,  5,  6,  8,  9, 10, 11, 12, 13, 14, 15]
			//
			// expand_ver2 = [
			//     # lane 0:
			//     0, 1, 2, 3,
			//     1, 2, 3, 4,
			//     2, 3, 4, 5,
			//     3, 4, 5, 6,
			//
			//     # lane 1:
			//     4, 5, 6, 7,
			//     5, 6, 7, 8,
			//     6, 7, 8, 9,
			//     7, 8, 9, 10,
			//
			//     # lane 2:
			//      8,  9, 10, 11,
			//      9, 10, 11, 12,
			//     10, 11, 12, 13,
			//     11, 12, 13, 14,
			//
			//     # lane 3 order: 13, 14, 15, 16 14, 15, 16, 17, 15, 16, 17, 18, 16, 17, 18, 19
			//     12, 13, 14, 15,
			//     13, 14, 15,  0,
			//     14, 15,  0,  1,
			//     15,  0,  1,  2,
			// ]
			const auto expand_ver2 = _mm512_setr_epi64(
				0x0403'0201'0302'0100,
				0x0605'0403'0504'0302,
				0x0807'0605'0706'0504,
				0x0a09'0807'0908'0706,
				0x0c0b'0a09'0b0a'0908,
				0x0e0d'0c0b'0d0c'0b0a,
				0x000f'0e0d'0f0e'0d0c,
				0x0201'000f'0100'0f0e
			);
			const auto v_0000_00c0 = _mm512_set1_epi32(0x0000'00c0);
			const auto v_0000_0080 = _mm512_set1_epi32(0x0000'0080);

			const auto merged = _mm512_mask_mov_epi32(lane_0, 0x1000, lane_1);
			const auto input = _mm512_shuffle_epi8(merged, expand_ver2);

			const auto t0 = _mm512_and_si512(input, v_0000_00c0);
			const auto leading_bytes = _mm512_cmpneq_epu32_mask(t0, v_0000_0080);
			const auto utf32 = expand_utf8_to_utf32(input);
			const auto out = _mm512_mask_compress_epi32(_mm512_setzero_si512(), leading_bytes, utf32);
			const auto valid_count = std::popcount(leading_bytes);

			if constexpr (Category == CharsCategory::UTF32)
			{
				if constexpr (Masked) { _mm512_mask_storeu_epi32(it_output_current, static_cast<__mmask16>((1 << valid_count) - 1), out); }
				else { _mm512_storeu_si512(it_output_current, out); }

				return valid_count;
			}
			else if constexpr (Category == CharsCategory::UTF16_LE or Category == CharsCategory::UTF16_BE)
			{
				return utf32_to_utf16<Category == CharsCategory::UTF16_BE, Masked>(out, byte_flip, valid_count, it_output_current);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		struct avx512_utf8_checker
		{
		private:
			template<int N>
				requires(N <= 32)
			static data_type prev(const data_type input, const data_type prev_input) noexcept
			{
				const auto move_mask = _mm512_setr_epi32(28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
				const auto rotated = _mm512_permutex2var_epi32(input, move_mask, prev_input);

				return _mm512_alignr_epi8(input, rotated, 16 - N);
			}

		public:
			// If this is nonzero, there has been a UTF-8 error.
			data_type error{};
			// The last input we received
			data_type prev_input_block{};
			// Whether the last input we received was incomplete (used for ASCII fast path)
			data_type prev_incomplete{};

		private:
			// Check whether the current bytes are valid UTF-8.
			void check_utf8_bytes(const data_type input, const data_type prev_input) noexcept
			{
				// Flip prev1...prev3, so we can easily determine if they are 2+, 3+ or 4+ lead bytes
				// (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
				const auto prev_1 = avx512_utf8_checker::prev<1>(input, prev_input);

				// special cases
				const auto source = [input, prev_1]() noexcept -> data_type
				{
					const auto mask1 = _mm512_setr_epi64(
						0x0202'0202'0202'0202,
						0x4915'0121'8080'8080,
						0x0202'0202'0202'0202,
						0x4915'0121'8080'8080,
						0x0202'0202'0202'0202,
						0x4915'0121'8080'8080,
						0x0202'0202'0202'0202,
						0x4915'0121'8080'8080);
					const auto mask2 = _mm512_setr_epi64(
						static_cast<long long>(0xcbcb'cb8b'8383'a3e7),
						static_cast<long long>(0xcbcb'dbcb'cbcb'cbcb),
						static_cast<long long>(0xcbcb'cb8b'8383'a3e7),
						static_cast<long long>(0xcbcb'dbcb'cbcb'cbcb),
						static_cast<long long>(0xcbcb'cb8b'8383'a3e7),
						static_cast<long long>(0xcbcb'dbcb'cbcb'cbcb),
						static_cast<long long>(0xcbcb'cb8b'8383'a3e7),
						static_cast<long long>(0xcbcb'dbcb'cbcb'cbcb));
					const auto mask3 = _mm512_setr_epi64(
						0x0101'0101'0101'0101,
						0x0101'0101'baba'aee6,
						0x0101'0101'0101'0101,
						0x0101'0101'baba'aee6,
						0x0101'0101'0101'0101,
						0x0101'0101'baba'aee6,
						0x0101'0101'0101'0101,
						0x0101'0101'baba'aee6);

					const auto v_0f = _mm512_set1_epi8(static_cast<char>(0x0f));

					const auto index1 = _mm512_and_si512(_mm512_srli_epi16(prev_1, 4), v_0f);
					const auto index2 = _mm512_and_si512(prev_1, v_0f);
					const auto index3 = _mm512_and_si512(_mm512_srli_epi16(input, 4), v_0f);

					const auto byte_1_high = _mm512_shuffle_epi8(mask1, index1);
					const auto byte_1_low = _mm512_shuffle_epi8(mask2, index2);
					const auto byte_2_high = _mm512_shuffle_epi8(mask3, index3);

					return _mm512_ternarylogic_epi64(byte_1_high, byte_1_low, byte_2_high, 128);
				}();

				// multibyte length
				const auto length = [input, prev_input, source]() noexcept -> data_type
				{
					const auto v_7f = _mm512_set1_epi8(static_cast<char>(0x7f));
					const auto v_80 = _mm512_set1_epi8(static_cast<char>(0x80));

					const auto prev_2 = avx512_utf8_checker::prev<2>(input, prev_input);
					const auto prev_3 = avx512_utf8_checker::prev<3>(input, prev_input);

					// Only 111????? will be > 0
					const auto third = _mm512_subs_epu8(prev_2, _mm512_set1_epi8(static_cast<char>(0b1110'0000 - 1)));
					// Only 1111???? will be > 0
					const auto fourth = _mm512_subs_epu8(prev_3, _mm512_set1_epi8(static_cast<char>(0b1111'0000 - 1)));
					const auto third_or_fourth = _mm512_or_si512(third, fourth);

					return _mm512_ternarylogic_epi32(_mm512_adds_epu8(v_7f, third_or_fourth), v_80, source, 0b110'1010);
				}();

				error = _mm512_or_si512(length, error);
			}

			// Return nonzero if there are incomplete multibyte characters at the end of the block:
			// e.g. if there is a 4-byte character, but it's 3 bytes from the end.
			void check_incomplete(const data_type input) noexcept
			{
				// If the previous input's last 3 bytes match this, they're too short (they ended at EOF):
				// ... 1111???? 111????? 11??????
				const auto max_value = _mm512_setr_epi64(
					static_cast<long long>(0xffff'ffff'ffff'ffff),
					static_cast<long long>(0xffff'ffff'ffff'ffff),
					static_cast<long long>(0xffff'ffff'ffff'ffff),
					static_cast<long long>(0xffff'ffff'ffff'ffff),
					static_cast<long long>(0xffff'ffff'ffff'ffff),
					static_cast<long long>(0xffff'ffff'ffff'ffff),
					static_cast<long long>(0xffff'ffff'ffff'ffff),
					static_cast<long long>(0xbfdf'efff'ffff'ffff));
				prev_incomplete = _mm512_subs_epu8(input, max_value);
			}

		public:
			void check_eof() noexcept
			{
				// The only problem that can happen at EOF is that a multi-byte character is too short
				// or a byte value too large in the last bytes: check_utf8_bytes::check_special_cases only
				// checks for bytes too large in the first of two bytes.

				// If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
				// possibly finish them.
				error = _mm512_or_si512(error, prev_incomplete);
			}

			[[nodiscard]] bool has_error() const noexcept { return _mm512_test_epi8_mask(error, error) != 0; }

			// returns true if ASCII.
			bool check_input(const data_type input) noexcept
			{
				const auto v_80 = _mm512_set1_epi8(static_cast<char>(0x80));

				if (const auto ascii = _mm512_test_epi8_mask(input, v_80);
					ascii == 0)
				{
					check_eof();
					return true;
				}

				check_utf8_bytes(input, prev_input_block);
				check_incomplete(input);
				prev_input_block = input;
				return false;
			}
		};

		template<CharsCategory InputCategory>
		class SimdUtf8Base
		{
		public:
			using scalar_type = std::conditional_t<InputCategory == CharsCategory::UTF8, Scalar<"utf8">, Scalar<"utf8_char">>;

			constexpr static auto input_category = scalar_type::input_category;
			using input_type = typename scalar_type::input_type;
			using char_type = typename scalar_type::char_type;
			using pointer_type = typename scalar_type::pointer_type;
			using size_type = typename scalar_type::size_type;

			template<bool ReturnResultType = false>
			[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				avx512_utf8_checker checker{};

				std::size_t count = 0;
				for (; it_input_current + 64 <= it_input_end; it_input_current += 64, count += 64)
				{
					const auto in = _mm512_loadu_si512(it_input_current);
					checker.check_input(in);

					if constexpr (ReturnResultType)
					{
						if (checker.has_error())
						{
							if (count != 0)
							{
								// Sometimes the error is only detected in the next chunk
								count -= 1;
							}

							auto result = scalar_type::rewind_and_validate(it_input_begin, it_input_begin + count, it_input_end);
							result.count += count;
							return result;
						}
					}
				}

				const auto in = _mm512_maskz_loadu_epi8((__mmask64{1} << (it_input_end - it_input_current)) - 1, it_input_current);
				checker.check_input(in);

				if constexpr (ReturnResultType)
				{
					if (checker.has_error())
					{
						if (count != 0)
						{
							// Sometimes the error is only detected in the next chunk
							count -= 1;
						}

						auto result = scalar_type::rewind_and_validate(it_input_begin, it_input_begin + count, it_input_end);
						result.count += count;
						return result;
					}
				}
				else { checker.check_eof(); }

				if constexpr (ReturnResultType) { return result_type{.error = ErrorCode::NONE, .count = input_length}; }
				else { return not checker.has_error(); }
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
				GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				if constexpr (OutputCategory == CharsCategory::ASCII)
				{
					const auto continuation = _mm512_set1_epi8(static_cast<char>(0b1011'1111));

					__m512i unrolled_length = _mm512_setzero_si512();
					auto reparation_length = static_cast<size_type>(0);

					while (it_input_current + sizeof(__m512i) <= it_input_end)
					{
						const auto iterations = static_cast<size_type>((it_input_end - it_input_current) / sizeof(__m512i));
						const auto this_turn_end = it_input_current + iterations * sizeof(__m512i) - sizeof(__m512i);

						for (; it_input_current + 8 * sizeof(__m512i) <= this_turn_end; it_input_current += 8 * sizeof(__m512i))
						{
							const auto in_0 = _mm512_loadu_si512(
								GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 0 * sizeof(__m512i)));
							const auto in_1 = _mm512_loadu_si512(
								GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 1 * sizeof(__m512i)));
							const auto in_2 = _mm512_loadu_si512(
								GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 2 * sizeof(__m512i)));
							const auto in_3 = _mm512_loadu_si512(
								GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 3 * sizeof(__m512i)));
							const auto in_4 = _mm512_loadu_si512(
								GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 4 * sizeof(__m512i)));
							const auto in_5 = _mm512_loadu_si512(
								GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 5 * sizeof(__m512i)));
							const auto in_6 = _mm512_loadu_si512(
								GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 6 * sizeof(__m512i)));
							const auto in_7 = _mm512_loadu_si512(
								GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 7 * sizeof(__m512i)));

							const auto mask_0 = _mm512_cmple_epi8_mask(in_0, continuation);
							const auto mask_1 = _mm512_cmple_epi8_mask(in_1, continuation);
							const auto mask_2 = _mm512_cmple_epi8_mask(in_2, continuation);
							const auto mask_3 = _mm512_cmple_epi8_mask(in_3, continuation);
							const auto mask_4 = _mm512_cmple_epi8_mask(in_4, continuation);
							const auto mask_5 = _mm512_cmple_epi8_mask(in_5, continuation);
							const auto mask_6 = _mm512_cmple_epi8_mask(in_6, continuation);
							const auto mask_7 = _mm512_cmple_epi8_mask(in_7, continuation);

							const auto mask_register = _mm512_set_epi64(mask_7, mask_6, mask_5, mask_4, mask_3, mask_2, mask_1, mask_0);

							unrolled_length = _mm512_add_epi64(unrolled_length, _mm512_popcnt_epi64(mask_register));
						}

						for (; it_input_current <= this_turn_end; it_input_current += sizeof(__m512i))
						{
							const auto in = _mm512_loadu_si512(
								GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i*, it_input_current + 0 * sizeof(__m512i))
							);
							const auto continuation_bitmask = _mm512_cmple_epi8_mask(in, continuation);
							reparation_length += std::popcount(continuation_bitmask);
						}
					}

					const auto half_0 = _mm512_extracti64x4_epi64(unrolled_length, 0);
					const auto half_1 = _mm512_extracti64x4_epi64(unrolled_length, 1);
					const auto result_length =
							// number of 512-bit chunks that fits into the length.
							input_length / sizeof(__m512i) * sizeof(__m512i) + //
							-reparation_length +
							-_mm256_extract_epi64(half_0, 0) +
							-_mm256_extract_epi64(half_0, 1) +
							-_mm256_extract_epi64(half_0, 2) +
							-_mm256_extract_epi64(half_0, 3) +
							-_mm256_extract_epi64(half_1, 0) +
							-_mm256_extract_epi64(half_1, 1) +
							-_mm256_extract_epi64(half_1, 2) +
							-_mm256_extract_epi64(half_1, 3);

					return result_length + scalar_type::code_points({it_input_current, static_cast<size_type>(it_input_end - it_input_current)});
				}
				else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8) { return input.size(); } // NOLINT(bugprone-branch-clone)
				else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE or OutputCategory == CharsCategory::UTF16)
				{
					auto result_length = static_cast<size_type>(0);

					for (; it_input_current + 64 <= it_input_end; it_input_current += 64)
					{
						// @see scalar_type::code_points	
						// @see scalar_type::length	

						const auto in = _mm512_loadu_si512(it_input_current);

						const auto utf8_continuation_mask = _mm512_cmple_epi8_mask(in, _mm512_set1_epi8(-65 + 1));
						// We count one word for anything that is not a continuation (so leading bytes).
						result_length += 64 - std::popcount(utf8_continuation_mask);

						const auto utf8_4_byte = _mm512_cmpge_epu8_mask(in, _mm512_set1_epi8(static_cast<char>(240)));
						result_length += std::popcount(utf8_4_byte);
					}

					return result_length + scalar_type::template length<OutputCategory>({it_input_current, static_cast<size_type>(it_input_end - it_input_current)});
				}
				else if constexpr (OutputCategory == CharsCategory::UTF32) { return length<CharsCategory::ASCII>(input); }
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
				GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
				GAL_PROMETHEUS_DEBUG_NOT_NULL(output);

				using output_pointer_type = typename output_type<OutputCategory>::pointer;
				// using output_char_type = typename output_type<OutputCategory>::value_type;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				if constexpr (OutputCategory == CharsCategory::ASCII)
				{
					const auto process =
							[&it_output_current]<bool MaskOut>(
						const input_type process_input,
						__mmask64& out_next_leading,
						__mmask64& out_next_bit6
					) noexcept -> bool
					{
						// 11111111111 ... 1100 0000
						const auto minus64 = _mm512_set1_epi8(-64);
						const auto one = _mm512_set1_epi8(1);

						const auto input_process_length = process_input.size();
						auto it_input_process_current = process_input.data();

						const auto load_mask = [](const auto length)
						{
							if constexpr (MaskOut) { return _bzhi_u64(~0ull, static_cast<unsigned int>(length)); }
							else { return 0xffff'ffff'ffff'ffff; }
						}(input_process_length);

						const auto in = _mm512_maskz_loadu_epi8(load_mask, it_input_process_current);
						const auto non_ascii = _mm512_movepi8_mask(in);

						if (non_ascii == 0)
						{
							if constexpr (MaskOut) { _mm512_mask_storeu_epi8(it_output_current, load_mask, in); }
							else { _mm512_storeu_si512(it_output_current, in); }

							it_output_current += input_process_length;
							return true;
						}

						const auto leading = _mm512_cmpge_epu8_mask(in, minus64);
						const auto high_bits = _mm512_xor_si512(in, _mm512_set1_epi8(-62));

						if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
						{
							if (const auto invalid_leading_bytes = _mm512_mask_cmpgt_epu8_mask(leading, high_bits, one);
								invalid_leading_bytes) { return false; }

							if (const auto leading_shift = (leading << 1) | out_next_leading;
								(non_ascii ^ leading) != leading_shift) { return false; }
						}

						const auto bit6 = _mm512_cmpeq_epi8_mask(high_bits, one);
						const auto retain = ~leading & load_mask;
						const auto written_out = std::popcount(retain);
						const auto store_mask = (__mmask64{1} << written_out) - 1;

						const auto out = _mm512_maskz_compress_epi8(
							retain,
							_mm512_mask_sub_epi8(in, (bit6 << 1) | out_next_bit6, in, minus64));

						_mm512_mask_storeu_epi8(it_output_current, store_mask, out);

						it_output_current += written_out;
						out_next_leading = leading >> 63;
						out_next_bit6 = bit6 >> 63;

						return true;
					};

					const auto fallback = [&it_output_current, it_output_begin](const input_type fallback_input) noexcept -> auto
					{
						const auto result = scalar_type::template convert<OutputCategory, ProcessPolicy, CheckNextBlock>(
							fallback_input,
							it_output_current
						);
						if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
						{
							return result_type{result.error, result.count + static_cast<std::size_t>(it_output_current - it_output_begin)};
						}
						else { return result; }
					};

					__mmask64 next_leading = 0;
					__mmask64 next_bit6 = 0;

					for (; it_input_current + 64 <= it_input_end; it_input_current += 64)
					{
						const auto process_result = process.template operator()<false>({it_input_current, 64}, next_leading, next_bit6);

						if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT) { GAL_PROMETHEUS_DEBUG_ASSUME(process_result == true); }
						else { if (not process_result) { return fallback({it_input_current, static_cast<size_type>(it_input_end - it_input_current)}); } }
					}

					if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
						remaining != 0)
					{
						const auto process_result = process.template operator()<true>({it_input_current, remaining}, next_leading, next_bit6);

						if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT) { GAL_PROMETHEUS_DEBUG_ASSUME(process_result == true); }
						else { if (not process_result) { return fallback({it_input_current, remaining}); } }
					}
				}
				else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
				{
					std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
					it_input_current += input_length;
					it_output_current += input_length;
				}
				else if constexpr (
					OutputCategory == CharsCategory::UTF16_LE or
					OutputCategory == CharsCategory::UTF16_BE or
					OutputCategory == CharsCategory::UTF32
				)
				{
					const auto byte_flip = _mm512_setr_epi64(
						0x0607'0405'0203'0001,
						0x0e0f'0c0d'0a0b'0809,
						0x0607'0405'0203'0001,
						0x0e0f'0c0d'0a0b'0809,
						0x0607'0405'0203'0001,
						0x0e0f'0c0d'0a0b'0809,
						0x0607'0405'0203'0001,
						0x0e0f'0c0d'0a0b'0809);

					if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
					{
						if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
						{
							constexpr auto is_big_endian = OutputCategory == CharsCategory::UTF16_BE;

							const auto process = [&it_input_current, &it_output_current, byte_flip]<bool MaskOut>(const auto remaining) noexcept -> bool
							{
								// clang-format off
								const auto mask_identity = _mm512_set_epi8(
									63,
									62,
									61,
									60,
									59,
									58,
									57,
									56,
									55,
									54,
									53,
									52,
									51,
									50,
									49,
									48,
									47,
									46,
									45,
									44,
									43,
									42,
									41,
									40,
									39,
									38,
									37,
									36,
									35,
									34,
									33,
									32,
									31,
									30,
									29,
									28,
									27,
									26,
									25,
									24,
									23,
									22,
									21,
									20,
									19,
									18,
									17,
									16,
									15,
									14,
									13,
									12,
									11,
									10,
									9,
									8,
									7,
									6,
									5,
									4,
									3,
									2,
									1,
									0
								);
								// clang-format on
								const auto v_c0c0_c0c0 = _mm512_set1_epi32(static_cast<int>(0xc0c0'c0c0));
								const auto v_0400_0400 = _mm512_set1_epi32(0x0400'0400);
								const auto v_8080_8080 = _mm512_set1_epi32(static_cast<int>(0x8080'8080));
								const auto v_0800_0800 = _mm512_set1_epi32(0x0800'0800);
								const auto v_d800_d800 = _mm512_set1_epi32(static_cast<int>(0xd800'd800));
								const auto v_f0f0_f0f0 = _mm512_set1_epi32(static_cast<int>(0xf0f0'f0f0));
								const auto v_dfdf_dfdf = _mm512_set_epi64(
									static_cast<long long>(0xffff'dfdf'dfdf'dfdf),
									static_cast<long long>(0xdfdf'dfdf'dfdf'dfdf),
									static_cast<long long>(0xdfdf'dfdf'dfdf'dfdf),
									static_cast<long long>(0xdfdf'dfdf'dfdf'dfdf),
									static_cast<long long>(0xdfdf'dfdf'dfdf'dfdf),
									static_cast<long long>(0xdfdf'dfdf'dfdf'dfdf),
									static_cast<long long>(0xdfdf'dfdf'dfdf'dfdf),
									static_cast<long long>(0xdfdf'dfdf'dfdf'dfdf)
								);
								const auto v_c2c2_c2c2 = _mm512_set1_epi32(static_cast<int>(0xc2c2'c2c2));
								const auto v_ffff_ffff = _mm512_set1_epi32(static_cast<int>(0xffff'ffff));
								const auto v_d7c0_d7c0 = _mm512_set1_epi32(static_cast<int>(0xd7c0'd7c0));
								const auto v_dc00_dc00 = _mm512_set1_epi32(static_cast<int>(0xdc00'dc00));

								const auto remaining_mask = [](const auto length) noexcept -> auto
								{
									if constexpr (MaskOut) { return _bzhi_u64(~0ull, static_cast<unsigned int>(length)); }
									else { return 0xffff'ffff'ffff'ffff; }
								}(remaining);

								const auto in = _mm512_maskz_loadu_epi8(remaining_mask, it_input_current);
								const auto mask_byte_1 = _mm512_mask_cmplt_epu8_mask(remaining_mask, in, v_8080_8080);

								// NOT(mask_byte_1) AND valid_input_length -- if all zeroes, then all ASCII
								if (_ktestc_mask64_u8(mask_byte_1, remaining_mask))
								{
									if constexpr (MaskOut)
									{
										it_input_current += remaining;

										const auto in_1 = [in, byte_flip]() noexcept -> auto
										{
											if constexpr (is_big_endian)
											{
												return _mm512_shuffle_epi8(_mm512_cvtepu8_epi16(_mm512_castsi512_si256(in)), byte_flip);
											}
											else { return _mm512_cvtepu8_epi16(_mm512_castsi512_si256(in)); }
										}();

										if (remaining <= 32)
										{
											_mm512_mask_storeu_epi16(it_output_current, (__mmask32{1} << remaining) - 1, in_1);
											it_output_current += remaining;
										}
										else
										{
											const auto in_2 = [in, byte_flip]() noexcept -> auto
											{
												if constexpr (is_big_endian)
												{
													return _mm512_shuffle_epi8(_mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(in, 1)), byte_flip);
												}
												else { return _mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(in, 1)); }
											}();

											_mm512_storeu_si512(it_output_current, in_1);
											it_output_current += 32;
											_mm512_mask_storeu_epi16(it_output_current, (__mmask32{1} << (remaining - 32)) - 1, in_2);
											it_output_current += remaining - 32;
										}
									}
									else
									{
										// we convert a full 64-byte block, writing 128 bytes.
										it_input_current += 64;

										const auto in_1 = [in, byte_flip]() noexcept -> auto
										{
											if constexpr (is_big_endian)
											{
												return _mm512_shuffle_epi8(_mm512_cvtepu8_epi16(_mm512_castsi512_si256(in)), byte_flip);
											}
											else { return _mm512_cvtepu8_epi16(_mm512_castsi512_si256(in)); }
										}();
										const auto in_2 = [in, byte_flip]() noexcept -> auto
										{
											if constexpr (is_big_endian)
											{
												return _mm512_shuffle_epi8(_mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(in, 1)), byte_flip);
											}
											else { return _mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(in, 1)); }
										}();

										_mm512_storeu_si512(it_output_current, in_1);
										it_output_current += 32;
										_mm512_storeu_si512(it_output_current, in_2);
										it_output_current += 32;
									}

									return true;
								}

								// classify characters further

								// 0xc0 <= in, 2, 3, or 4 leading byte
								const auto mask_byte_234 = _mm512_cmp_epu8_mask(v_c0c0_c0c0, in, _MM_CMPINT_LE);
								// 0xdf < in,  3 or 4 leading byte
								const auto mask_byte_34 = _mm512_cmp_epu8_mask(v_dfdf_dfdf, in, _MM_CMPINT_LT);

								// 0xc0 <= in < 0xc2 (illegal two byte sequence)
								if (const auto two_bytes = _mm512_mask_cmp_epu8_mask(mask_byte_234, in, v_c2c2_c2c2, _MM_CMPINT_LT);
									_ktestz_mask64_u8(two_bytes, two_bytes) == 0)
								{
									// Overlong 2-byte sequence
									return false;
								}

								if (_ktestz_mask64_u8(mask_byte_34, mask_byte_34) == 0)
								{
									// We have a 3-byte sequence and/or a 2-byte sequence, or possibly even a 4-byte sequence

									// 0xf0 <= zmm0 (4 byte start bytes)
									const auto mask_byte_4 = _mm512_cmp_epu8_mask(in, v_f0f0_f0f0, _MM_CMPINT_NLT);
									const auto mask_not_ascii = [remaining_mask, mask_byte_1]() noexcept -> auto
									{
										if constexpr (MaskOut) { return _kand_mask64(_knot_mask64(mask_byte_1), remaining_mask); }
										else { return _knot_mask64(mask_byte_1); }
									}();

									const auto mask_pattern_1 = _kshiftli_mask64(mask_byte_234, 1);
									const auto mask_patten_2 = _kshiftli_mask64(mask_byte_34, 2);
									if (mask_byte_4 == 0)
									{
										// expected continuation bytes
										const auto mask_combing = _kor_mask64(mask_pattern_1, mask_patten_2);
										const auto mask_byte_1234 = _kor_mask64(mask_byte_1, mask_byte_234);

										// mismatched continuation bytes
										if constexpr (MaskOut) { if (mask_combing != _kxor_mask64(remaining_mask, mask_byte_1234)) { return false; } }
										else
										{
											// XNOR of mask_combing and mask_byte_1234 should be all zero if they differ the presence of a 1 bit indicates that they overlap.
											if (const auto v = _kxnor_mask64(mask_combing, mask_byte_1234); not _kortestz_mask64_u8(v, v))
											{
												return false;
											}
										}

										// identifying the last bytes of each sequence to be decoded
										const auto mend = [mask_byte_1234, remaining]() noexcept -> auto
										{
											if constexpr (MaskOut) { return _kor_mask64(_kshiftri_mask64(mask_byte_1234, 1), __mmask64{1} << (remaining - 1)); }
											else { return _kshiftri_mask64(mask_byte_1234, 1); }
										}();

										const auto last_and_third = _mm512_maskz_compress_epi8(mend, mask_identity);
										const auto last_and_third_u16 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(last_and_third));
										// ASCII: 00000000  other: 11000000
										const auto non_ascii_tags = _mm512_maskz_mov_epi8(mask_not_ascii, v_c0c0_c0c0);
										// high two bits cleared where not ASCII
										const auto cleared_bytes = _mm512_andnot_si512(non_ascii_tags, in);
										// bytes that precede non-ASCII bytes
										const auto mask_before_non_ascii = _kshiftri_mask64(mask_not_ascii, 1);
										const auto before_ascii_bytes = _mm512_maskz_mov_epi8(mask_before_non_ascii, cleared_bytes);
										// the last byte of each character
										const auto last_bytes = _mm512_maskz_permutexvar_epi8(0x5555'5555'5555'5555, last_and_third_u16, cleared_bytes);

										// indices of the second last bytes
										const auto index_of_second_last_bytes = _mm512_add_epi16(v_ffff_ffff, last_and_third_u16);
										// shifted into position
										const auto second_last_bytes = _mm512_slli_epi16(
											// the second last bytes (of two, three byte seq, surrogates)
											_mm512_maskz_permutexvar_epi8(0x5555'5555'5555'5555, index_of_second_last_bytes, before_ascii_bytes),
											6);

										// indices of the third last bytes
										const auto index_of_third_last_bytes = _mm512_add_epi16(v_ffff_ffff, index_of_second_last_bytes);
										// shifted into position
										const auto third_last_bytes = _mm512_slli_epi16(
											// the third last bytes (of three byte sequences, high surrogate)
											_mm512_maskz_permutexvar_epi8(0x5555'5555'5555'5555,
											                              index_of_third_last_bytes,
											                              // only those that are the third last byte of a sequence
											                              _mm512_maskz_mov_epi8(mask_byte_34, cleared_bytes)),
											12);

										// the elements of out excluding the last element if it happens to be a high surrogate
										const auto out = _mm512_ternarylogic_epi32(last_bytes, second_last_bytes, third_last_bytes, 254);
										// Encodings out of range
										{
											// the location of 3-byte sequence start bytes in the input code units in word_out corresponding to 3-byte sequences.
											const auto m3 = static_cast<__mmask32>(_pext_u64((mask_byte_34 & (remaining_mask ^ mask_byte_4)) << 2, mend));
											const auto mask_out_less_than_0x800 = _mm512_mask_cmplt_epu16_mask(m3, out, v_0800_0800);
											const auto mask_out_minus_0x800 = _mm512_sub_epi16(out, v_d800_d800);
											const auto mask_out_too_small = _mm512_mask_cmplt_epu16_mask(m3, mask_out_minus_0x800, v_0800_0800);
											if (_kor_mask32(mask_out_less_than_0x800, mask_out_too_small) != 0) { return false; }
										}

										// we adjust mend at the end of the output.
										const auto mask_processed = [mend, remaining_mask]() noexcept -> auto
										{
											if constexpr (MaskOut) { return _pdep_u64(0xffff'ffff, _kand_mask64(mend, remaining_mask)); }
											else { return _pdep_u64(0xffff'ffff, mend); }
										}();

										const auto num_out = std::popcount(mask_processed);

										if constexpr (is_big_endian)
										{
											_mm512_mask_storeu_epi16(it_output_current, static_cast<__mmask32>((__mmask64{1} << num_out) - 1), _mm512_shuffle_epi8(out, byte_flip));
										}
										else { _mm512_mask_storeu_epi16(it_output_current, static_cast<__mmask32>((__mmask64{1} << num_out) - 1), out); }

										it_input_current += 64 - std::countl_zero(mask_processed);
										it_output_current += num_out;
										return true;
									}

									// We have a 4-byte sequence, this is the general case.
									const auto mask_pattern_3 = _kshiftli_mask64(mask_byte_4, 3);
									// expected continuation bytes
									const auto mask_combing = _kor_mask64(_kor_mask64(mask_pattern_1, mask_patten_2), mask_pattern_3);
									const auto mask_byte_1234 = _kor_mask64(mask_byte_1, mask_byte_234);

									// identifying the last bytes of each sequence to be decoded
									const auto mend = [mask_byte_1234, mask_pattern_3, remaining]() noexcept -> auto
									{
										if constexpr (MaskOut)
										{
											return _kor_mask64(
												_kor_mask64(_kshiftri_mask64(_kor_mask64(mask_pattern_3, mask_byte_1234), 1), mask_pattern_3),
												__mmask64{1} << (remaining - 1));
										}
										else { return _kor_mask64(_kshiftri_mask64(_kor_mask64(mask_pattern_3, mask_byte_1234), 1), mask_pattern_3); }
									}();

									const auto last_and_third = _mm512_maskz_compress_epi8(mend, mask_identity);
									const auto last_and_third_u16 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(last_and_third));
									// ASCII: 00000000  other: 11000000
									const auto non_ascii_tags = _mm512_maskz_mov_epi8(mask_not_ascii, v_c0c0_c0c0);
									// high two bits cleared where not ASCII
									const auto cleared_bytes = _mm512_andnot_si512(non_ascii_tags, in);
									// bytes that precede non-ASCII bytes
									const auto mask_before_non_ascii = _kshiftri_mask64(mask_not_ascii, 1);
									const auto before_ascii_bytes = _mm512_maskz_mov_epi8(mask_before_non_ascii, cleared_bytes);
									// the last byte of each character
									const auto last_bytes = _mm512_maskz_permutexvar_epi8(0x5555'5555'5555'5555, last_and_third_u16, cleared_bytes);

									// indices of the second last bytes
									const auto index_of_second_last_bytes = _mm512_add_epi16(v_ffff_ffff, last_and_third_u16);
									// shifted into position
									const auto second_last_bytes = _mm512_slli_epi16(
										// the second last bytes (of two, three byte seq, surrogates)
										_mm512_maskz_permutexvar_epi8(0x5555'5555'5555'5555, index_of_second_last_bytes, before_ascii_bytes),
										6);

									// indices of the third last bytes
									const auto index_of_third_last_bytes = _mm512_add_epi16(v_ffff_ffff, index_of_second_last_bytes);
									// shifted into position
									const auto third_last_bytes = _mm512_slli_epi16(
										// the third last bytes (of three byte sequences, high surrogate)
										_mm512_maskz_permutexvar_epi8(
											0x5555'5555'5555'5555,
											index_of_third_last_bytes,
											// only those that are the third last byte of a sequence
											_mm512_maskz_mov_epi8(mask_byte_34, cleared_bytes)
										),
										12);

									const auto third_second_and_last_bytes =
											_mm512_ternarylogic_epi32(last_bytes, second_last_bytes, third_last_bytes, 254);
									const auto mask_mp3_64 = _pext_u64(mask_pattern_3, mend);
									const auto mask_mp3_low = static_cast<__mmask32>(mask_mp3_64);
									const auto mask_mp3_high = static_cast<__mmask32>(mask_mp3_64 >> 1);
									// low surrogate: 1101110000000000, other:  0000000000000000
									const auto mask_low_surrogate = _mm512_maskz_mov_epi16(mask_mp3_low, v_dc00_dc00);
									const auto tagged_low_surrogate = _mm512_or_si512(third_second_and_last_bytes, mask_low_surrogate);
									const auto shifted4_third_second_and_last_bytes = _mm512_srli_epi16(third_second_and_last_bytes, 4);

									// the elements of out excluding the last element if it happens to be a high surrogate
									const auto out = _mm512_mask_add_epi16(
										tagged_low_surrogate,
										mask_mp3_high,
										shifted4_third_second_and_last_bytes,
										v_d7c0_d7c0
									);

									// mismatched continuation bytes
									if constexpr (MaskOut) { if (mask_combing != _kxor_mask64(remaining_mask, mask_byte_1234)) { return false; } }
									else
									{
										// XNOR of mask_combing and mask_byte_1234 should be all zero if they differ the presence of a 1 bit indicates that they overlap.
										if (const auto v = _kxnor_mask64(mask_combing, mask_byte_1234); not _kortestz_mask64_u8(v, v)) { return false; }
									}

									// Encodings out of range
									{
										// the location of 3-byte sequence start bytes in the input code units in word_out corresponding to 3-byte sequences.
										const auto m3 = static_cast<__mmask32>(_pext_u64(mask_byte_34 & (remaining_mask ^ mask_byte_4) << 2, mend));
										const auto mask_out_less_than_0x800 = _mm512_mask_cmplt_epu16_mask(m3, out, v_0800_0800);
										const auto mask_out_minus_0x800 = _mm512_sub_epi16(out, v_d800_d800);
										const auto mask_out_too_small = _mm512_mask_cmplt_epu16_mask(m3, mask_out_minus_0x800, v_0800_0800);
										const auto mask_out_greater_equal_0x400 =
												_mm512_mask_cmpge_epu16_mask(mask_mp3_high, mask_out_minus_0x800, v_0400_0400);
										if (_kortestz_mask32_u8(mask_out_greater_equal_0x400,
										                        _kor_mask32(mask_out_less_than_0x800, mask_out_too_small)) != 0) { return false; }
									}

									// we adjust mend at the end of the output.
									const auto mask_processed = [m = ~(mask_mp3_high & 0x8000'0000), mend, remaining_mask]() noexcept -> auto
									{
										if constexpr (MaskOut) { return _pdep_u64(m, _kand_mask64(mend, remaining_mask)); }
										else { return _pdep_u64(m, mend); }
									}();

									const auto num_out = std::popcount(mask_processed);

									if constexpr (is_big_endian)
									{
										_mm512_mask_storeu_epi16(
											it_output_current,
											(__mmask32{1} << num_out) - 1,
											_mm512_shuffle_epi8(out, byte_flip)
										);
									}
									else { _mm512_mask_storeu_epi16(it_output_current, (__mmask32{1} << num_out) - 1, out); }

									it_input_current += 64 - std::countl_zero(mask_processed);
									it_output_current += num_out;
									return true;
								}

								// all ASCII or 2 byte
								const auto continuation_or_ascii = [mask_byte_234, remaining_mask]() noexcept -> auto
								{
									if constexpr (MaskOut) { return _kand_mask64(_knot_mask64(mask_byte_234), remaining_mask); }
									else { return _knot_mask64(mask_byte_234); }
								}();

								// on top of -0xc0 we subtract -2 which we get back later of the continuation byte tags
								const auto leading_two_bytes = _mm512_maskz_sub_epi8(mask_byte_234, in, v_c2c2_c2c2);
								const auto leading_mask = [mask_byte_1, mask_byte_234, remaining_mask]() noexcept -> auto
								{
									if constexpr (MaskOut) { return _kand_mask64(_kor_mask64(mask_byte_1, mask_byte_234), remaining_mask); }
									else { return _kor_mask64(mask_byte_1, mask_byte_234); }
								}();

								if constexpr (MaskOut)
								{
									if (_kshiftli_mask64(mask_byte_234, 1) != _kxor_mask64(remaining_mask, leading_mask)) { return false; }
								}
								else
								{
									if (const auto v = _kxnor_mask64(_kshiftli_mask64(mask_byte_234, 1), leading_mask); not _kortestz_mask64_u8(v, v))
									{
										return false;
									}
								}

								if constexpr (MaskOut)
								{
									it_input_current += 64 - std::countl_zero(_pdep_u64(0xffff'ffff, continuation_or_ascii));
								}
								else
								{
									// In the two-byte/ASCII scenario, we are easily latency bound,
									// so we want to increment the input buffer as quickly as possible.
									// We process 32 bytes unless the byte at index 32 is a continuation byte,
									// in which case we include it as well for a total of 33 bytes.
									// Note that if x is an ASCII byte, then the following is false:
									// int8_t(x) <= int8_t(0xc0) under two's complement.
									it_input_current += 32;
									if (static_cast<std::int8_t>(*it_input_current) <= static_cast<std::int8_t>(0xc0))
									{
										it_input_current += 1;
									}
								}

								const auto out = [&]() noexcept -> auto
								{
									const auto lead = _mm512_slli_epi16(_mm512_cvtepu8_epi16(_mm512_castsi512_si256(_mm512_maskz_compress_epi8(leading_mask, leading_two_bytes))), 6);

									const auto follow = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(_mm512_maskz_compress_epi8(continuation_or_ascii, in)));

									if constexpr (is_big_endian) { return _mm512_shuffle_epi8(_mm512_add_epi16(follow, lead), byte_flip); }
									else { return _mm512_add_epi16(follow, lead); }
								}();

								if constexpr (MaskOut)
								{
									const auto num_out = std::popcount(_pdep_u64(0xffff'ffff, leading_mask));
									_mm512_mask_storeu_epi16(it_output_current, (__mmask32{1} << num_out) - 1, out);

									it_output_current += num_out;
								}
								else
								{
									const auto num_out = std::popcount(leading_mask);
									_mm512_mask_storeu_epi16(it_output_current, (__mmask32{1} << num_out) - 1, out);

									it_output_current += num_out;
								}

								return true;
							};

							bool success = true;
							while (success)
							{
								if (it_input_current + 64 <= it_input_end)
								{
									success = process.template operator()<false>(it_input_end - it_input_current);
								}
								else if (it_input_current < it_input_end)
								{
									success = process.template operator()<true>(it_input_end - it_input_current);
								}
								else { break; }
							}

							if (not success)
							{
								if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
								{
									auto result =
											scalar_type::template rewind_and_convert<OutputCategory>(
												it_input_begin,
												{it_input_current, static_cast<size_type>(it_input_end - it_input_current)},
												it_output_current
											);
									result.count += (it_input_current - it_input_begin);
									if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE) { return result; }
									else { return result.count; }
								}
								else { return 0; }
							}
						}
						else if constexpr (OutputCategory == CharsCategory::UTF32)
						{
							avx512_utf8_checker checker{};
							const auto process = [&it_input_current, it_input_end, &it_output_current, byte_flip, &checker]() noexcept -> bool
							{
								// In the main loop, we consume 64 bytes per iteration, but we access 64 + 4 bytes.
								// We check for it_input_current + 64 + 64 <= it_input_end because
								// we want to do mask-less writes without overruns.
								while (it_input_current + 64 + 64 <= it_input_end)
								{
									const auto in = _mm512_loadu_si512(it_input_current);
									if (checker.check_input(in))
									{
										it_output_current += icelake_utf8_detail::store_ascii<OutputCategory>(in, byte_flip, it_output_current);
										it_input_current += 64;
										continue;
									}

									if (checker.has_error()) { return false; }

									const auto lane_0 = icelake_utf8_detail::broadcast<0>(in);
									const auto lane_1 = icelake_utf8_detail::broadcast<1>(in);
									const auto lane_2 = icelake_utf8_detail::broadcast<2>(in);
									const auto lane_3 = icelake_utf8_detail::broadcast<3>(in);
									const auto lane_4 = _mm512_set1_epi32(static_cast<int>(memory::unaligned_load<std::uint32_t>(it_input_current + 64)));

									auto [vec_0, valid_count_0] = icelake_utf8_detail::expand_and_identify(lane_0, lane_1);
									auto [vec_1, valid_count_1] = icelake_utf8_detail::expand_and_identify(lane_1, lane_2); // NOLINT(readability-suspicious-call-argument)

									if (valid_count_0 + valid_count_1 <= 16)
									{
										vec_0 = _mm512_mask_expand_epi32(
											vec_0,
											static_cast<__mmask16>(((1 << valid_count_1) - 1) << valid_count_0),
											vec_1
										);
										valid_count_0 += valid_count_1;
										vec_0 = icelake_utf8_detail::expand_utf8_to_utf32(vec_0);

										it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
											vec_0,
											byte_flip,
											valid_count_0,
											it_output_current
										);
									}
									else
									{
										vec_0 = icelake_utf8_detail::expand_utf8_to_utf32(vec_0);
										vec_1 = icelake_utf8_detail::expand_utf8_to_utf32(vec_1);

										it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
											vec_0,
											byte_flip,
											valid_count_0,
											it_output_current
										);
										it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
											vec_1,
											byte_flip,
											valid_count_1,
											it_output_current
										);
									}

									auto [vec_2, valid_count_2] = icelake_utf8_detail::expand_and_identify(lane_2, lane_3);
									auto [vec_3, valid_count_3] = icelake_utf8_detail::expand_and_identify(lane_3, lane_4);

									if (valid_count_2 + valid_count_3 <= 16)
									{
										vec_2 = _mm512_mask_expand_epi32(
											vec_2,
											static_cast<__mmask16>(((1 << valid_count_3) - 1) << valid_count_2),
											vec_3
										);
										valid_count_2 += valid_count_3;
										vec_2 = icelake_utf8_detail::expand_utf8_to_utf32(vec_2);

										it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
											vec_2,
											byte_flip,
											valid_count_2,
											it_output_current
										);
									}
									else
									{
										vec_2 = icelake_utf8_detail::expand_utf8_to_utf32(vec_2);
										vec_3 = icelake_utf8_detail::expand_utf8_to_utf32(vec_3);

										it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
											vec_2,
											byte_flip,
											valid_count_2,
											it_output_current
										);
										it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
											vec_3,
											byte_flip,
											valid_count_3,
											it_output_current
										);
									}

									it_input_current += 4 * 16;
								}

								auto it_valid_input_current = it_input_current;

								// For the final pass, we validate 64 bytes,
								// but we only transcode 3*16 bytes,
								// so we may end up double-validating 16 bytes.
								if (it_input_current + 64 <= it_input_end)
								{
									if (const auto in = _mm512_loadu_si512(it_input_current);
										checker.check_input(in))
									{
										it_output_current += icelake_utf8_detail::store_ascii<OutputCategory>(in, byte_flip, it_output_current);
										it_input_current += 64;
									}
									else if (checker.has_error()) { return false; }
									else
									{
										const auto lane_0 = icelake_utf8_detail::broadcast<0>(in);
										const auto lane_1 = icelake_utf8_detail::broadcast<1>(in);
										const auto lane_2 = icelake_utf8_detail::broadcast<2>(in);
										const auto lane_3 = icelake_utf8_detail::broadcast<3>(in);

										auto [vec_0, valid_count_0] = icelake_utf8_detail::expand_and_identify(lane_0, lane_1);
										auto [vec_1, valid_count_1] = icelake_utf8_detail::expand_and_identify(lane_1, lane_2); // NOLINT(readability-suspicious-call-argument)

										if (valid_count_0 + valid_count_1 <= 16)
										{
											vec_0 = _mm512_mask_expand_epi32(
												vec_0,
												static_cast<__mmask16>(((1 << valid_count_1) - 1) << valid_count_0),
												vec_1
											);
											valid_count_0 += valid_count_1;
											vec_0 = icelake_utf8_detail::expand_utf8_to_utf32(vec_0);

											it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, true>(
												vec_0,
												byte_flip,
												valid_count_0,
												it_output_current
											);
										}
										else
										{
											vec_0 = icelake_utf8_detail::expand_utf8_to_utf32(vec_0);
											vec_1 = icelake_utf8_detail::expand_utf8_to_utf32(vec_1);

											it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, true>(
												vec_0,
												byte_flip,
												valid_count_0,
												it_output_current
											);
											it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, true>(
												vec_1,
												byte_flip,
												valid_count_1,
												it_output_current
											);
										}

										it_output_current += icelake_utf8_detail::transcode_16<OutputCategory, true>(lane_2, lane_3, byte_flip, it_output_current);
										it_input_current += 3 * 16;
									}
									it_valid_input_current += 4 * 16;
								}

								{
									const auto in = _mm512_maskz_loadu_epi8(
										(__mmask64{1} << (it_input_end - it_valid_input_current)) - 1,
										it_valid_input_current
									);
									checker.check_input(in);
								}
								checker.check_eof();
								return not checker.has_error();
							};

							if (
								const auto success = process();
								not success
							)
							{
								auto result =
										scalar_type::template rewind_and_convert<OutputCategory>(
											it_input_begin,
											{it_input_current, static_cast<size_type>(it_input_end - it_input_current)},
											it_output_current
										);
								result.count += (it_input_current - it_input_begin);
								if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE) { return result; }
								else { return result.count; }
							}

							if (it_input_current != it_input_end)
							{
								// the AVX512 procedure looks up 4 bytes forward,
								// and correctly converts multibyte chars even if their continuation bytes lie outside 16-byte window.
								// It means, we have to skip continuation bytes from the beginning it_input_current, as they were already consumed.
								while (it_input_current != it_input_end and ((*it_input_current & 0xc0) == 0x80)) { it_input_current += 1; }

								if (it_input_current != it_input_end)
								{
									auto result = scalar_type::template convert<OutputCategory, ProcessPolicy, CheckNextBlock>(
										{it_input_current, static_cast<size_type>(it_input_end - it_input_current)},
										it_output_current
									);

									result.count += (it_input_current - it_input_begin);
									if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE) { return result; }
									else { return result.count; }
								}
							}
						}
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
					}
					else
					{
						const auto v_0080 = _mm512_set1_epi8(static_cast<char>(0x80));

						// In the main loop, we consume 64 bytes per iteration, but we access 64 + 4 bytes.
						// We check for it_input_current + 64 + 64 <= it_input_end because
						// we want to do mask-less writes without overruns.
						while (it_input_current + 64 + 64 <= it_input_end)
						{
							const auto in = _mm512_loadu_si512(it_input_current);

							if (const auto ascii = _mm512_test_epi8_mask(in, v_0080);
								ascii == 0)
							{
								it_output_current += icelake_utf8_detail::store_ascii<OutputCategory>(in, byte_flip, it_output_current);
								it_input_current += 64;
								continue;
							}

							const auto lane_0 = icelake_utf8_detail::broadcast<0>(in);
							const auto lane_1 = icelake_utf8_detail::broadcast<1>(in);
							const auto lane_2 = icelake_utf8_detail::broadcast<2>(in);
							const auto lane_3 = icelake_utf8_detail::broadcast<3>(in);
							const auto lane_4 = _mm512_set1_epi32(static_cast<int>(memory::unaligned_load<std::uint32_t>(it_input_current + 64)));

							auto [vec_0, valid_count_0] = icelake_utf8_detail::expand_and_identify(lane_0, lane_1);
							auto [vec_1, valid_count_1] = icelake_utf8_detail::expand_and_identify(lane_1, lane_2); // NOLINT(readability-suspicious-call-argument)

							if (valid_count_0 + valid_count_1 <= 16)
							{
								vec_0 = _mm512_mask_expand_epi32(
									vec_0,
									static_cast<__mmask16>(((1 << valid_count_1) - 1) << valid_count_0),
									vec_1
								);
								valid_count_0 += valid_count_1;
								vec_0 = icelake_utf8_detail::expand_utf8_to_utf32(vec_0);

								it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
									vec_0,
									byte_flip,
									valid_count_0,
									it_output_current
								);
							}
							else
							{
								vec_0 = icelake_utf8_detail::expand_utf8_to_utf32(vec_0);
								vec_1 = icelake_utf8_detail::expand_utf8_to_utf32(vec_1);

								it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
									vec_0,
									byte_flip,
									valid_count_0,
									it_output_current
								);
								it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
									vec_1,
									byte_flip,
									valid_count_1,
									it_output_current
								);
							}

							auto [vec_2, valid_count_2] = icelake_utf8_detail::expand_and_identify(lane_2, lane_3);
							auto [vec_3, valid_count_3] = icelake_utf8_detail::expand_and_identify(lane_3, lane_4);

							if (valid_count_2 + valid_count_3 <= 16)
							{
								vec_2 = _mm512_mask_expand_epi32(
									vec_2,
									static_cast<__mmask16>(((1 << valid_count_3) - 1) << valid_count_2),
									vec_3
								);
								valid_count_2 += valid_count_3;
								vec_2 = icelake_utf8_detail::expand_utf8_to_utf32(vec_2);

								it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
									vec_2,
									byte_flip,
									valid_count_2,
									it_output_current
								);
							}
							else
							{
								vec_2 = icelake_utf8_detail::expand_utf8_to_utf32(vec_2);
								vec_3 = icelake_utf8_detail::expand_utf8_to_utf32(vec_3);

								it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
									vec_2,
									byte_flip,
									valid_count_2,
									it_output_current
								);
								it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, false>(
									vec_3,
									byte_flip,
									valid_count_3,
									it_output_current
								);
							}

							it_input_current += 4 * 16;
						}

						if (it_input_current + 64 <= it_input_current)
						{
							const auto in = _mm512_loadu_si512(it_input_current);

							if (const auto ascii = _mm512_test_epi8_mask(in, v_0080);
								ascii == 0)
							{
								it_output_current += icelake_utf8_detail::store_ascii<OutputCategory>(in, byte_flip, it_output_current);
								it_input_current += 64;
							}
							else
							{
								const auto lane_0 = icelake_utf8_detail::broadcast<0>(in);
								const auto lane_1 = icelake_utf8_detail::broadcast<1>(in);
								const auto lane_2 = icelake_utf8_detail::broadcast<2>(in);
								const auto lane_3 = icelake_utf8_detail::broadcast<3>(in);

								auto [vec_0, valid_count_0] = icelake_utf8_detail::expand_and_identify(lane_0, lane_1);
								auto [vec_1, valid_count_1] = icelake_utf8_detail::expand_and_identify(lane_1, lane_2); // NOLINT(readability-suspicious-call-argument)

								if (valid_count_0 + valid_count_1 <= 16)
								{
									vec_0 = _mm512_mask_expand_epi32(
										vec_0,
										static_cast<__mmask16>(((1 << valid_count_1) - 1) << valid_count_0),
										vec_1
									);
									valid_count_0 += valid_count_1;
									vec_0 = icelake_utf8_detail::expand_utf8_to_utf32(vec_0);

									it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, true>(
										vec_0,
										byte_flip,
										valid_count_0,
										it_output_current
									);
								}
								else
								{
									vec_0 = icelake_utf8_detail::expand_utf8_to_utf32(vec_0);
									vec_1 = icelake_utf8_detail::expand_utf8_to_utf32(vec_1);

									it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, true>(
										vec_0,
										byte_flip,
										valid_count_0,
										it_output_current
									);
									it_output_current += icelake_utf8_detail::store_utf16_or_utf32<OutputCategory, true>(
										vec_1,
										byte_flip,
										valid_count_1,
										it_output_current
									);
								}

								it_output_current += icelake_utf8_detail::transcode_16<OutputCategory, true>(lane_2, lane_3, byte_flip, it_output_current);
								it_input_current += 3 * 16;
							}
						}

						if (it_input_current != it_input_end)
						{
							// the AVX512 procedure looks up 4 bytes forward,
							// and correctly converts multibyte chars even if their continuation bytes lie outside 16-byte window.
							// It means, we have to skip continuation bytes from the beginning it_input_current, as they were already consumed.
							while (it_input_current != it_input_end and ((*it_input_current & 0xc0) == 0x80)) { it_input_current += 1; }

							if (it_input_current != it_input_end)
							{
								auto result = scalar_type::template convert<OutputCategory, InputProcessPolicy::RETURN_RESULT_TYPE, CheckNextBlock>(
									{it_input_current, static_cast<size_type>(it_input_end - it_input_current)},
									it_output_current
								);

								result.count += (it_input_current - it_input_begin);
								if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE) { return result; }
								else { return result.count; }
							}
						}
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("Unknown or unsupported `OutputCategory` (we don't know the `endian` by UTF16, so it's not allowed to use it here).");
				}

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
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	using icelake_utf8_detail::avx512_utf8_checker;

	template<>
	class Simd<"icelake.utf8"> : public icelake_utf8_detail::SimdUtf8Base<CharsCategory::UTF8> {};

	template<>
	class Simd<"icelake.utf8_char"> : public icelake_utf8_detail::SimdUtf8Base<CharsCategory::UTF8_CHAR> {};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
