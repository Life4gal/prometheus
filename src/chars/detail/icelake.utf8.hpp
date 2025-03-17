// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if defined(GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED)

namespace detail::utf8::icelake
{
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

	[[nodiscard]] inline auto expand_and_identify(const data_type lane_0, const data_type lane_1, std::uint8_t& valid_count) noexcept -> data_type
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

		const auto v_00c0 = _mm512_set1_epi32(0x00c0);
		const auto v_0080 = _mm512_set1_epi32(0x0080);

		const auto merged = _mm512_mask_mov_epi32(lane_0, 0x1000, lane_1);
		const auto input = _mm512_shuffle_epi8(merged, expand_ver2);
		const auto t0 = _mm512_and_si512(input, v_00c0);
		const auto leading_bytes = _mm512_cmpneq_epu32_mask(t0, v_0080);

		valid_count = static_cast<std::uint8_t>(std::popcount(leading_bytes));
		return _mm512_mask_compress_epi32(_mm512_setzero_si512(), leading_bytes, input);
	}

	[[nodiscard]] inline auto expand_to_utf32(const data_type data, const data_type char_class) noexcept -> data_type
	{
		//  Input:
		//  - utf8: bytes stored at separate 32-bit code units
		//  - valid: which code units have valid UTF-8 characters
		// 
		//  Bit layout of single word.
		//  We show 4 cases for each possible UTF-8 character encoding.
		//  The `?` denotes bits we must not assume their value.
		// 
		//  |10dd.dddd|10cc.cccc|10bb.bbbb|1111.0aaa| 4-byte char
		//  |????.????    |10cc.cccc|10bb.bbbb|1110.aaaa| 3-byte char
		//  |????.????    |????.???? |10bb.bbbb|110a.aaaa| 2-byte char
		//  |????.????    |????.???? |????.????    |0aaa.aaaa| ASCII char
		//   byte 3        byte 2      byte 1        byte 0

		const auto v_3f3f_3f7f = _mm512_set1_epi32(0x3f3f'3f7f);
		const auto v_0140_0140 = _mm512_set1_epi32(0x0140'0140);
		const auto v_0001_1000 = _mm512_set1_epi32(0x0001'1000);

		// ReSharper disable once CppJoinDeclarationAndAssignment
		data_type result;

		// Reset control bits of continuation bytes and the MSB of the leading byte,
		// this makes all bytes unsigned (and does not alter ASCII char).
		// 
		// |00dd.dddd|00cc.cccc|00bb.bbbb|0111.0aaa| 4-byte char
		// |00??.????   |00cc.cccc|00bb.bbbb|0110.aaaa| 3-byte char
		// |00??.????   |00??.???? |00bb.bbbb|010a.aaaa| 2-byte char
		// |00??.????   |00??.???? |00??.????   |0aaa.aaaa| ASCII char
		result = _mm512_and_si512(data, v_3f3f_3f7f);

		// Swap and join fields A-B and C-D
		// 
		// |0000.cccc|ccdd.dddd|0001.110a|aabb.bbbb| 4-byte char
		// |0000.cccc|cc??.????   |0001.10aa|aabb.bbbb| 3-byte char
		// |0000.????|????.????    |0001.0aaa|aabb.bbbb| 2-byte char
		// |0000.????|????.????    |000a.aaaa|aa??.????   | ASCII char
		result = _mm512_maddubs_epi16(result, v_0140_0140);

		// Swap and join fields AB & CD
		// 
		// |0000.0001|110a.aabb|bbbb.cccc|ccdd.dddd| 4-byte char
		// |0000.0001|10aa.aabb|bbbb.cccc|cc??.????   | 3-byte char
		// |0000.0001|0aaa.aabb|bbbb.????|????.????   | 2-byte char
		// |0000.000a|aaaa.aa?? |????.????  |????.????    | ASCII char
		result = _mm512_madd_epi16(result, v_0001_1000);

		// Shift left the values by variable amounts to reset highest UTF-8 bits
		//
		// |aaab.bbbb|bccc.cccd|dddd.d000|0000.0000| 4-byte char -- by 11
		// |aaaa.bbbb|bbcc.cccc|????.??00   |0000.0000| 3-byte char -- by 10
		// |aaaa.abbb|bbb?.????|????.???0    |0000.0000| 2-byte char -- by 9
		// |aaaa.aaa? |????.????  |????.????    |?000.0000| ASCII char -- by 7
		{
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
			const auto shift_left_v3 = _mm512_setr_epi64(
				// clang-format off
				0x0707'0707'0707'0707, 0x0b0a'0909'0000'0000,
				0x0707'0707'0707'0707, 0x0b0a'0909'0000'0000,
				0x0707'0707'0707'0707, 0x0b0a'0909'0000'0000,
				0x0707'0707'0707'0707, 0x0b0a'0909'0000'0000
				// clang-format on
			);
			const auto shift = _mm512_shuffle_epi8(shift_left_v3, char_class);
			result = _mm512_sllv_epi32(result, shift);
		}

		// Shift right the values by variable amounts to reset lowest bits
		//
		// |0000.0000|000a.aabb|bbbb.cccc |ccdd.dddd| 4-byte char -- by 11
		// |0000.0000|0000.0000|aaaa.bbbb |bbcc.cccc | 3-byte char -- by 16
		// |0000.0000|0000.0000|0000.0aaa |aabb.bbbb| 2-byte char -- by 21
		// |0000.0000|0000.0000|0000.0000 |0aaa.aaaa | ASCII char -- by 25
		{
			// 4 * [25, 25, 25, 25, 25, 25, 25, 25, 0, 0, 0, 0, 21, 21, 16, 11]
			const auto shift_right = _mm512_setr_epi64(
				// clang-format off
				0x1919'1919'1919'1919, 0x0b10'1515'0000'0000,
				0x1919'1919'1919'1919, 0x0b10'1515'0000'0000,
				0x1919'1919'1919'1919, 0x0b10'1515'0000'0000,
				0x1919'1919'1919'1919, 0x0b10'1515'0000'0000
				// clang-format on
			);
			const auto shift = _mm512_shuffle_epi8(shift_right, char_class);
			result = _mm512_srlv_epi32(result, shift);
		}

		return result;
	}

	[[nodiscard]] inline auto expand_to_utf32(const data_type data) noexcept -> data_type
	{
		const auto v_0000_000f = _mm512_set1_epi32(0x0000'000f);
		const auto v_8080_8000 = _mm512_set1_epi32(static_cast<int>(0x8080'8000));

		const auto char_class = _mm512_ternarylogic_epi32(_mm512_srli_epi32(data, 4), v_0000_000f, v_8080_8000, 0xea);
		return expand_to_utf32(data, char_class);
	}

	template<CharsType OutputType>
	auto write_utf16_pure(output_type_of<CharsType::UTF16>::pointer& output, const data_type data, const data_type byte_flip) noexcept -> void
	{
		constexpr auto iterations = std::ptrdiff_t{32};
		static_assert(sizeof(data_type) / sizeof(output_type_of<CharsType::UTF16>::value_type) == iterations);

		const auto h0 = _mm512_castsi512_si256(data);
		const auto h1 = _mm512_extracti64x4_epi64(data, 1);

		const auto o0 = _mm512_cvtepu8_epi16(h0);
		const auto o1 = _mm512_cvtepu8_epi16(h1);

		if constexpr (common::not_native_endian<OutputType>())
		{
			_mm512_storeu_si512(output + 0 * iterations, _mm512_shuffle_epi8(o0, byte_flip));
			_mm512_storeu_si512(output + 2 * iterations, _mm512_shuffle_epi8(o1, byte_flip));
		}
		else
		{
			_mm512_storeu_si512(output + 0 * iterations, o0);
			_mm512_storeu_si512(output + 2 * iterations, o1);
		}

		output += 2 * iterations;
	}

	inline auto write_utf32_pure(output_type_of<CharsType::UTF32>::pointer& output, const data_type data) noexcept -> void
	{
		// utf8::icelake::advance_of_utf32
		constexpr auto iterations = std::ptrdiff_t{16};
		static_assert(sizeof(data_type) / sizeof(output_type_of<CharsType::UTF32>::value_type) == iterations);

		const auto t0 = _mm512_castsi512_si128(data);
		const auto t1 = _mm512_extracti32x4_epi32(data, 1);
		const auto t2 = _mm512_extracti32x4_epi32(data, 2);
		const auto t3 = _mm512_extracti32x4_epi32(data, 3);

		_mm512_storeu_si512(output + 0 * iterations, _mm512_cvtepu8_epi32(t0));
		_mm512_storeu_si512(output + 1 * iterations, _mm512_cvtepu8_epi32(t1));
		_mm512_storeu_si512(output + 2 * iterations, _mm512_cvtepu8_epi32(t2));
		_mm512_storeu_si512(output + 3 * iterations, _mm512_cvtepu8_epi32(t3));

		output += 4 * iterations;
	}

	template<CharsType OutputType>
	auto write_utf16_from_utf32(output_type_of<CharsType::UTF16>::pointer& output, const data_type data, const std::size_t length, const data_type byte_flip) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length > 0);

		const auto v_0000_ffff = _mm512_set1_epi32(0x0000'ffff);
		const auto v_0001_0000 = _mm512_set1_epi32(0x0001'0000);
		const auto v_ffff_0000 = _mm512_set1_epi32(static_cast<int>(0xffff'0000));
		const auto v_fc00_fc00 = _mm512_set1_epi32(static_cast<int>(0xfc00'fc00));
		const auto v_d800_dc00 = _mm512_set1_epi32(static_cast<int>(0xd800'dc00));

		const auto length_mask = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));

		const auto surrogate_pair_mask = _mm512_mask_cmpgt_epu32_mask(length_mask, data, v_0000_ffff);

		// check if we have any surrogate pairs
		if (surrogate_pair_mask)
		{
			const auto out = _mm512_cvtepi32_epi16(data);

			if constexpr (common::not_native_endian<OutputType>())
			{
				_mm256_mask_storeu_epi16(output, length_mask, _mm256_shuffle_epi8(out, _mm512_castsi512_si256(byte_flip)));
			}
			else
			{
				_mm256_mask_storeu_epi16(output, length_mask, out);
			}

			output += length;
		}

		const auto length_total = length + std::popcount(surrogate_pair_mask);
		const auto length_total_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length_total));

		// build surrogate pair code units in 32-bit lanes

		// t0 = 8 x [000000000000aaaa|aaaaaabbbbbbbbbb]
		const auto t0 = _mm512_sub_epi32(data, v_0001_0000);
		// t1 = 8 x [000000aaaaaaaaaa|bbbbbbbbbb000000]
		const auto t1 = _mm512_slli_epi32(t0, 6);
		// t2 = 8 x [000000aaaaaaaaaa|aaaaaabbbbbbbbbb] -- copy hi word from t1 to t0
		// 0xe4 = (t1 and v_ffff_0000) or (t0 and not v_ffff_0000)
		const auto t2 = _mm512_ternarylogic_epi32(t1, t0, v_ffff_0000, 0xe4);
		// t2 = 8 x [110110aaaaaaaaaa|110111bbbbbbbbbb] -- copy hi word from t1 to t0
		// 0xba = (t2 and not v_fc00_fc000) or v_d800_dc00
		const auto t3 = _mm512_ternarylogic_epi32(t2, v_fc00_fc00, v_d800_dc00, 0xba);
		const auto t4 = _mm512_mask_blend_epi32(surrogate_pair_mask, data, t3);
		const auto t5 = [t4, byte_flip]() noexcept
		{
			if constexpr (const auto out = _mm512_ror_epi32(t4, 16);
				common::not_native_endian<OutputType>())
			{
				return _mm512_shuffle_epi8(out, byte_flip);
			}
			else
			{
				std::ignore = byte_flip;
				return out;
			}
		}();

		const auto non_zero = _kor_mask32(0xaaaa'aaaa, _mm512_cmpneq_epi16_mask(t5, _mm512_setzero_si512()));
		// fixme
		// _mm512_mask_compressstoreu_epi16(output, non_zero, t5);
		_mm512_mask_storeu_epi16(
			output,
			length_total_mask,
			_mm512_maskz_compress_epi16(non_zero, t5)
		);

		output += length_total;
	}

	inline auto write_utf32(output_type_of<CharsType::UTF32>::pointer& output, const data_type data, const std::size_t length) noexcept -> void
	{
		const auto mask = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));

		_mm512_mask_storeu_epi32(output, mask, data);
		output += length;
	}

	inline auto transcode_16(
		output_type_of<CharsType::UTF32>::pointer& output,
		const data_type lane_2,
		const data_type lane_3
	) noexcept -> void
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

		const auto merged = _mm512_mask_mov_epi32(lane_2, 0x1000, lane_3);
		const auto data = _mm512_shuffle_epi8(merged, expand_ver2);

		const auto t0 = _mm512_and_si512(data, v_0000_00c0);
		const auto leading_bytes = _mm512_cmpneq_epu32_mask(t0, v_0000_0080);
		const auto utf32 = expand_to_utf32(data);
		const auto out = _mm512_mask_compress_epi32(_mm512_setzero_si512(), leading_bytes, utf32);

		const auto valid_count = std::popcount(leading_bytes);

		const auto mask = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(valid_count)));
		_mm512_mask_storeu_epi32(output, mask, out);
		output += valid_count;
	}

	struct avx512_utf8_checker
	{
	private:
		template<int N>
			requires(N <= 32)
		static auto prev(const data_type input, const data_type prev_input) noexcept -> data_type
		{
			const auto move_mask = _mm512_setr_epi32(28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
			const auto rotated = _mm512_permutex2var_epi32(input, move_mask, prev_input);

			return _mm512_alignr_epi8(input, rotated, 16 - N);
		}

	public:
		// If this is nonzero, there has been a UTF-8 error
		data_type error{};
		// The last input we received
		data_type prev_data_block{};
		// Whether the last input we received was incomplete (used for ASCII fast path)
		data_type prev_incomplete{};

	private:
		// Check whether the current bytes are valid UTF-8.
		auto check_utf8_bytes(const data_type data, const data_type prev_data) noexcept -> void
		{
			// Flip prev1...prev3, so we can easily determine if they are 2+, 3+ or 4+ lead bytes
			// (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
			const auto prev_1 = prev<1>(data, prev_data);

			// special cases
			const auto source = [data, prev_1]() noexcept -> data_type
			{
				const auto mask1 = _mm512_setr_epi64(
					0x0202'0202'0202'0202,
					0x4915'0121'8080'8080,
					0x0202'0202'0202'0202,
					0x4915'0121'8080'8080,
					0x0202'0202'0202'0202,
					0x4915'0121'8080'8080,
					0x0202'0202'0202'0202,
					0x4915'0121'8080'8080
				);
				const auto mask2 = _mm512_setr_epi64(
					static_cast<long long>(0xcbcb'cb8b'8383'a3e7),
					static_cast<long long>(0xcbcb'dbcb'cbcb'cbcb),
					static_cast<long long>(0xcbcb'cb8b'8383'a3e7),
					static_cast<long long>(0xcbcb'dbcb'cbcb'cbcb),
					static_cast<long long>(0xcbcb'cb8b'8383'a3e7),
					static_cast<long long>(0xcbcb'dbcb'cbcb'cbcb),
					static_cast<long long>(0xcbcb'cb8b'8383'a3e7),
					static_cast<long long>(0xcbcb'dbcb'cbcb'cbcb)
				);
				const auto mask3 = _mm512_setr_epi64(
					0x0101'0101'0101'0101,
					0x0101'0101'baba'aee6,
					0x0101'0101'0101'0101,
					0x0101'0101'baba'aee6,
					0x0101'0101'0101'0101,
					0x0101'0101'baba'aee6,
					0x0101'0101'0101'0101,
					0x0101'0101'baba'aee6
				);

				const auto v_0f = _mm512_set1_epi8(static_cast<char>(0x0f));

				const auto index1 = _mm512_and_si512(_mm512_srli_epi16(prev_1, 4), v_0f);
				const auto index2 = _mm512_and_si512(prev_1, v_0f);
				const auto index3 = _mm512_and_si512(_mm512_srli_epi16(data, 4), v_0f);

				const auto byte_1_high = _mm512_shuffle_epi8(mask1, index1);
				const auto byte_1_low = _mm512_shuffle_epi8(mask2, index2);
				const auto byte_2_high = _mm512_shuffle_epi8(mask3, index3);

				return _mm512_ternarylogic_epi64(byte_1_high, byte_1_low, byte_2_high, 128);
			}();

			// multibyte length
			const auto length = [data, prev_data, source]() noexcept -> data_type
			{
				const auto v_7f = _mm512_set1_epi8(static_cast<char>(0x7f));
				const auto v_80 = _mm512_set1_epi8(static_cast<char>(0x80));

				const auto prev_2 = avx512_utf8_checker::prev<2>(data, prev_data);
				const auto prev_3 = avx512_utf8_checker::prev<3>(data, prev_data);

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
		auto check_incomplete(const data_type data) noexcept -> void
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
				static_cast<long long>(0xbfdf'efff'ffff'ffff)
			);

			prev_incomplete = _mm512_subs_epu8(data, max_value);
		}

	public:
		auto check_eof() noexcept -> void
		{
			// The only problem that can happen at EOF is that a multibyte character is too short
			// or a byte value too large in the last bytes: check_utf8_bytes::check_special_cases only
			// checks for bytes too large in the first of two bytes.

			// If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
			// possibly finish them.
			error = _mm512_or_si512(error, prev_incomplete);
		}

		[[nodiscard]] bool has_error() const noexcept
		{
			return _mm512_test_epi8_mask(error, error) != 0;
		}

		// returns true if ASCII
		[[nodiscard]] auto check_data(const data_type data) noexcept -> bool
		{
			const auto v_80 = _mm512_set1_epi8(static_cast<char>(0x80));

			if (const auto ascii = _mm512_test_epi8_mask(data, v_80);
				ascii == 0)
			{
				check_eof();
				return true;
			}

			check_utf8_bytes(data, prev_data_block);
			check_incomplete(data);
			prev_data_block = data;
			return false;
		}
	};
}

#endif
