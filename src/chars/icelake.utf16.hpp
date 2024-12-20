// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if defined(GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED)

#include <bit>
#include <functional>
#include <numeric>
#include <string>
#include <algorithm>

#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

#include <prometheus/macro.hpp>

#include <chars/encoding.hpp>
#include <chars/scalar.utf16.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

template<>
class gal::prometheus::chars::Simd<"icelake.utf16">
{
public:
	using scalar_type = Scalar<"utf16">;

	constexpr static auto chars_type = scalar_type::chars_type;

	using input_type = scalar_type::input_type;
	using char_type = scalar_type::char_type;
	using pointer_type = scalar_type::pointer_type;
	using size_type = scalar_type::size_type;

private:
	using data_type = __m512i;

	constexpr static std::size_t size_per_char = sizeof(char_type);
	constexpr static std::size_t advance_per_step = sizeof(data_type) / size_per_char;

	template<typename OutChar>
	constexpr static std::size_t advance_per_step_with =
			// latin(1 => 1): load(advance_per_step) ==> store(dest)
			// utf8(1 => 2): load(advance_per_step) ==> store(low) + store(high)
			// utf32(1 => 2): load(advance_per_step) ==> store(low) + store(high)
			advance_per_step;

public:
	template<bool ReturnResultType = false, std::endian SourceEndian = std::endian::native>
	[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		// clang-format off
		const auto byte_flip = _mm512_setr_epi64(
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
		);
		// clang-format on

		constexpr auto step = 1 * advance_per_step;
		// keep an overlap of one code unit
		constexpr auto step_keep_high_surrogate = step - 1;

		while (it_input_current + step <= it_input_end)
		{
			const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

			const auto in = [](const auto c, [[maybe_unused]] const auto bf) noexcept
			{
				if constexpr (const auto v = _mm512_loadu_si512(c);
					SourceEndian != std::endian::native)
				{
					return _mm512_shuffle_epi8(v, bf);
				}
				else
				{
					return v;
				}
			}(it_input_current, byte_flip);
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
					else
					{
						return false;
					}
				}

				if (const auto ends_with_high = ((high_surrogates & 0x8000'0000) != 0);
					ends_with_high)
				{
					it_input_current += step_keep_high_surrogate;
				}
				else
				{
					it_input_current += step;
				}
			}

			it_input_current += step;
		}

		if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
			remaining != 0)
		{
			const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

			const auto in = [](const auto c, const auto in_length, [[maybe_unused]] const auto bf) noexcept
			{
				const auto mask = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(in_length)));
				if constexpr (const auto v = _mm512_maskz_loadu_epi16(mask, c);
					SourceEndian != std::endian::native)
				{
					return _mm512_shuffle_epi8(v, bf);
				}
				else
				{
					return v;
				}
			}(it_input_current, remaining, byte_flip);
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
					else
					{
						return false;
					}
				}
			}
		}

		if constexpr (ReturnResultType)
		{
			return result_type{.error = ErrorCode::NONE, .count = input_length};
		}
		else
		{
			return true;
		}
	}

	template<bool ReturnResultType = false, std::endian SourceEndian = std::endian::native>
	[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		return Simd::validate<SourceEndian, ReturnResultType>({input, std::char_traits<char_type>::length(input)});
	}

	// note: we are not BOM aware
	template<CharsType OutputType, std::endian SourceEndian = std::endian::native>
	[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		// clang-format off
		const auto byte_flip = _mm512_setr_epi64(
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
		);
		// clang-format on

		// ReSharper disable CppClangTidyBugproneBranchClone
		if constexpr (OutputType == CharsType::LATIN)
		{
			return input.size();
		}
		// ReSharper restore CppClangTidyBugproneBranchClone
		else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
		{
			constexpr auto step = 1 * advance_per_step;

			// ReSharper disable CppInconsistentNaming
			// ReSharper disable IdentifierTypo
			const auto v_007f = _mm512_set1_epi16(0x007f);
			const auto v_07ff = _mm512_set1_epi16(0x07ff);
			const auto v_dfff = _mm512_set1_epi16(static_cast<short>(0xdfff));
			const auto v_d800 = _mm512_set1_epi16(static_cast<short>(0xd800));
			// ReSharper restore IdentifierTypo
			// ReSharper restore CppInconsistentNaming

			auto result_length = static_cast<size_type>(0);
			for (; it_input_current + step <= it_input_end; it_input_current += step)
			{
				const auto in = [](const auto c, [[maybe_unused]] const auto bf) noexcept
				{
					if constexpr (const auto v = _mm512_loadu_si512(c);
						SourceEndian != std::endian::native)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(it_input_current, byte_flip);

				const auto ascii_bitmask = _mm512_cmple_epu16_mask(in, v_007f);
				const auto two_bytes_bitmask = _mm512_mask_cmple_epu16_mask(~ascii_bitmask, in, v_07ff);
				const auto surrogates_bitmask =
						_mm512_mask_cmple_epu16_mask(~(ascii_bitmask | two_bytes_bitmask), in, v_dfff) &
						_mm512_mask_cmpge_epu16_mask(~(ascii_bitmask | two_bytes_bitmask), in, v_d800);

				const auto ascii_count = std::popcount(ascii_bitmask);
				const auto two_bytes_count = std::popcount(two_bytes_bitmask);
				const auto surrogates_bytes_count = std::popcount(surrogates_bitmask);
				const auto three_bytes_count = step - ascii_count - two_bytes_count - surrogates_bytes_count;

				result_length +=
						1 * ascii_count +
						2 * two_bytes_count +
						2 * surrogates_bytes_count +
						3 * three_bytes_count;
			}

			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				result_length += scalar_type::length<OutputType, SourceEndian>({it_input_current, remaining});
			}

			return result_length;
		}
		// ReSharper disable CppClangTidyBugproneBranchClone
		else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
		{
			return input.size();
		}
		// ReSharper restore CppClangTidyBugproneBranchClone
		else if constexpr (OutputType == CharsType::UTF32)
		{
			constexpr auto step = 1 * advance_per_step;

			const auto low = _mm512_set1_epi16(static_cast<short>(0xdc00));
			const auto high = _mm512_set1_epi16(static_cast<short>(0xdfff));

			auto result_length = static_cast<size_type>(0);
			for (; it_input_current + step <= it_input_end; it_input_current += step)
			{
				const auto in = [](const auto c, [[maybe_unused]] const auto bf) noexcept
				{
					if constexpr (const auto v = _mm512_loadu_si512(c);
						SourceEndian != std::endian::native)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(it_input_current, byte_flip);

				const auto not_high_surrogate_bitmask =
						_mm512_cmpgt_epu16_mask(in, high) |
						_mm512_cmplt_epu16_mask(in, low);

				const auto not_high_surrogate_count = std::popcount(not_high_surrogate_bitmask);

				result_length += not_high_surrogate_count;
			}

			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				result_length += scalar_type::length<OutputType, SourceEndian>({it_input_current, remaining});
			}

			return result_length;
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}

	// note: we are not BOM aware
	template<CharsType OutputCategory, std::endian SourceEndian = std::endian::native>
	[[nodiscard]] constexpr static auto length(const pointer_type input) noexcept -> size_type
	{
		return Simd::length<OutputCategory, SourceEndian>({input, std::char_traits<char_type>::length(input)});
	}

	template<
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(
		const input_type input,
		typename output_type_of<OutputType>::pointer output
	) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);
		if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
		{
			// fixme: error C2187
			// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(Simd::validate<false, SourceEndian>(input));
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((Simd::validate<false, SourceEndian>(input)));
		}

		using output_pointer_type = typename output_type_of<OutputType>::pointer;
		using output_char_type = typename output_type_of<OutputType>::value_type;

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		const output_pointer_type it_output_begin = output;
		output_pointer_type it_output_current = it_output_begin;

		// clang-format off
		const auto byte_flip = _mm512_setr_epi64(
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
		);
		// clang-format on

		if constexpr (OutputType == CharsType::LATIN)
		{
			constexpr auto step = 1 * advance_per_step_with<output_char_type>;

			// ReSharper disable once CppInconsistentNaming
			const auto v_00ff = _mm512_set1_epi16(0xff);
			// clang-format off
			const auto shuffle_mask = _mm512_set_epi8(
				00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
				00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
				62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 
				30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8u, 06, 04, 02, 00
			);
			// clang-format on

			for (; it_input_current + step <= it_input_end; it_input_current += step, it_output_current += step)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				const auto in = [](const pointer_type c, [[maybe_unused]] const __m512i bf) noexcept
				{
					if constexpr (const auto v = _mm512_loadu_si512(c);
						SourceEndian != std::endian::little)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(it_input_current, byte_flip);

				if (_mm512_cmpgt_epu16_mask(in, v_00ff))
				{
					if constexpr (
						ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
						ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
					)
					{
						return 0;
					}
					else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
					{
						const auto it = std::ranges::find_if(
							it_input_current,
							it_input_current + step,
							[](const auto data) noexcept
							{
								if constexpr (SourceEndian != std::endian::little)
								{
									return std::byteswap(data) > 0xff;
								}
								else
								{
									return data > 0xff;
								}
							}
						);
						const auto extra = std::ranges::distance(it_input_current, it);
						{
							std::ranges::transform(
								std::ranges::subrange{it_input_current, it},
								it_output_current,
								[](const auto word) noexcept -> output_char_type
								{
									return static_cast<output_char_type>(word);
								}
							);
							it_output_current += extra;
						}

						return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + extra};
					}
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				}

				_mm256_storeu_si256(
					GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(
						__m256i*,
						it_output_current
					),
					_mm512_castsi512_si256(_mm512_permutexvar_epi8(shuffle_mask, in))
				);
			}

			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				const auto mask = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(remaining)));
				const auto in = [](const pointer_type c, const __mmask32 m, [[maybe_unused]] const __m512i bf) noexcept
				{
					if constexpr (const auto v = _mm512_maskz_loadu_epi16(m, c);
						SourceEndian != std::endian::little)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(it_input_current, mask, byte_flip);

				if (_mm512_cmpgt_epu16_mask(in, v_00ff))
				{
					if constexpr (
						ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
						ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
					)
					{
						return 0;
					}
					else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
					{
						const auto it = std::ranges::find_if(
							it_input_current,
							it_input_current + remaining,
							[](const auto data) noexcept
							{
								if constexpr (SourceEndian != std::endian::little)
								{
									return std::byteswap(data) > 0xff;
								}
								else
								{
									return data > 0xff;
								}
							}
						);
						const auto extra = std::ranges::distance(it_input_current, it);
						{
							std::ranges::transform(
								std::ranges::subrange{it_input_current, it},
								it_output_current,
								[](const auto word) noexcept -> output_char_type
								{
									return static_cast<output_char_type>(word);
								}
							);
							it_output_current += extra;
						}

						return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + extra};
					}
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				}

				_mm256_mask_storeu_epi8(it_output_current, mask, _mm512_castsi512_si256(_mm512_permutexvar_epi8(shuffle_mask, in)));
				it_input_current += remaining;
				it_output_current += remaining;
			}
		}
		else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
		{
			constexpr auto step = 1 * advance_per_step_with<output_char_type>;
			// keep an overlap of one code unit
			constexpr auto step_keep_high_surrogate = step - 1;

			struct process_result
			{
				// 0~31
				std::uint8_t processed_input;
				// processed_input + ?
				std::uint8_t num_output;
				bool end_with_surrogate;

				std::uint8_t pad;
			};
			static_assert(sizeof(process_result) == sizeof(std::uint32_t));

			const auto process = [](
				const __m512i current_in,
				const __mmask32 current_in_mask,
				const size_type current_in_length,
				const bool current_end_with_surrogate,
				output_pointer_type current_out
			) noexcept -> process_result
			{
				// ReSharper disable CppInconsistentNaming
				// ReSharper disable IdentifierTypo
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
				// ReSharper restore IdentifierTypo
				// ReSharper restore CppInconsistentNaming

				const auto is_234_byte = _mm512_mask_cmpge_epu16_mask(current_in_mask, current_in, v_0000_0080);
				if (_ktestz_mask32_u8(current_in_mask, is_234_byte))
				{
					// ASCII only
					_mm512_mask_cvtepi16_storeu_epi8(current_out, current_in_mask, current_in);

					return process_result
					{
							.processed_input = static_cast<std::uint8_t>(current_in_length),
							.num_output = static_cast<std::uint8_t>(current_in_length),
							.end_with_surrogate = false,
							.pad = 0
					};
				}

				const auto is_12_byte = _mm512_cmplt_epu16_mask(current_in, v_0000_0800);
				if (_ktestc_mask32_u8(is_12_byte, current_in_mask))
				{
					// 1/2 byte only

					// (A|B)&C
					const auto two_bytes = _mm512_ternarylogic_epi32(
						_mm512_slli_epi16(current_in, 8),
						_mm512_srli_epi16(current_in, 6),
						v_0000_3f3f,
						0xa8
					);
					const auto compare_mask = _mm512_mask_blend_epi16(current_in_mask, v_0000_ffff, v_0000_0800);
					const auto in = _mm512_mask_add_epi16(current_in, is_234_byte, two_bytes, v_0000_80c0);
					const auto smoosh = _mm512_cmpge_epu8_mask(in, compare_mask);

					const auto out = _mm512_maskz_compress_epi8(smoosh, in);
					const auto out_mask = _pext_u64(smoosh, smoosh);

					_mm512_mask_storeu_epi8(current_out, out_mask, out);

					return process_result
					{
							.processed_input = static_cast<std::uint8_t>(current_in_length),
							.num_output = static_cast<std::uint8_t>(current_in_length + std::popcount(_cvtmask32_u32(is_234_byte))),
							.end_with_surrogate = false,
							.pad = 0
					};
				}

				auto low = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(current_in));
				auto high = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(current_in, 1));
				auto tag_low = v_8080_e000;
				auto tag_high = v_8080_e000;

				const auto high_surrogate_mask = _mm512_mask_cmpeq_epu16_mask(
					current_in_mask,
					_mm512_and_epi32(current_in, v_0000_fc00),
					v_0000_d800
				);
				const auto low_surrogate_mask = _mm512_cmpeq_epu16_mask(
					_mm512_and_epi32(current_in, v_0000_fc00),
					v_0000_dc00
				);

				bool end_with_surrogate = false;
				if (not _kortestz_mask32_u8(high_surrogate_mask, low_surrogate_mask))
				{
					// handle surrogates

					const auto high_surrogate_mask_high = static_cast<__mmask16>(high_surrogate_mask >> 16);
					const auto high_surrogate_mask_low = static_cast<__mmask16>(high_surrogate_mask);

					low = [low, high_surrogate_mask_low](const auto l) mutable noexcept
					{
						low = _mm512_mask_slli_epi32(low, high_surrogate_mask_low, low, 10);
						low = _mm512_mask_add_epi32(low, high_surrogate_mask_low, low, l);
						return low;
					}(_mm512_add_epi32(_mm512_alignr_epi32(high, low, 1), v_fca0_2400));
					high = [high, high_surrogate_mask_high](const auto h) mutable noexcept
					{
						high = _mm512_mask_slli_epi32(high, high_surrogate_mask_high, high, 10);
						high = _mm512_mask_add_epi32(high, high_surrogate_mask_high, high, h);

						return high;
					}(_mm512_add_epi32(_mm512_alignr_epi32(low, high, 1), v_fca0_2400));

					tag_low = _mm512_mask_mov_epi32(tag_low, high_surrogate_mask_low, v_8080_80f0);
					tag_high = _mm512_mask_mov_epi32(tag_high, high_surrogate_mask_high, v_8080_80f0);

					end_with_surrogate = high_surrogate_mask >> 30;

					// check for mismatched surrogates
					if (((high_surrogate_mask << 1) | +current_end_with_surrogate) ^ low_surrogate_mask)
					{
						const auto low_no_high = low_surrogate_mask & ~((high_surrogate_mask << 1) | +current_end_with_surrogate);
						const auto high_no_low = high_surrogate_mask & ~(low_surrogate_mask >> 1);
						const auto length = std::countr_zero(low_no_high | high_no_low);

						return process_result
						{
								.processed_input = static_cast<std::uint8_t>(length),
								.num_output = 0,
								.end_with_surrogate = current_end_with_surrogate,
								.pad = 0
						};
					}
				}

				high = _mm512_maskz_mov_epi32(_cvtu32_mask16(0x0000'7fff), high);

				const auto out_mask = _kandn_mask32(low_surrogate_mask, current_in_mask);
				const auto out_mask_high = static_cast<__mmask16>(out_mask >> 16);
				const auto out_mask_low = static_cast<__mmask16>(out_mask);

				const auto magic_low = _mm512_mask_blend_epi32(out_mask_low, v_ffff_ffff, v_0001_0101);
				const auto magic_high = _mm512_mask_blend_epi32(out_mask_high, v_ffff_ffff, v_0001_0101);

				const auto is_1_byte = _knot_mask32(is_234_byte);

				const auto is_1_byte_high = static_cast<__mmask16>(is_1_byte >> 16);
				const auto is_1_byte_low = static_cast<__mmask16>(is_1_byte);

				const auto is_12_byte_high = static_cast<__mmask16>(is_12_byte >> 16);
				const auto is_12_byte_low = static_cast<__mmask16>(is_12_byte);

				tag_low = _mm512_mask_mov_epi32(tag_low, is_12_byte_low, v_80c0_0000);
				tag_high = _mm512_mask_mov_epi32(tag_high, is_12_byte_high, v_80c0_0000);

				const auto multi_shift_low = _mm512_mask_slli_epi32(
					_mm512_ternarylogic_epi32(
						_mm512_multishift_epi64_epi8(v_2026_2c32_0006_0c12, low),
						v_3f3f_3f3f,
						tag_low,
						0xea
					),
					is_1_byte_low,
					low,
					24
				);
				const auto multi_shift_high = _mm512_mask_slli_epi32(
					_mm512_ternarylogic_epi32(
						_mm512_multishift_epi64_epi8(v_2026_2c32_0006_0c12, high),
						v_3f3f_3f3f,
						tag_high,
						0xea
					),
					is_1_byte_high,
					high,
					24
				);

				const auto want_low = _mm512_cmpge_epu8_mask(multi_shift_low, magic_low);
				const auto want_high = _mm512_cmpge_epu8_mask(multi_shift_high, magic_high);

				const auto out_low = _mm512_maskz_compress_epi8(want_low, multi_shift_low);
				const auto out_high = _mm512_maskz_compress_epi8(want_high, multi_shift_high);

				const auto want_low_length = std::popcount(want_low);
				const auto want_high_length = std::popcount(want_high);
				const auto want_low_mask = _pext_u64(want_low, want_low);
				const auto want_high_mask = _pext_u64(want_high, want_high);

				_mm512_mask_storeu_epi8(current_out + 0, want_low_mask, out_low);
				_mm512_mask_storeu_epi8(current_out + want_low_length, want_high_mask, out_high);

				return process_result
				{
						.processed_input = static_cast<std::uint8_t>(current_in_length),
						.num_output = static_cast<std::uint8_t>(want_low_length + want_high_length),
						.end_with_surrogate = end_with_surrogate,
						.pad = 0
				};
			};

			bool end_with_surrogate = false;
			while (it_input_current + step <= it_input_end)
			{
				const auto in = [](const pointer_type c, [[maybe_unused]] const __m512i bf) noexcept
				{
					if constexpr (const auto v = _mm512_loadu_si512(c);
						SourceEndian != std::endian::native)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(it_input_current, byte_flip);
				constexpr auto in_mask = static_cast<__mmask32>(0x7fff'ffff);

				const auto result = process(in, in_mask, step_keep_high_surrogate, end_with_surrogate, it_output_current);
				if (result.processed_input != step_keep_high_surrogate)
				{
					// surrogate mismatch

					const auto valid_in_mask = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(result.processed_input)));
					const auto valid_in = _mm512_maskz_mov_epi16(valid_in_mask, in);

					const auto valid_result = process(valid_in, valid_in_mask, result.processed_input, end_with_surrogate, it_output_current);
					it_input_current += valid_result.processed_input;
					it_output_current += valid_result.num_output;

					if constexpr (
						ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
						ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
					)
					{
						return 0;
					}
					else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
					{
						return result_type{.error = ErrorCode::SURROGATE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
					}
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				}

				it_input_current += result.processed_input;
				it_output_current += result.num_output;
				end_with_surrogate = result.end_with_surrogate;
			}

			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				const auto in_mask = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(remaining)));
				const auto in = [](const auto c, const auto m, [[maybe_unused]] const auto bf) noexcept
				{
					if constexpr (const auto v = _mm512_maskz_loadu_epi16(m, c);
						SourceEndian != std::endian::native)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(it_input_current, in_mask, byte_flip);

				const auto result = process(in, in_mask, remaining, end_with_surrogate, it_output_current);
				if (result.processed_input != remaining)
				{
					// surrogate mismatch

					const auto valid_in_mask = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(result.processed_input)));
					const auto valid_in = _mm512_maskz_mov_epi16(valid_in_mask, in);

					const auto valid_result = process(valid_in, valid_in_mask, result.processed_input, end_with_surrogate, it_output_current);
					it_input_current += valid_result.processed_input;
					it_output_current += valid_result.num_output;

					if constexpr (
						ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
						ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
					)
					{
						return 0;
					}
					else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
					{
						return result_type{.error = ErrorCode::SURROGATE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
					}
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				}

				it_input_current += result.processed_input;
				it_output_current += result.num_output;
				end_with_surrogate = result.end_with_surrogate;
			}
		}
		else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE)
		{
			constexpr auto source_little = SourceEndian == std::endian::native;
			constexpr auto dest_little = OutputType == CharsType::UTF16_LE;

			if constexpr ((source_little and dest_little) or (not source_little and not dest_little))
			{
				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
			}
			else
			{
				Simd::flip_endian(input, it_output_current);
			}

			it_input_current += input_length;
			it_output_current += input_length;
		}
		else if constexpr (OutputType == CharsType::UTF32)
		{
			constexpr auto step = 2 * advance_per_step_with<output_char_type>;
			// keep an overlap of one code unit
			constexpr auto step_keep_high_surrogate = step - 1;

			struct process_result
			{
				// 0~31
				std::uint8_t processed_input;
				// processed_input + ?
				std::uint8_t num_output;
				bool end_with_surrogate;

				std::uint8_t pad;
			};
			static_assert(sizeof(process_result) == sizeof(std::uint32_t));

			const auto process = [](
				const __m512i current_in,
				const __mmask32 current_in_mask,
				const size_type current_in_length,
				const bool current_end_with_surrogate,
				output_pointer_type current_out
			) noexcept -> process_result
			{
				const auto v_0000_fc00 = _mm512_set1_epi16(static_cast<short>(0x0000'fc00));
				const auto v_0000_d800 = _mm512_set1_epi16(static_cast<short>(0x0000'd800));
				const auto v_0000_dc00 = _mm512_set1_epi16(static_cast<short>(0x0000'dc00));

				const auto high_surrogate_mask = _mm512_mask_cmpeq_epu16_mask(
					current_in_mask,
					_mm512_and_epi32(current_in, v_0000_fc00),
					v_0000_d800
				);
				const auto low_surrogate_mask = _mm512_cmpeq_epi16_mask(
					_mm512_and_epi32(current_in, v_0000_fc00),
					v_0000_dc00
				);

				if (not _kortestz_mask32_u8(high_surrogate_mask, low_surrogate_mask))
				{
					// handle surrogates

					const bool end_with_surrogate = high_surrogate_mask >> 30;

					// check for mismatched surrogates
					if (((high_surrogate_mask << 1) | +current_end_with_surrogate) ^ low_surrogate_mask)
					{
						const auto low_no_high = low_surrogate_mask & ~((high_surrogate_mask << 1) | +current_end_with_surrogate);
						const auto high_no_low = high_surrogate_mask & ~(low_surrogate_mask >> 1);
						const auto length = std::countr_zero(low_no_high | high_no_low);

						return process_result
						{
								.processed_input = static_cast<std::uint8_t>(length),
								.num_output = 0,
								.end_with_surrogate = current_end_with_surrogate,
								.pad = 0
						};
					}

					const auto high_surrogate_mask_high = static_cast<__mmask16>(high_surrogate_mask >> 16);
					const auto high_surrogate_mask_low = static_cast<__mmask16>(high_surrogate_mask);

					// Input surrogate pair:
					// |1101.11aa.aaaa.aaaa|1101.10bb.bbbb.bbbb|
					//  low surrogate            high surrogate

					// Expand all code units to 32-bit code units
					// in > |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0000.0000.0000.1101.10bb.bbbb.bbbb|
					const auto low = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(current_in));
					const auto high = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(current_in, 1));

					// Shift by one 16-bit word to align low surrogates with high surrogates
					// in >          |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0000.0000.0000.1101.10bb.bbbb.bbbb|
					// shifted >  |????. ????.  ????.  ????. ????.  ????.????. ???? |0000.0000.0000.0000.1101.11aa. aaaa. aaaa|
					const auto shifted_low = _mm512_alignr_epi32(high, low, 1);
					const auto shifted_high = _mm512_alignr_epi32(_mm512_setzero_si512(), high, 1);

					// Align all high surrogates in low and high by shifting to the left by 10 bits
					// |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0011.0110.bbbb.bbbb.bb00.0000.0000|
					const auto aligned_low = _mm512_mask_slli_epi32(low, high_surrogate_mask_low, low, 10);
					const auto aligned_high = _mm512_mask_slli_epi32(high, high_surrogate_mask_high, high, 10);

					// Remove surrogate prefixes and add offset 0x10000 by adding in, shifted and constant in
					// constant > |1111.1100.1010.0000.0010.0100.0000.0000|1111.1100.1010.0000.0010. 0100.0000. 0000|
					// in >            |0000.0000.0000.0000.1101.11aa.aaaa.aaaa |0000.0011.0110.bbbb.bbbb.bb00.0000.0000|
					// shifted >    |????.  ????. ????. ????.  ????.  ????.????. ????  |0000.0000.0000.0000.1101. 11aa. aaaa. aaaa|
					const auto constant = _mm512_set1_epi32(static_cast<int>(0b1111'1100'1010'0000'0010'0100'0000'0000));

					const auto added_low = _mm512_mask_add_epi32(aligned_low, high_surrogate_mask_low, aligned_low, shifted_low);
					const auto added_high = _mm512_mask_add_epi32(aligned_high, high_surrogate_mask_high, aligned_high, shifted_high);

					const auto utf32_low = _mm512_mask_add_epi32(added_low, high_surrogate_mask_low, added_low, constant);
					const auto utf32_high = _mm512_mask_add_epi32(added_high, high_surrogate_mask_high, added_high, constant);

					const auto valid = ~low_surrogate_mask & current_in_mask;
					const auto valid_high = static_cast<__mmask16>(valid >> 16);
					const auto valid_low = static_cast<__mmask16>(valid);

					const auto out_low = _mm512_maskz_compress_epi32(valid_low, utf32_low);
					const auto out_high = _mm512_maskz_compress_epi32(valid_high, utf32_high);

					const auto low_length = std::popcount(valid_low);
					const auto high_length = std::popcount(valid_high);
					const auto low_mask = static_cast<__mmask16>(_pext_u32(valid_low, valid_low));
					const auto high_mask = static_cast<__mmask16>(_pext_u32(valid_high, valid_high));

					_mm512_mask_storeu_epi32(current_out + 0, low_mask, out_low);
					_mm512_mask_storeu_epi32(current_out + low_length, high_mask, out_high);

					return process_result
					{
							.processed_input = static_cast<std::uint8_t>(current_in_length),
							.num_output = static_cast<std::uint8_t>(low_length + high_length),
							.end_with_surrogate = end_with_surrogate,
							.pad = 0
					};
				}

				// no surrogates

				const auto valid = ~low_surrogate_mask & current_in_mask;
				const auto valid_high = static_cast<__mmask16>(valid >> 16);
				const auto valid_low = static_cast<__mmask16>(valid);

				const auto out_low = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(current_in));
				const auto out_high = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(current_in, 1));

				const auto low_length = std::popcount(valid_low);
				const auto high_length = std::popcount(valid_high);
				const auto low_mask = static_cast<__mmask16>(_pext_u32(valid_low, valid_low));
				const auto high_mask = static_cast<__mmask16>(_pext_u32(valid_high, valid_high));

				_mm512_mask_storeu_epi32(current_out + 0, low_mask, out_low);
				_mm512_mask_storeu_epi32(current_out + low_length, high_mask, out_high);

				return process_result
				{
						.processed_input = static_cast<std::uint8_t>(current_in_length),
						.num_output = static_cast<std::uint8_t>(low_length + high_length),
						.end_with_surrogate = false,
						.pad = 0
				};
			};

			bool end_with_surrogate = false;
			while (it_input_current + step <= it_input_end)
			{
				const auto in = [](const pointer_type c, [[maybe_unused]] const __m512i bf) noexcept
				{
					if constexpr (const auto v = _mm512_loadu_si512(c);
						SourceEndian != std::endian::native)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(it_input_current, byte_flip);
				const auto in_mask = _cvtu32_mask32(0x7fff'ffff);

				const auto result = process(in, in_mask, step_keep_high_surrogate, end_with_surrogate, it_output_current);
				if (result.processed_input != step_keep_high_surrogate)
				{
					// surrogate mismatch

					const auto valid_in_mask = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(result.processed_input)));
					const auto valid_in = _mm512_maskz_mov_epi16(valid_in_mask, in);

					const auto valid_result = process(valid_in, valid_in_mask, result.processed_input, end_with_surrogate, it_output_current);
					it_input_current += valid_result.processed_input;
					it_output_current += valid_result.num_output;

					if constexpr (
						ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
						ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
					)
					{
						return 0;
					}
					else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
					{
						return result_type{.error = ErrorCode::SURROGATE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
					}
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				}

				it_input_current += result.processed_input;
				it_output_current += result.num_output;
				end_with_surrogate = result.end_with_surrogate;
			}

			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				const auto in_mask = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(remaining)));
				const auto in = [](const pointer_type c, const __mmask32 m, [[maybe_unused]] const __m512i bf) noexcept
				{
					if constexpr (const auto v = _mm512_maskz_loadu_epi16(m, c);
						SourceEndian != std::endian::native)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(it_input_current, in_mask, byte_flip);

				const auto result = process(in, in_mask, remaining, end_with_surrogate, it_output_current);
				if (result.processed_input != remaining)
				{
					// surrogate mismatch

					const auto valid_in_mask = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(result.processed_input)));
					const auto valid_in = _mm512_maskz_mov_epi16(valid_in_mask, in);

					const auto valid_result = process(valid_in, valid_in_mask, result.processed_input, end_with_surrogate, it_output_current);
					it_input_current += valid_result.processed_input;
					it_output_current += valid_result.num_output;

					if constexpr (
						ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
						ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
					)
					{
						return 0;
					}
					else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
					{
						return result_type{.error = ErrorCode::SURROGATE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
					}
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				}

				it_input_current += result.processed_input;
				it_output_current += result.num_output;
				end_with_surrogate = result.end_with_surrogate;
			}
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("Unknown or unsupported `OutputType` (we don't know the `endian` by UTF16, so it's not allowed to use it here).");
		}

		if constexpr (
			ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
			ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
		)
		{
			return static_cast<std::size_t>(it_output_current - it_output_begin);
		}
		else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
		{
			return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}

	template<
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(
		const pointer_type input,
		typename output_type_of<OutputType>::pointer output
	) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
	{
		return Simd::convert<OutputType, SourceEndian, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, output);
	}

	template<
		typename StringType,
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
		requires requires(StringType& string)
		{
			string.resize(std::declval<size_type>());
			{
				string.data()
			} -> std::convertible_to<typename output_type_of<OutputType>::pointer>;
		}
	[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> StringType
	{
		StringType result{};
		result.resize(length<OutputType, SourceEndian>(input));

		std::ignore = Simd::convert<OutputType, SourceEndian, ProcessPolicy>(input, result.data());
		return result;
	}

	template<
		typename StringType,
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
		requires requires(StringType& string)
		{
			string.resize(std::declval<size_type>());
			{
				string.data()
			} -> std::convertible_to<typename output_type_of<OutputType>::pointer>;
		}
	[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> StringType
	{
		StringType result{};
		result.resize(length<OutputType, SourceEndian>(input));

		return Simd::convert<OutputType, SourceEndian, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
	}

	template<
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
	{
		std::basic_string<typename output_type_of<OutputType>::value_type> result{};
		result.resize(length<OutputType, SourceEndian>(input));

		std::ignore = Simd::convert<OutputType, SourceEndian, ProcessPolicy>(input, result.data());
		return result;
	}

	template<
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
	{
		std::basic_string<typename output_type_of<OutputType>::value_type> result{};
		result.resize(length<OutputType, SourceEndian>(input));

		return Simd::convert<OutputType, SourceEndian, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
	}

	/*constexpr*/
	static auto flip_endian(const input_type input, const output_type_of<chars_type>::pointer output) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		const auto it_output_begin = output;
		auto it_output_current = it_output_begin;

		// clang-format off
		const auto byte_flip = _mm512_setr_epi64(
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
			0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
		);
		// clang-format on

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
			} -> std::convertible_to<output_type_of<chars_type>::pointer>;
		}
	[[nodiscard]] /*constexpr*/ static auto flip_endian(const input_type input) noexcept -> StringType
	{
		StringType result{};
		result.resize(length<chars_type>(input));

		Simd::flip_endian(input, result.data());
		return result;
	}

	[[nodiscard]] /*constexpr*/ static auto flip_endian(const input_type input) noexcept -> std::basic_string<output_type_of<chars_type>::value_type>
	{
		std::basic_string<output_type_of<chars_type>::value_type> result{};
		result.resize(length<chars_type>(input));

		flip_endian(input, result.data());
		return result;
	}
};

#endif
