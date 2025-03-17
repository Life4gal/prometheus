// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

#include <chars/icelake.hpp>

#include <ranges>
#include <algorithm>

#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

#include <prometheus/macro.hpp>

#include <functional/functor.hpp>
#include <memory/rw.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <chars/scalar.hpp>

namespace
{
	using namespace gal::prometheus;
	using namespace chars;

	using data_type = __m512i;

	namespace common
	{
		template<CharsType InputType>
			requires (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
		[[nodiscard]] constexpr auto sign_of(const data_type data) noexcept -> auto
		{
			struct sign_type
			{
			private:
				__mmask64 mask_;

			public:
				constexpr explicit sign_type(const __mmask64 mask) noexcept
					: mask_{mask} {}

				/**
				 * @brief Get the underlying mask of the current block.
				 */
				[[nodiscard]] constexpr auto mask() const noexcept -> __mmask64
				{
					return mask_;
				}

				/**
				 * @brief Whether all sign bits are 0, in other words, whether the current block is all ASCII.
				 */
				[[nodiscard]] constexpr auto pure() const noexcept -> bool
				{
					return mask_ == 0;
				}

				/**
				 * @brief Get the number of non-ASCII in the current block.
				 */
				[[nodiscard]] constexpr auto count() const noexcept -> std::size_t
				{
					return std::popcount(mask_);
				}

				/**
				 * @brief Get the number of consecutive ASCII at the beginning.
				 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
				 * ^-----^ start_count
				 *                                                             ^----------^ end_count           
				 */
				[[nodiscard]] constexpr auto start_count() const noexcept -> std::size_t
				{
					return std::countr_zero(mask_);
				}

				/**
				 * @brief Get the number of consecutive ASCII at the ending.
				 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
				 * ^-----^ start_count
				 *                                                             ^----------^ end_count       
				 */
				[[nodiscard]] constexpr auto end_count() const noexcept -> std::size_t
				{
					return std::countl_zero(mask_);
				}
			};

			return sign_type{_mm512_movepi8_mask(data)};
		}

		template<CharsType Type>
			requires (
				Type == CharsType::UTF16_LE or
				Type == CharsType::UTF16_BE or
				Type == CharsType::UTF16
			)
		[[nodiscard]] constexpr auto not_native_endian() noexcept -> bool
		{
			return Type == CharsType::UTF16 or (Type == CharsType::UTF16_LE) != (std::endian::native == std::endian::little);
		}
	}
}

#include <chars/detail/icelake.utf8.hpp>
#include <chars/detail/icelake.utf32.hpp>

namespace
{
	namespace latin
	{
		using input_type = chars::latin::input_type;
		// using char_type = chars::latin::char_type;
		using size_type = chars::latin::size_type;
		using pointer_type = chars::latin::pointer_type;

		constexpr auto advance_of_latin = sizeof(data_type) / sizeof(input_type_of<CharsType::LATIN>::value_type);
		constexpr auto advance_of_utf8 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF8>::value_type);
		constexpr auto advance_of_utf16 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16>::value_type);
		constexpr auto advance_of_utf32 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF32>::value_type);

		static_assert(advance_of_utf8 == sizeof(data_type) / sizeof(input_type_of<CharsType::UTF8_CHAR>::value_type));
		static_assert(advance_of_utf16 == sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16_LE>::value_type));
		static_assert(advance_of_utf16 == sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16_BE>::value_type));

		template<CharsType InputType>
			requires (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE
			)
		[[nodiscard]] constexpr auto to_native_utf16(const data_type data) noexcept -> data_type
		{
			if constexpr (common::not_native_endian<InputType>())
			{
				const auto byte_flip = _mm512_setr_epi64(
					// clang-format off
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
					// clang-format on
				);

				return _mm512_shuffle_epi8(data, byte_flip);
			}
			else
			{
				return data;
			}
		}

		namespace icelake
		{
			[[nodiscard]] constexpr auto validate(const input_type input) noexcept -> result_error_input_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				while (it_input_current + advance_of_latin <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_latin};
					#endif

					const auto data = _mm512_loadu_si512(it_input_current);

					if (const auto sign = common::sign_of<CharsType::LATIN>(data);
						not sign.pure())
					{
						it_input_current += sign.start_count();

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

						return {.error = ErrorCode::TOO_LARGE, .input = current_input_length};
					}

					it_input_current += advance_of_latin;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_latin);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(remaining));
					const auto data = _mm512_maskz_loadu_epi8(mask, it_input_current);

					if (const auto sign = common::sign_of<CharsType::LATIN>(data);
						not sign.pure())
					{
						it_input_current += sign.start_count();

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

						return {.error = ErrorCode::TOO_LARGE, .input = current_input_length};
					}

					it_input_current += remaining;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				return {.error = ErrorCode::NONE, .input = current_input_length};
			}

			template<CharsType OutputType>
			[[nodiscard]] constexpr auto length(const input_type input) noexcept -> size_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				// ReSharper disable CppClangTidyBugproneBranchClone
				if constexpr (OutputType == CharsType::LATIN)
				{
					return input.size();
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					const auto input_length = input.size();

					const pointer_type it_input_begin = input.data();
					pointer_type it_input_current = it_input_begin;
					const pointer_type it_input_end = it_input_begin + input_length;

					// number of 512-bit chunks that fits into the length
					size_type output_length = input_length / advance_of_utf8 * advance_of_utf8;

					while (it_input_current + advance_of_utf8 <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf8};
						#endif

						const auto data = _mm512_loadu_si512(it_input_current);

						if (const auto sign = common::sign_of<CharsType::LATIN>(data);
							not sign.pure())
						{
							output_length += sign.count();
						}

						it_input_current += advance_of_utf8;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf8);

					if (remaining != 0)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
						#endif

						// fallback
						output_length += Scalar::length<CharsType::LATIN, OutputType>({it_input_current, static_cast<std::size_t>(remaining)});
					}

					return output_length;
				}
				// ReSharper disable CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					return input.size();
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return input.size();
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::UTF8_CHAR or
					OutputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto write_utf8(
				const typename output_type_of<OutputType>::pointer output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				std::ignore = Correct;

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				const auto transform = [&]<bool MaskOut>(const data_type data, const std::size_t data_length) noexcept -> void
				{
					if constexpr (not MaskOut)
					{
						GAL_PROMETHEUS_ERROR_ASSUME(data_length == static_cast<std::size_t>(advance_of_utf8));
					}

					const auto sign = common::sign_of<CharsType::LATIN>(data);

					const auto non_ascii = sign.mask();
					const auto non_ascii_high = static_cast<std::uint32_t>(non_ascii >> 32);
					const auto non_ascii_low = static_cast<std::uint32_t>(non_ascii);

					const auto ascii = ~non_ascii;
					const auto ascii_high = static_cast<std::uint64_t>(static_cast<std::uint32_t>(ascii >> 32));
					const auto ascii_low = static_cast<std::uint64_t>(static_cast<std::uint32_t>(ascii));

					// Here we invert it (~) to generate a final mask used to compress only the needed bytes,
					// the bits in ascii are inverted and zeros are interspersed in between them.
					constexpr auto alternate_bits = 0x5555'5555'5555'5555ull;
					const auto mask_high = ~_pdep_u64(ascii_high, alternate_bits);
					const auto mask_low = ~_pdep_u64(ascii_low, alternate_bits);

					// Interleave bytes from top and bottom halves.
					// We permute the 64-byte 'data' so that we can later apply
					// different transformations on the lower 32 bytes and the upper 32 bytes.
					//
					//  _mm512_permutexvar_epi8 lets us reorder the bytes in 'data' based on
					//  a specified permutation vector.
					//
					//  In this particular permutation, the goal is to transform an original layout
					//  (e.g., [a0, a1, ..., a31, b0, b1, ..., b31]) into an interleaved layout
					//  ([a0, b0, a1, b1, ..., a31, b31]) or a similar pattern that suits
					//  the subsequent processing steps.
					const auto source_interleaved = _mm512_permutexvar_epi8(
						_mm512_set_epi32(
							// clang-format off
							0x3f1f'3e1e, 0x3d1d'3c1c, 0x3b1b'3a1a, 0x3919'3818,
							0x3717'3616, 0x3515'3414, 0x3313'3212, 0x3111'3010,
							0x2f0f'2e0e, 0x2d0d'2c0c, 0x2b0b'2a0a, 0x2909'2808,
							0x2707'2606, 0x2505'2404, 0x2303'2202, 0x2101'2000
							// clang-format on
						),
						data
					);

					// Mask to denote whether the byte is a leading byte that is not ascii
					// binary representation of 192: 1100'0000
					const auto sixth = _mm512_cmpge_epu8_mask(data, _mm512_set1_epi8(static_cast<char>(192)));
					const auto sixth_high = static_cast<__mmask32>(sixth >> 32);
					const auto sixth_low = static_cast<__mmask32>(sixth);

					const auto output_low = [](const data_type interleaved, const __mmask32 s, const __mmask64 mask) noexcept -> auto
					{
						// Upscale the bytes to 16-bit value, adding the 0b1100'0010 leading byte in the process.
						// We adjust for the bytes that have their two most significant bits.
						// This takes care of the first 32 bytes, assuming we interleaved the bytes.
						// binary representation of 194: 1100'0010
						auto v = _mm512_shldi_epi16(interleaved, _mm512_set1_epi8(static_cast<char>(194)), 8);
						v = _mm512_mask_add_epi16(
							v,
							s,
							v,
							// 1- 0x4000 = 1100 0000 0000 0001
							_mm512_set1_epi16(1 - 0x4000)
						);
						// prune redundant bytes
						return _mm512_maskz_compress_epi8(mask, v);
					}(source_interleaved, sixth_low, mask_low);

					const auto output_high = [](const data_type interleaved, const __mmask32 s, const __mmask64 mask) noexcept -> auto
					{
						// in the second 32-bit half, set first or second option based on whether original input is leading byte (second case) or not (first case).
						const auto leading = _mm512_mask_blend_epi16(
							s,
							// 0000 0000 1101 0010
							_mm512_set1_epi16(0x00c2),
							// 0100 0000 1100 0011
							_mm512_set1_epi16(0x40c3)
						);
						const auto v = _mm512_ternarylogic_epi32(
							interleaved,
							leading,
							_mm512_set1_epi16(static_cast<short>(0xff00)),
							// (interleaved & 0xff00) ^ leading
							(240 & 170) ^ 204
						);
						// prune redundant bytes
						return _mm512_maskz_compress_epi8(mask, v);
					}(source_interleaved, sixth_high, mask_high);

					const auto length_total = static_cast<unsigned int>(data_length + std::popcount(non_ascii));

					if constexpr (MaskOut)
					{
						// is the second half of the input vector used?
						if (data_length <= 32)
						{
							const auto mask = _bzhi_u64(~static_cast<unsigned long long>(0), length_total);
							_mm512_mask_storeu_epi8(it_output_current + 0, mask, output_low);

							it_input_current += data_length;
							it_output_current += length_total;

							return;
						}
					}

					const auto low_length = 32 + static_cast<unsigned int>(std::popcount(non_ascii_low));
					// const auto high_length = static_cast<unsigned int>(data_length - 32) + static_cast<unsigned int>(std::popcount(non_ascii_high));
					std::ignore = non_ascii_high;
					const auto high_length = length_total - low_length;

					const auto low_mask = _bzhi_u64(~static_cast<unsigned long long>(0), low_length);
					const auto high_mask = _bzhi_u64(~static_cast<unsigned long long>(0), high_length);

					if constexpr (MaskOut)
					{
						_mm512_mask_storeu_epi8(it_output_current + 0, low_mask, output_low);
					}
					else
					{
						_mm512_storeu_si512(it_output_current + 0, output_low);
					}
					_mm512_mask_storeu_epi8(it_output_current + low_length, high_mask, output_high);

					it_input_current += data_length;
					it_output_current += length_total;
				};

				// if there's at least 128 bytes remaining, we don't need to mask the output
				while (it_input_current + 2 * advance_of_utf8 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf8};
					#endif

					const auto data = _mm512_loadu_si512(it_input_current);

					if constexpr (Pure)
					{
						_mm512_storeu_si512(it_output_current, data);

						it_input_current += advance_of_utf8;
						it_output_current += advance_of_utf8;
					}
					else
					{
						if (const auto sign = common::sign_of<CharsType::LATIN>(data);
							sign.pure())
						{
							_mm512_storeu_si512(it_output_current, data);

							it_input_current += advance_of_utf8;
							it_output_current += advance_of_utf8;
						}
						else
						{
							transform.template operator()<false>(data, advance_of_utf8);
						}
					}
				}

				// in the last 128 bytes, the first 64 may require masking the output
				if (it_input_current + advance_of_utf8 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf8};
					#endif

					const auto data = _mm512_loadu_si512(it_input_current);

					if constexpr (Pure)
					{
						_mm512_storeu_si512(it_output_current, data);

						it_input_current += advance_of_utf8;
						it_output_current += advance_of_utf8;
					}
					else
					{
						transform.template operator()<true>(data, advance_of_utf8);
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf8);

				// with the last 64 bytes, the input also needs to be masked
				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(remaining));
					const auto data = _mm512_maskz_loadu_epi8(mask, it_input_current);

					if constexpr (Pure)
					{
						const auto out_mask = _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(remaining));
						_mm512_mask_storeu_epi8(it_output_current, out_mask, data);

						it_input_current += remaining;
						it_output_current += remaining;
					}
					else
					{
						transform.template operator()<true>(data, remaining);
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::UTF16_LE or
					OutputType == CharsType::UTF16_BE
				)
			[[nodiscard]] constexpr auto write_utf16(
				const typename output_type_of<OutputType>::pointer output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				std::ignore = Pure;
				std::ignore = Correct;

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				while (it_input_current + advance_of_utf16 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf16};
					#endif

					// Load 32 Latin1 characters into a 256-bit register
					const auto m256 = _mm256_loadu_si256(
						GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(
							const __m256i *,
							it_input_current
						)
					);
					// Zero extend each set of 8-bit characters to 32 16-bit integers
					const auto data = _mm512_cvtepu8_epi16(m256);

					const auto native_data = latin::to_native_utf16<OutputType>(data);
					_mm512_storeu_si512(it_output_current, native_data);

					it_input_current += advance_of_utf16;
					it_output_current += advance_of_utf16;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf16);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(remaining));
					const auto m256 = _mm256_maskz_loadu_epi8(mask, it_input_current);
					// Zero extend each set of 8-bit characters to 32 16-bit integers
					const auto data = _mm512_cvtepu8_epi16(m256);

					const auto native_data = latin::to_native_utf16<OutputType>(data);
					_mm512_mask_storeu_epi16(it_output_current, mask, native_data);

					it_input_current += remaining;
					it_output_current += remaining;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::UTF32
				)
			[[nodiscard]] constexpr auto write_utf32(
				const typename output_type_of<OutputType>::pointer output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				std::ignore = Pure;
				std::ignore = Correct;

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				while (it_input_current + advance_of_utf32 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf32};
					#endif

					// Load 16 Latin1 characters into a 128-bit register
					const auto m128 = _mm_loadu_si128(
						GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(
							const __m128i *,
							it_input_current
						)
					);
					// Zero extend each set of 8-bit characters to 16 32-bit integers
					const auto data = _mm512_cvtepu8_epi32(m128);

					_mm512_storeu_si512(it_output_current, data);

					it_input_current += advance_of_utf32;
					it_output_current += advance_of_utf32;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf32);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(remaining)));
					const auto m128 = _mm_maskz_loadu_epi8(mask, it_input_current);
					// Zero extend each set of 8-bit characters to 16 32-bit integers
					const auto data = _mm512_cvtepu8_epi32(m128);

					_mm512_mask_storeu_epi32(it_output_current, mask, data);

					it_input_current += remaining;
					it_output_current += remaining;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}
		}
	}

	namespace utf8
	{
		constexpr auto advance_of_latin = sizeof(data_type) / sizeof(input_type_of<CharsType::LATIN>::value_type);
		constexpr auto advance_of_utf8 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF8>::value_type);
		constexpr auto advance_of_utf16 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16>::value_type);
		constexpr auto advance_of_utf32 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF32>::value_type);

		static_assert(advance_of_utf8 == sizeof(data_type) / sizeof(input_type_of<CharsType::UTF8_CHAR>::value_type));
		static_assert(advance_of_utf16 == sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16_LE>::value_type));
		static_assert(advance_of_utf16 == sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16_BE>::value_type));

		template<CharsType InputType>
			requires (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE
			)
		[[nodiscard]] constexpr auto to_native_utf16(const data_type data) noexcept -> data_type
		{
			if constexpr (common::not_native_endian<InputType>())
			{
				const auto byte_flip = _mm512_setr_epi64(
					// clang-format off
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
					// clang-format on
				);

				return _mm512_shuffle_epi8(data, byte_flip);
			}
			else
			{
				return data;
			}
		}

		namespace icelake
		{
			template<CharsType InputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto validate(const input_type_of<InputType> input) noexcept -> result_error_input_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				using input_type = input_type_of<InputType>;
				using pointer_type = typename input_type::const_pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				detail::utf8::icelake::avx512_utf8_checker checker{};

				const auto do_fallback = [&]() noexcept -> result_error_input_type
				{
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

					if (it_input_current == it_input_begin)
					[[unlikely]]
					{
						const auto result = Scalar::validate<InputType>({it_input_current, it_input_end});
						return {.error = result.error, .input = current_input_length + result.input};
					}

					const auto result = [=, current = it_input_current - 1]() noexcept
					{
						if constexpr (InputType == CharsType::UTF8_CHAR)
						{
							return utf8_char::scalar::rewind_and_validate(it_input_begin, current, it_input_end);
						}
						else
						{
							return chars::utf8::scalar::rewind_and_validate(it_input_begin, current, it_input_end);
						}
					}();
					return {.error = result.error, .input = result.input + current_input_length};
				};

				while (it_input_current + advance_of_utf8 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf8};
					#endif

					if (const auto data = _mm512_loadu_si512(it_input_current);
						checker.check_data(data))
					{
						it_input_current += advance_of_utf8;
						continue;
					}

					if (checker.has_error())
					{
						return do_fallback();
					}

					it_input_current += advance_of_utf8;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf8);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(remaining));
					if (const auto data = _mm512_maskz_loadu_epi8(mask, it_input_current);
						not checker.check_data(data) and checker.has_error())
					{
						return do_fallback();
					}

					it_input_current += remaining;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				return {.error = ErrorCode::NONE, .input = current_input_length};
			}

			template<CharsType InputType, CharsType OutputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto length(const input_type_of<InputType> input) noexcept -> typename input_type_of<InputType>::size_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				using input_type = input_type_of<InputType>;
				using pointer_type = typename input_type::const_pointer;
				using size_type = typename input_type::size_type;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				constexpr auto advance = advance_of_latin;
				static_assert(advance == sizeof(__m512i));

				if constexpr (OutputType == CharsType::LATIN)
				{
					const auto continuation = _mm512_set1_epi8(static_cast<char>(0b1011'1111));

					auto unrolled_length = _mm512_setzero_si512();
					// number of 512-bit chunks that fits into the length
					auto result_length = static_cast<size_type>(input_length / advance * advance);

					while (it_input_current + advance <= it_input_end)
					{
						const auto iterations = static_cast<size_type>((it_input_end - it_input_current) / advance);
						const auto this_turn_end = it_input_current + iterations * advance - advance;

						while (it_input_current + 8 * advance <= this_turn_end)
						{
							const auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current);

							const auto data_0 = _mm512_loadu_si512(p + 0);
							const auto data_1 = _mm512_loadu_si512(p + 1);
							const auto data_2 = _mm512_loadu_si512(p + 2);
							const auto data_3 = _mm512_loadu_si512(p + 3);
							const auto data_4 = _mm512_loadu_si512(p + 4);
							const auto data_5 = _mm512_loadu_si512(p + 5);
							const auto data_6 = _mm512_loadu_si512(p + 6);
							const auto data_7 = _mm512_loadu_si512(p + 7);

							const auto mask_0 = _mm512_cmple_epi8_mask(data_0, continuation);
							const auto mask_1 = _mm512_cmple_epi8_mask(data_1, continuation);
							const auto mask_2 = _mm512_cmple_epi8_mask(data_2, continuation);
							const auto mask_3 = _mm512_cmple_epi8_mask(data_3, continuation);
							const auto mask_4 = _mm512_cmple_epi8_mask(data_4, continuation);
							const auto mask_5 = _mm512_cmple_epi8_mask(data_5, continuation);
							const auto mask_6 = _mm512_cmple_epi8_mask(data_6, continuation);
							const auto mask_7 = _mm512_cmple_epi8_mask(data_7, continuation);

							const auto mask_register = _mm512_set_epi64(mask_7, mask_6, mask_5, mask_4, mask_3, mask_2, mask_1, mask_0);

							unrolled_length = _mm512_add_epi64(unrolled_length, _mm512_popcnt_epi64(mask_register));

							it_input_current += 8 * advance;
						}

						while (it_input_current <= this_turn_end)
						{
							const auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m512i *, it_input_current);

							const auto data = _mm512_loadu_si512(p);

							const auto continuation_bitmask = _mm512_cmple_epi8_mask(data, continuation);
							result_length -= std::popcount(continuation_bitmask);

							it_input_current += advance;
						}
					}
					result_length -= _mm512_reduce_add_epi64(unrolled_length);

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance);

					if (remaining != 0)
					{
						result_length += Scalar::length<InputType, OutputType>({it_input_current, static_cast<std::size_t>(remaining)});
					}

					return result_length;
				}
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					return input.size();
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					auto result_length = static_cast<size_type>(0);
					while (it_input_current + advance <= it_input_end)
					{
						const auto data = _mm512_loadu_si512(it_input_current);

						const auto utf8_continuation_mask = _mm512_cmple_epi8_mask(data, _mm512_set1_epi8(-64));
						// We count one word for anything that is not a continuation (so leading bytes)
						result_length += advance - std::popcount(utf8_continuation_mask);

						const auto utf8_4_byte = _mm512_cmpge_epu8_mask(data, _mm512_set1_epi8(static_cast<char>(240)));
						result_length += std::popcount(utf8_4_byte);

						it_input_current += advance;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance);

					if (remaining != 0)
					{
						// fallback
						result_length += Scalar::length<InputType, OutputType>({it_input_current, static_cast<std::size_t>(remaining)});
					}

					return result_length;
				}
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return icelake::length<InputType, CharsType::LATIN>(input);
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::LATIN
				         )
			[[nodiscard]] constexpr auto write_latin(
				const typename output_type_of<OutputType>::pointer output,
				const input_type_of<InputType> input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using input_type = input_type_of<InputType>;
				using pointer_type = typename input_type::const_pointer;
				using size_type = typename input_type::size_type;

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				const auto do_process = [&]<bool MaskOut>(
					const pointer_type current,
					const size_type length,
					__mmask64& out_next_leading,
					__mmask64& out_next_bit6
				) noexcept -> __mmask64
				{
					if constexpr (not MaskOut)
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == advance_of_latin);
					}
					if constexpr (Pure)
					{
						std::ignore = out_next_leading;
						std::ignore = out_next_bit6;
					}

					// 1100 0000
					const auto v_minus64 = _mm512_set1_epi8(-64);
					// 1100 0010
					const auto v_minus62 = _mm512_set1_epi8(-62);
					const auto v_0001 = _mm512_set1_epi8(1);

					const auto mask = _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(length));
					const auto data = [=]() noexcept -> data_type
					{
						if constexpr (MaskOut)
						{
							return _mm512_maskz_loadu_epi8(mask, current);
						}
						else
						{
							return _mm512_loadu_si512(current);
						}

						// warning: no return statement in function returning non-void [-Wreturn-type]
						#if defined(GAL_PROMETHEUS_COMPILER_GNU)
						GAL_PROMETHEUS_ERROR_UNREACHABLE();
						#endif
					}();

					const auto write_pure = [&]() noexcept -> void
					{
						if constexpr (MaskOut)
						{
							_mm512_mask_storeu_epi8(it_output_current, mask, data);
						}
						else
						{
							_mm512_storeu_si512(it_output_current, data);
						}

						it_output_current += length;
					};

					if constexpr (Pure)
					{
						write_pure();
						return 0;
					}
					else
					{
						const auto non_ascii = _mm512_movepi8_mask(data);
						if (non_ascii == 0)
						{
							write_pure();
							return 0;
						}

						const auto leading = _mm512_cmpge_epu8_mask(data, v_minus64);
						const auto high_bits = _mm512_xor_si512(data, v_minus62);

						if constexpr (not Correct)
						{
							if (const auto invalid_leading_bytes = _mm512_mask_cmpgt_epu8_mask(leading, high_bits, v_0001);
								invalid_leading_bytes)
							{
								return invalid_leading_bytes;
							}

							if (const auto leading_shift = (leading << 1) | out_next_leading;
								(non_ascii ^ leading) != leading_shift)
							{
								return leading_shift;
							}
						}

						const auto bit6 = _mm512_cmpeq_epi8_mask(high_bits, v_0001);
						const auto sub = _mm512_mask_sub_epi8(data, (bit6 << 1) | out_next_bit6, data, v_minus64);
						const auto retain = ~leading & mask;
						const auto num_out = std::popcount(retain);

						const auto out_mask = _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(num_out));
						const auto out = _mm512_maskz_compress_epi8(retain, sub);

						_mm512_mask_storeu_epi8(it_output_current, out_mask, out);

						it_output_current += num_out;
						out_next_leading = leading >> 63;
						out_next_bit6 = bit6 >> 63;

						return 0;
					}
				};
				const auto do_fallback = [&](const __mmask64 mask) noexcept -> result_error_input_output_type
				{
					const auto extra_valid = static_cast<size_type>(std::countr_zero(mask));
					const auto result_valid = Scalar::convert<InputType, OutputType, false, true>(it_output_current, {it_input_current, extra_valid});

					it_input_current += extra_valid;
					it_output_current += result_valid.output;

					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

					// fixme
					const auto result = Scalar::convert<InputType, OutputType, false, false>(it_output_current, {it_input_current, it_input_end});
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(result.has_error());
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(result.input == 0);
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(result.output == 0);

					return {.error = result.error, .input = current_input_length + result.input, .output = current_output_length + result.output};
				};

				__mmask64 next_leading = 0;
				__mmask64 next_bit6 = 0;
				while (it_input_current + advance_of_latin <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_latin};
					#endif

					const auto mask = do_process.template operator()<false>(it_input_current, advance_of_latin, next_leading, next_bit6);

					if constexpr (Pure or Correct)
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(mask == 0);
					}
					else
					{
						if (mask != 0)
						{
							return do_fallback(mask);
						}
					}

					it_input_current += advance_of_latin;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_latin);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = do_process.template operator()<true>(it_input_current, static_cast<size_type>(remaining), next_leading, next_bit6);

					if constexpr (Pure or Correct)
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(mask == 0);
					}
					else
					{
						if (mask != 0)
						{
							return do_fallback(mask);
						}
					}

					it_input_current += remaining;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::UTF16_LE or
					         OutputType == CharsType::UTF16_BE
				         )
			[[nodiscard]] constexpr auto write_utf16(
				const typename output_type_of<OutputType>::pointer output,
				const input_type_of<InputType> input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using input_type = input_type_of<InputType>;
				using pointer_type = typename input_type::const_pointer;
				using size_type = typename input_type::size_type;

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				constexpr auto advance = 2 * advance_of_utf16;

				const auto do_process = [&]<bool MaskOut>() noexcept
				{
					const auto mask_identity = _mm512_set_epi8(
						// clang-format off
						63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48,
						47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
						31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
						15, 14, 13, 12, 11, 10, 9u, 8u, 07, 06, 05, 04, 03, 02, 01, 00
						// clang-format on
					);

					// ReSharper disable CppInconsistentNaming
					// ReSharper disable IdentifierTypo

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

					// ReSharper restore IdentifierTypo
					// ReSharper restore CppInconsistentNaming

					const auto length = [=]() noexcept
					{
						if constexpr (MaskOut)
						{
							return static_cast<size_type>(it_input_end - it_input_current);
						}
						else
						{
							return advance;
						}
					}();

					const auto mask = _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(length));
					const auto data = [=]() noexcept -> data_type
					{
						if constexpr (MaskOut)
						{
							return _mm512_maskz_loadu_epi8(mask, it_input_current);
						}
						else
						{
							return _mm512_loadu_si512(it_input_current);
						}

						// warning: no return statement in function returning non-void [-Wreturn-type]
						#if defined(GAL_PROMETHEUS_COMPILER_GNU)
						GAL_PROMETHEUS_ERROR_UNREACHABLE();
						#endif
					}();

					const auto write_pure = [&]() noexcept -> void
					{
						static_assert(advance == 64);

						const auto data_1 = [=]() noexcept -> auto
						{
							const auto out = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(data));
							const auto native_out = utf8::to_native_utf16<OutputType>(out);

							return native_out;
						}();

						if constexpr (MaskOut)
						{
							// is the second half of the input vector used?
							if (length <= 32)
							{
								const auto out_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length));

								_mm512_mask_storeu_epi16(it_output_current, out_mask, data_1);

								it_input_current += length;
								it_output_current += length;

								return;
							}
						}

						const auto data_2 = [=]() noexcept -> auto
						{
							const auto out = _mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(data, 1));
							const auto native_out = utf8::to_native_utf16<OutputType>(out);

							return native_out;
						}();

						_mm512_storeu_si512(it_output_current + 0, data_1);
						if constexpr (MaskOut)
						{
							const auto out_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length - 32));
							_mm512_mask_storeu_epi16(it_output_current + 32, out_mask, data_2);
						}
						else
						{
							_mm512_storeu_si512(it_output_current + 32, data_2);
						}

						it_input_current += length;
						it_output_current += length;
					};

					if constexpr (Pure)
					{
						write_pure();

						return true;
					}
					else
					{
						const auto mask_byte_1 = _mm512_mask_cmplt_epu8_mask(mask, data, v_8080_8080);
						// NOT(mask_byte_1) AND valid_input_length -- if all zeroes, then all ASCII
						if (_ktestc_mask64_u8(mask_byte_1, mask))
						{
							write_pure();

							return true;
						}

						// classify characters further

						// 0xc0 <= in, 2, 3, or 4 leading byte
						const auto mask_byte_234 = _mm512_cmple_epu8_mask(v_c0c0_c0c0, data);
						// 0xdf < in,  3 or 4 leading byte
						const auto mask_byte_34 = _mm512_cmplt_epu8_mask(v_dfdf_dfdf, data);

						if constexpr (not Correct)
						{
							// 0xc0 <= data < 0xc2 (illegal two byte sequence)
							if (const auto two_bytes = _mm512_mask_cmplt_epu8_mask(mask_byte_234, data, v_c2c2_c2c2);
								_ktestz_mask64_u8(two_bytes, two_bytes) == 0)
							{
								// Overlong 2-byte sequence
								return false;
							}
						}

						if (_ktestz_mask64_u8(mask_byte_34, mask_byte_34) == 0)
						{
							// We have a 3-byte sequence and/or a 2-byte sequence, or possibly even a 4-byte sequence

							// 0xf0 <= zmm0 (4 byte start bytes)
							const auto mask_byte_4 = _mm512_cmpge_epu8_mask(data, v_f0f0_f0f0);
							const auto mask_not_ascii = [=]() noexcept -> __mmask64
							{
								if constexpr (const auto out = _knot_mask64(mask_byte_1);
									MaskOut)
								{
									return _kand_mask64(out, mask);
								}
								else
								{
									return out;
								}

								// warning: no return statement in function returning non-void [-Wreturn-type]
								#if defined(GAL_PROMETHEUS_COMPILER_GNU)
								GAL_PROMETHEUS_ERROR_UNREACHABLE();
								#endif
							}();

							const auto mask_pattern_1 = _kshiftli_mask64(mask_byte_234, 1);
							const auto mask_patten_2 = _kshiftli_mask64(mask_byte_34, 2);
							if (mask_byte_4 == 0)
							{
								// expected continuation bytes
								const auto mask_combing = _kor_mask64(mask_pattern_1, mask_patten_2);
								const auto mask_byte_1234 = _kor_mask64(mask_byte_1, mask_byte_234);

								if constexpr (not Correct)
								{
									// mismatched continuation bytes
									if constexpr (MaskOut)
									{
										if (mask_combing != _kxor_mask64(mask, mask_byte_1234))
										{
											return false;
										}
									}
									else
									{
										// XNOR of mask_combing and mask_byte_1234 should be all zero if they differ the presence of a 1 bit indicates that they overlap.
										if (const auto v = _kxnor_mask64(mask_combing, mask_byte_1234);
											not _kortestz_mask64_u8(v, v))
										{
											return false;
										}
									}
								}

								// identifying the last bytes of each sequence to be decoded
								const auto mend = [=]() noexcept -> auto
								{
									if constexpr (const auto out = _kshiftri_mask64(mask_byte_1234, 1);
										MaskOut)
									{
										return _kor_mask64(out, __mmask64{1} << (length - 1));
									}
									else
									{
										return out;
									}
								}();

								const auto last_and_third = _mm512_maskz_compress_epi8(mend, mask_identity);
								const auto last_and_third_u16 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(last_and_third));
								// ASCII: 00000000  other: 11000000
								const auto non_ascii_tags = _mm512_maskz_mov_epi8(mask_not_ascii, v_c0c0_c0c0);
								// high two bits cleared where not ASCII
								const auto cleared_bytes = _mm512_andnot_si512(non_ascii_tags, data);
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
									6
								);

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
									12
								);

								// the elements of out excluding the last element if it happens to be a high surrogate
								const auto out = _mm512_ternarylogic_epi32(last_bytes, second_last_bytes, third_last_bytes, 254);
								const auto native_out = utf8::to_native_utf16<OutputType>(out);

								if constexpr (not Correct)
								{
									// Encodings out of range

									// the location of 3-byte sequence start bytes in the input code units in out corresponding to 3-byte sequences.
									const auto m3 = static_cast<__mmask32>(_pext_u64((mask_byte_34 & (mask ^ mask_byte_4)) << 2, mend));
									const auto mask_out_less_than_0x800 = _mm512_mask_cmplt_epu16_mask(m3, out, v_0800_0800);
									const auto mask_out_minus_0x800 = _mm512_sub_epi16(out, v_d800_d800);
									const auto mask_out_too_small = _mm512_mask_cmplt_epu16_mask(m3, mask_out_minus_0x800, v_0800_0800);
									if (_kor_mask32(mask_out_less_than_0x800, mask_out_too_small) != 0)
									{
										return false;
									}
								}

								// we adjust mend at the end of the output.
								const auto mask_processed = [=]() noexcept -> __mmask64
								{
									if constexpr (MaskOut)
									{
										return _pdep_u64(0xffff'ffff, _kand_mask64(mend, mask));
									}
									else
									{
										return _pdep_u64(0xffff'ffff, mend);
									}

									// warning: no return statement in function returning non-void [-Wreturn-type]
									#if defined(GAL_PROMETHEUS_COMPILER_GNU)
									GAL_PROMETHEUS_ERROR_UNREACHABLE();
									#endif
								}();

								const auto num_out = std::popcount(mask_processed);
								const auto out_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(num_out));

								_mm512_mask_storeu_epi16(it_output_current, out_mask, native_out);

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
							const auto mend = [=]() noexcept -> __mmask64
							{
								if constexpr (const auto out = _kor_mask64(_kshiftri_mask64(_kor_mask64(mask_pattern_3, mask_byte_1234), 1), mask_pattern_3);
									MaskOut)
								{
									return _kor_mask64(out, __mmask64{1} << (length - 1));
								}
								else
								{
									return out;
								}

								// warning: no return statement in function returning non-void [-Wreturn-type]
								#if defined(GAL_PROMETHEUS_COMPILER_GNU)
								GAL_PROMETHEUS_ERROR_UNREACHABLE();
								#endif
							}();

							const auto last_and_third = _mm512_maskz_compress_epi8(mend, mask_identity);
							const auto last_and_third_u16 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(last_and_third));
							// ASCII: 00000000  other: 11000000
							const auto non_ascii_tags = _mm512_maskz_mov_epi8(mask_not_ascii, v_c0c0_c0c0);
							// high two bits cleared where not ASCII
							const auto cleared_bytes = _mm512_andnot_si512(non_ascii_tags, data);
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
								6
							);

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
								12
							);

							const auto third_second_and_last_bytes = _mm512_ternarylogic_epi32(last_bytes, second_last_bytes, third_last_bytes, 254);
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
							const auto native_out = utf8::to_native_utf16<OutputType>(out);

							if constexpr (not Correct)
							{
								// mismatched continuation bytes
								if constexpr (MaskOut)
								{
									if (mask_combing != _kxor_mask64(mask, mask_byte_1234))
									{
										return false;
									}
								}
								else
								{
									// XNOR of mask_combing and mask_byte_1234 should be all zero if they differ the presence of a 1 bit indicates that they overlap.
									if (const auto v = _kxnor_mask64(mask_combing, mask_byte_1234);
										not _kortestz_mask64_u8(v, v))
									{
										return false;
									}
								}

								// Encodings out of range
								// the location of 3-byte sequence start bytes in the input code units in out corresponding to 3-byte sequences.
								const auto m3 = static_cast<__mmask32>(_pext_u64(mask_byte_34 & (mask ^ mask_byte_4) << 2, mend));
								const auto mask_out_less_than_0x800 = _mm512_mask_cmplt_epu16_mask(m3, out, v_0800_0800);
								const auto mask_out_minus_0x800 = _mm512_sub_epi16(out, v_d800_d800);
								const auto mask_out_too_small = _mm512_mask_cmplt_epu16_mask(m3, mask_out_minus_0x800, v_0800_0800);
								const auto mask_out_greater_equal_0x400 = _mm512_mask_cmpge_epu16_mask(mask_mp3_high, mask_out_minus_0x800, v_0400_0400);
								if (_kortestz_mask32_u8(mask_out_greater_equal_0x400, _kor_mask32(mask_out_less_than_0x800, mask_out_too_small)) != 0)
								{
									return false;
								}
							}

							// we adjust mend at the end of the output.
							const auto mask_processed = [=, m = ~(mask_mp3_high & 0x8000'0000)]() noexcept -> __mmask64
							{
								if constexpr (MaskOut)
								{
									return _pdep_u64(m, _kand_mask64(mend, mask));
								}
								else
								{
									return _pdep_u64(m, mend);
								}

								// warning: no return statement in function returning non-void [-Wreturn-type]
								#if defined(GAL_PROMETHEUS_COMPILER_GNU)
								GAL_PROMETHEUS_ERROR_UNREACHABLE();
								#endif
							}();

							const auto num_out = std::popcount(mask_processed);
							const auto out_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(num_out));

							_mm512_mask_storeu_epi16(it_output_current, out_mask, native_out);

							it_input_current += 64 - std::countl_zero(mask_processed);
							it_output_current += num_out;

							return true;
						}

						// all ASCII or 2 byte
						const auto continuation_or_ascii = [=]() noexcept -> __mmask64
						{
							if constexpr (MaskOut)
							{
								return _kand_mask64(_knot_mask64(mask_byte_234), mask);
							}
							else
							{
								return _knot_mask64(mask_byte_234);
							}

							// warning: no return statement in function returning non-void [-Wreturn-type]
							#if defined(GAL_PROMETHEUS_COMPILER_GNU)
							GAL_PROMETHEUS_ERROR_UNREACHABLE();
							#endif
						}();

						// on top of -0xc0 we subtract -2 which we get back later of the continuation byte tags
						const auto leading_two_bytes = _mm512_maskz_sub_epi8(mask_byte_234, data, v_c2c2_c2c2);
						const auto leading_mask = [=]() noexcept -> auto
						{
							if constexpr (MaskOut)
							{
								return _kand_mask64(_kor_mask64(mask_byte_1, mask_byte_234), mask);
							}
							else
							{
								return _kor_mask64(mask_byte_1, mask_byte_234);
							}
						}();

						if constexpr (not Correct)
						{
							if constexpr (MaskOut)
							{
								if (_kshiftli_mask64(mask_byte_234, 1) != _kxor_mask64(mask, leading_mask))
								{
									return false;
								}
							}
							else
							{
								if (const auto v = _kxnor_mask64(_kshiftli_mask64(mask_byte_234, 1), leading_mask);
									not _kortestz_mask64_u8(v, v))
								{
									return false;
								}
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
							// contain zero for ascii, and the data
							auto lead = _mm512_maskz_compress_epi8(leading_mask, leading_two_bytes);
							// zero extended into code units
							lead = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(lead));
							// shifted into position
							lead = _mm512_slli_epi16(lead, 6);

							// the last bytes of each sequence
							auto follow = _mm512_maskz_compress_epi8(continuation_or_ascii, data);
							// zero extended into code units
							follow = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(follow));

							// combining lead and follow
							const auto final = _mm512_add_epi16(follow, lead);
							const auto native_final = utf8::to_native_utf16<OutputType>(final);

							return native_final;
						}();

						if constexpr (MaskOut)
						{
							const auto num_out = std::popcount(_pdep_u64(0xffff'ffff, leading_mask));
							const auto out_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(num_out));

							_mm512_mask_storeu_epi16(it_output_current, out_mask, out);

							it_output_current += num_out;
						}
						else
						{
							const auto num_out = std::popcount(leading_mask);
							const auto out_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(num_out));

							_mm512_mask_storeu_epi16(it_output_current, out_mask, out);

							it_output_current += num_out;
						}

						return true;
					}
				};
				const auto do_fallback = [&]() noexcept -> result_error_input_output_type
				{
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

					if (current_input_length >= advance)
					{
						// We must check whether we are the fourth continuation byte
						const auto c1 = static_cast<std::uint8_t>(*it_input_current - 0);
						const auto c2 = static_cast<std::uint8_t>(*it_input_current - 1);
						const auto c3 = static_cast<std::uint8_t>(*it_input_current - 2);
						const auto c4 = static_cast<std::uint8_t>(*it_input_current - 3);

						if (
							(c1 & 0xc0) == 0x80 and
							(c2 & 0xc0) == 0x80 and
							(c3 & 0xc0) == 0x80 and
							(c4 & 0xc0) == 0x80
						)
						{
							return {.error = ErrorCode::TOO_LONG, .input = current_input_length, .output = current_output_length};
						}
					}

					const auto result = [=, remaining = static_cast<size_type>(it_input_end - it_input_current)]() noexcept
					{
						if constexpr (InputType == CharsType::UTF8_CHAR)
						{
							if constexpr (OutputType == CharsType::UTF16_LE)
							{
								return utf8_char::scalar::rewind_and_write_utf16_le(it_output_current, it_input_begin, {it_input_current, remaining});
							}
							else
							{
								return utf8_char::scalar::rewind_and_write_utf16_be(it_output_current, it_input_begin, {it_input_current, remaining});
							}
						}
						else
						{
							if constexpr (OutputType == CharsType::UTF16_LE)
							{
								return chars::utf8::scalar::rewind_and_write_utf16_le(it_output_current, it_input_begin, {it_input_current, remaining});
							}
							else
							{
								return chars::utf8::scalar::rewind_and_write_utf16_be(it_output_current, it_input_begin, {it_input_current, remaining});
							}
						}
					}();
					return {.error = result.error, .input = result.input + current_input_length, .output = result.output + current_output_length};
				};

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto success = do_process.template operator()<false>();

					if constexpr (Pure or Correct)
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(success);
					}
					else
					{
						if (not success)
						{
							return do_fallback();
						}
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto success = do_process.template operator()<true>();

					if constexpr (Pure or Correct)
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(success);
					}
					else
					{
						if (not success)
						{
							return do_fallback();
						}
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::UTF32
				         )
			[[nodiscard]] constexpr auto write_utf32(
				const typename output_type_of<OutputType>::pointer output,
				const input_type_of<InputType> input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using input_type = input_type_of<InputType>;
				using pointer_type = typename input_type::const_pointer;
				using size_type = typename input_type::size_type;

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				constexpr auto advance = 4 * advance_of_utf32;
				constexpr auto invalid_count = std::numeric_limits<std::uint8_t>::max();

				const auto do_fallback = [&]() noexcept -> result_error_input_output_type
				{
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

					if (current_input_length >= advance)
					{
						const auto c1 = static_cast<std::uint8_t>(*it_input_current + 0);

						if ((c1 & 0xc0) != 0x80)
						{
							// We might have an error that occurs right before `it_input_current`.
							// This is only a concern if `c1` is not a continuation byte.
							it_input_current -= 1;
						}
						else
						{
							// We must check whether we are the fourth continuation byte.
							const auto c2 = static_cast<std::uint8_t>(*it_input_current - 1);
							const auto c3 = static_cast<std::uint8_t>(*it_input_current - 2);
							const auto c4 = static_cast<std::uint8_t>(*it_input_current - 3);

							if (
								(c2 & 0xc0) == 0x80 and
								(c3 & 0xc0) == 0x80 and
								(c4 & 0xc0) == 0x80
							)
							{
								return {.error = ErrorCode::TOO_LONG, .input = current_input_length, .output = current_output_length};
							}
						}
					}

					const auto remaining = static_cast<size_type>(it_input_end - it_input_current);

					const auto result = [=]() noexcept
					{
						if constexpr (InputType == CharsType::UTF8_CHAR)
						{
							return utf8_char::scalar::rewind_and_write_utf32(it_output_current, it_input_begin, {it_input_current, remaining});
						}
						else
						{
							return chars::utf8::scalar::rewind_and_write_utf32(it_output_current, it_input_begin, {it_input_current, remaining});
						}
					}();
					return {.error = result.error, .input = result.input + current_input_length, .output = result.output + current_output_length};
				};

				detail::utf8::icelake::avx512_utf8_checker checker{};

				// In the main loop, we consume 64 bytes per iteration, but we access 64 + 4 bytes.
				while (it_input_current + advance + sizeof(std::uint32_t) <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance + sizeof(std::uint32_t)};
					#endif

					const auto data = _mm512_loadu_si512(it_input_current);
					if (checker.check_data(data))
					{
						detail::utf8::icelake::write_utf32_pure(it_output_current, data);

						it_input_current += advance;
						continue;
					}

					if (checker.has_error())
					{
						return do_fallback();
					}

					const auto lane_0 = detail::utf8::icelake::broadcast<0>(data);
					const auto lane_1 = detail::utf8::icelake::broadcast<1>(data);
					const auto lane_2 = detail::utf8::icelake::broadcast<2>(data);
					const auto lane_3 = detail::utf8::icelake::broadcast<3>(data);
					const auto lane_4 = _mm512_set1_epi32(static_cast<int>(memory::unaligned_load<std::uint32_t>(it_input_current + advance)));

					auto valid_count_0 = invalid_count;
					auto vec_0 = detail::utf8::icelake::expand_and_identify(lane_0, lane_1, valid_count_0);
					GAL_PROMETHEUS_ERROR_ASSUME(valid_count_0 != invalid_count);
					auto valid_count_1 = invalid_count;
					auto vec_1 = detail::utf8::icelake::expand_and_identify(lane_1, lane_2, valid_count_1); // NOLINT(readability-suspicious-call-argument)
					GAL_PROMETHEUS_ERROR_ASSUME(valid_count_1 != invalid_count);

					if (valid_count_0 + valid_count_1 <= advance_of_utf32)
					{
						const auto mask_0 = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(valid_count_1));
						const auto mask_1 = static_cast<__mmask16>(mask_0 << valid_count_0);

						const auto expanded = _mm512_mask_expand_epi32(vec_0, mask_1, vec_1);
						vec_0 = detail::utf8::icelake::expand_to_utf32(expanded);
						valid_count_0 += valid_count_1;

						detail::utf8::icelake::write_utf32(it_output_current, vec_0, valid_count_0);
					}
					else
					{
						vec_0 = detail::utf8::icelake::expand_to_utf32(vec_0);
						vec_1 = detail::utf8::icelake::expand_to_utf32(vec_1);

						detail::utf8::icelake::write_utf32(it_output_current, vec_0, valid_count_0);
						detail::utf8::icelake::write_utf32(it_output_current, vec_1, valid_count_1);
					}

					auto valid_count_2 = invalid_count;
					auto vec_2 = detail::utf8::icelake::expand_and_identify(lane_2, lane_3, valid_count_2);
					GAL_PROMETHEUS_ERROR_ASSUME(valid_count_2 != invalid_count);
					auto valid_count_3 = invalid_count;
					auto vec_3 = detail::utf8::icelake::expand_and_identify(lane_3, lane_4, valid_count_3);
					GAL_PROMETHEUS_ERROR_ASSUME(valid_count_3 != invalid_count);

					if (valid_count_2 + valid_count_3 <= advance_of_utf32)
					{
						const auto mask_0 = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(valid_count_3));
						const auto mask_1 = static_cast<__mmask16>(mask_0 << valid_count_2);

						const auto expanded = _mm512_mask_expand_epi32(vec_2, mask_1, vec_1);
						vec_2 = detail::utf8::icelake::expand_to_utf32(expanded);
						valid_count_2 += valid_count_3;

						detail::utf8::icelake::write_utf32(it_output_current, vec_2, valid_count_2);
					}
					else
					{
						vec_2 = detail::utf8::icelake::expand_to_utf32(vec_2);
						vec_3 = detail::utf8::icelake::expand_to_utf32(vec_3);

						detail::utf8::icelake::write_utf32(it_output_current, vec_2, valid_count_2);
						detail::utf8::icelake::write_utf32(it_output_current, vec_3, valid_count_3);
					}

					it_input_current += advance;
				}

				auto it_valid_input_current = it_input_current;

				// For the final pass, we validate 64 bytes,
				// but we only transcode 3*16 bytes,
				// so we may end up double-validating 16 bytes.
				if (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					if (const auto data = _mm512_loadu_si512(it_input_current);
						checker.check_data(data))
					{
						detail::utf8::icelake::write_utf32_pure(it_output_current, data);

						it_input_current += advance;
					}
					else if (checker.has_error())
					{
						return do_fallback();
					}
					else
					{
						// 64
						const auto lane_0 = detail::utf8::icelake::broadcast<0>(data);
						const auto lane_1 = detail::utf8::icelake::broadcast<1>(data);
						const auto lane_2 = detail::utf8::icelake::broadcast<2>(data);
						const auto lane_3 = detail::utf8::icelake::broadcast<3>(data);

						auto valid_count_0 = invalid_count;
						auto vec_0 = detail::utf8::icelake::expand_and_identify(lane_0, lane_1, valid_count_0);
						GAL_PROMETHEUS_ERROR_ASSUME(valid_count_0 != invalid_count);
						auto valid_count_1 = invalid_count;
						auto vec_1 = detail::utf8::icelake::expand_and_identify(lane_1, lane_2, valid_count_1); // NOLINT(readability-suspicious-call-argument)
						GAL_PROMETHEUS_ERROR_ASSUME(valid_count_1 != invalid_count);

						if (valid_count_0 + valid_count_1 <= advance_of_utf32)
						{
							const auto mask_0 = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(valid_count_1));
							const auto mask_1 = static_cast<__mmask16>(mask_0 << valid_count_0);

							const auto expanded = _mm512_mask_expand_epi32(vec_0, mask_1, vec_1);
							vec_0 = detail::utf8::icelake::expand_to_utf32(expanded);
							valid_count_0 += valid_count_1;

							detail::utf8::icelake::write_utf32(it_output_current, vec_0, valid_count_0);
						}
						else
						{
							vec_0 = detail::utf8::icelake::expand_to_utf32(vec_0);
							vec_1 = detail::utf8::icelake::expand_to_utf32(vec_1);

							detail::utf8::icelake::write_utf32(it_output_current, vec_0, valid_count_0);
							detail::utf8::icelake::write_utf32(it_output_current, vec_1, valid_count_1);
						}

						detail::utf8::icelake::transcode_16(it_output_current, lane_2, lane_3);

						it_input_current += 3 * advance_of_utf32;
					}

					it_valid_input_current += advance;
				}

				{
					const auto remaining = it_input_end - it_valid_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance);

					if (remaining != 0)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
						#endif

						const auto mask = _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(remaining));
						const auto data = _mm512_maskz_loadu_epi8(mask, it_valid_input_current);

						std::ignore = checker.check_data(data);
					}
					checker.check_eof();

					if (checker.has_error())
					{
						return do_fallback();
					}
				}
				{
					// the AVX512 procedure looks up 4 bytes forward,
					// and correctly converts multibyte chars even if their continuation bytes lie outside 16-byte window.
					// It means, we have to skip continuation bytes from the beginning `it_input_current`, as they were already consumed.
					it_input_current = std::ranges::find_if(
						it_input_current,
						it_input_end,
						[](const auto c) noexcept -> bool
						{
							return (static_cast<std::uint8_t>(c) & 0xc0) != 0x80;
						}
					);

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance);

					if (remaining != 0)
					{
						const auto result = Scalar::convert<InputType, OutputType, Pure, Correct>(it_output_current, {it_input_current, static_cast<size_type>(remaining)});

						if constexpr (Correct)
						{
							it_input_current += remaining;
							it_output_current += result.output;
						}
						else if constexpr (Pure)
						{
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
							it_input_current += result.input;
							it_output_current += result.input;
						}
						else
						{
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
							it_input_current += result.input;
							it_output_current += result.output;
						}

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						if constexpr (Correct)
						{
							return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
						}
						else
						{
							return {.error = result.error, .input = current_input_length, .output = current_output_length};
						}
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			// UTF8_CHAR => UTF8
			// UTF8 => UTF8_CHAR
			template<CharsType InputType, CharsType OutputType>
				requires (
					         InputType == CharsType::UTF8_CHAR and
					         OutputType == CharsType::UTF8
				         ) or
				         (
					         InputType == CharsType::UTF8 and
					         OutputType == CharsType::UTF8_CHAR
				         )
			[[nodiscard]] constexpr auto transform(
				const typename output_type_of<OutputType>::pointer output,
				const input_type_of<InputType> input
			) noexcept -> result_error_input_type
			{
				using char_type = typename input_type_of<InputType>::value_type;

				if (const auto result = icelake::validate<InputType>(input);
					result.has_error())
				{
					std::memcpy(output, input.data(), result.input * sizeof(char_type));
					return {.error = result.error, .input = result.input};
				}

				std::memcpy(output, input.data(), input.size() * sizeof(char_type));
				return {.error = ErrorCode::NONE, .input = input.size()};
			}
		}
	}

	namespace utf16
	{
		using input_type = chars::utf16::input_type;
		// using char_type = chars::utf16::char_type;
		using size_type = chars::utf16::size_type;
		using pointer_type = chars::utf16::pointer_type;

		constexpr auto advance_of_latin = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16>::value_type);
		constexpr auto advance_of_utf8 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16>::value_type);
		constexpr auto advance_of_utf16 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16>::value_type);
		constexpr auto advance_of_utf32 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16>::value_type);

		template<CharsType InputType>
			requires (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE or
				InputType == CharsType::UTF16
			)
		[[nodiscard]] constexpr auto read_native(const pointer_type source) noexcept -> data_type
		{
			const auto data = _mm512_loadu_si512(source);

			if constexpr (common::not_native_endian<InputType>())
			{
				const auto byte_flip = _mm512_setr_epi64(
					// clang-format off
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
					// clang-format on
				);

				return _mm512_shuffle_epi8(data, byte_flip);
			}
			else
			{
				return data;
			}
		}

		template<CharsType InputType>
			requires (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE or
				InputType == CharsType::UTF16
			)
		[[nodiscard]] constexpr auto read_native(const pointer_type source, const std::size_t length) noexcept -> data_type
		{
			const auto mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length));
			const auto data = _mm512_maskz_loadu_epi16(mask, source);

			if constexpr (common::not_native_endian<InputType>())
			{
				const auto byte_flip = _mm512_setr_epi64(
					// clang-format off
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
					// clang-format on
				);

				return _mm512_shuffle_epi8(data, byte_flip);
			}
			else
			{
				return data;
			}
		}

		namespace icelake
		{
			template<CharsType InputType>
				requires (
					InputType == CharsType::UTF16_LE or
					InputType == CharsType::UTF16_BE
				)
			[[nodiscard]] constexpr auto validate(const input_type input) noexcept -> result_error_input_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				// keep an overlap of one code unit
				constexpr auto advance_keep_high_surrogate = advance_of_utf16 - 1;

				const auto v_d800 = _mm512_set1_epi16(static_cast<short>(0xd800));
				const auto v_0800 = _mm512_set1_epi16(0x0800);
				const auto v_0400 = _mm512_set1_epi16(0x0400);

				while (it_input_current + advance_of_utf16 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf16};
					#endif

					const auto data = utf16::read_native<InputType>(it_input_current);
					const auto diff = _mm512_sub_epi16(data, v_d800);

					if (const auto surrogates = _mm512_cmplt_epu16_mask(diff, v_0800);
						surrogates)
					{
						const auto high_surrogates = _mm512_cmplt_epu16_mask(diff, v_0400);
						const auto low_surrogates = surrogates ^ high_surrogates;
						// high must be followed by low
						if ((high_surrogates << 1) != low_surrogates)
						{
							const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

							const auto extra_high = std::countr_zero(static_cast<std::uint32_t>(high_surrogates & ~(low_surrogates >> 1)));
							const auto extra_low = std::countr_zero(static_cast<std::uint32_t>(low_surrogates & ~(high_surrogates << 1)));

							return {.error = ErrorCode::SURROGATE, .input = current_input_length + std::ranges::min(extra_high, extra_low)};
						}

						if (const auto ends_with_high = ((high_surrogates & 0x8000'0000) != 0);
							ends_with_high)
						{
							// advance only by 31 code units so that we start with the high surrogate on the next round.
							it_input_current += advance_keep_high_surrogate;
						}
						else
						{
							it_input_current += advance_of_utf16;
						}
					}

					it_input_current += advance_of_utf16;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf16);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto data = utf16::read_native<InputType>(it_input_current, static_cast<std::size_t>(remaining));
					const auto diff = _mm512_sub_epi16(data, v_d800);

					if (const auto surrogates = _mm512_cmplt_epu16_mask(diff, v_0800);
						surrogates)
					{
						const auto high_surrogates = _mm512_cmplt_epu16_mask(diff, v_0400);
						const auto low_surrogates = surrogates ^ high_surrogates;
						// high must be followed by low
						if ((high_surrogates << 1) != low_surrogates)
						{
							const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

							const auto extra_high = std::countr_zero(static_cast<std::uint32_t>(high_surrogates & ~(low_surrogates >> 1)));
							const auto extra_low = std::countr_zero(static_cast<std::uint32_t>(low_surrogates & ~(high_surrogates << 1)));

							return {.error = ErrorCode::SURROGATE, .input = current_input_length + std::ranges::min(extra_high, extra_low)};
						}
					}

					it_input_current += remaining;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				return {.error = ErrorCode::NONE, .input = current_input_length};
			}

			template<CharsType InputType, CharsType OutputType>
				requires (
					InputType == CharsType::UTF16_LE or
					InputType == CharsType::UTF16_BE
				)
			[[nodiscard]] constexpr auto length(const input_type input) noexcept -> size_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				// ReSharper disable CppClangTidyBugproneBranchClone
				if constexpr (OutputType == CharsType::LATIN)
				{
					return input.size();
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					// ReSharper disable CppInconsistentNaming
					// ReSharper disable IdentifierTypo

					const auto v_007f = _mm512_set1_epi16(0x007f);
					const auto v_07ff = _mm512_set1_epi16(0x07ff);
					const auto v_dfff = _mm512_set1_epi16(static_cast<short>(0xdfff));
					const auto v_d800 = _mm512_set1_epi16(static_cast<short>(0xd800));

					// ReSharper restore IdentifierTypo
					// ReSharper restore CppInconsistentNaming

					auto result_length = static_cast<size_type>(0);
					while (it_input_current + advance_of_utf8 <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf8};
						#endif

						const auto data = utf16::read_native<InputType>(it_input_current);

						const auto ascii_bitmask = _mm512_cmple_epu16_mask(data, v_007f);
						const auto two_bytes_bitmask = _mm512_mask_cmple_epu16_mask(~ascii_bitmask, data, v_07ff);
						const auto surrogates_bitmask =
								_mm512_mask_cmple_epu16_mask(~(ascii_bitmask | two_bytes_bitmask), data, v_dfff) &
								_mm512_mask_cmpge_epu16_mask(~(ascii_bitmask | two_bytes_bitmask), data, v_d800);

						const auto ascii_count = std::popcount(ascii_bitmask);
						const auto two_bytes_count = std::popcount(two_bytes_bitmask);
						const auto surrogates_bytes_count = std::popcount(surrogates_bitmask);
						const auto three_bytes_count = advance_of_utf8 - ascii_count - two_bytes_count - surrogates_bytes_count;

						result_length +=
								1 * ascii_count +
								2 * two_bytes_count +
								2 * surrogates_bytes_count +
								3 * three_bytes_count;
						it_input_current += advance_of_utf8;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf8);

					if (remaining != 0)
					{
						result_length += Scalar::length<InputType, OutputType>({it_input_current, static_cast<std::size_t>(remaining)});
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
					const auto low = _mm512_set1_epi16(static_cast<short>(0xdc00));
					const auto high = _mm512_set1_epi16(static_cast<short>(0xdfff));

					auto result_length = static_cast<size_type>(0);
					while (it_input_current + advance_of_utf32 <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf32};
						#endif

						const auto data = utf16::read_native<InputType>(it_input_current);

						const auto not_high_surrogate_bitmask =
								_mm512_cmpgt_epu16_mask(data, high) |
								_mm512_cmplt_epu16_mask(data, low);

						const auto not_high_surrogate_count = std::popcount(not_high_surrogate_bitmask);

						result_length += not_high_surrogate_count;
						it_input_current += advance_of_utf32;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf32);

					if (remaining != 0)
					{
						result_length += Scalar::length<InputType, OutputType>({it_input_current, static_cast<std::size_t>(remaining)});
					}

					return result_length;
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF16_LE or
					         InputType == CharsType::UTF16_BE
				         ) and
				         (
					         OutputType == CharsType::LATIN
				         )
			[[nodiscard]] constexpr auto write_latin(
				const typename output_type_of<OutputType>::pointer output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				// ReSharper disable CppInconsistentNaming
				// ReSharper disable IdentifierTypo

				const auto v_00ff = _mm512_set1_epi16(0x00ff);

				// ReSharper restore IdentifierTypo
				// ReSharper restore CppInconsistentNaming

				const auto do_write = [&]<bool MaskOut>(const data_type data, const std::size_t data_length) noexcept -> void
				{
					if constexpr (not MaskOut)
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data_length == advance_of_latin);
					}

					const auto shuffle_mask = _mm512_set_epi8(
						// clang-format off
						00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
						00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
						62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 
						30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8u, 06, 04, 02, 00
						// clang-format on
					);

					if constexpr (const auto out = _mm512_castsi512_si256(_mm512_permutexvar_epi8(shuffle_mask, data));
						MaskOut)
					{
						const auto source_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(data_length));

						_mm256_mask_storeu_epi8(it_output_current, source_mask, out);
					}
					else
					{
						auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m256i*, it_output_current);

						_mm256_storeu_si256(p, out);
					}
				};

				while (it_input_current + advance_of_latin <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_latin};
					#endif

					const auto data = utf16::read_native<InputType>(it_input_current);

					if constexpr (not Pure or not Correct)
					{
						if (const auto mask = _mm512_cmpgt_epu16_mask(data, v_00ff);
							mask)
						{
							const auto extra = static_cast<std::size_t>(std::countr_zero(mask));
							const auto result = Scalar::convert<InputType, OutputType, false, true>(it_output_current, {it_input_current, extra});
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(result.output == extra);

							it_input_current += extra;
							it_output_current += result.output;

							const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
							const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

							return {.error = ErrorCode::TOO_LARGE, .input = current_input_length, .output = current_output_length};
						}
					}

					do_write.template operator()<false>(data, advance_of_latin);

					it_input_current += advance_of_latin;
					it_output_current += advance_of_latin;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_latin);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto data = utf16::read_native<InputType>(it_input_current, static_cast<std::size_t>(remaining));

					if constexpr (not Pure or not Correct)
					{
						if (const auto mask = _mm512_cmpgt_epu16_mask(data, v_00ff);
							mask)
						{
							const auto extra = static_cast<std::size_t>(std::countr_zero(mask));
							const auto result = Scalar::convert<InputType, OutputType, false, true>(it_output_current, {it_input_current, extra});
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(result.output == extra);

							it_input_current += extra;
							it_output_current += result.output;

							const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
							const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

							return {.error = ErrorCode::TOO_LARGE, .input = current_input_length, .output = current_output_length};
						}
					}

					do_write.template operator()<true>(data, static_cast<std::size_t>(remaining));

					it_input_current += remaining;
					it_output_current += remaining;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF16_LE or
					         InputType == CharsType::UTF16_BE
				         ) and
				         (
					         OutputType == CharsType::UTF8_CHAR or
					         OutputType == CharsType::UTF8
				         )
			[[nodiscard]] constexpr auto write_utf8(
				const typename output_type_of<OutputType>::pointer output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				// keep an overlap of one code unit
				constexpr auto advance_keep_high_surrogate = advance_of_utf8 - 1;

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

				const auto do_process = [&](
					const data_type data,
					const std::size_t data_length,
					const bool end_with_surrogate
				) noexcept -> process_result
				{
					if constexpr (Pure or Correct)
					{
						std::ignore = end_with_surrogate;
					}

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

					const auto data_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(data_length));

					if constexpr (Pure)
					{
						_mm512_mask_cvtepi16_storeu_epi8(it_output_current, data_mask, data);

						return process_result
						{
								.processed_input = static_cast<std::uint8_t>(data_length),
								.num_output = static_cast<std::uint8_t>(data_length),
								.end_with_surrogate = false,
								.pad = 0
						};
					}
					else
					{
						const auto is_234_byte = _mm512_mask_cmpge_epu16_mask(data_mask, data, v_0000_0080);
						if (_ktestz_mask32_u8(data_mask, is_234_byte))
						{
							// ASCII only
							_mm512_mask_cvtepi16_storeu_epi8(it_output_current, data_mask, data);

							return process_result
							{
									.processed_input = static_cast<std::uint8_t>(data_length),
									.num_output = static_cast<std::uint8_t>(data_length),
									.end_with_surrogate = false,
									.pad = 0
							};
						}

						const auto is_12_byte = _mm512_cmplt_epu16_mask(data, v_0000_0800);
						if (_ktestc_mask32_u8(is_12_byte, data_mask))
						{
							// 1/2 byte only

							// (A|B)&C
							const auto two_bytes = _mm512_ternarylogic_epi32(
								_mm512_slli_epi16(data, 8),
								_mm512_srli_epi16(data, 6),
								v_0000_3f3f,
								0xa8
							);
							const auto compare_mask = _mm512_mask_blend_epi16(data_mask, v_0000_ffff, v_0000_0800);
							const auto in = _mm512_mask_add_epi16(data, is_234_byte, two_bytes, v_0000_80c0);
							const auto smoosh = _mm512_cmpge_epu8_mask(in, compare_mask);

							const auto out = _mm512_maskz_compress_epi8(smoosh, in);
							const auto out_mask = _pext_u64(smoosh, smoosh);

							_mm512_mask_storeu_epi8(it_output_current, out_mask, out);

							return process_result
							{
									.processed_input = static_cast<std::uint8_t>(data_length),
									.num_output = static_cast<std::uint8_t>(data_length + std::popcount(is_234_byte)),
									.end_with_surrogate = false,
									.pad = 0
							};
						}

						auto low = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(data));
						auto high = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(data, 1));
						auto tag_low = v_8080_e000;
						auto tag_high = v_8080_e000;

						const auto high_surrogate_mask = _mm512_mask_cmpeq_epu16_mask(
							data_mask,
							_mm512_and_epi32(data, v_0000_fc00),
							v_0000_d800
						);
						const auto low_surrogate_mask = _mm512_cmpeq_epu16_mask(
							_mm512_and_epi32(data, v_0000_fc00),
							v_0000_dc00
						);

						bool this_end_with_surrogate = false;
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

							this_end_with_surrogate = high_surrogate_mask >> 30;

							if (not Correct)
							{
								// check for mismatched surrogates
								if (((high_surrogate_mask << 1) | +end_with_surrogate) ^ low_surrogate_mask)
								{
									const auto low_no_high = low_surrogate_mask & ~((high_surrogate_mask << 1) | +end_with_surrogate);
									const auto high_no_low = high_surrogate_mask & ~(low_surrogate_mask >> 1);
									const auto length = std::countr_zero(low_no_high | high_no_low);

									return process_result
									{
											.processed_input = static_cast<std::uint8_t>(length),
											.num_output = 0,
											.end_with_surrogate = end_with_surrogate,
											.pad = 0
									};
								}
							}
						}

						high = _mm512_maskz_mov_epi32(_cvtu32_mask16(0x0000'7fff), high);

						const auto out_mask = _kandn_mask32(low_surrogate_mask, data_mask);
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

						_mm512_mask_storeu_epi8(it_output_current + 0, want_low_mask, out_low);
						_mm512_mask_storeu_epi8(it_output_current + want_low_length, want_high_mask, out_high);

						return process_result
						{
								.processed_input = static_cast<std::uint8_t>(data_length),
								.num_output = static_cast<std::uint8_t>(want_low_length + want_high_length),
								.end_with_surrogate = this_end_with_surrogate,
								.pad = 0
						};
					}
				};

				bool end_with_surrogate = false;
				while (it_input_current + advance_of_utf8 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf8};
					#endif

					const auto data = utf16::read_native<InputType>(it_input_current);

					if (const auto result = do_process(data, advance_keep_high_surrogate, end_with_surrogate);
						result.processed_input != advance_keep_high_surrogate)
					{
						// surrogate mismatch

						const auto valid_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(result.processed_input));
						const auto valid_data = _mm512_maskz_mov_epi16(valid_mask, data);
						const auto valid_result = do_process(valid_data, result.processed_input, end_with_surrogate);

						it_input_current += valid_result.processed_input;
						it_output_current += valid_result.num_output;

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						return {.error = ErrorCode::SURROGATE, .input = current_input_length, .output = current_output_length};
					}
					else
					{
						it_input_current += result.processed_input;
						it_output_current += result.num_output;
						end_with_surrogate = result.end_with_surrogate;
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf8);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto data = utf16::read_native<InputType>(it_input_current, static_cast<std::size_t>(remaining));

					if (const auto result = do_process(data, static_cast<std::size_t>(remaining), end_with_surrogate);
						result.processed_input != remaining)
					{
						// surrogate mismatch

						const auto valid_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(result.processed_input));
						const auto valid_data = _mm512_maskz_mov_epi16(valid_mask, data);
						const auto valid_result = do_process(valid_data, result.processed_input, end_with_surrogate);

						it_input_current += valid_result.processed_input;
						it_output_current += valid_result.num_output;

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						return {.error = ErrorCode::SURROGATE, .input = current_input_length, .output = current_output_length};
					}
					else
					{
						it_input_current += result.processed_input;
						it_output_current += result.num_output;
						end_with_surrogate = result.end_with_surrogate;
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF16_LE or
					         InputType == CharsType::UTF16_BE
				         ) and
				         (
					         OutputType == CharsType::UTF32
				         )
			[[nodiscard]] constexpr auto write_utf32(
				const typename output_type_of<OutputType>::pointer output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				// keep an overlap of one code unit
				constexpr auto advance_keep_high_surrogate = advance_of_utf32 - 1;

				struct process_result
				{
					// 0~31
					std::uint8_t processed_input;
					// processed_input + ?
					std::uint8_t num_output;
					// keep track if the 31st word is a high surrogate as a carry
					std::uint8_t surrogate_carry;

					bool error;
				};
				static_assert(sizeof(process_result) == sizeof(std::uint32_t));

				constexpr auto data_length_full_block = std::numeric_limits<std::size_t>::max();
				const auto do_process = [&]<bool MaskOut>(
					const data_type data,
					const std::size_t data_length,
					const bool surrogate_carry
				) noexcept -> process_result
				{
					if constexpr (Pure or Correct)
					{
						std::ignore = surrogate_carry;
					}

					if constexpr (MaskOut)
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data_length != data_length_full_block);
					}
					else
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data_length == data_length_full_block);
					}

					const auto data_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(data_length));
					if constexpr (not MaskOut)
					{
						std::ignore = data_mask;
					}

					const auto [high_surrogate_mask, low_surrogate_mask] = [data, data_mask]() noexcept
					{
						const auto v_0000_fc00 = _mm512_set1_epi16(static_cast<short>(0x0000'fc00));
						const auto v_0000_d800 = _mm512_set1_epi16(static_cast<short>(0x0000'd800));
						const auto v_0000_dc00 = _mm512_set1_epi16(static_cast<short>(0x0000'dc00));

						const auto v = _mm512_and_si512(data, v_0000_fc00);
						const auto low = _mm512_cmpeq_epi16_mask(v, v_0000_dc00);

						if constexpr (MaskOut)
						{
							const auto high = _mm512_mask_cmpeq_epu16_mask(data_mask, v, v_0000_d800);

							return std::make_pair(high, low);
						}
						else
						{
							std::ignore = data_mask;

							const auto high = _mm512_cmpeq_epi16_mask(v, v_0000_d800);

							return std::make_pair(high, low);
						}
					}();

					if constexpr (not Pure)
					{
						if (const auto mask = _kortestz_mask32_u8(high_surrogate_mask, low_surrogate_mask);
							mask == 0)
						{
							// handle surrogates

							const auto this_surrogate_carry = static_cast<std::uint8_t>((high_surrogate_mask >> 30) & 0x01);

							if constexpr (not Correct)
							{
								// check for mismatched surrogates
								if (((high_surrogate_mask << 1) | +(surrogate_carry)) ^ low_surrogate_mask)
								{
									const auto low_no_high = low_surrogate_mask & ~((high_surrogate_mask << 1) | +(surrogate_carry));
									const auto high_no_low = high_surrogate_mask & ~(low_surrogate_mask >> 1);
									const auto length = std::countr_zero(low_no_high | high_no_low);

									return process_result
									{
											.processed_input = static_cast<std::uint8_t>(length),
											.num_output = 0,
											.surrogate_carry = surrogate_carry,
											.error = true
									};
								}
							}

							const auto high_surrogate_mask_high = static_cast<__mmask16>(high_surrogate_mask >> 16);
							const auto high_surrogate_mask_low = static_cast<__mmask16>(high_surrogate_mask);

							// ReSharper disable CommentTypo

							// Input surrogate pair:
							// |1101.11aa.aaaa.aaaa|1101.10bb.bbbb.bbbb|
							//  low surrogate            high surrogate

							// Expand all code units to 32-bit code units
							// in > |0000.0000.0000.0000.1101.11aa.aaaa.aaaa|0000.0000.0000.0000.1101.10bb.bbbb.bbbb|
							const auto low = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(data));
							const auto high = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(data, 1));

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

							// ReSharper restore CommentTypo

							const auto added_low = _mm512_mask_add_epi32(aligned_low, high_surrogate_mask_low, aligned_low, shifted_low);
							const auto added_high = _mm512_mask_add_epi32(aligned_high, high_surrogate_mask_high, aligned_high, shifted_high);

							const auto utf32_low = _mm512_mask_add_epi32(added_low, high_surrogate_mask_low, added_low, constant);
							const auto utf32_high = _mm512_mask_add_epi32(added_high, high_surrogate_mask_high, added_high, constant);

							const auto valid = ~low_surrogate_mask & data_mask;
							const auto valid_high = static_cast<__mmask16>(valid >> 16);
							const auto valid_low = static_cast<__mmask16>(valid);

							const auto output_low = _mm512_maskz_compress_epi32(valid_low, utf32_low);
							const auto output_high = _mm512_maskz_compress_epi32(valid_high, utf32_high);

							const auto low_length = std::popcount(valid_low);
							const auto high_length = std::popcount(valid_high);
							const auto low_mask = static_cast<__mmask16>(_pext_u32(valid_low, valid_low));
							const auto high_mask = static_cast<__mmask16>(_pext_u32(valid_high, valid_high));

							// [in][in]...[in][in][0][0]...[0][0](1) + [in][in]...[in][in][0][0]...[0][0](2) ==>
							// [out][out]...[out][out][0][0]...[0][0](1) ==>
							// [out][out]...[out][out](1)[out][out]...[out][out](2)
							if constexpr (MaskOut)
							{
								// is the second half of the input vector used?
								if (data_length > 16)
								{
									_mm512_mask_storeu_epi32(it_output_current + 0, low_mask, output_low);
									_mm512_mask_storeu_epi32(it_output_current + low_length, high_mask, output_high);
								}
								else
								{
									_mm512_mask_storeu_epi32(it_output_current + 0, low_mask, output_low);
								}

								return process_result
								{
										.processed_input = static_cast<std::uint8_t>(data_length),
										.num_output = static_cast<std::uint8_t>(low_length + high_length),
										.surrogate_carry = this_surrogate_carry,
										.error = false
								};
							}
							else
							{
								_mm512_storeu_si512(it_output_current + 0, output_low);
								_mm512_mask_storeu_epi32(it_output_current + low_length, high_mask, output_high);

								return process_result
								{
										// keep an overlap of one code unit
										.processed_input = static_cast<std::uint8_t>(advance_keep_high_surrogate),
										// overwrite last one code unit
										.num_output = static_cast<std::uint8_t>(low_length + high_length - 1),
										.surrogate_carry = this_surrogate_carry,
										.error = false
								};
							}
						}
					}

					// no surrogates

					// 0~15
					const auto out_low = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(data));
					// 16~31
					const auto out_high = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(data, 1));

					if constexpr (MaskOut)
					{
						// [in][in][in]...[in][0][0]...[0][0] ==>
						// [out][out][out]...[out]

						const auto valid = ~low_surrogate_mask & data_mask;
						const auto valid_high = static_cast<__mmask16>(valid >> 16);
						const auto valid_low = static_cast<__mmask16>(valid);

						const auto low_length = std::popcount(valid_low);
						const auto high_length = std::popcount(valid_high);
						const auto low_mask = static_cast<__mmask16>(_pext_u32(valid_low, valid_low));
						const auto high_mask = static_cast<__mmask16>(_pext_u32(valid_high, valid_high));

						_mm512_mask_storeu_epi32(it_output_current + 0, low_mask, out_low);
						_mm512_mask_storeu_epi32(it_output_current + low_length, high_mask, out_high);

						return process_result
						{
								.processed_input = static_cast<std::uint8_t>(data_length),
								.num_output = static_cast<std::uint8_t>(low_length + high_length),
								.surrogate_carry = 0,
								.error = false
						};
					}
					else
					{
						// [in][in][in]...[in][in][in] ==>
						// [out][out][out]...[out][out][out]

						_mm512_storeu_si512(it_output_current + 0, out_low);
						_mm512_storeu_si512(it_output_current + advance_of_utf32 / 2, out_high);

						return process_result
						{
								.processed_input = static_cast<std::uint8_t>(advance_of_utf32),
								.num_output = static_cast<std::uint8_t>(advance_of_utf32),
								.surrogate_carry = 0,
								.error = false
						};
					}
				};

				std::uint8_t surrogate_carry = 0;
				while (it_input_current + advance_of_utf32 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf32};
					#endif

					const auto data = utf16::read_native<InputType>(it_input_current);

					if (const auto result = do_process.template operator()<false>(data, data_length_full_block, surrogate_carry);
						result.error)
					{
						// surrogate mismatch

						const auto valid_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(result.processed_input));
						const auto valid_data = _mm512_maskz_mov_epi16(valid_mask, data);
						const auto valid_result = do_process.template operator()<false>(valid_data, result.processed_input, surrogate_carry);

						it_input_current += valid_result.processed_input;
						it_output_current += valid_result.num_output;

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						return {.error = ErrorCode::SURROGATE, .input = current_input_length, .output = current_output_length};
					}
					else
					{
						it_input_current += result.processed_input;
						it_output_current += result.num_output;
						surrogate_carry = result.surrogate_carry;
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf32);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto data = utf16::read_native<InputType>(it_input_current, static_cast<std::size_t>(remaining));

					if (const auto result = do_process.template operator()<true>(data, static_cast<std::size_t>(remaining), surrogate_carry);
						result.error)
					{
						// surrogate mismatch

						const auto valid_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(result.processed_input));
						const auto valid_data = _mm512_maskz_mov_epi16(valid_mask, data);
						const auto valid_result = do_process.template operator()<true>(valid_data, result.processed_input, surrogate_carry);

						it_input_current += valid_result.processed_input;
						it_output_current += valid_result.num_output;

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						return {.error = ErrorCode::SURROGATE, .input = current_input_length, .output = current_output_length};
					}
					else
					{
						it_input_current += result.processed_input;
						it_output_current += result.num_output;
						surrogate_carry = result.surrogate_carry;
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			constexpr auto flip(
				const output_type_of<CharsType::UTF16>::pointer output,
				const input_type_of<CharsType::UTF16> input
			) noexcept -> void
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const auto it_output_begin = output;
				auto it_output_current = it_output_begin;

				while (it_input_current + advance_of_utf16 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf16};
					#endif

					const auto data = utf16::read_native<CharsType::UTF16>(it_input_current);

					_mm512_storeu_si512(it_output_current, data);

					it_input_current += advance_of_utf16;
					it_output_current += advance_of_utf16;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf16);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(remaining));
					const auto data = utf16::read_native<CharsType::UTF16>(it_input_current, static_cast<std::size_t>(remaining));

					_mm512_mask_storeu_epi16(it_output_current, mask, data);
				}
			}

			template<CharsType InputType, CharsType OutputType>
				requires (
					         InputType == CharsType::UTF16_LE and
					         OutputType == CharsType::UTF16_BE
				         ) or
				         (
					         InputType == CharsType::UTF16_BE and
					         OutputType == CharsType::UTF16_LE
				         )
			constexpr auto transform(
				const typename output_type_of<OutputType>::pointer output,
				const input_type_of<InputType> input
			) noexcept -> result_error_input_type
			{
				if (const auto result = icelake::validate<InputType>(input);
					result.has_error())
				{
					icelake::flip(output, {input.data(), result.input});
					return {.error = result.error, .input = result.input};
				}

				icelake::flip(output, input);
				return {.error = ErrorCode::NONE, .input = input.size()};
			}
		}
	}

	namespace utf32
	{
		using input_type = chars::utf32::input_type;
		// using char_type = chars::utf32::char_type;
		using size_type = chars::utf32::size_type;
		using pointer_type = chars::utf32::pointer_type;

		constexpr auto advance_of_latin = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF32>::value_type);
		constexpr auto advance_of_utf8 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF32>::value_type);
		constexpr auto advance_of_utf16 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF32>::value_type);
		constexpr auto advance_of_utf32 = sizeof(data_type) / sizeof(input_type_of<CharsType::UTF32>::value_type);

		namespace icelake
		{
			[[nodiscard]] constexpr auto validate(const input_type input) noexcept -> result_error_input_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const auto do_check = [&](const data_type data) noexcept -> result_error_input_type
				{
					const auto offset = _mm512_set1_epi32(static_cast<int>(0xffff'2000));
					const auto standard_max = _mm512_set1_epi32(0x0010'ffff);
					const auto standard_offset_max = _mm512_set1_epi32(static_cast<int>(0xffff'f7ff));

					const auto value_offset = _mm512_add_epi32(data, offset);

					const auto outside_range = _mm512_cmpgt_epu32_mask(data, standard_max);
					const auto surrogate_range = _mm512_cmpgt_epu32_mask(value_offset, standard_offset_max);

					if (outside_range | surrogate_range)
					{
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

						const auto outside_index = std::countr_zero(outside_range);
						const auto surrogate_index = std::countr_zero(surrogate_range);

						if (outside_index < surrogate_index)
						{
							return {.error = ErrorCode::TOO_LARGE, .input = current_input_length + outside_index};
						}
						return {.error = ErrorCode::SURROGATE, .input = current_input_length + surrogate_index};
					}

					return {.error = ErrorCode::NONE, .input = input_length};
				};

				while (it_input_current + advance_of_utf32 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf32};
					#endif

					const auto data = _mm512_loadu_si512(it_input_current);

					if (const auto result = do_check(data);
						result.has_error())
					{
						return result;
					}

					it_input_current += advance_of_utf32;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf32);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(remaining)));
					const auto data = _mm512_maskz_loadu_epi32(mask, it_input_current);

					if (const auto result = do_check(data);
						result.has_error())
					{
						return result;
					}

					it_input_current += remaining;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				return {.error = ErrorCode::NONE, .input = current_input_length};
			}

			template<CharsType OutputType>
			[[nodiscard]] constexpr auto length(const input_type input) noexcept -> size_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				// ReSharper disable CppClangTidyBugproneBranchClone
				if constexpr (OutputType == CharsType::LATIN)
				{
					return input.size();
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
				{
					// ReSharper disable CppInconsistentNaming
					// ReSharper disable IdentifierTypo

					// Max code point for 1-byte UTF-8
					const auto v_007f = _mm512_set1_epi32(0x007f);
					// Max code point for 2-byte UTF-8
					const auto v_07ff = _mm512_set1_epi32(0x07ff);
					// Max code point for 3-byte UTF-8
					const auto v_ffff = _mm512_set1_epi32(0xffff);

					// ReSharper restore IdentifierTypo
					// ReSharper restore CppInconsistentNaming

					auto output_length = static_cast<size_type>(0);
					while (it_input_current + advance_of_utf8 <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf8};
						#endif

						const auto data = _mm512_loadu_si512(it_input_current);

						const auto ascii_bitmask = _mm512_cmple_epu32_mask(
							data,
							v_007f
						);
						const auto two_bytes_bitmask = _mm512_mask_cmple_epu32_mask(
							_mm512_knot(ascii_bitmask),
							data,
							v_07ff
						);
						const auto three_bytes_bitmask = _mm512_mask_cmple_epu32_mask(
							_mm512_knot(_mm512_kor(ascii_bitmask, two_bytes_bitmask)),
							data,
							v_ffff
						);

						const auto ascii_count = std::popcount(ascii_bitmask);
						const auto two_bytes_count = std::popcount(two_bytes_bitmask);
						const auto three_bytes_count = std::popcount(three_bytes_bitmask);
						const auto four_bytes_count = advance_of_utf8 - ascii_count - two_bytes_count - three_bytes_count;

						output_length += ascii_count + 2 * two_bytes_count + 3 * three_bytes_count + 4 * four_bytes_count;
						it_input_current += advance_of_utf8;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf8);

					if (remaining != 0)
					{
						// fallback
						output_length += Scalar::length<CharsType::UTF32, OutputType>({it_input_current, static_cast<std::size_t>(remaining)});
					}

					return output_length;
				}
				else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
				{
					// ReSharper disable CppInconsistentNaming
					// ReSharper disable IdentifierTypo

					// For each 32-bit code point c:
					//   - If c <= 0xffff, it fits in a single UTF-16 unit.
					//   - If c > 0xffff (i.e. in [0x10000..0x10ffff]), it requires two UTF-16 units (a surrogate pair).  
					const auto v_ffff = _mm512_set1_epi32(0xffff);

					// ReSharper restore IdentifierTypo
					// ReSharper restore CppInconsistentNaming

					auto output_length = static_cast<size_type>(0);
					while (it_input_current + advance_of_utf16 <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance_of_utf16};
						#endif

						const auto data = _mm512_loadu_si512(it_input_current);

						const auto surrogates_bitmask = _mm512_cmpgt_epu32_mask(data, v_ffff);
						// 1 + (surrogate ? 1 : 0)
						output_length += advance_of_utf16 + std::popcount(surrogates_bitmask);

						it_input_current += advance_of_utf16;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf16);

					if (remaining != 0)
					{
						// fallback
						output_length += Scalar::length<CharsType::UTF32, OutputType>({it_input_current, static_cast<std::size_t>(remaining)});
					}

					return output_length;
				}
				// ReSharper disable CppClangTidyBugproneBranchClone
				else if constexpr (OutputType == CharsType::UTF32)
				{
					return input.size();
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::LATIN
				)
			[[nodiscard]] constexpr auto write_latin(
				const typename output_type_of<OutputType>::pointer output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				// ReSharper disable CppInconsistentNaming
				// ReSharper disable IdentifierTypo

				// Max code point for Latin1
				const auto v_00ff = _mm512_set1_epi32(0x00ff);

				// Gathers the lowest byte of each 32-bit code point in 'data' into a contiguous 16-byte __m128i register.
				const auto shuffle_mask = _mm512_set_epi8(
					// clang-format off
					00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
					00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
					00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
					60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8u, 04, 00
					// clang-format on
				);

				// ReSharper restore IdentifierTypo
				// ReSharper restore CppInconsistentNaming

				const auto write_tail_block = [&](const __mmask16 mask) noexcept -> result_error_input_output_type
				{
					if constexpr (Correct)
					{
						std::ignore = mask;
						GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();
					}
					else
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(mask != 0);

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						const auto extra = static_cast<size_type>(std::countr_zero(mask));
						const auto result = Scalar::convert<CharsType::UTF32, OutputType, Pure, Correct>(it_output_current, {it_input_current, extra});

						if constexpr (Pure)
						{
							return {.error = ErrorCode::TOO_LARGE, .input = current_input_length + result.input, .output = current_output_length + result.input};
						}
						else
						{
							return {.error = ErrorCode::TOO_LARGE, .input = current_input_length + result.input, .output = current_output_length + result.output};
						}
					}
				};

				while (it_input_current + advance_of_latin <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_latin};
					#endif

					const auto data = _mm512_loadu_si512(it_input_current);

					if (const auto mask = _mm512_cmpgt_epu32_mask(data, v_00ff);
						mask != 0)
					{
						return write_tail_block(mask);
					}

					const auto out = _mm512_castsi512_si128(_mm512_permutexvar_epi8(shuffle_mask, data));
					auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current);
					_mm_storeu_si128(p, out);

					it_input_current += advance_of_latin;
					it_output_current += advance_of_latin;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_latin);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(remaining)));
					const auto data = _mm512_maskz_loadu_epi32(mask, it_input_current);

					if (const auto latin_mask = _mm512_cmpgt_epu32_mask(data, v_00ff);
						latin_mask != 0)
					{
						return write_tail_block(latin_mask);
					}

					const auto out = _mm512_castsi512_si128(_mm512_permutexvar_epi8(shuffle_mask, data));
					_mm_mask_storeu_epi8(it_output_current, mask, out);

					it_input_current += remaining;
					it_output_current += remaining;
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::UTF8_CHAR or
					OutputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto write_utf8(
				const typename output_type_of<OutputType>::pointer output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				// ReSharper disable CppInconsistentNaming
				// ReSharper disable IdentifierTypo
				// ReSharper disable CommentTypo

				// const auto v_0010_ffff = _mm256_set1_epi32(0x0010'ffff);
				const auto v_7fff_ffff = _mm256_set1_epi32(0x7fff'ffff);
				const auto v_0000_ff80 = _mm256_set1_epi16(static_cast<short>(0x0000'ff80));
				const auto v_0000_0000 = _mm256_setzero_si256();
				const auto v_0000_f800 = _mm256_set1_epi16(static_cast<short>(0x0000'f800));
				const auto v_0000_1f00 = _mm256_set1_epi16(0x0000'1f00);
				const auto v_0000_003f = _mm256_set1_epi16(0x0000'003f);
				const auto v_0000_c080 = _mm256_set1_epi16(static_cast<short>(0x0000'c080));
				const auto v_ffff_0000 = _mm256_set1_epi32(static_cast<int>(0xffff'0000));
				const auto v_0000_d800 = _mm256_set1_epi16(static_cast<short>(0x0000'd800));

				// ReSharper restore CommentTypo
				// ReSharper restore IdentifierTypo
				// ReSharper restore CppInconsistentNaming

				const auto write_tail_block = [&](const __mmask16 mask, const ErrorCode error) noexcept -> result_error_input_output_type
				{
					if constexpr (Correct)
					{
						std::ignore = mask;
						std::ignore = error;
						GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();
					}
					else
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(mask != 0);

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						const auto extra = static_cast<size_type>(std::countr_zero(mask));
						const auto result = Scalar::convert<CharsType::UTF32, OutputType, Pure, Correct>(it_output_current, {it_input_current, extra});

						if constexpr (Pure)
						{
							return {.error = error, .input = current_input_length + result.input, .output = current_output_length + result.input};
						}
						else
						{
							return {.error = error, .input = current_input_length + result.input, .output = current_output_length + result.output};
						}
					}
				};

				while (it_input_current + advance_of_utf8 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf8};
					#endif

					const auto data = _mm512_loadu_si512(it_input_current);
					const auto low = _mm512_castsi512_si256(data);
					const auto high = _mm512_extracti64x4_epi64(data, 1);

					if constexpr (not Correct)
					{
						// ReSharper disable CommentTypo

						// Check for too large input
						// if (const auto mask = _mm512_cmpgt_epu32_mask(data, v_0010_ffff);
						if (const auto mask = _mm512_cmpgt_epu32_mask(data, _mm512_set1_epi32(0x0010'ffff));
							mask != 0)
						{
							return write_tail_block(mask, ErrorCode::TOO_LARGE);
						}

						// ReSharper restore CommentTypo
					}

					// Pack 32-bit UTF-32 code units to 16-bit UTF-16 code units with unsigned saturation
					const auto in_16_packed = _mm256_packus_epi32(
						_mm256_and_si256(low, v_7fff_ffff),
						_mm256_and_si256(high, v_7fff_ffff)
					);
					const auto in_16 = _mm256_permute4x64_epi64(in_16_packed, 0b1101'1000);

					// Try to apply UTF-16 => UTF-8 routine on 256 bits
					if (_mm256_testz_si256(in_16, v_0000_ff80))
					{
						// ascii only

						const auto in_16_low = _mm256_castsi256_si128(in_16);
						const auto in_16_high = _mm256_extracti128_si256(in_16, 1);

						// pack the bytes
						const auto utf8_packed = _mm_packus_epi16(in_16_low, in_16_high);

						// store 16 bytes
						auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current);
						_mm_storeu_si128(p, utf8_packed);

						it_input_current += advance_of_utf8;
						it_output_current += advance_of_utf8;

						// we are done for this round
						continue;
					}

					// no bits set above 7th bit
					const auto one_byte_byte_mask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_0000_ff80), v_0000_0000);
					const auto one_byte_bit_mask = static_cast<std::uint32_t>(_mm256_movemask_epi8(one_byte_byte_mask));

					// no bits set above 11th bit
					const auto one_or_two_bytes_byte_mask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_0000_f800), v_0000_0000);
					const auto one_or_two_bytes_bit_mask = static_cast<std::uint32_t>(_mm256_movemask_epi8(one_or_two_bytes_byte_mask));

					if (one_or_two_bytes_bit_mask == 0xffff'ffff)
					{
						// ReSharper disable CommentTypo

						// prepare 2-bytes values
						// input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
						// expected output   : [110a|aaaa|10bb|bbbb] x 8

						// t0 = [000a|aaaa|bbbb|bb00]
						const auto t0 = _mm256_slli_epi16(in_16, 2);
						// t1 = [000a|aaaa|0000|0000]
						const auto t1 = _mm256_and_si256(t0, v_0000_1f00);
						// t2 = [0000|0000|00bb|bbbb]
						const auto t2 = _mm256_and_si256(in_16, v_0000_003f);
						// t3 = [000a|aaaa|00bb|bbbb]
						const auto t3 = _mm256_or_si256(t1, t2);
						// t4 = [110a|aaaa|10bb|bbbb]
						const auto t4 = _mm256_or_si256(t3, v_0000_c080);

						// ReSharper restore CommentTypo

						// merge ascii and 2-bytes codewords
						const auto utf8_unpacked = _mm256_blendv_epi8(t4, in_16, one_byte_byte_mask);

						// prepare bitmask for 8-bits lookup
						const auto mask_0 = static_cast<std::uint32_t>(one_byte_bit_mask & 0x5555'5555);
						const auto mask_1 = static_cast<std::uint32_t>(mask_0 >> 7);
						const auto mask = static_cast<std::uint32_t>((mask_0 | mask_1) & 0x00ff'00ff);

						// pack the bytes
						using type = std::decay_t<decltype(detail::utf32::icelake::utf16_to_utf8::_1_2[0])>;
						// length + data
						static_assert(std::ranges::size(type{}) == 1 + advance_of_utf8);

						const auto index_0 = static_cast<std::uint8_t>(mask);
						const auto index_1 = static_cast<std::uint8_t>(mask >> 16);

						const auto& data_0 = detail::utf32::icelake::utf16_to_utf8::_1_2[index_0];
						const auto& data_1 = detail::utf32::icelake::utf16_to_utf8::_1_2[index_1];

						const auto length_0 = data_0.front();
						const auto length_1 = data_1.front();
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_0 <= advance_of_utf8);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_1 <= advance_of_utf8);

						const auto* row_0 = data_0.data() + 1;
						const auto* row_1 = data_1.data() + 1;

						const auto shuffle_0 = _mm_loadu_si128(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_0));
						const auto shuffle_1 = _mm_loadu_si128(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_1));

						const auto utf8_packed = _mm256_shuffle_epi8(utf8_unpacked, _mm256_setr_m128i(shuffle_0, shuffle_1));

						const auto utf8_packed_low = _mm256_castsi256_si128(utf8_packed);
						const auto utf8_packed_high = _mm256_extracti128_si256(utf8_packed, 1);

						// store the bytes
						auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current);

						_mm_storeu_si128(p, utf8_packed_low);
						it_output_current += length_0;
						_mm_storeu_si128(p, utf8_packed_high);
						it_output_current += length_1;

						it_input_current += advance_of_utf8;

						// we are done for this round
						continue;
					}

					// check for overflow in packing
					const auto saturation_byte_mask = _mm256_cmpeq_epi32(_mm256_and_si256(_mm256_or_si256(low, high), v_ffff_0000), v_0000_0000);
					const auto saturation_bit_mask = static_cast<std::uint32_t>(_mm256_movemask_epi8(saturation_byte_mask));

					if (saturation_bit_mask == 0xffff'ffff)
					{
						// code units from register produce either 1, 2 or 3 UTF-8 bytes

						if constexpr (not Correct)
						{
							const auto forbidden_byte_mask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_0000_f800), v_0000_d800);
							if (const auto mask = _mm256_movepi16_mask(forbidden_byte_mask);
								mask != 0)
							{
								return write_tail_block(mask, ErrorCode::SURROGATE);
							}
						}

						// ReSharper disable CommentTypo

						// In this branch we handle three cases:
						// 1. [0000|0000|0ccc|cccc] => [0ccc|cccc]                                                - single UFT-8 byte
						// 2. [0000|0bbb|bbcc|cccc] => [110b|bbbb], [10cc|cccc]                         - two UTF-8 bytes
						// 3. [aaaa|bbbb|bbcc|cccc] => [1110|aaaa], [10bb|bbbb], [10cc|cccc]     - three UTF-8 bytes

						// We expand the input word (16-bit) into two code units (32-bit), thus we have room for four bytes.
						// However, we need five distinct bit layouts.
						// Note that the last byte in cases #2 and #3 is the same.

						// We precompute byte 1 for case #1 and the common byte for cases #2 and #3 in register t2.
						// We precompute byte 1 for case #3 and -- **conditionally** -- precompute either byte 1 for case #2 or byte 2 for case #3.
						// Note that they differ by exactly one bit.
						// Finally, from these two code units we build proper UTF-8 sequence, taking into account the case (i.e, the number of bytes to write).

						// input  : [aaaa|bbbb|bbcc|cccc]
						// output :
						//         t2 => [0ccc|cccc] [10cc|cccc]
						//         s4 => [1110|aaaa] ([110b|bbbb] or [10bb|bbbb])

						const auto dup_even = _mm256_setr_epi16(
							// clang-format off
							0x0000, 0x0202, 0x0404, 0x0606,
							0x0808, 0x0a0a, 0x0c0c, 0x0e0e,
							0x0000, 0x0202, 0x0404, 0x0606,
							0x0808, 0x0a0a, 0x0c0c, 0x0e0e
							// clang-format on
						);

						// [aaaa|bbbb|bbcc|cccc] => [bbcc|cccc|bbcc|cccc]
						const auto t0 = _mm256_shuffle_epi8(in_16, dup_even);
						// [bbcc|cccc|bbcc|cccc] => [00cc|cccc|0bcc|cccc]
						const auto t1 = _mm256_and_si256(t0, _mm256_set1_epi16(0b0011'1111'0111'1111));
						// [00cc|cccc|0bcc|cccc] => [10cc|cccc|0bcc|cccc]
						const auto t2 = _mm256_or_si256(t1, _mm256_set1_epi16(static_cast<short>(0b1000'0000'0000'0000)));

						// [aaaa|bbbb|bbcc|cccc] =>  [0000|aaaa|bbbb|bbcc]
						const auto s0 = _mm256_srli_epi16(in_16, 4);
						// [0000|aaaa|bbbb|bbcc] => [0000|aaaa|bbbb|bb00]
						const auto s1 = _mm256_and_si256(s0, _mm256_set1_epi16(0b0000'1111'1111'1100));
						// [0000|aaaa|bbbb|bb00] => [00bb|bbbb|0000|aaaa]
						const auto s2 = _mm256_maddubs_epi16(s1, _mm256_set1_epi16(0x0140));
						// [00bb|bbbb|0000|aaaa] => [11bb|bbbb|1110|aaaa]
						const auto s3 = _mm256_or_si256(s2, _mm256_set1_epi16(static_cast<short>(0b1100'0000'1110'0000)));
						const auto s4 = _mm256_xor_si256(
							s3,
							_mm256_andnot_si256(
								one_or_two_bytes_byte_mask,
								_mm256_set1_epi16(0b0100'0000'0000'0000)
							)
						);

						// ReSharper restore CommentTypo

						// expand code units 16-bit => 32-bit
						const auto out_0 = _mm256_unpacklo_epi16(t2, s4);
						const auto out_1 = _mm256_unpackhi_epi16(t2, s4);
						// compress 32-bit code units into 1, 2 or 3 bytes -- 2 x shuffle
						const auto mask =
								(one_byte_bit_mask & 0x5555'5555) |
								(one_or_two_bytes_bit_mask & 0xaaaa'aaaa);

						using type = std::decay_t<decltype(detail::utf32::icelake::utf16_to_utf8::_1_2_3[0])>;
						// length + data
						static_assert(std::ranges::size(type{}) == 1 + advance_of_utf8);

						const auto index_0 = static_cast<std::uint8_t>(mask);
						const auto index_1 = static_cast<std::uint8_t>(mask >> 8);
						const auto index_2 = static_cast<std::uint8_t>(mask >> 16);
						const auto index_3 = static_cast<std::uint8_t>(mask >> 24);

						const auto& data_0 = detail::utf32::icelake::utf16_to_utf8::_1_2_3[index_0];
						const auto& data_1 = detail::utf32::icelake::utf16_to_utf8::_1_2_3[index_1];
						const auto& data_2 = detail::utf32::icelake::utf16_to_utf8::_1_2_3[index_2];
						const auto& data_3 = detail::utf32::icelake::utf16_to_utf8::_1_2_3[index_3];

						const auto length_0 = data_0.front();
						const auto length_1 = data_1.front();
						const auto length_2 = data_2.front();
						const auto length_3 = data_3.front();
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_0 <= advance_of_utf8);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_1 <= advance_of_utf8);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_2 <= advance_of_utf8);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_3 <= advance_of_utf8);

						const auto* row_0 = data_0.data() + 1;
						const auto* row_1 = data_1.data() + 1;
						const auto* row_2 = data_2.data() + 1;
						const auto* row_3 = data_3.data() + 1;

						const auto shuffle_0 = _mm_loadu_si128(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_0));
						const auto shuffle_1 = _mm_loadu_si128(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_1));
						const auto shuffle_2 = _mm_loadu_si128(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_2));
						const auto shuffle_3 = _mm_loadu_si128(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_3));

						const auto utf8_0 = _mm_shuffle_epi8(_mm256_castsi256_si128(out_0), shuffle_0);
						const auto utf8_1 = _mm_shuffle_epi8(_mm256_castsi256_si128(out_1), shuffle_1);
						const auto utf8_2 = _mm_shuffle_epi8(_mm256_extracti128_si256(out_0, 1), shuffle_2);
						const auto utf8_3 = _mm_shuffle_epi8(_mm256_extracti128_si256(out_1, 1), shuffle_3);

						// store the bytes
						auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m128i*, it_output_current);

						_mm_storeu_si128(p, utf8_0);
						it_output_current += length_0;
						_mm_storeu_si128(p, utf8_1);
						it_output_current += length_1;
						_mm_storeu_si128(p, utf8_2);
						it_output_current += length_2;
						_mm_storeu_si128(p, utf8_3);
						it_output_current += length_3;

						it_input_current += advance_of_utf8;

						// we are done for this round
						continue;
					}

					// at least one 32-bit word is larger than 0xffff => it will produce four UTF-8 bytes (like emoji)
					// scalar fallback
					const auto fallback_end = it_input_current + advance_of_utf8;
					while (it_input_current < fallback_end)
					{
						const auto [length, error] = Scalar::convert<CharsType::UTF32, OutputType, Pure, Correct>(it_output_current, it_input_current, fallback_end);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == 1);

						if constexpr (Correct)
						{
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(error == ErrorCode::NONE);
						}
						else
						{
							const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
							const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

							return {.error = error, .input = current_input_length, .output = current_output_length};
						}
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf8);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto result = Scalar::convert<CharsType::UTF32, OutputType, Pure, Correct>(it_output_current, {it_input_current, static_cast<std::size_t>(remaining)});

					if constexpr (Correct)
					{
						it_input_current += remaining;
						it_output_current += result.output;
					}
					else if constexpr (Pure)
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
						it_input_current += result.input;
						it_output_current += result.input;
					}
					else
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
						it_input_current += result.input;
						it_output_current += result.output;
					}

					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

					if constexpr (Correct)
					{
						return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
					}
					else
					{
						return {.error = result.error, .input = current_input_length, .output = current_output_length};
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::UTF16_LE or
					OutputType == CharsType::UTF16_BE
				)
			[[nodiscard]] constexpr auto write_utf16(
				const typename output_type_of<OutputType>::pointer output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				// ReSharper disable CppInconsistentNaming
				// ReSharper disable IdentifierTypo

				const auto v_ffff_0000 = _mm512_set1_epi32(static_cast<int>(0xffff'0000));
				const auto v_0000_0000 = _mm512_setzero_si512();
				const auto v_0000_f800 = _mm512_set1_epi16(static_cast<short>(0x0000'f800));
				const auto v_0000_d800 = _mm512_set1_epi16(static_cast<short>(0x0000'd800));
				const auto v_0010_ffff = _mm512_set1_epi32(0x0010'ffff);
				const auto v_0001_0000 = _mm512_set1_epi32(0x0001'0000);
				const auto v_03ff_0000 = _mm512_set1_epi32(0x03ff'0000);
				const auto v_0000_03ff = _mm512_set1_epi32(0x0000'03ff);
				const auto v_dc00_d800 = _mm512_set1_epi32(static_cast<int>(0xdc00'd800));

				// ReSharper restore IdentifierTypo
				// ReSharper restore CppInconsistentNaming

				const auto to_native_utf16 = functional::overloaded{
						[](const data_type data) noexcept -> data_type
						{
							if constexpr (common::not_native_endian<OutputType>())
							{
								const auto shuffle_mask = _mm512_set_epi8(
									// clang-format off
									14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 
									14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 
									14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 
									14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1
									// clang-format on
								);

								return _mm512_shuffle_epi8(data, shuffle_mask);
							}
							else
							{
								return data;
							}
						},
						[](const __m256i data) noexcept -> __m256i
						{
							if constexpr (common::not_native_endian<OutputType>())
							{
								const auto shuffle_mask = _mm256_setr_epi8(
									// clang-format off
								    01, 00, 03, 02, 05, 04, 07, 06, 9u, 8u, 11, 10, 13, 12, 15, 14,
								    01, 00, 03, 02, 05, 04, 07, 06, 9u, 8u, 11, 10, 13, 12, 15, 14
									// clang-format on
								);

								return _mm256_shuffle_epi8(data, shuffle_mask);
							}
							else
							{
								return data;
							}
						}
				};

				const auto do_write_surrogate = [&](const data_type data, const __mmask16 surrogate_mask, const __mmask32 out_mask) noexcept -> size_type
				{
					const auto sub = _mm512_mask_sub_epi32(data, surrogate_mask, data, v_0001_0000);

					auto v1 = _mm512_mask_slli_epi32(sub, surrogate_mask, sub, 16);
					v1 = _mm512_mask_and_epi32(sub, surrogate_mask, v1, v_03ff_0000);
					auto v2 = _mm512_mask_srli_epi32(sub, surrogate_mask, sub, 10);
					v2 = _mm512_mask_and_epi32(sub, surrogate_mask, v2, v_0000_03ff);

					const auto v = _mm512_or_si512(v1, v2);
					const auto out = _mm512_mask_add_epi32(sub, surrogate_mask, v, v_dc00_d800);
					const auto native_out = to_native_utf16(out);

					const auto num_out = std::popcount(out_mask);
					const auto num_out_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(num_out));

					// fixme
					// _mm512_mask_compressstoreu_epi16(it_output_current, out_mask, native_out);
					_mm512_mask_storeu_epi16(
						it_output_current,
						num_out_mask,
						_mm512_maskz_compress_epi16(out_mask, native_out)
					);

					it_output_current += num_out;
					return num_out;
				};

				while (it_input_current + advance_of_utf16 <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance_of_utf16};
					#endif

					const auto data = _mm512_loadu_si512(it_input_current);

					if constexpr (Pure)
					{
						const auto out = _mm512_cvtepi32_epi16(data);
						const auto native_out = to_native_utf16(out);

						auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m256i*, it_output_current);
						_mm256_storeu_si256(p, native_out);

						it_input_current += advance_of_utf16;
						it_output_current += advance_of_utf16;
					}
					else
					{
						// no bits set above 16th bit => can pack to UTF16 without surrogate pairs
						if (const auto saturation_mask = _mm512_cmpeq_epi32_mask(_mm512_and_si512(data, v_ffff_0000), v_0000_0000);
							saturation_mask == 0xffff)
						{
							const auto forbidden_byte_mask = _mm512_cmpeq_epi32_mask(_mm512_and_si512(data, v_0000_f800), v_0000_d800);

							const auto utf16_packed = [=]() noexcept
							{
								const auto out = _mm512_cvtepi32_epi16(data);
								const auto native_out = to_native_utf16(out);

								return native_out;
							}();

							if (Correct)
							{
								GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(forbidden_byte_mask == 0);
							}
							else
							{
								if (forbidden_byte_mask != 0)
								{
									const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
									const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

									const auto extra = std::countr_zero(forbidden_byte_mask);
									const auto extra_mask = static_cast<__mmask16>(_blsmsk_u32(forbidden_byte_mask) >> 1);

									_mm256_mask_storeu_epi16(it_output_current, extra_mask, utf16_packed);

									return {.error = ErrorCode::SURROGATE, .input = current_input_length + extra, .output = current_output_length + extra};
								}
							}

							auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m256i*, it_output_current);
							_mm256_storeu_si256(p, utf16_packed);

							it_input_current += advance_of_utf16;
							it_output_current += advance_of_utf16;
						}
						else
						{
							auto out_mask = ~_pdep_u32(saturation_mask, 0xaaaa'aaaa);
							const auto surrogate_mask = static_cast<__mmask16>(~saturation_mask);

							if constexpr (not Correct)
							{
								const auto error_surrogate = _mm512_mask_cmpeq_epi32_mask(saturation_mask, _mm512_and_si512(data, v_0000_f800), v_0000_d800);
								const auto error_too_large = _mm512_mask_cmpgt_epu32_mask(surrogate_mask, data, v_0010_ffff);

								if (error_surrogate or error_too_large)
								{
									const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
									const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

									const auto surrogate_index = std::countr_zero(error_surrogate);
									const auto outside_index = std::countr_zero(error_too_large);

									if (outside_index < surrogate_index)
									{
										out_mask &= _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(2 * outside_index));
										const auto extra = do_write_surrogate(data, surrogate_mask, out_mask);

										return {.error = ErrorCode::TOO_LARGE, .input = current_input_length + outside_index, .output = current_output_length + extra};
									}

									out_mask &= _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(2 * surrogate_index));
									const auto extra = do_write_surrogate(data, surrogate_mask, out_mask);
									return {.error = ErrorCode::SURROGATE, .input = current_input_length + surrogate_index, .output = current_output_length + extra};
								}
							}

							std::ignore = do_write_surrogate(data, surrogate_mask, out_mask);
							it_input_current += advance_of_utf16;
						}
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < advance_of_utf16);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto mask = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(remaining)));
					const auto data = _mm512_maskz_loadu_epi32(mask, it_input_current);

					if constexpr (Pure)
					{
						const auto out = _mm512_cvtepi32_epi16(data);
						const auto native_out = to_native_utf16(out);

						_mm256_mask_storeu_epi16(it_output_current, mask, native_out);
					}
					else
					{
						if (const auto saturation_mask = static_cast<__mmask16>(_mm512_cmpeq_epi32_mask(_mm512_and_si512(data, v_ffff_0000), v_0000_0000) & mask);
							saturation_mask == mask)
						{
							const auto forbidden_byte_mask = _mm512_cmpeq_epi32_mask(_mm512_and_si512(data, v_0000_f800), v_0000_d800);

							const auto utf16_packed = [=]() noexcept
							{
								const auto out = _mm512_cvtepi32_epi16(data);
								const auto native_out = to_native_utf16(out);

								return native_out;
							}();

							if (Correct)
							{
								GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(forbidden_byte_mask == 0);
							}
							else
							{
								if (forbidden_byte_mask != 0)
								{
									const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
									const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

									const auto extra = std::countr_zero(forbidden_byte_mask);
									const auto extra_mask = static_cast<__mmask16>(_blsmsk_u32(forbidden_byte_mask) >> 1);

									_mm256_mask_storeu_epi16(it_output_current, extra_mask, utf16_packed);

									return {.error = ErrorCode::SURROGATE, .input = current_input_length + extra, .output = current_output_length + extra};
								}
							}

							_mm256_mask_storeu_epi16(it_output_current, mask, utf16_packed);

							it_input_current += remaining;
							it_output_current += remaining;
						}
						else
						{
							const auto out_max_mask = _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(2 * remaining));

							auto out_mask = ~_pdep_u32(saturation_mask, 0xaaaa'aaaa) & out_max_mask;
							const auto surrogate_mask = static_cast<__mmask16>(static_cast<__mmask16>(~saturation_mask) & mask);

							if constexpr (not Correct)
							{
								const auto error_surrogate = _mm512_mask_cmpeq_epi32_mask(saturation_mask, _mm512_and_si512(data, v_0000_f800), v_0000_d800);
								const auto error_too_large = _mm512_mask_cmpgt_epu32_mask(surrogate_mask, data, v_0010_ffff);

								if (error_surrogate or error_too_large)
								{
									const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
									const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

									const auto surrogate_index = std::countr_zero(error_surrogate);
									const auto outside_index = std::countr_zero(error_too_large);

									if (outside_index < surrogate_index)
									{
										out_mask &= _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(2 * outside_index));
										const auto extra = do_write_surrogate(data, surrogate_mask, out_mask);

										return {.error = ErrorCode::TOO_LARGE, .input = current_input_length + outside_index, .output = current_output_length + extra};
									}

									out_mask &= _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(2 * surrogate_index));
									const auto extra = do_write_surrogate(data, surrogate_mask, out_mask);
									return {.error = ErrorCode::SURROGATE, .input = current_input_length + surrogate_index, .output = current_output_length + extra};
								}
							}

							std::ignore = do_write_surrogate(data, surrogate_mask, out_mask);
							it_input_current += remaining;
						}
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}
		}
	}
}

namespace gal::prometheus::chars
{
	namespace latin::icelake
	{
		auto validate(const input_type input) noexcept -> result_error_input_type
		{
			return ::latin::icelake::validate(input);
		}

		auto validate(const pointer_type input) noexcept -> result_error_input_type
		{
			return validate({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_latin(const input_type input) noexcept -> size_type
		{
			return input.size();
		}

		auto length_for_latin(pointer_type input) noexcept -> size_type
		{
			return length_for_latin({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_utf8(const input_type input) noexcept -> size_type
		{
			const auto length = ::latin::icelake::length<CharsType::UTF8_CHAR>(input);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == ::latin::icelake::length<CharsType::UTF8>(input));

			return length;
		}

		auto length_for_utf8(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf8({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_utf16(const input_type input) noexcept -> size_type
		{
			const auto length = ::latin::icelake::length<CharsType::UTF16>(input);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == ::latin::icelake::length<CharsType::UTF16_LE>(input));
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == ::latin::icelake::length<CharsType::UTF16_BE>(input));

			return length;
		}

		auto length_for_utf16(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf16({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_utf32(const input_type input) noexcept -> size_type
		{
			return ::latin::icelake::length<CharsType::UTF32>(input);
		}

		auto length_for_utf32(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf32({input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::latin::icelake::write_utf8<CharsType::UTF8_CHAR, false, false>(output, input);
		}

		auto write_utf8(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_pure(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::latin::icelake::write_utf8<CharsType::UTF8_CHAR, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf8_pure(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_correct(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::latin::icelake::write_utf8<CharsType::UTF8_CHAR, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf8_correct(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf8_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::latin::icelake::write_utf8<CharsType::UTF8, false, false>(output, input);
		}

		auto write_utf8(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_pure(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::latin::icelake::write_utf8<CharsType::UTF8, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf8_pure(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_correct(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::latin::icelake::write_utf8<CharsType::UTF8, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf8_correct(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf8_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::latin::icelake::write_utf16<CharsType::UTF16_LE, false, false>(output, input);
		}

		auto write_utf16_le(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf16_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le_pure(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::latin::icelake::write_utf16<CharsType::UTF16_LE, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf16_le_pure(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_le_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le_correct(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::latin::icelake::write_utf16<CharsType::UTF16_LE, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf16_le_correct(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf16_le_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::latin::icelake::write_utf16<CharsType::UTF16_BE, false, false>(output, input);
		}

		auto write_utf16_be(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf16_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be_pure(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::latin::icelake::write_utf16<CharsType::UTF16_BE, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf16_be_pure(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_be_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be_correct(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::latin::icelake::write_utf16<CharsType::UTF16_BE, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf16_be_correct(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf16_be_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::latin::icelake::write_utf32<CharsType::UTF32, false, false>(output, input);
		}

		auto write_utf32(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf32(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_pure(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::latin::icelake::write_utf32<CharsType::UTF32, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf32_pure(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf32_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_correct(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::latin::icelake::write_utf32<CharsType::UTF32, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf32_correct(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf32_correct(output, {input, std::char_traits<char_type>::length(input)});
		}
	}

	namespace utf8_char::icelake
	{
		auto validate(const input_type input) noexcept -> result_error_input_type
		{
			return ::utf8::icelake::validate<CharsType::UTF8_CHAR>(input);
		}

		auto validate(const pointer_type input) noexcept -> result_error_input_type
		{
			return validate({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_latin(const input_type input) noexcept -> size_type
		{
			return ::utf8::icelake::length<CharsType::UTF8_CHAR, CharsType::LATIN>(input);
		}

		auto length_for_latin(const pointer_type input) noexcept -> size_type
		{
			return length_for_latin({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_utf8(const input_type input) noexcept -> size_type
		{
			return input.size();
		}

		auto length_for_utf8(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf8({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_utf16(const input_type input) noexcept -> size_type
		{
			const auto length = ::utf8::icelake::length<CharsType::UTF8_CHAR, CharsType::UTF16>(input);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((length == ::utf8::icelake::length<CharsType::UTF8_CHAR, CharsType::UTF16_LE>(input)));
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((length == ::utf8::icelake::length<CharsType::UTF8_CHAR, CharsType::UTF16_BE>(input)));

			return length;
		}

		auto length_for_utf16(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf16({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_utf32(const input_type input) noexcept -> size_type
		{
			return ::utf8::icelake::length<CharsType::UTF8_CHAR, CharsType::UTF32>(input);
		}

		auto length_for_utf32(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf32({input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf8::icelake::write_latin<CharsType::UTF8_CHAR, CharsType::LATIN, false, false>(output, input);
		}

		auto write_latin(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_latin(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_pure(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf8::icelake::write_latin<CharsType::UTF8_CHAR, CharsType::LATIN, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_latin_pure(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_latin_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_correct(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf8::icelake::write_latin<CharsType::UTF8_CHAR, CharsType::LATIN, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_latin_correct(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_latin_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf8::icelake::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_LE, false, false>(output, input);
		}

		auto write_utf16_le(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf16_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le_pure(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf8::icelake::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_LE, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf16_le_pure(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_le_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le_correct(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf8::icelake::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_LE, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf16_le_correct(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf16_le_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf8::icelake::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_BE, false, false>(output, input);
		}

		auto write_utf16_be(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf16_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be_pure(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf8::icelake::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_BE, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf16_be_pure(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_be_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be_correct(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf8::icelake::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_BE, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf16_be_correct(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf16_be_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf8::icelake::write_utf32<CharsType::UTF8_CHAR, CharsType::UTF32, false, false>(output, input);
		}

		auto write_utf32(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf32(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_pure(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf8::icelake::write_utf32<CharsType::UTF8_CHAR, CharsType::UTF32, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf32_pure(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf32_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_correct(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf8::icelake::write_utf32<CharsType::UTF8_CHAR, CharsType::UTF32, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf32_correct(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf32_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			return ::utf8::icelake::transform<CharsType::UTF8_CHAR, CharsType::UTF8>(output, input);
		}

		auto write_utf8(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
		}
	}

	namespace utf8::icelake
	{
		auto validate(const input_type input) noexcept -> result_error_input_type
		{
			return ::utf8::icelake::validate<CharsType::UTF8>(input);
		}

		auto validate(const pointer_type input) noexcept -> result_error_input_type
		{
			return validate({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_latin(const input_type input) noexcept -> size_type
		{
			return ::utf8::icelake::length<CharsType::UTF8, CharsType::LATIN>(input);
		}

		auto length_for_latin(const pointer_type input) noexcept -> size_type
		{
			return length_for_latin({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_utf8(const input_type input) noexcept -> size_type
		{
			return input.size();
		}

		auto length_for_utf8(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf8({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_utf16(const input_type input) noexcept -> size_type
		{
			const auto length = ::utf8::icelake::length<CharsType::UTF8, CharsType::UTF16>(input);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((length == ::utf8::icelake::length<CharsType::UTF8, CharsType::UTF16_LE>(input)));
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((length == ::utf8::icelake::length<CharsType::UTF8, CharsType::UTF16_BE>(input)));

			return length;
		}

		auto length_for_utf16(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf16({input, std::char_traits<char_type>::length(input)});
		}

		auto length_for_utf32(const input_type input) noexcept -> size_type
		{
			return ::utf8::icelake::length<CharsType::UTF8, CharsType::UTF32>(input);
		}

		auto length_for_utf32(pointer_type input) noexcept -> size_type
		{
			return length_for_utf32({input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf8::icelake::write_latin<CharsType::UTF8, CharsType::LATIN, false, false>(output, input);
		}

		auto write_latin(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_latin(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_pure(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf8::icelake::write_latin<CharsType::UTF8, CharsType::LATIN, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_latin_pure(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_latin_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_correct(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf8::icelake::write_latin<CharsType::UTF8, CharsType::LATIN, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_latin_correct(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_latin_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf8::icelake::write_utf16<CharsType::UTF8, CharsType::UTF16_LE, false, false>(output, input);
		}

		auto write_utf16_le(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf16_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le_pure(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf8::icelake::write_utf16<CharsType::UTF8, CharsType::UTF16_LE, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf16_le_pure(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_le_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le_correct(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf8::icelake::write_utf16<CharsType::UTF8, CharsType::UTF16_LE, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf16_le_correct(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf16_le_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf8::icelake::write_utf16<CharsType::UTF8, CharsType::UTF16_BE, false, false>(output, input);
		}

		auto write_utf16_be(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf16_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be_pure(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf8::icelake::write_utf16<CharsType::UTF8, CharsType::UTF16_BE, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf16_be_pure(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_be_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be_correct(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf8::icelake::write_utf16<CharsType::UTF8, CharsType::UTF16_BE, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf16_be_correct(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf16_be_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf8::icelake::write_utf32<CharsType::UTF8, CharsType::UTF32, false, false>(output, input);
		}

		auto write_utf32(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf32(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_pure(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf8::icelake::write_utf32<CharsType::UTF8, CharsType::UTF32, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf32_pure(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf32_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_correct(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf8::icelake::write_utf32<CharsType::UTF8, CharsType::UTF32, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf32_correct(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf32_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			return ::utf8::icelake::transform<CharsType::UTF8, CharsType::UTF8_CHAR>(output, input);
		}

		auto write_utf8(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
		}
	}

	namespace utf16::icelake
	{
		[[nodiscard]] auto validate_le(const input_type input) noexcept -> result_error_input_type
		{
			return ::utf16::icelake::validate<CharsType::UTF16_LE>(input);
		}

		[[nodiscard]] auto validate_le(const pointer_type input) noexcept -> result_error_input_type
		{
			return validate_le({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto validate_be(const input_type input) noexcept -> result_error_input_type
		{
			return ::utf16::icelake::validate<CharsType::UTF16_BE>(input);
		}

		[[nodiscard]] auto validate_be(const pointer_type input) noexcept -> result_error_input_type
		{
			return validate_be({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_le_for_latin(const input_type input) noexcept -> size_type
		{
			return ::utf16::icelake::length<CharsType::UTF16_LE, CharsType::LATIN>(input);
		}

		[[nodiscard]] auto length_le_for_latin(const pointer_type input) noexcept -> size_type
		{
			return length_le_for_latin({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_be_for_latin(const input_type input) noexcept -> size_type
		{
			return ::utf16::icelake::length<CharsType::UTF16_BE, CharsType::LATIN>(input);
		}

		[[nodiscard]] auto length_be_for_latin(const pointer_type input) noexcept -> size_type
		{
			return length_be_for_latin({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_le_for_utf8(const input_type input) noexcept -> size_type
		{
			return ::utf16::icelake::length<CharsType::UTF16_LE, CharsType::UTF8_CHAR>(input);
		}

		[[nodiscard]] auto length_le_for_utf8(const pointer_type input) noexcept -> size_type
		{
			return length_le_for_utf8({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_be_for_utf8(const input_type input) noexcept -> size_type
		{
			return ::utf16::icelake::length<CharsType::UTF16_BE, CharsType::UTF8_CHAR>(input);
		}

		[[nodiscard]] auto length_be_for_utf8(const pointer_type input) noexcept -> size_type
		{
			return length_be_for_utf8({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_for_utf16(const input_type input) noexcept -> size_type
		{
			return input.size();
		}

		[[nodiscard]] auto length_for_utf16(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf16({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_le_for_utf32(const input_type input) noexcept -> size_type
		{
			return ::utf16::icelake::length<CharsType::UTF16_LE, CharsType::UTF32>(input);
		}

		[[nodiscard]] auto length_le_for_utf32(const pointer_type input) noexcept -> size_type
		{
			return length_le_for_utf32({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_be_for_utf32(const input_type input) noexcept -> size_type
		{
			return ::utf16::icelake::length<CharsType::UTF16_BE, CharsType::UTF32>(input);
		}

		[[nodiscard]] auto length_be_for_utf32(const pointer_type input) noexcept -> size_type
		{
			return length_be_for_utf32({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_latin_le(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf16::icelake::write_latin<CharsType::UTF16_LE, CharsType::LATIN, false, false>(output, input);
		}

		[[nodiscard]] auto write_latin_le(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_latin_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_latin_be(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf16::icelake::write_latin<CharsType::UTF16_BE, CharsType::LATIN, false, false>(output, input);
		}

		[[nodiscard]] auto write_latin_be(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_latin_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_pure_le(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf16::icelake::write_latin<CharsType::UTF16_LE, CharsType::LATIN, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_latin_pure_le(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_latin_pure_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_pure_be(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf16::icelake::write_latin<CharsType::UTF16_BE, CharsType::LATIN, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_latin_pure_be(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_latin_pure_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_correct_le(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf16::icelake::write_latin<CharsType::UTF16_LE, CharsType::LATIN, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_latin_correct_le(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_latin_correct_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_correct_be(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf16::icelake::write_latin<CharsType::UTF16_BE, CharsType::LATIN, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_latin_correct_be(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_latin_correct_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf8_le(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf16::icelake::write_utf8<CharsType::UTF16_LE, CharsType::UTF8_CHAR, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf8_le(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf8_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf8_be(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf16::icelake::write_utf8<CharsType::UTF16_BE, CharsType::UTF8_CHAR, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf8_be(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf8_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_pure_le(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf16::icelake::write_utf8<CharsType::UTF16_LE, CharsType::UTF8_CHAR, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf8_pure_le(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8_pure_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_pure_be(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf16::icelake::write_utf8<CharsType::UTF16_BE, CharsType::UTF8_CHAR, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf8_pure_be(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8_pure_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_correct_le(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf16::icelake::write_utf8<CharsType::UTF16_LE, CharsType::UTF8_CHAR, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf8_correct_le(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf8_correct_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_correct_be(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf16::icelake::write_utf8<CharsType::UTF16_BE, CharsType::UTF8_CHAR, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf8_correct_be(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf8_correct_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf8_le(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf16::icelake::write_utf8<CharsType::UTF16_LE, CharsType::UTF8, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf8_le(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf8_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf8_be(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf16::icelake::write_utf8<CharsType::UTF16_BE, CharsType::UTF8, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf8_be(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf8_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_pure_le(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf16::icelake::write_utf8<CharsType::UTF16_LE, CharsType::UTF8, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf8_pure_le(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8_pure_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_pure_be(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf16::icelake::write_utf8<CharsType::UTF16_BE, CharsType::UTF8, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf8_pure_be(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8_pure_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_correct_le(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf16::icelake::write_utf8<CharsType::UTF16_LE, CharsType::UTF8, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf8_correct_le(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf8_correct_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_correct_be(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf16::icelake::write_utf8<CharsType::UTF16_BE, CharsType::UTF8, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf8_correct_be(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf8_correct_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf32_le(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf16::icelake::write_utf32<CharsType::UTF16_LE, CharsType::UTF32, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf32_le(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf32_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf32_be(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf16::icelake::write_utf32<CharsType::UTF16_BE, CharsType::UTF32, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf32_be(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf32_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_pure_le(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf16::icelake::write_utf32<CharsType::UTF16_LE, CharsType::UTF32, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf32_pure_le(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf32_pure_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_pure_be(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf16::icelake::write_utf32<CharsType::UTF16_BE, CharsType::UTF32, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf32_pure_be(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf32_pure_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_correct_le(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf16::icelake::write_utf32<CharsType::UTF16_LE, CharsType::UTF32, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf32_correct_le(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf32_correct_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf32_correct_be(
			const output_type_of<CharsType::UTF32>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf16::icelake::write_utf32<CharsType::UTF16_BE, CharsType::UTF32, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf32_correct_be(
			const output_type_of<CharsType::UTF32>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf32_correct_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf16_le(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			return ::utf16::icelake::transform<CharsType::UTF16_LE, CharsType::UTF16_BE>(output, input);
		}

		[[nodiscard]] auto write_utf16_le(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf16_be(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			return ::utf16::icelake::transform<CharsType::UTF16_BE, CharsType::UTF16_LE>(output, input);
		}

		[[nodiscard]] auto write_utf16_be(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto flip(
			const output_type_of<CharsType::UTF16>::pointer output,
			const input_type input
		) noexcept -> void
		{
			return ::utf16::icelake::flip(output, input);
		}

		auto flip(
			const output_type_of<CharsType::UTF16>::pointer output,
			const pointer_type input
		) noexcept -> void
		{
			return flip(output, {input, std::char_traits<char_type>::length(input)});
		}
	}

	namespace utf32::icelake
	{
		[[nodiscard]] auto validate(const input_type input) noexcept -> result_error_input_type
		{
			return ::utf32::icelake::validate(input);
		}

		[[nodiscard]] auto validate(const pointer_type input) noexcept -> result_error_input_type
		{
			return validate({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_for_latin(const input_type input) noexcept -> size_type
		{
			return ::utf32::icelake::length<CharsType::LATIN>(input);
		}

		[[nodiscard]] auto length_for_latin(const pointer_type input) noexcept -> size_type
		{
			return length_for_latin({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_for_utf8(const input_type input) noexcept -> size_type
		{
			const auto length = ::utf32::icelake::length<CharsType::UTF8_CHAR>(input);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == ::utf32::icelake::length<CharsType::UTF8>(input));

			return length;
		}

		[[nodiscard]] auto length_for_utf8(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf8({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_for_utf16(const input_type input) noexcept -> size_type
		{
			const auto length = ::utf32::icelake::length<CharsType::UTF16>(input);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == ::utf32::icelake::length<CharsType::UTF16_LE>(input));
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == ::utf32::icelake::length<CharsType::UTF16_BE>(input));

			return length;
		}

		[[nodiscard]] auto length_for_utf16(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf16({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto length_for_utf32(const input_type input) noexcept -> size_type
		{
			return input.size();
		}

		[[nodiscard]] auto length_for_utf32(const pointer_type input) noexcept -> size_type
		{
			return length_for_utf32({input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_latin(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf32::icelake::write_latin<CharsType::LATIN, false, false>(output, input);
		}

		[[nodiscard]] auto write_latin(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_latin(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_pure(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf32::icelake::write_latin<CharsType::LATIN, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_latin_pure(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_latin_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_latin_correct(
			const output_type_of<CharsType::LATIN>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf32::icelake::write_latin<CharsType::LATIN, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_latin_correct(
			const output_type_of<CharsType::LATIN>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_latin_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf8(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf32::icelake::write_utf8<CharsType::UTF8_CHAR, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf8(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_pure(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf32::icelake::write_utf8<CharsType::UTF8_CHAR, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf8_pure(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_correct(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf32::icelake::write_utf8<CharsType::UTF8_CHAR, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf8_correct(
			const output_type_of<CharsType::UTF8_CHAR>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf8_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf8(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf32::icelake::write_utf8<CharsType::UTF8, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf8(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_pure(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf32::icelake::write_utf8<CharsType::UTF8, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf8_pure(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf8_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf8_correct(
			const output_type_of<CharsType::UTF8>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf32::icelake::write_utf8<CharsType::UTF8, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf8_correct(
			const output_type_of<CharsType::UTF8>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf8_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf16_le(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf32::icelake::write_utf16<CharsType::UTF16_LE, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf16_le(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf16_le(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le_pure(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf32::icelake::write_utf16<CharsType::UTF16_LE, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf16_le_pure(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_le_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_le_correct(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf32::icelake::write_utf16<CharsType::UTF16_LE, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf16_le_correct(
			const output_type_of<CharsType::UTF16_LE>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf16_le_correct(output, {input, std::char_traits<char_type>::length(input)});
		}

		[[nodiscard]] auto write_utf16_be(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_output_type
		{
			return ::utf32::icelake::write_utf16<CharsType::UTF16_BE, false, false>(output, input);
		}

		[[nodiscard]] auto write_utf16_be(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_output_type
		{
			return write_utf16_be(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be_pure(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_error_input_type
		{
			const auto result = ::utf32::icelake::write_utf16<CharsType::UTF16_BE, true, false>(output, input);
			return {.error = result.error, .input = result.input};
		}

		auto write_utf16_be_pure(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_error_input_type
		{
			return write_utf16_be_pure(output, {input, std::char_traits<char_type>::length(input)});
		}

		auto write_utf16_be_correct(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const input_type input
		) noexcept -> result_output_type
		{
			const auto result = ::utf32::icelake::write_utf16<CharsType::UTF16_BE, false, true>(output, input);
			return {.output = result.output};
		}

		auto write_utf16_be_correct(
			const output_type_of<CharsType::UTF16_BE>::pointer output,
			const pointer_type input
		) noexcept -> result_output_type
		{
			return write_utf16_be_correct(output, {input, std::char_traits<char_type>::length(input)});
		}
	}

	[[nodiscard]] auto Icelake::encoding_of(const std::span<const char8_t> input) noexcept -> EncodingType
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

		if (const auto bom = bom_of(input);
			bom != EncodingType::UNKNOWN)
		{
			return bom;
		}

		const auto input_length = input.size();

		const auto it_input_begin = input.data();
		auto it_input_current = it_input_begin;
		const auto it_input_end = it_input_begin + input_length;

		// utf8
		bool utf8 = true;
		detail::utf8::icelake::avx512_utf8_checker checker{};
		// utf16
		bool utf16 = (input_length % 2) == 0;
		bool utf16_ends_with_high = false;
		// utf32
		bool utf32 = (input_length % 4) == 0;

		const auto do_check = [&]<bool Tail>(const data_type data) noexcept -> void
		{
			// utf8
			const auto offset = _mm512_set1_epi32(static_cast<int>(0xffff'2000));
			// utf16
			const auto v_d800 = _mm512_set1_epi16(static_cast<short>(0xd800));
			const auto v_0800 = _mm512_set1_epi16(0x0800);
			const auto v_0400 = _mm512_set1_epi16(0x0400);
			// utf32
			// const auto offset = _mm512_set1_epi32(static_cast<int>(0xffff'2000));
			const auto standard_max = _mm512_set1_epi32(0x0010'ffff);
			const auto standard_offset_max = _mm512_set1_epi32(static_cast<int>(0xffff'f7ff));

			// ::utf8::icelake::validate()
			if (utf8)
			{
				if (not checker.check_data(data))
				{
					if constexpr (Tail)
					{
						checker.check_eof();
					}

					if (checker.has_error())
					{
						utf8 = false;
					}
				}
			}

			// ::utf16::icelake::validate()
			if (utf16)
			{
				const auto diff = _mm512_sub_epi16(data, v_d800);
				if (const auto surrogates = _mm512_cmplt_epu16_mask(diff, v_0800);
					surrogates)
				{
					const auto high_surrogates = _mm512_cmplt_epu16_mask(diff, v_0400);
					const auto low_surrogates = surrogates ^ high_surrogates;

					if (((high_surrogates << 1) | +utf16_ends_with_high) != low_surrogates)
					{
						utf16 = false;
					}
					utf16_ends_with_high = (high_surrogates & 0x8000'0000) != 0;
				}
			}

			// ::utf32::icelake::validate()
			if (utf32)
			{
				const auto value_offset = _mm512_add_epi32(data, offset);

				const auto outside_range = _mm512_cmpgt_epu32_mask(data, standard_max);
				const auto surrogate_range = _mm512_cmpgt_epu32_mask(value_offset, standard_offset_max);

				if (outside_range | surrogate_range)
				{
					utf32 = false;
				}
			}
		};

		while (it_input_current + ::utf8::advance_of_utf8 <= it_input_end)
		{
			#if GAL_PROMETHEUS_COMPILER_DEBUG
			[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + ::utf8::advance_of_utf8};
			#endif

			const auto data = _mm512_loadu_si512(it_input_current);

			do_check.operator()<false>(data);

			it_input_current += ::utf8::advance_of_utf8;
		}

		const auto remaining = it_input_end - it_input_current;
		GAL_PROMETHEUS_ERROR_ASSUME(static_cast<std::size_t>(remaining) < ::utf8::advance_of_utf8);

		if (remaining != 0)
		{
			#if GAL_PROMETHEUS_COMPILER_DEBUG
			[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
			#endif

			const auto mask = _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(remaining));
			const auto data = _mm512_maskz_loadu_epi8(mask, it_input_current);

			do_check.operator()<true>(data);

			it_input_current += remaining;
		}

		// ==================================================
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);

		auto all_possible = std::to_underlying(EncodingType::UNKNOWN);

		if (utf8)
		{
			all_possible |= std::to_underlying(EncodingType::UTF8);
		}

		if (utf16)
		{
			all_possible |= std::to_underlying(EncodingType::UTF16_LE);
		}

		if (utf32)
		{
			all_possible |= std::to_underlying(EncodingType::UTF32_LE);
		}

		return static_cast<EncodingType>(all_possible);
	}

	[[nodiscard]] auto Icelake::encoding_of(const std::span<const char> input) noexcept -> EncodingType
	{
		static_assert(sizeof(char) == sizeof(char8_t));

		const auto* char8_string = GAL_PROMETHEUS_SEMANTIC_UNRESTRICTED_CHAR_POINTER_CAST(char8_t, input.data());
		return encoding_of({char8_string, input.size()});
	}
}

#endif
