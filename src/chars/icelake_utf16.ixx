// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif
#include <prometheus/macro.hpp>

export module gal.prometheus.chars:icelake.utf16;

import std;
import gal.prometheus.meta;
import gal.prometheus.memory;
GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

import :encoding;
import :scalar.utf16;

#else
#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif
#include <cstring>

#include <prometheus/macro.hpp>
#include <chars/encoding.ixx>
#include <meta/meta.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

// ReSharper disable once CppRedundantNamespaceDefinition
GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::chars)
{
	template<>
	class Simd<"icelake.utf16">
	{
	public:
		using scalar_type = Scalar<"utf16">;

		constexpr static auto input_category = scalar_type::input_category;
		using input_type = scalar_type::input_type;
		using char_type = scalar_type::char_type;
		using pointer_type = scalar_type::pointer_type;
		using size_type = scalar_type::size_type;

		template<std::endian Endian, bool ReturnResultType = false>
		[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const auto byte_flip = _mm512_setr_epi64(
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809);

			for (; it_input_current + 32 <= it_input_end; it_input_current += 32)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				const auto in = [byte_flip](const auto c) noexcept
				{
					if constexpr (Endian == std::endian::native)
					{
						#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
						// error : lambda capture 'byte_flip' is not used [-Werror,-Wunused-lambda-capture]
						(void)byte_flip;
						#endif

						return _mm512_loadu_si512(c);
					}
					else { return _mm512_shuffle_epi8(_mm512_loadu_si512(c), byte_flip); }
				}(it_input_current);
				const auto diff = _mm512_sub_epi16(in, _mm512_set1_epi16(static_cast<short>(0xd800)));

				if (const auto surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(0x0800));
					surrogates)
				{
					const auto high_surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(0x0400));
					const auto low_surrogates = surrogates ^ high_surrogates;
					// high must be followed by low
					if ((high_surrogates << 1) != low_surrogates)
					{
						if constexpr (ReturnResultType)
						{
							const auto extra_high = std::countr_zero(static_cast<std::uint32_t>(high_surrogates & ~(low_surrogates >> 1)));
							const auto extra_low = std::countr_zero(static_cast<std::uint32_t>(low_surrogates & ~(high_surrogates << 1)));

							return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error + std::ranges::min(extra_high, extra_low)};
						}
						else { return false; }
					}

					if (const auto ends_with_high = ((high_surrogates & 0x8000'0000) != 0);
						ends_with_high)
					{
						// advance only by 31 code units so that we start with the high surrogate on the next round.
						// see `for (; it_input_current + 32 <= it_input_end; it_input_current += 32)`
						it_input_current -= 1;
					}
				}
			}

			if (it_input_current < it_input_end)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				const auto in = [byte_flip](const auto c, const auto e) noexcept
				{
					if constexpr (Endian == std::endian::native)
					{
						#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
						// error : lambda capture 'byte_flip' is not used [-Werror,-Wunused-lambda-capture]
						(void)byte_flip;
						#endif

						return _mm512_maskz_loadu_epi16((__mmask32{1} << (e - c)) - 1, c);
					}
					else { return _mm512_shuffle_epi8(_mm512_maskz_loadu_epi16((__mmask32{1} << (e - c)) - 1, c), byte_flip); }
				}(it_input_current, it_input_end);
				const auto diff = _mm512_sub_epi16(in, _mm512_set1_epi16(static_cast<short>(0xd800)));

				if (const auto surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(0x0800));
					surrogates)
				{
					const auto high_surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(0x0400));
					const auto low_surrogates = surrogates ^ high_surrogates;
					// high must be followed by low
					if ((high_surrogates << 1) != low_surrogates)
					{
						if constexpr (ReturnResultType)
						{
							const auto extra_high = std::countr_zero(static_cast<std::uint32_t>(high_surrogates & ~(low_surrogates >> 1)));
							const auto extra_low = std::countr_zero(static_cast<std::uint32_t>(low_surrogates & ~(high_surrogates << 1)));

							return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error + std::ranges::min(extra_high, extra_low)};
						}
						else { return false; }
					}
				}
			}

			if constexpr (ReturnResultType) { return result_type{.error = ErrorCode::NONE, .count = input_length}; }
			else { return true; }
		}

		template<std::endian Endian, bool ReturnResultType = false>
		[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
		{
			return validate<Endian, ReturnResultType>({input, std::char_traits<char_type>::length(input)});
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory, std::endian Endian = std::endian::native>
		[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

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

			if constexpr (OutputCategory == CharsCategory::ASCII) { return input.size(); } // NOLINT(bugprone-branch-clone)
			else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
			{
				const auto v_007f = _mm512_set1_epi16(0x007f);
				const auto v_07ff = _mm512_set1_epi16(0x07ff);
				const auto v_dfff = _mm512_set1_epi16(static_cast<short>(0xdfff));
				const auto v_d800 = _mm512_set1_epi16(static_cast<short>(0xd800));

				auto result_length = static_cast<size_type>(0);

				for (; it_input_current + 32 <= it_input_end; it_input_current += 32)
				{
					const auto in = [byte_flip](const auto c) noexcept
					{
						if constexpr (Endian == std::endian::native)
						{
							#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
							// error : lambda capture 'byte_flip' is not used [-Werror,-Wunused-lambda-capture]
							(void)byte_flip;
							#endif

							return _mm512_loadu_si512(c);
						}
						else
						{
							return _mm512_shuffle_epi8(_mm512_loadu_si512(c), byte_flip);
						}
					}(it_input_current);

					const auto ascii_bitmask = _mm512_cmple_epu16_mask(in, v_007f);
					const auto two_bytes_bitmask = _mm512_mask_cmple_epu16_mask(~ascii_bitmask, in, v_07ff);
					const auto surrogates_bitmask =
							_mm512_mask_cmple_epu16_mask(~(ascii_bitmask | two_bytes_bitmask), in, v_dfff) &
							_mm512_mask_cmpge_epu16_mask(~(ascii_bitmask | two_bytes_bitmask), in, v_d800);

					const auto ascii_count = std::popcount(ascii_bitmask);
					const auto two_bytes_count = std::popcount(two_bytes_bitmask);
					const auto surrogates_bytes_count = std::popcount(surrogates_bitmask);
					const auto three_bytes_count = 32 - ascii_count - two_bytes_count - surrogates_bytes_count;

					result_length += ascii_count + 2 * two_bytes_count + 2 * surrogates_bytes_count + 3 * three_bytes_count;
				}

				return result_length + scalar_type::length<OutputCategory, Endian>({it_input_current, input_length - (it_input_current - it_input_begin)});
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE or OutputCategory == CharsCategory::UTF16)
			{
				return input.size();
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				const auto low = _mm512_set1_epi16(static_cast<short>(0xdc00));
				const auto high = _mm512_set1_epi16(static_cast<short>(0xdfff));

				auto result_length = static_cast<size_type>(0);

				for (; it_input_current + 32 <= it_input_end; it_input_current += 32)
				{
					const auto in = [byte_flip](const auto c) noexcept
					{
						if constexpr (Endian == std::endian::native)
						{
							#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
							// error : lambda capture 'byte_flip' is not used [-Werror,-Wunused-lambda-capture]
							(void)byte_flip;
							#endif

							return _mm512_loadu_si512(c);
						}
						else { return _mm512_shuffle_epi8(_mm512_loadu_si512(c), byte_flip); }
					}(it_input_current);

					const auto not_high_surrogate_bitmask = static_cast<std::uint64_t>(_mm512_cmpgt_epu16_mask(in, high) | _mm512_cmplt_epu16_mask(in, low));

					const auto not_high_surrogate_count = std::popcount(not_high_surrogate_bitmask);

					result_length += not_high_surrogate_count;
				}

				return result_length + scalar_type::length<OutputCategory, Endian>({it_input_current, input_length - (it_input_current - it_input_begin)});
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory, std::endian Endian = std::endian::native>
		[[nodiscard]] constexpr static auto length(const pointer_type input) noexcept -> size_type
		{
			return length<OutputCategory, Endian>({input, std::char_traits<char_type>::length(input)});
		}

		template<
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			std::endian Endian = std::endian::native,
			bool CheckNextBlock = true
		>
		[[nodiscard]] constexpr static auto convert(
			const input_type input,
			typename output_type<OutputCategory>::pointer output
		) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);
			if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(validate<Endian>(input));
			}

			using output_pointer_type = typename output_type<OutputCategory>::pointer;
			using output_char_type = typename output_type<OutputCategory>::value_type;

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const output_pointer_type it_output_begin = output;
			output_pointer_type it_output_current = it_output_begin;

			const auto byte_flip = _mm512_setr_epi64(
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809);

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				const auto v_00ff = _mm512_set1_epi32(0x00ff);
				const auto shuffle_mask = _mm512_set_epi8(
					// clang-format off
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					62,
					60,
					58,
					56,
					54,
					52,
					50,
					48,
					46,
					44,
					42,
					40,
					38,
					36,
					34,
					32,
					30,
					28,
					26,
					24,
					22,
					20,
					18,
					16,
					14,
					12,
					10,
					8,
					6,
					4,
					2,
					0
					// clang-format on
				);

				for (; it_input_current + 32 <= it_input_end; it_input_current += 32, it_output_current += 32)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					const auto in = [byte_flip](const auto c) noexcept
					{
						if constexpr ((Endian == std::endian::little) == (OutputCategory == CharsCategory::UTF16_LE))
						{
							#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
							// error : lambda capture 'byte_flip' is not used [-Werror,-Wunused-lambda-capture]
							(void)byte_flip;
							#endif

							return _mm512_loadu_si512(c);
						}
						else { return _mm512_shuffle_epi8(_mm512_loadu_si512(c), byte_flip); }
					}(it_input_current);

					if (_mm512_cmpgt_epu16_mask(in, v_00ff))
					{
						if constexpr (
							ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
							ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
						) { return 0; }
						else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
						{
							const auto it = std::ranges::find_if(
								it_input_current,
								it_input_current + 32,
								[](const auto data) noexcept { return data > 0xff; });
							// fixme: write or ignore?
							{
								std::ranges::transform(
									std::ranges::subrange{it_input_current, it},
									it_output_current,
									[](const auto word) noexcept -> output_char_type
									{
										return static_cast<output_char_type>(word);
									}
								);
								it_output_current += std::ranges::distance(it_input_current, it);
							}

							return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + std::ranges::distance(it_input_current, it)};
						}
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
					}

					_mm256_storeu_si256(
						GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m256i*, it_output_current),
						_mm512_castsi512_si256(_mm512_permutexvar_epi8(shuffle_mask, in))
					);
				}

				if (const auto remaining = it_input_end - it_input_current;
					remaining != 0)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					const auto mask = static_cast<__mmask16>((1 << remaining) - 1);
					const auto in = [byte_flip, mask](const auto c) noexcept
					{
						if constexpr ((Endian == std::endian::little) == (OutputCategory == CharsCategory::UTF16_LE))
						{
							#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
							// error : lambda capture 'byte_flip' is not used [-Werror,-Wunused-lambda-capture]
							(void)byte_flip;
							#endif

							return _mm512_maskz_loadu_epi16(mask, c);
						}
						else { return _mm512_shuffle_epi8(_mm512_maskz_loadu_epi16(mask, c), byte_flip); }
					}(it_input_current);

					if (_mm512_cmpgt_epu16_mask(in, v_00ff))
					{
						if constexpr (
							ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
							ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
						) { return 0; }
						else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
						{
							const auto it = std::ranges::find_if(
								it_input_current,
								it_input_current + remaining,
								[](const auto data) noexcept { return data > 0xff; });
							// fixme: write or ignore?
							{
								std::ranges::transform(
									std::ranges::subrange{it_input_current, it},
									it_output_current,
									[](const auto word) noexcept -> output_char_type
									{
										return static_cast<output_char_type>(word);
									}
								);
								it_output_current += std::ranges::distance(it_input_current, it);
							}

							return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + std::ranges::distance(it_input_current, it)};
						}
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
					}

					_mm256_mask_storeu_epi8(it_output_current, mask, _mm512_castsi512_si256(_mm512_permutexvar_epi8(shuffle_mask, in)));
					it_input_current += remaining;
					it_output_current += remaining;
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
			{
				int advanced_iteration = 0;
				int carry = 0;

				const auto process = functional::y_combinator{
						[&](auto self, const __m512i current_in, const __mmask32 current_in_mask) noexcept -> void
						{
							const auto v_0000_0080 = _mm512_set1_epi16(0x0000'0080);
							const auto v_0000_3f3f = _mm512_set1_epi16(0x0000'3f3f);
							const auto v_0000_ffff = _mm512_set1_epi16(static_cast<short>(0x0000'ffff));
							const auto v_0000_0800 = _mm512_set1_epi16(0x0000'0800);
							const auto v_0000_80c0 = _mm512_set1_epi16(static_cast<short>(0x0000'80c0));
							const auto v_8080_e000 = _mm512_set1_epi32(static_cast<int>(0x8080'e000));
							const auto v_0000_fc00 = _mm512_set1_epi16(static_cast<short>(0x0000'fc00));
							const auto v_0000_d800 = _mm512_set1_epi16(static_cast<short>(0x0000'd800));
							const auto v_0000_dc00 = _mm512_set1_epi16(static_cast<short>(0x0000'dc00));
							const auto v_8080_80f0 = _mm512_set1_epi32(static_cast<int>(0x8080'80f0));
							const auto v_fca0_2400 = _mm512_set1_epi32(static_cast<int>(0xfca0'2400));
							const auto v_80c0_0000 = _mm512_set1_epi32(static_cast<int>(0x80c0'0000));
							const auto v_ffff_ffff = _mm512_set1_epi32(static_cast<int>(0xffff'ffff));
							const auto v_0001_0101 = _mm512_set1_epi32(0x0001'0101);
							const auto v_3f3f_3f3f = _mm512_set1_epi32(0x3f3f'3f3f);

							const auto v_2026_2c32_0006_0c12 = _mm512_set1_epi64(0x2026'2c32'0006'0c12);

							const auto is_234_byte = _mm512_mask_cmp_epu16_mask(current_in_mask, current_in, v_0000_0080, _MM_CMPINT_NLT);
							// ASCII only path
							if (_ktestz_mask32_u8(current_in_mask, is_234_byte))
							{
								_mm512_mask_cvtepi16_storeu_epi8(it_output_current, current_in_mask, current_in);
								it_output_current += 31;

								carry = 0;
								return;
							}

							const auto is_12_byte = _mm512_cmp_epu16_mask(current_in, v_0000_0800, _MM_CMPINT_LT);
							// 1/2 byte path
							if (_ktestc_mask32_u8(is_12_byte, current_in_mask))
							{
								// (A|B)&C
								const auto two_bytes = _mm512_ternarylogic_epi32(
									_mm512_slli_epi16(current_in, 8),
									_mm512_srli_epi16(current_in, 6),
									v_0000_3f3f,
									0xa8
								);
								const auto compare_mask = _mm512_mask_blend_epi16(current_in_mask, v_0000_ffff, v_0000_0800);
								const auto in = _mm512_mask_add_epi16(current_in, is_234_byte, two_bytes, v_0000_80c0);
								const auto smoosh = _mm512_cmp_epu8_mask(in, compare_mask, _MM_CMPINT_NLT);
								const auto out = _mm512_maskz_compress_epi8(smoosh, in);

								_mm512_mask_storeu_epi8(
									it_output_current,
									_cvtu64_mask64(_pext_u64(_cvtmask64_u64(smoosh), _cvtmask64_u64(smoosh))),
									out
								);
								it_output_current += 31 + std::popcount(_cvtmask32_u32(is_234_byte));

								carry = 0;
								return;
							}

							auto low = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(current_in));
							auto high = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(current_in, 1));
							auto tag_low = v_8080_e000;
							auto tag_high = v_8080_e000;

							const auto masked_in = _mm512_and_epi32(current_in, v_0000_fc00);
							const auto high_surrogate = _mm512_mask_cmp_epu16_mask(current_in_mask, masked_in, v_0000_d800, _MM_CMPINT_EQ);
							const auto low_surrogate = _mm512_cmp_epu16_mask(masked_in, v_0000_dc00, _MM_CMPINT_EQ);

							int carry_out = 0;
							// handle surrogates
							if (not _kortestz_mask32_u8(high_surrogate, low_surrogate))
							{
								const auto high_surrogate_high = _kshiftri_mask32(high_surrogate, 16);

								tag_low = _mm512_mask_mov_epi32(tag_low, static_cast<__mmask16>(high_surrogate), v_8080_80f0);
								tag_high = _mm512_mask_mov_epi32(tag_high, static_cast<__mmask16>(high_surrogate_high), v_8080_80f0);

								low = [low, high_surrogate](const auto l) mutable noexcept
								{
									low = _mm512_mask_slli_epi32(low, static_cast<__mmask16>(high_surrogate), low, 10);
									low = _mm512_mask_add_epi32(low, static_cast<__mmask16>(high_surrogate), low, l);
									return low;
								}(_mm512_add_epi32(_mm512_alignr_epi32(high, low, 1), v_fca0_2400));
								high = [high, high_surrogate_high](const auto h) mutable noexcept
								{
									high = _mm512_mask_slli_epi32(high, static_cast<__mmask16>(high_surrogate_high), high, 10);
									high = _mm512_mask_add_epi32(high, static_cast<__mmask16>(high_surrogate_high), high, h);

									return high;
								}(_mm512_add_epi32(_mm512_alignr_epi32(low, high, 1), v_fca0_2400));

								carry_out = static_cast<int>(_cvtu32_mask32(_kshiftri_mask32(high_surrogate, 30)));

								const auto low_mask = _cvtmask32_u32(low_surrogate);
								const auto high_mask = _cvtmask32_u32(high_surrogate);
								// check for mismatched surrogates
								if ((high_mask + high_mask + carry) ^ low_mask)
								{
									const auto low_no_high = low_mask & ~(high_mask + high_mask + carry);
									const auto high_no_low = high_mask & ~(low_mask >> 1);
									const auto length = std::countr_zero(low_no_high | high_no_low);

									const auto in_mask = __mmask32{0x7fff'ffff} & ((1 << length) - 1);
									const auto in = _mm512_maskz_mov_epi16(in_mask, current_in);

									advanced_iteration = length - 31;

									return self(in, in_mask);
								}
							}

							high = _mm512_maskz_mov_epi32(_cvtu32_mask16(0x0000'7fff), high);
							carry = carry_out;

							const auto out_mask = _kandn_mask32(low_surrogate, current_in_mask);
							const auto out_mask_high = _kshiftri_mask32(out_mask, 16);
							const auto magic_low = _mm512_mask_blend_epi32(static_cast<__mmask16>(out_mask), v_ffff_ffff, v_0001_0101);
							const auto magic_high = _mm512_mask_blend_epi32(static_cast<__mmask16>(out_mask_high), v_ffff_ffff, v_0001_0101);

							const auto is_1_byte = _knot_mask32(is_234_byte);
							const auto is_1_byte_high = _kshiftri_mask32(is_1_byte, 16);
							const auto is_12_byte_high = _kshiftri_mask32(is_12_byte, 16);

							tag_low = _mm512_mask_mov_epi32(tag_low, static_cast<__mmask16>(is_12_byte), v_80c0_0000);
							tag_high = _mm512_mask_mov_epi32(tag_high, static_cast<__mmask16>(is_12_byte_high), v_80c0_0000);

							const auto multi_shift_low = _mm512_mask_slli_epi32(
								_mm512_ternarylogic_epi32(
									_mm512_multishift_epi64_epi8(v_2026_2c32_0006_0c12, low),
									v_3f3f_3f3f,
									tag_low,
									0xea),
								static_cast<__mmask16>(is_1_byte),
								low,
								24);
							const auto multi_shift_high = _mm512_mask_slli_epi32(
								_mm512_ternarylogic_epi32(
									_mm512_multishift_epi64_epi8(v_2026_2c32_0006_0c12, high),
									v_3f3f_3f3f,
									tag_high,
									0xea),
								static_cast<__mmask16>(is_1_byte_high),
								high,
								24);

							const auto want_low = _mm512_cmp_epu8_mask(multi_shift_low, magic_low, _MM_CMPINT_NLT);
							const auto want_high = _mm512_cmp_epu8_mask(multi_shift_high, magic_high, _MM_CMPINT_NLT);
							const auto want_low_mask = _cvtmask64_u64(want_low);
							const auto want_high_mask = _cvtmask64_u64(want_high);
							const auto length_low = std::popcount(want_low_mask);
							const auto length_high = std::popcount(want_high_mask);

							const auto out_low = _mm512_maskz_compress_epi8(want_low, multi_shift_low);
							const auto out_high = _mm512_maskz_compress_epi8(want_high, multi_shift_high);

							_mm512_mask_storeu_epi8(it_output_current, _cvtu64_mask64(_pext_u64(want_low_mask, want_low_mask)), out_low);
							it_output_current += length_low;
							_mm512_mask_storeu_epi8(it_output_current, _cvtu64_mask64(_pext_u64(want_high_mask, want_high_mask)), out_high);
							it_output_current += length_high;
						}
				};

				for (; it_input_current + 32 <= it_input_end; it_input_current += 31)
				{
					const auto in = [byte_flip](const auto c) noexcept
					{
						if constexpr (Endian == std::endian::native)
						{
							#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
							// error : lambda capture 'byte_flip' is not used [-Werror,-Wunused-lambda-capture]
							(void)byte_flip;
							#endif

							return _mm512_loadu_si512(c);
						}
						else
						{
							return _mm512_shuffle_epi8(_mm512_loadu_si512(c), byte_flip);
						}
					}(it_input_current);
					const auto in_mask = _cvtu32_mask32(0x7fff'ffff);

					process(in, in_mask);
				}
				it_output_current -= advanced_iteration;

				if (const auto remaining = it_input_end - it_input_current;
					remaining != 0)
				{
					const auto in_mask = _cvtu32_mask32((1 << remaining) - 1);
					const auto in = [byte_flip, in_mask](const auto c) noexcept
					{
						if constexpr (Endian == std::endian::native)
						{
							#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
							// error : lambda capture 'byte_flip' is not used [-Werror,-Wunused-lambda-capture]
							(void)byte_flip;
							#endif

							return _mm512_maskz_loadu_epi16(in_mask, c);
						}
						else { return _mm512_shuffle_epi8(_mm512_maskz_loadu_epi16(in_mask, c), byte_flip); }
					}(it_input_current);

					advanced_iteration = static_cast<int>(remaining - 31);
					it_input_current += remaining;

					process(in, in_mask);
					it_output_current -= advanced_iteration;
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				if constexpr ((Endian == std::endian::little) == (OutputCategory == CharsCategory::UTF16_LE))
				{
					std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				}
				else
				{
					flip_endian(input, it_output_current);
				}

				it_input_current += input_length;
				it_output_current += input_length;
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				const auto process = [&]() noexcept -> bool
				{
					const auto v_0000_fc00 = _mm512_set1_epi16(static_cast<short>(0x0000'fc00));
					const auto v_0000_d800 = _mm512_set1_epi16(static_cast<short>(0x0000'd800));
					const auto v_0000_dc00 = _mm512_set1_epi16(static_cast<short>(0x0000'dc00));

					__mmask32 carry = 0;

					while (it_input_current + 32 <= it_input_end)
					{
						const auto in = [byte_flip](const auto c) noexcept
						{
							if constexpr (Endian == std::endian::native)
							{
								#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
								// error : lambda capture 'byte_flip' is not used [-Werror,-Wunused-lambda-capture]
								(void)byte_flip;
								#endif

								return _mm512_loadu_si512(c);
							}
							else { return _mm512_shuffle_epi8(_mm512_loadu_si512(c), byte_flip); }
						}(it_input_current);

						const auto high_bitmask = _mm512_cmpeq_epi16_mask(_mm512_and_si512(in, v_0000_fc00), v_0000_d800);
						const auto low_bitmask = _mm512_cmpeq_epi16_mask(_mm512_and_si512(in, v_0000_fc00), v_0000_dc00);

						if (high_bitmask | low_bitmask)
						{
							// surrogate pair(s) in a register
							// A high surrogate must be followed by low one and a low one must be preceded by a high one.
							// If valid, value should be equal to 0
							const auto value = low_bitmask ^ (carry | (high_bitmask << 1));

							if (value == 0)
							{
								// valid case
								// Input surrogate pair:
								// |1101.11aa.aaaa.aaaa|1101.10bb.bbbb.bbbb|
								//     low surrogate      high surrogate

								// Expand all code units to 32-bit code units
								// in > |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0000.0000.0000.1101.10bb.bbbb.bbbb|
								const auto first = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(in));
								const auto second = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(in, 1));

								// Shift by one 16-bit word to align low surrogates with high surrogates
								// in >       |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0000.0000.0000.1101.10bb.bbbb.bbbb|
								// shifted >  |????.????.????.????.????.????.????.????|0000.0000.0000.0000.1101.11aa.aaaa.aaaa|
								const auto shifted_first = _mm512_alignr_epi32(second, first, 1);
								const auto shifted_second = _mm512_alignr_epi32(_mm512_setzero_si512(), second, 1);

								// Align all high surrogates in first and second by shifting to the left by 10 bits
								// |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0011.0110.bbbb.bbbb.bb00.0000.0000|
								const auto aligned_first = _mm512_mask_slli_epi32(first, static_cast<__mmask16>(high_bitmask), first, 10);
								const auto aligned_second = _mm512_mask_slli_epi32(second, high_bitmask >> 16, second, 10);

								// Remove surrogate prefixes and add offset 0x0001'0000 by adding in, shifted and constant
								// constant > |1111.1100.1010.0000.0010.0100.0000.0000|1111.1100.1010.0000.0010.0100.0000.0000|
								// in >       |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0011.0110.bbbb.bbbb.bb00.0000.0000|
								// shifted >  |????.????.????.????.????.????.????.????|0000.0000.0000.0000.1101.11aa.aaaa.aaaa|
								const auto constant = _mm512_set1_epi32(static_cast<int>(0b1111'1100'1010'0000'0010'0100'0000'0000));

								const auto added_first = _mm512_mask_add_epi32(aligned_first, static_cast<__mmask16>(high_bitmask), aligned_first, shifted_first);
								const auto added_second = _mm512_mask_add_epi32(aligned_second, high_bitmask >> 16, aligned_second, shifted_second);

								const auto utf32_first = _mm512_mask_add_epi32(added_first, static_cast<__mmask16>(high_bitmask), added_first, constant);
								const auto utf32_second = _mm512_mask_add_epi32(added_second, high_bitmask >> 16, added_second, constant);

								// Store all valid UTF-32 code units (low surrogate positions and 32nd word are invalid)
								const auto valid = ~low_bitmask & 0x7fff'ffff;

								const auto compressed_first = _mm512_maskz_compress_epi32(static_cast<__mmask16>(valid), utf32_first);
								const auto compressed_second = _mm512_maskz_compress_epi32(valid >> 16, utf32_second);

								const auto length_first = std::popcount(static_cast<std::uint16_t>(valid));
								const auto length_second = std::popcount(static_cast<std::uint16_t>(valid >> 16));

								_mm512_storeu_si512(it_output_current, compressed_first);
								it_output_current += length_first;
								// _mm512_storeu_si512(it_output_current, compressed_second);
								_mm512_mask_storeu_epi32(it_output_current, static_cast<__mmask16>(1 << length_second) - 1, compressed_second);
								it_output_current += length_second;

								// Only process 31 code units, but keep track if the 31st word is a high surrogate as a carry
								it_input_current += 31;
								carry = (high_bitmask >> 30) & 0x1;
							}
							else
							{
								// invalid case
								it_input_current += carry;
								return false;
							}
						}
						else
						{
							// no surrogates
							// extend all thirty-two 16-bit code units to thirty-two 32-bit code units
							_mm512_storeu_si512(it_output_current, _mm512_cvtepu16_epi32(_mm512_castsi512_si256(in)));
							it_output_current += 16;
							_mm512_storeu_si512(it_output_current, _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(in, 1)));
							it_output_current += 16;

							it_input_current += 32;
							carry = 0;
						}
					}

					it_input_current += carry;
					return true;
				};

				const auto result = process();
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					not result or remaining != 0)
				{
					if (const auto scalar_result = scalar_type::convert<OutputCategory, ProcessPolicy, Endian, CheckNextBlock>(
							{it_input_current, remaining},
							it_output_current
						);
						not scalar_result)
					{
						if constexpr (
							ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
							ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
						) { return 0; }
						else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
						{
							return result_type{.error = scalar_result.error, .count = length_if_error + scalar_result.count};
						}
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
					}
					else
					{
						if constexpr (
							ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
							ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
						) { it_output_current += scalar_result; }
						else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE) { it_input_current += scalar_result.count; }
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
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
			std::endian Endian = std::endian::native,
			bool CheckNextBlock = true
		>
		[[nodiscard]] constexpr static auto convert(
			const pointer_type input,
			typename output_type<OutputCategory>::pointer output
		) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
		{
			return convert<OutputCategory, ProcessPolicy, Endian, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, output);
		}

		template<
			typename StringType,
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			std::endian Endian = std::endian::native,
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
			result.resize(length<OutputCategory, Endian>(input));

			(void)convert<OutputCategory, ProcessPolicy, Endian, CheckNextBlock>(input, result.data());
			return result;
		}

		template<
			typename StringType,
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			std::endian Endian = std::endian::native,
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
			result.resize(length<OutputCategory, Endian>(input));

			return convert<OutputCategory, ProcessPolicy, Endian, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, result.data());
		}

		template<
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			std::endian Endian = std::endian::native,
			bool CheckNextBlock = true
		>
		[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
		{
			std::basic_string<typename output_type<OutputCategory>::value_type> result{};
			result.resize(length<OutputCategory, Endian>(input));

			(void)convert<OutputCategory, ProcessPolicy, Endian, CheckNextBlock>(input, result.data());
			return result;
		}

		template<
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			std::endian Endian = std::endian::native,
			bool CheckNextBlock = true
		>
		[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
		{
			std::basic_string<typename output_type<OutputCategory>::value_type> result{};
			result.resize(length<OutputCategory, Endian>(input));

			return convert<OutputCategory, ProcessPolicy, Endian, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, result.data());
		}

		/*constexpr*/
		static auto flip_endian(const input_type input, const output_type<input_category>::pointer output) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const auto it_output_begin = output;
			auto it_output_current = it_output_begin;

			const auto byte_flip = _mm512_setr_epi64(
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001,
				0x0e0f'0c0d'0a0b'0809);

			for (; it_input_current + 32 <= it_input_end; it_input_current += 32, it_output_current += 32)
			{
				const auto utf16 = _mm512_shuffle_epi8(_mm512_loadu_si512(it_input_current), byte_flip);
				_mm512_storeu_si512(it_output_current, utf16);
			}

			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				const auto mask = static_cast<__mmask32>((1 << remaining) - 1);
				const auto utf16 = _mm512_shuffle_epi8(_mm512_maskz_loadu_epi16(mask, it_input_current), byte_flip);
				_mm512_mask_storeu_epi16(it_output_current, mask, utf16);
			}
		}

		template<typename StringType>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<output_type<input_category>::pointer>;
			}
		[[nodiscard]] /*constexpr*/ static auto flip_endian(const input_type input) noexcept -> StringType
		{
			StringType result{};
			result.resize(length<input_category>(input));

			flip_endian(input, result.data());
			return result;
		}

		[[nodiscard]] /*constexpr*/ static auto flip_endian(const input_type input) noexcept -> std::basic_string<output_type<input_category>::value_type>
		{
			std::basic_string<output_type<input_category>::value_type> result{};
			result.resize(length<input_category>(input));

			flip_endian(input, result.data());
			return result;
		}
	};

	template<>
	struct simd_processor_of<CharsCategory::UTF16, "icelake">
	{
		using type = Simd<"icelake.utf16">;
	};

	template<>
	struct simd_processor_of<CharsCategory::UTF16_LE, "icelake"> : simd_processor_of<CharsCategory::UTF16, "icelake"> {};

	template<>
	struct simd_processor_of<CharsCategory::UTF16_BE, "icelake"> : simd_processor_of<CharsCategory::UTF16, "icelake"> {};
} // namespace gal::prometheus::chars
