// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <intrin.h>

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:icelake.ascii;

import std;
import gal.prometheus.string;
import gal.prometheus.error;
import gal.prometheus.utility;

import :encoding;
import :converter;
import :scalar;

namespace gal::prometheus::chars
{
	template<>
	class Simd<"ascii">
	{
	public:
		using scalar_type					 = Scalar<"ascii">;

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

			const auto		   ascii			= _mm512_set1_epi8(static_cast<char>(0x80));
			// used iff not ReturnResultType
			auto			   running_or		= _mm512_setzero_si512();

			for (; it_input_current + 64 <= it_input_end; it_input_current += 64)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				const auto in			   = _mm512_loadu_si512(it_input_current);

				if constexpr (ReturnResultType)
				{
					if (const auto not_ascii = _mm512_cmp_epu8_mask(in, ascii, _MM_CMPINT_NLT);
						not_ascii) { return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + std::countr_zero(not_ascii)}; }
				}
				else
				{
					// running_or | (in & ascii)
					running_or = _mm512_ternarylogic_epi32(running_or, in, ascii, 0xf8);
				}
			}

			if constexpr (ReturnResultType)
			{
				const auto in = _mm512_maskz_loadu_epi8((__mmask64{1} << (it_input_end - it_input_current)) - 1, it_input_current);
				if (const auto not_ascii = _mm512_cmp_epu8_mask(in, ascii, _MM_CMPINT_NLT);
					not_ascii) { return result_type{.error = ErrorCode::TOO_LARGE, .count = static_cast<std::size_t>(it_input_current - it_input_begin) + std::countr_zero(not_ascii)}; }

				return result_type{.error = ErrorCode::NONE, .count = input_length};
			}
			else
			{
				if (it_input_current < it_input_end)
				{
					const auto in = _mm512_maskz_loadu_epi8((__mmask64{1} << (it_input_end - it_input_current)) - 1, it_input_current);
					// running_or | (in & ascii)
					running_or	  = _mm512_ternarylogic_epi32(running_or, in, ascii, 0xf8);
				}

				return _mm512_test_epi8_mask(running_or, running_or) == 0;
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

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				return input.size();
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				const auto input_length	 = input.size();
				const auto input_data	 = input.data();

				// number of 512-bit chunks that fits into the length.
				auto	   result_length = input_length / sizeof(__m512i) * sizeof(__m512i);
				auto	   eight_64bits	 = _mm512_setzero_si512();
				auto	   i			 = static_cast<size_type>(0);

				while (i + sizeof(__m512i) <= input_length)
				{
					const auto iterations = std::ranges::min(static_cast<size_type>((input_length - i) / sizeof(__m512i)), static_cast<size_type>(255));
					const auto max_i	  = i + iterations * sizeof(__m512i) - sizeof(__m512i);

					auto	   runner	  = _mm512_setzero_si512();

					for (; i + 4 * sizeof(__m512i) <= max_i; i += 4 * sizeof(__m512i))
					{
						const auto in_0		   = _mm512_loadu_si512(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m512i *, input_data + i + 0 * sizeof(__m512i)));
						const auto in_1		   = _mm512_loadu_si512(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m512i *, input_data + i + 1 * sizeof(__m512i)));
						const auto in_2		   = _mm512_loadu_si512(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m512i *, input_data + i + 2 * sizeof(__m512i)));
						const auto in_3		   = _mm512_loadu_si512(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m512i *, input_data + i + 3 * sizeof(__m512i)));

						const auto mask_0	   = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in_0);
						const auto mask_1	   = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in_1);
						const auto mask_2	   = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in_2);
						const auto mask_3	   = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in_3);

						const auto not_ascii_0 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask_0, 0xff);
						const auto not_ascii_1 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask_1, 0xff);
						const auto not_ascii_2 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask_2, 0xff);
						const auto not_ascii_3 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask_3, 0xff);

						runner				   = _mm512_sub_epi8(runner, not_ascii_0);
						runner				   = _mm512_sub_epi8(runner, not_ascii_1);
						runner				   = _mm512_sub_epi8(runner, not_ascii_2);
						runner				   = _mm512_sub_epi8(runner, not_ascii_3);
					}

					for (; i <= max_i; i += sizeof(__m512i))
					{
						const auto in		 = _mm512_loadu_si512(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m512i *, input_data + i + 0 * sizeof(__m512i)));
						const auto mask		 = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in);
						const auto not_ascii = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask, 0xff);
						runner				 = _mm512_sub_epi8(runner, not_ascii);
					}

					eight_64bits = _mm512_add_epi64(eight_64bits, _mm512_sad_epu8(runner, _mm512_setzero_si512()));
				}

				const auto half_0 = _mm512_extracti64x4_epi64(eight_64bits, 0);
				const auto half_1 = _mm512_extracti64x4_epi64(eight_64bits, 1);
				result_length += _mm256_extract_epi64(half_0, 0) +
								 _mm256_extract_epi64(half_0, 1) +
								 _mm256_extract_epi64(half_0, 2) +
								 _mm256_extract_epi64(half_0, 3) +
								 _mm256_extract_epi64(half_1, 0) +
								 _mm256_extract_epi64(half_1, 1) +
								 _mm256_extract_epi64(half_1, 2) +
								 _mm256_extract_epi64(half_1, 3);

				return result_length + instance::scalar::ascii.length<OutputCategory>({input_data + i, input_length - i});
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				return instance::scalar::ascii.length<OutputCategory>(input);
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				return instance::scalar::ascii.length<OutputCategory>(input);
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
				const auto process = []<bool MaskOut>(const auto in, const auto in_length, output_pointer_type out) noexcept -> size_type
				{
					constexpr auto alternate_bits	 = 0x5555'5555'5555'5555ull;

					const auto	   non_ascii		 = _mm512_movepi8_mask(in);
					const auto	   ascii			 = ~non_ascii;
					// Mask to denote whether the byte is a leading byte that is not ascii.
					// binary representation of -64: 1100'0000
					const auto	   sixth			 = _mm512_cmpge_epu8_mask(in, _mm512_set1_epi8(static_cast<char>(-64)));

					// the bits in ascii are inverted and zeros are interspersed in between them.
					const auto	   mask_a			 = ~_pdep_u64(ascii, alternate_bits);
					const auto	   mask_b			 = ~_pdep_u64(ascii >> 32, alternate_bits);
					// interleave bytes from top and bottom halves (abcd...ABCD -> aAbBcCdD).
					const auto	   input_interleaved = _mm512_permutexvar_epi8(
							// clang-format off
							_mm512_set_epi32(
									0x3f1f'3e1e,0x3d1d'3c1c,0x3b1b'3a1a,0x3919'3818,
									0x3717'3616,0x3515'3414,0x3313'3212,0x3111'3010,
									0x2f0f'2e0e,0x2d0d'2c0c,0x2b0b'2a0a,0x2909'2808,
									0x2707'2606,0x2505'2404,0x2303'2202,0x2101'2000
									),
							// clang-format on
							in);

					const auto output_a = [](const auto interleaved, const auto s, const auto mask) noexcept -> auto
					{
						// Upscale the bytes to 16-bit value, adding the 0b11000010 leading byte in the process.
						// binary representation of -62: 1100'0010
						const auto v = _mm512_shldi_epi16(interleaved, _mm512_set1_epi8(static_cast<char>(-62)), 8);
						// prune redundant bytes
						return _mm512_maskz_compress_epi8(
								mask,
								// We adjust for the bytes that have their two most significant bits. This takes care of the first 32 bytes, assuming we interleaved the bytes.
								_mm512_mask_add_epi16(
										v,
										s,
										v,
										// 1- 0x4000 = 1100 0000 0000 0001
										_mm512_set1_epi16(1 - 0x4000)));
					}(input_interleaved, sixth, mask_a);

					const auto output_b = [](const auto interleaved, const auto s, const auto mask) noexcept -> auto
					{
						// in the second 32-bit half, set first or second option based on whether original input is leading byte (second case) or not (first case).
						const auto leading = _mm512_mask_blend_epi16(
								static_cast<__mmask32>(s >> 32),
								// 0000 0000 1101 0010
								_mm512_set1_epi16(0x00c2),
								// 0100 0000 1100 0011
								_mm512_set1_epi16(0x40c3));
						// prune redundant bytes
						return _mm512_maskz_compress_epi8(
								mask,
								_mm512_ternarylogic_epi32(
										interleaved,
										leading,
										_mm512_set1_epi16(static_cast<short>(0xff00)),
										// (interleaved & 0xff00) ^ leading
										(240 & 170) ^ 204));
					}(input_interleaved, sixth, mask_b);

					const auto out_size	  = in_length + std::popcount(non_ascii);
					const auto out_size_a = std::popcount(utility::truncate<std::uint32_t>(non_ascii)) + 32;

					if constexpr (MaskOut)
					{
						// is the second half of the input vector used?
						if (in_length > 32)
						{
							_mm512_mask_storeu_epi8(out, _bzhi_u64(~0ull, out_size_a), output_a);
							_mm512_mask_storeu_epi8(out + out_size_a, _bzhi_u64(~0ull, out_size - out_size_a), output_b);
						}
						else { _mm512_mask_storeu_epi8(out, _bzhi_u64(~0ull, out_size), output_a); }
					}
					else
					{
						_mm512_storeu_si512(out, output_a);
						_mm512_storeu_si512(out + out_size_a, output_b);
					}

					return out_size;
				};

				const auto process_or_just_store = [process](const auto in, output_pointer_type out) noexcept -> size_type
				{
					const auto non_ascii = _mm512_movepi8_mask(in);
					const auto count	 = std::popcount(non_ascii);
					GAL_PROMETHEUS_ASSUME(count >= 0);
					if (count != 0)
					{
						return process.template operator()<false>(in, 64, out);
					}

					_mm512_storeu_si512(out, in);
					return 64;
				};

				// if there's at least 128 bytes remaining, we don't need to mask the output.
				for (; it_input_current + 128 <= it_input_end; it_input_current += 64)
				{
					const auto in = _mm512_loadu_si512(it_input_current);
					it_output_current += process_or_just_store(in, it_output_current);
				}

				// in the last 128 bytes, the first 64 may require masking the output
				if (it_input_current + 64 <= it_input_end)
				{
					const auto							  in = _mm512_loadu_si512(it_input_current);
					it_output_current += process.template operator()<true>(in, 64, it_output_current);
					it_input_current += 64;
				}

				// with the last 64 bytes, the input also needs to be masked
				if (it_input_current < it_input_end)
				{
					const auto							  in = _mm512_maskz_loadu_epi8(_bzhi_u64(~0ull, it_input_end - it_input_current), it_input_current);
					it_output_current += process.template operator()<true>(in, it_input_end - it_input_current, it_output_current);
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				constexpr auto is_big_endian = OutputCategory != CharsCategory::UTF16_LE;

				const auto	   byte_flip	 = _mm512_setr_epi64(
						0x0607'0405'0203'0001,
						0x0e0f'0c0d'0a0b'0809,
						0x0607'0405'0203'0001,
						0x0e0f'0c0d'0a0b'0809,
						0x0607'0405'0203'0001,
						0x0e0f'0c0d'0a0b'0809,
						0x0607'0405'0203'0001,
						0x0e0f'0c0d'0a0b'0809);

				// Round down to nearest multiple of 32
				const auto rounded_input_length = input_length & ~0x1f;
				const auto it_rounded_input_end = it_input_begin + rounded_input_length;

				for (; it_input_current < it_rounded_input_end; it_input_current += 32, it_output_current += 32)
				{
					// Load 32 Latin1 characters into a 256-bit register
					const auto in  = _mm256_loadu_si256(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m256i *, it_input_current));
					// Zero extend each set of 8 Latin1 characters to 32 16-bit integers
					const auto out = [&byte_flip](const auto i) noexcept -> auto
					{
						if constexpr (is_big_endian) { return _mm512_shuffle_epi8(_mm512_cvtepu8_epi16(i), byte_flip); }
						else { return _mm512_cvtepu8_epi16(i); }
					}(in);
					// Store the results back to memory
					_mm512_storeu_si512(it_output_current, out);
				}

				if (const auto remaining = input_length - rounded_input_length;
					remaining != 0)
				{
					const auto mask = (__mmask32{1} << (input_length - rounded_input_length)) - 1;
					GAL_PROMETHEUS_DEBUG_ASSUME(it_input_current == it_rounded_input_end);
					const auto in  = _mm256_maskz_loadu_epi8(mask, it_input_current);
					// Zero extend each set of 8 Latin1 characters to 32 16-bit integers
					const auto out = [&byte_flip](const auto i) noexcept -> auto
					{
						if constexpr (is_big_endian) { return _mm512_shuffle_epi8(_mm512_cvtepu8_epi16(i), byte_flip); }
						else { return _mm512_cvtepu8_epi16(i); }
					}(in);
					// Store the results back to memory
					_mm512_mask_storeu_epi16(it_output_current, mask, out);
					it_input_current += remaining;
					it_output_current += remaining;
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				// Round down to nearest multiple of 16
				const auto rounded_input_length = input_length & ~0x1f;

				for (const auto it_rounded_input_end = it_input_begin + rounded_input_length; it_input_current < it_rounded_input_end; it_input_current += 16, it_output_current += 16)
				{
					// Load 16 Latin1 characters into a 128-bit register
					const auto in  = _mm_loadu_si128(GAL_PROMETHEUS_TRIVIAL_REINTERPRET_CAST(const __m128i *, it_input_current));
					// Zero extend each set of 8 Latin1 characters to 16 32-bit integers
					const auto out = _mm512_cvtepu8_epi32(in);
					// Store the results back to memory
					_mm512_storeu_si512(it_output_current, out);
				}

				if (rounded_input_length != input_length)
				{
					const auto processed_length = instance::scalar::ascii.convert<CharsCategory::UTF32, InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT, CheckNextBlock>({it_input_current, input_length - rounded_input_length}, it_output_current);
					it_input_current += processed_length;
					it_output_current += processed_length;
				}
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
			requires requires(StringType &string) {
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
			requires requires(StringType &string) {
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
		constexpr Simd<"ascii"> ascii{};
	}
}// namespace gal::prometheus::chars
