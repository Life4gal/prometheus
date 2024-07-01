// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <intrin.h>
#include <prometheus/macro.hpp>

export module gal.prometheus.chars:icelake.ascii;

import std;
import gal.prometheus.error;
import gal.prometheus.meta;
import gal.prometheus.memory;

import :encoding;
import :scalar.ascii;

#else
#include <intrin.h>

#include <prometheus/macro.hpp>
#include <chars/encoding.ixx>
#include <error/error.ixx>
#include <meta/meta.ixx>
#endif

// ReSharper disable once CppRedundantNamespaceDefinition
GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::chars)
{
	template<>
	class Simd<"ascii">
	{
	public:
		using scalar_type = Scalar<"ascii">;

		constexpr static auto input_category = scalar_type::input_category;
		using input_type = scalar_type::input_type;
		using char_type = scalar_type::char_type;
		using pointer_type = scalar_type::pointer_type;
		using size_type = scalar_type::size_type;

		template<bool ReturnResultType = false>
		[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const auto ascii = _mm512_set1_epi8(static_cast<char>(0x80));
			// used iff not ReturnResultType
			auto running_or = _mm512_setzero_si512();

			for (; it_input_current + 64 <= it_input_end; it_input_current += 64)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				const auto in = _mm512_loadu_si512(it_input_current);

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
					not_ascii)
				{
					return result_type{
							.error = ErrorCode::TOO_LARGE,
							.count = static_cast<std::size_t>(it_input_current - it_input_begin) + std::countr_zero(not_ascii)
					};
				}

				return result_type{.error = ErrorCode::NONE, .count = input_length};
			}
			else
			{
				if (it_input_current < it_input_end)
				{
					const auto in = _mm512_maskz_loadu_epi8((__mmask64{1} << (it_input_end - it_input_current)) - 1, it_input_current);
					// running_or | (in & ascii)
					running_or = _mm512_ternarylogic_epi32(running_or, in, ascii, 0xf8);
				}

				return _mm512_test_epi8_mask(running_or, running_or) == 0;
			}
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

			if constexpr (OutputCategory == CharsCategory::ASCII) { return input.size(); } // NOLINT(bugprone-branch-clone)
			else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
			{
				auto eight_64_bits = _mm512_setzero_si512();

				while (it_input_current + sizeof(__m512i) <= it_input_end)
				{
					const auto iterations = std::ranges::min(
						static_cast<size_type>((it_input_end - it_input_current) / sizeof(__m512i)),
						static_cast<size_type>(255)
					);
					const auto this_turn_end = it_input_current + iterations * sizeof(__m512i) - sizeof(__m512i);

					auto runner = _mm512_setzero_si512();

					for (; it_input_current + 4 * sizeof(__m512i) <= this_turn_end; it_input_current += 4 * sizeof(__m512i))
					{
						// Load four __m512i vectors
						const auto in_0 = _mm512_loadu_si512(
							GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 0 * sizeof(__m512i)));
						const auto in_1 = _mm512_loadu_si512(
							GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 1 * sizeof(__m512i)));
						const auto in_2 = _mm512_loadu_si512(
							GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 2 * sizeof(__m512i)));
						const auto in_3 = _mm512_loadu_si512(
							GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current + 3 * sizeof(__m512i)));

						// Generate four masks
						const auto mask_0 = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in_0);
						const auto mask_1 = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in_1);
						const auto mask_2 = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in_2);
						const auto mask_3 = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in_3);

						// Apply the masks and subtract from the runner
						const auto not_ascii_0 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask_0, 0xff);
						const auto not_ascii_1 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask_1, 0xff);
						const auto not_ascii_2 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask_2, 0xff);
						const auto not_ascii_3 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask_3, 0xff);

						runner = _mm512_sub_epi8(runner, not_ascii_0);
						runner = _mm512_sub_epi8(runner, not_ascii_1);
						runner = _mm512_sub_epi8(runner, not_ascii_2);
						runner = _mm512_sub_epi8(runner, not_ascii_3);
					}

					for (; it_input_current <= this_turn_end; it_input_current += sizeof(__m512i))
					{
						const auto in = _mm512_loadu_si512(
							GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i*, it_input_current + 0 * sizeof(__m512i)));
						const auto mask = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), in);
						const auto not_ascii = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask, 0xff);
						runner = _mm512_sub_epi8(runner, not_ascii);
					}

					eight_64_bits = _mm512_add_epi64(eight_64_bits, _mm512_sad_epu8(runner, _mm512_setzero_si512()));
				}

				const auto half_0 = _mm512_extracti64x4_epi64(eight_64_bits, 0);
				const auto half_1 = _mm512_extracti64x4_epi64(eight_64_bits, 1);

				const auto result_length =
						// number of 512-bit chunks that fits into the length.
						input_length / sizeof(__m512i) * sizeof(__m512i) + //
						_mm256_extract_epi64(half_0, 0) +
						_mm256_extract_epi64(half_0, 1) +
						_mm256_extract_epi64(half_0, 2) +
						_mm256_extract_epi64(half_0, 3) +
						_mm256_extract_epi64(half_1, 0) +
						_mm256_extract_epi64(half_1, 1) +
						_mm256_extract_epi64(half_1, 2) +
						_mm256_extract_epi64(half_1, 3);

				return result_length + scalar_type::length<OutputCategory>(
					       {it_input_current, static_cast<size_type>(it_input_end - it_input_current)}
				       );
			}
			else if constexpr (
				OutputCategory == CharsCategory::UTF16_LE or
				OutputCategory == CharsCategory::UTF16_BE or
				OutputCategory == CharsCategory::UTF16 or
				OutputCategory == CharsCategory::UTF32
			)
			{
				return input.size();
			}
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
				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				it_input_current += input_length;
				it_output_current += input_length;
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
			{
				const auto process = []<bool MaskOut>(const auto in, const auto in_length, output_pointer_type out) noexcept -> size_type
				{
					constexpr auto alternate_bits = 0x5555'5555'5555'5555ull;

					const auto non_ascii = _mm512_movepi8_mask(in);
					const auto ascii = ~non_ascii;
					// Mask to denote whether the byte is a leading byte that is not ascii.
					// binary representation of -64: 1100'0000
					const auto sixth = _mm512_cmpge_epu8_mask(in, _mm512_set1_epi8(static_cast<char>(-64)));

					// the bits in ascii are inverted and zeros are interspersed in between them.
					const auto mask_a = ~_pdep_u64(ascii, alternate_bits);
					const auto mask_b = ~_pdep_u64(ascii >> 32, alternate_bits);
					// interleave bytes from top and bottom halves (abcd...ABCD -> aAbBcCdD).
					const auto input_interleaved = _mm512_permutexvar_epi8(
						// clang-format off
						_mm512_set_epi32(
							0x3f1f'3e1e,
							0x3d1d'3c1c,
							0x3b1b'3a1a,
							0x3919'3818,
							0x3717'3616,
							0x3515'3414,
							0x3313'3212,
							0x3111'3010,
							0x2f0f'2e0e,
							0x2d0d'2c0c,
							0x2b0b'2a0a,
							0x2909'2808,
							0x2707'2606,
							0x2505'2404,
							0x2303'2202,
							0x2101'2000
						),
						// clang-format on
						in);

					const auto output_a = [](const auto interleaved, const auto s, const auto mask) noexcept -> auto
					{
						// Upscale the bytes to 16-bit value, adding the 0b1100'0010 leading byte in the process.
						// We adjust for the bytes that have their two most significant bits. This takes care of the first 32 bytes, assuming we interleaved the bytes.
						// binary representation of -62: 1100'0010
						auto v = _mm512_shldi_epi16(interleaved, _mm512_set1_epi8(static_cast<char>(-62)), 8);
						v = _mm512_mask_add_epi16(
							v,
							static_cast<__mmask32>(s),
							v,
							// 1- 0x4000 = 1100 0000 0000 0001
							_mm512_set1_epi16(1 - 0x4000)
						);
						// prune redundant bytes
						return _mm512_maskz_compress_epi8(mask, v);
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
						const auto v = _mm512_ternarylogic_epi32(
							interleaved,
							leading,
							_mm512_set1_epi16(static_cast<short>(0xff00)),
							// (interleaved & 0xff00) ^ leading
							(240 & 170) ^ 204
						);
						// prune redundant bytes
						return _mm512_maskz_compress_epi8(mask, v);
					}(input_interleaved, sixth, mask_b);

					const auto out_size = static_cast<unsigned int>(in_length + std::popcount(non_ascii));
					const auto out_size_a = static_cast<unsigned int>(std::popcount(static_cast<std::uint32_t>(non_ascii))) + 32;

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
					const auto count = std::popcount(non_ascii);
					GAL_PROMETHEUS_DEBUG_AXIOM(count >= 0);
					if (count != 0) { return process.template operator()<false>(in, 64, out); }

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
					const auto in = _mm512_loadu_si512(it_input_current);
					it_output_current += process.template operator()<true>(in, 64, it_output_current);
					it_input_current += 64;
				}

				// with the last 64 bytes, the input also needs to be masked
				if (it_input_current < it_input_end)
				{
					const auto in = _mm512_maskz_loadu_epi8(
						_bzhi_u64(
							~0ull,
							static_cast<unsigned>(it_input_end - it_input_current)
						),
						it_input_current
					);
					it_output_current += process.template operator()<true>(in, it_input_end - it_input_current, it_output_current);
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				constexpr auto is_big_endian = OutputCategory != CharsCategory::UTF16_LE;

				const auto byte_flip = _mm512_setr_epi64(
					0x0607'0405'0203'0001,
					0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001,
					0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001,
					0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001,
					0x0e0f'0c0d'0a0b'0809
				);

				// Round down to nearest multiple of 32
				const auto rounded_input_length = input_length & ~0x1f;
				const auto it_rounded_input_end = it_input_begin + rounded_input_length;

				for (; it_input_current < it_rounded_input_end; it_input_current += 32, it_output_current += 32)
				{
					// Load 32 ascii characters into a 256-bit register
					const auto in = _mm256_loadu_si256(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m256i *, it_input_current));
					// Zero extend each set of 8 ascii characters to 32 16-bit integers
					const auto out = [&byte_flip](const auto i) noexcept -> auto
					{
						if constexpr (is_big_endian) { return _mm512_shuffle_epi8(_mm512_cvtepu8_epi16(i), byte_flip); }
						else { return _mm512_cvtepu8_epi16(i); }
					}(in);
					// Store the results back to memory
					_mm512_storeu_si512(it_output_current, out);
				}

				GAL_PROMETHEUS_DEBUG_AXIOM(it_input_current == it_rounded_input_end);

				if (const auto remaining = input_length - rounded_input_length;
					remaining != 0)
				{
					const auto mask = (__mmask32{1} << (input_length - rounded_input_length)) - 1;
					const auto in = _mm256_maskz_loadu_epi8(mask, it_input_current);
					// Zero extend each set of 8 ascii characters to 32 16-bit integers
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
				const auto it_rounded_input_end = it_input_begin + rounded_input_length;

				for (; it_input_current < it_rounded_input_end; it_input_current += 16, it_output_current += 16)
				{
					// Load 16 ascii characters into a 128-bit register
					const auto in = _mm_loadu_si128(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m128i *, it_input_current));
					// Zero extend each set of 8 ascii characters to 16 32-bit integers
					const auto out = _mm512_cvtepu8_epi32(in);
					// Store the results back to memory
					_mm512_storeu_si512(it_output_current, out);
				}

				GAL_PROMETHEUS_DEBUG_AXIOM(it_input_current == it_rounded_input_end);

				if (const auto remaining = input_length - rounded_input_length;
					remaining != 0)
				{
					// fixme: We can't assume that conversions are always successful
					const auto processed_length =
							scalar_type::convert<
								CharsCategory::UTF32,
								InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT,
								CheckNextBlock
							>(
								{it_input_current, input_length - rounded_input_length},
								it_output_current
							);
					it_input_current += processed_length;
					it_output_current += processed_length;
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
		) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t> //
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
