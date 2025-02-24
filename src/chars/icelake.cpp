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

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto advance_of() noexcept -> std::ptrdiff_t
		{
			std::ignore = OutputType;

			return sizeof(data_type) / sizeof(typename input_type_of<OutputType>::value_type);
		}

		template<CharsType OutputType>
		constexpr auto mask_of(const std::size_t length) noexcept -> auto
		{
			// ReSharper disable once CppRedundantCastExpression
			const auto m_64 = static_cast<__mmask64>(_bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(length)));
			// ReSharper disable once CppRedundantCastExpression
			const auto m_32 = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));
			const auto m_16 = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));

			if constexpr (
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
			{
				return m_64;
			}
			else if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE or
				OutputType == CharsType::UTF16
			)
			{
				return m_32;
			}
			else if constexpr (
				OutputType == CharsType::UTF32
			)
			{
				return m_16;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
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
			if constexpr (const auto mask = common::mask_of<OutputType>(length);
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
			constexpr auto advance = common::advance_of<OutputType>();

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
			constexpr auto advance = common::advance_of<OutputType>();
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length < advance);

			if constexpr (const auto mask = common::mask_of<OutputType>(length);
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

				constexpr auto advance = common::advance_of<CharsType::LATIN>();

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
					constexpr auto advance = common::advance_of<CharsType::LATIN>();

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
						output_length += chars::latin::scalar::length_for_utf8({it_input_current, static_cast<std::size_t>(remaining)});
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

				const auto advance = common::advance_of<OutputType>();
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

				const auto advance = common::advance_of<OutputType>();

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

				const auto advance = common::advance_of<OutputType>();

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
}
