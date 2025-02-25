// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

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
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <chars/detail/icelake.utf32.hpp>

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
	}

	namespace latin
	{
		using input_type = chars::latin::input_type;
		// using char_type = chars::latin::char_type;
		using size_type = chars::latin::size_type;
		using pointer_type = chars::latin::pointer_type;

		template<std::endian SourceEndian>
		[[nodiscard]] constexpr auto to_native_utf16(const data_type data) noexcept -> data_type
		{
			if constexpr (SourceEndian != std::endian::native)
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

		template<CharsType Type>
			requires (Type == CharsType::UTF16_LE or Type == CharsType::UTF16_BE)
		[[nodiscard]] constexpr auto to_native_utf16(const data_type data) noexcept -> data_type
		{
			return latin::to_native_utf16<Type == CharsType::UTF16_LE ? std::endian::little : std::endian::big>(data);
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto advance_of() noexcept -> std::ptrdiff_t
		{
			return sizeof(data_type) / sizeof(typename input_type_of<OutputType>::value_type);
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto mask_of(const std::size_t length) noexcept -> auto
		{
			if constexpr (
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
			{
				// ReSharper disable once CppRedundantCastExpression
				const auto m_64 = static_cast<__mmask64>(_bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(length)));
				return m_64;
			}
			else if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE or
				OutputType == CharsType::UTF16
			)
			{
				// ReSharper disable once CppRedundantCastExpression
				const auto m_32 = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));
				return m_32;
			}
			else if constexpr (
				OutputType == CharsType::UTF32
			)
			{
				const auto m_16 = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));
				return m_16;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto read(const pointer_type source) noexcept -> data_type
		{
			if constexpr (
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
			{
				return _mm512_loadu_si512(source);
			}
			else if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE or
				OutputType == CharsType::UTF16
			)
			{
				const auto m256 = _mm256_loadu_si256(
					GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(
						const __m256i *,
						source
					)
				);

				// Zero extend each set of 8 ascii characters to 32 16-bit integers
				return _mm512_cvtepu8_epi16(m256);
			}
			else if constexpr (
				OutputType == CharsType::UTF32
			)
			{
				const auto m128 = _mm_loadu_si128(
					GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(
						const __m128i *,
						source
					)
				);

				// Zero extend each set of 8 ascii characters to 16 32-bit integers
				return _mm512_cvtepu8_epi32(m128);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto read(const pointer_type source, const std::size_t length) noexcept -> data_type
		{
			if constexpr (const auto mask = latin::mask_of<OutputType>(length);
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
			{
				return _mm512_maskz_loadu_epi8(mask, source);
			}
			else if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE or
				OutputType == CharsType::UTF16
			)
			{
				const auto m256 = _mm256_maskz_loadu_epi8(mask, source);

				// Zero extend each set of 8 ascii characters to 32 16-bit integers
				return _mm512_cvtepu8_epi16(m256);
			}
			else if constexpr (
				OutputType == CharsType::UTF32
			)
			{
				const auto m128 = _mm_maskz_loadu_epi8(mask, source);

				// Zero extend each set of 8 ascii characters to 16 32-bit integers
				return _mm512_cvtepu8_epi32(m128);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		template<CharsType OutputType>
		constexpr auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data
		) noexcept -> void
		{
			constexpr auto advance = latin::advance_of<OutputType>();

			if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE
			)
			{
				const auto native_data = latin::to_native_utf16<OutputType>(data);
				_mm512_storeu_si512(output, native_data);
			}
			else
			{
				_mm512_storeu_si512(output, data);
			}

			output += advance;
		}

		template<CharsType OutputType>
		constexpr auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> void
		{
			constexpr auto advance = latin::advance_of<OutputType>();
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length < advance);

			if constexpr (const auto mask = latin::mask_of<OutputType>(length);
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
			{
				_mm512_mask_storeu_epi8(output, mask, data);
			}
			else if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE
			)
			{
				const auto native_data = latin::to_native_utf16<OutputType>(data);
				_mm512_mask_storeu_epi16(output, mask, native_data);
			}
			else if constexpr (
				OutputType == CharsType::UTF32
			)
			{
				_mm512_mask_storeu_epi32(output, mask, data);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}

			output += length;
		}

		namespace icelake
		{
			[[nodiscard]] constexpr auto validate(const input_type input) noexcept -> result_error_input_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				constexpr auto advance = latin::advance_of<CharsType::LATIN>();

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = latin::read<CharsType::LATIN>(it_input_current);

					if (const auto sign = common::sign_of<CharsType::LATIN>(data);
						not sign.pure())
					{
						it_input_current += sign.start_count();

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

						return {.error = ErrorCode::TOO_LARGE, .input = current_input_length};
					}

					it_input_current += advance;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto data = latin::read<CharsType::LATIN>(it_input_current, static_cast<std::size_t>(remaining));

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
					constexpr auto advance = latin::advance_of<CharsType::LATIN>();

					const auto input_length = input.size();

					const pointer_type it_input_begin = input.data();
					pointer_type it_input_current = it_input_begin;
					const pointer_type it_input_end = it_input_begin + input_length;

					// number of 512-bit chunks that fits into the length
					size_type output_length = input_length / advance * advance;

					while (it_input_current + advance <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
						#endif

						const auto data = latin::read<OutputType>(it_input_current);

						if (const auto sign = common::sign_of<CharsType::LATIN>(data);
							not sign.pure())
						{
							output_length += sign.count();
						}

						it_input_current += advance;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

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

				const auto advance = latin::advance_of<OutputType>();
				const auto transform = [&]<bool MaskOut>(const data_type data, const std::size_t data_length) noexcept -> void
				{
					if constexpr (not MaskOut)
					{
						GAL_PROMETHEUS_ERROR_ASSUME(data_length == static_cast<std::size_t>(advance));
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

					const auto out_size = static_cast<unsigned int>(data_length + std::popcount(non_ascii));
					const auto out_size_low = 32 + static_cast<unsigned int>(std::popcount(non_ascii_low));

					const auto do_write_full = [&]() noexcept -> void
					{
						const auto out_size_high = static_cast<unsigned int>(data_length - 32) + static_cast<unsigned int>(std::popcount(non_ascii_high));

						latin::write<OutputType>(it_output_current, output_low, out_size_low);
						latin::write<OutputType>(it_output_current, output_high, out_size_high);
					};

					if constexpr (MaskOut)
					{
						// is the second half of the input vector used?
						if (data_length > 32)
						{
							do_write_full();
						}
						else
						{
							latin::write<OutputType>(it_output_current, output_low, out_size);
						}
					}
					else
					{
						do_write_full();
					}

					it_input_current += data_length;
				};
				const auto write_pure = functional::overloaded{
						[&](const data_type data) noexcept -> void
						{
							latin::write<OutputType>(it_output_current, data);
						},
						[&](const data_type data, const std::size_t data_length) noexcept -> void
						{
							latin::write<OutputType>(it_output_current, data, data_length);
						}
				};

				// if there's at least 128 bytes remaining, we don't need to mask the output
				while (it_input_current + 2 * advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
					#endif

					const auto data = latin::read<OutputType>(it_input_current);

					if constexpr (Pure)
					{
						write_pure(data);
						it_input_current += advance;
					}
					else
					{
						if (const auto sign = common::sign_of<CharsType::LATIN>(data);
							sign.pure())
						{
							write_pure(data);
							it_input_current += advance;
						}
						else
						{
							transform.template operator()<false>(data, advance);
						}
					}
				}

				// in the last 128 bytes, the first 64 may require masking the output
				if (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
					#endif

					const auto data = latin::read<OutputType>(it_input_current);

					if constexpr (Pure)
					{
						write_pure(data);
						it_input_current += advance;
					}
					else
					{
						transform.template operator()<true>(data, advance);
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					// with the last 64 bytes, the input also needs to be masked
					const auto data = latin::read<OutputType>(it_input_current, static_cast<std::size_t>(remaining));

					if constexpr (Pure)
					{
						write_pure(data, static_cast<std::size_t>(remaining));
						it_input_current += remaining;
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

				const auto advance = latin::advance_of<OutputType>();

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = latin::read<OutputType>(it_input_current);

					latin::write<OutputType>(it_output_current, data);

					it_input_current += advance;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto data = latin::read<OutputType>(it_input_current, static_cast<std::size_t>(remaining));

					latin::write<OutputType>(it_output_current, data, static_cast<std::size_t>(remaining));

					it_input_current += remaining;
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

				const auto advance = latin::advance_of<OutputType>();

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = latin::read<OutputType>(it_input_current);

					latin::write<OutputType>(it_output_current, data);

					it_input_current += advance;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto data = latin::read<OutputType>(it_input_current, static_cast<std::size_t>(remaining));

					latin::write<OutputType>(it_output_current, data, static_cast<std::size_t>(remaining));

					it_input_current += remaining;
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
		template<std::endian SourceEndian>
		[[nodiscard]] constexpr auto to_native_utf16(const data_type data) noexcept -> data_type
		{
			if constexpr (SourceEndian != std::endian::native)
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

		template<CharsType Type>
			requires (Type == CharsType::UTF16_LE or Type == CharsType::UTF16_BE)
		[[nodiscard]] constexpr auto to_native_utf16(const data_type data) noexcept -> data_type
		{
			return latin::to_native_utf16<Type == CharsType::UTF16_LE ? std::endian::little : std::endian::big>(data);
		}

		template<CharsType InputType, CharsType OutputType>
			requires (
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
		[[nodiscard]] constexpr auto advance_of() noexcept -> std::ptrdiff_t
		{
			return sizeof(data_type) / sizeof(typename input_type_of<OutputType>::value_type);
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto mask_of(const std::size_t length) noexcept -> auto
		{
			if constexpr (
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
			{
				// ReSharper disable once CppRedundantCastExpression
				const auto m_64 = static_cast<__mmask64>(_bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(length)));
				return m_64;
			}
			else if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE or
				OutputType == CharsType::UTF16
			)
			{
				// ReSharper disable once CppRedundantCastExpression
				const auto m_32 = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));
				return m_32;
			}
			else if constexpr (
				OutputType == CharsType::UTF32
			)
			{
				const auto m_16 = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));
				return m_16;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		namespace icelake
		{
			// todo
		}
	}

	namespace utf16
	{
		namespace icelake
		{
			// todo
		}
	}

	namespace utf32
	{
		using input_type = chars::utf32::input_type;
		using char_type = chars::utf32::char_type;
		using size_type = chars::utf32::size_type;
		using pointer_type = chars::utf32::pointer_type;

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto advance_of() noexcept -> std::ptrdiff_t
		{
			std::ignore = OutputType;
			return sizeof(data_type) / sizeof(char_type);
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto mask_of(const std::size_t length) noexcept -> auto
		{
			std::ignore = OutputType;

			const auto m_16 = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));

			return m_16;
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto read(const pointer_type source) noexcept -> data_type
		{
			std::ignore = OutputType;

			return _mm512_loadu_si512(source);
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto read(const pointer_type source, const std::size_t length) noexcept -> data_type
		{
			const auto mask = utf32::mask_of<OutputType>(length);

			return _mm512_maskz_loadu_epi32(mask, source);
		}

		// advance_of<OutputType> * 8 bits
		template<CharsType OutputType>
			requires (
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
		constexpr auto write_16_x_8(
			typename output_type_of<OutputType>::pointer& output,
			const __m128i data
		) noexcept -> void
		{
			constexpr auto advance = utf32::advance_of<OutputType>();

			auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m128i*, output);

			_mm_storeu_si128(p, data);
			output += advance;
		}

		// advance_of<OutputType> * 8 bits
		template<CharsType OutputType>
			requires (
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
		constexpr auto write_16_x_8(
			typename output_type_of<OutputType>::pointer& output,
			const __m128i data,
			const std::size_t length
		) noexcept -> void
		{
			constexpr auto advance = utf32::advance_of<OutputType>();
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length < advance);

			const auto mask = utf32::mask_of<OutputType>(length);

			_mm_mask_storeu_epi8(output, mask, data);
			output += length;
		}

		// advance_of<OutputType> * 16 bits
		template<CharsType OutputType>
		constexpr auto write_16_x_16(
			typename output_type_of<OutputType>::pointer& output,
			const __m256i data
		) noexcept -> void
		{
			constexpr auto advance = utf32::advance_of<OutputType>();

			auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(__m256i*, output);

			_mm256_storeu_si256(p, data);
			output += advance;
		}

		namespace icelake
		{
			[[nodiscard]] constexpr auto validate(const input_type input) noexcept -> result_error_input_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				constexpr auto advance = utf32::advance_of<CharsType::UTF32>();

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

					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

					const auto outside_range = _mm512_cmpgt_epu32_mask(data, standard_max);
					const auto surrogate_range = _mm512_cmpgt_epu32_mask(value_offset, standard_offset_max);

					if (outside_range | surrogate_range)
					{
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

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = utf32::read<CharsType::UTF32>(it_input_current);

					if (const auto result = do_check(data);
						result.has_error())
					{
						return result;
					}

					it_input_current += advance;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto data = utf32::read<CharsType::UTF32>(it_input_current, static_cast<std::size_t>(remaining));

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

				constexpr auto advance = utf32::advance_of<CharsType::UTF32>();

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
					while (it_input_current + advance <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
						#endif

						const auto data = utf32::read<OutputType>(it_input_current);

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
						const auto four_bytes_count = advance - ascii_count - two_bytes_count - three_bytes_count;

						output_length += ascii_count + 2 * two_bytes_count + 3 * three_bytes_count + 4 * four_bytes_count;
						it_input_current += advance;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

					if (remaining != 0)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
						#endif

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
					while (it_input_current + advance <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
						#endif

						const auto data = utf32::read<OutputType>(it_input_current);

						const auto surrogates_bitmask = _mm512_cmpgt_epu32_mask(data, v_ffff);
						// 1 + (surrogate ? 1 : 0)
						output_length += advance + std::popcount(surrogates_bitmask);

						it_input_current += advance;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

					if (remaining != 0)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
						#endif

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

				const auto advance = utf32::advance_of<OutputType>();

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

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = utf32::read<OutputType>(it_input_current);

					if (const auto mask = _mm512_cmpgt_epu32_mask(data, v_00ff);
						mask != 0)
					{
						return write_tail_block(mask);
					}

					const auto out = _mm512_castsi512_si128(_mm512_permutexvar_epi8(shuffle_mask, data));
					utf32::write_16_x_8<OutputType>(it_output_current, out);

					it_input_current += advance;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto data = utf32::read<OutputType>(it_input_current, static_cast<std::size_t>(remaining));

					if (const auto mask = _mm512_cmpgt_epu32_mask(data, v_00ff);
						mask != 0)
					{
						return write_tail_block(mask);
					}

					const auto out = _mm512_castsi512_si128(_mm512_permutexvar_epi8(shuffle_mask, data));
					utf32::write_16_x_8<OutputType>(it_output_current, out, remaining);

					it_input_current += remaining;
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

				const auto advance = utf32::advance_of<OutputType>();

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

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
					#endif

					const auto data = utf32::read<OutputType>(it_input_current);
					const auto low = _mm512_castsi512_si256(data);
					const auto high = _mm512_extracti64x4_epi64(data, 1);

					if constexpr (const auto in_max = _mm256_max_epu32(low, high);
						not Correct
					)
					{
						// ReSharper disable CommentTypo

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
						utf32::write_16_x_8<OutputType>(it_output_current, utf8_packed);
						it_input_current += advance;

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

						// 1.prepare 2-bytes values
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

						// 2.merge ascii and 2-bytes codewords
						const auto utf8_unpacked = _mm256_blendv_epi8(t4, in_16, one_byte_byte_mask);

						// 3.prepare bitmask for 8-bits lookup
						const auto mask_0 = static_cast<std::uint32_t>(one_byte_bit_mask & 0x5555'5555);
						const auto mask_1 = static_cast<std::uint32_t>(mask_0 >> 7);
						const auto mask = static_cast<std::uint32_t>((mask_0 | mask_1) & 0x00ff'00ff);

						// 4.pack the bytes
						using type = std::decay_t<decltype(detail::utf32::icelake::utf16_to_utf8::_1_2[0])>;
						// length + data
						static_assert(std::ranges::size(type{}) == 1 + advance);

						const auto index_0 = static_cast<std::uint8_t>(mask);
						const auto index_1 = static_cast<std::uint8_t>(mask >> 16);

						const auto& data_0 = detail::utf32::icelake::utf16_to_utf8::_1_2[index_0];
						const auto& data_1 = detail::utf32::icelake::utf16_to_utf8::_1_2[index_1];

						const auto length_0 = data_0.front();
						const auto length_1 = data_1.front();
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_0 <= advance);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_1 <= advance);

						const auto* row_0 = data_0.data() + 1;
						const auto* row_1 = data_1.data() + 1;

						const auto shuffle_0 = _mm_loadu_si128(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_0));
						const auto shuffle_1 = _mm_loadu_si128(GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(const __m128i*, row_1));

						const auto utf8_packed = _mm256_shuffle_epi8(utf8_unpacked, _mm256_setr_m128i(shuffle_0, shuffle_1));

						// 5.store the bytes
						utf32::write_16_x_8<OutputType>(it_output_current, _mm256_castsi256_si128(utf8_packed));
						it_output_current -= (advance - length_0);
						utf32::write_16_x_8<OutputType>(it_output_current, _mm256_extracti128_si256(utf8_packed, 1));
						it_output_current -= (advance - length_1);

						it_input_current += advance;

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
						// However, we need five distinct bit layouts. Note that the last byte in cases #2 and #3 is the same.

						// We precompute byte 1 for case #1 and the common byte for cases #2 and #3 in register t2.
						// We precompute byte 1 for case #3 and -- **conditionally** -- precompute either byte 1 for case #2 or byte 2 for case #3. Note that they differ by exactly one bit.
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
						static_assert(std::ranges::size(type{}) == 1 + advance);

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
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_0 <= advance);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_1 <= advance);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_2 <= advance);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length_3 <= advance);

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

						utf32::write_16_x_8<OutputType>(it_output_current, utf8_0);
						it_output_current -= (advance - length_0);
						utf32::write_16_x_8<OutputType>(it_output_current, utf8_1);
						it_output_current -= (advance - length_1);
						utf32::write_16_x_8<OutputType>(it_output_current, utf8_2);
						it_output_current -= (advance - length_2);
						utf32::write_16_x_8<OutputType>(it_output_current, utf8_3);
						it_output_current -= (advance - length_3);

						it_input_current += advance;

						// we are done for this round
						continue;
					}

					// at least one 32-bit word is larger than 0xffff => it will produce four UTF-8 bytes (like emoji)
					// scalar fallback
					const auto fallback_end = it_input_current + advance;
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
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

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

				constexpr auto not_native_endian = (OutputType == CharsType::UTF16_LE) != (std::endian::native == std::endian::little);

				const auto advance = utf32::advance_of<OutputType>();

				// ReSharper disable CppInconsistentNaming
				// ReSharper disable IdentifierTypo

				const auto v_ffff_0000 = _mm512_set1_epi32(static_cast<int>(0xffff'0000));
				const auto v_0000_0000 = _mm512_setzero_si512();
				const auto v_0000_f800 = _mm512_set1_epi16(static_cast<short>(0x0000'f800));
				const auto v_0000_d800 = _mm512_set1_epi16(static_cast<short>(0x0000'd800));
				const auto v_7fff_ffff = _mm256_set1_epi32(0x7fff'ffff);

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

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
					#endif

					const auto data = utf32::read<OutputType>(it_input_current);
					const auto low = _mm512_castsi512_si256(data);
					const auto high = _mm512_extracti64x4_epi64(data, 1);

					// no bits set above 16th bit => can pack to UTF16 without surrogate pairs
					if (const auto saturation_mask = _mm512_cmpeq_epi32_mask(_mm512_and_si512(data, v_ffff_0000), v_0000_0000);
						saturation_mask == 0xffff)
					{
						if (const auto mask = _mm512_cmpeq_epi32_mask(_mm512_and_si512(data, v_0000_f800), v_0000_d800);
							mask != 0)
						{
							return write_tail_block(mask, ErrorCode::SURROGATE);
						}

						const auto in_16_packed = _mm256_packus_epi32(
							_mm256_and_si256(low, v_7fff_ffff),
							_mm256_and_si256(high, v_7fff_ffff)
						);
						const auto in_16 = [](const auto p) noexcept
						{
							if constexpr (not_native_endian)
							{
								const auto shuffle_mask = _mm256_setr_epi8(
									// clang-format off
								    01, 00, 03, 02, 05, 04, 07, 06,
								    9u, 8u, 11, 10, 13, 12, 15, 14,
								    17, 16, 19, 18, 21, 20, 23, 22,
								    25, 24, 27, 26, 29, 28, 31, 30
									// clang-format on
								);

								return _mm256_shuffle_epi8(p, shuffle_mask);
							}
							else
							{
								return p;
							}
						}(in_16_packed);

						utf32::write_16_x_16<OutputType>(it_output_current, in_16);

						it_input_current += advance;
					}
					else
					{
						// scalar fallback
						const auto fallback_end = it_input_current + advance;
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
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					// scalar fallback
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
}
