// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if defined(GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED)

#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

#include <chars/encoding.hpp>

namespace gal::prometheus::chars
{
	struct icelake_block
	{
		using data_type = __m512i;
		using mask_type = __mmask64;

		// ===============================
		// READ

		/**
		 * @note When processing in SIMD mode, data `block` is used for reading and writing,
		 * which means that the number of characters processed depends on the largest (character) size of the input/output.
		 */
		template<CharsType InputType, CharsType OutputType>
		[[nodiscard]] constexpr static auto advance_of() noexcept -> std::ptrdiff_t
		{
			constexpr auto i = sizeof(typename input_type_of<InputType>::value_type);
			constexpr auto o = sizeof(typename input_type_of<OutputType>::value_type);
			return sizeof(data_type) / std::ranges::max(i, o);
		}

		/**
		 * @return (1 << length) - 1
		 */
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			if constexpr (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				// ReSharper disable once CppRedundantCastExpression
				return static_cast<__mmask64>(_bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(length)));
			}
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE or
				InputType == CharsType::UTF16
			)
			{
				// ReSharper disable once CppRedundantCastExpression
				return static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));
			}
			else if constexpr (
				InputType == CharsType::UTF32
			)
			{
				return static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance_of<InputType, OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing,
		 * which means that the number of characters processed depends on the largest (character) size of the input/output.
		 */
		template<CharsType InputType, CharsType OutputType>
		[[nodiscard]] constexpr static auto read(const typename input_type_of<InputType>::pointer source) noexcept -> data_type
		{
			if constexpr (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
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
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE or
				InputType == CharsType::UTF16
			)
			{
				// todo
				return {};
			}
			else if constexpr (
				InputType == CharsType::UTF32
			)
			{
				// todo
				return {};
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing,
		 * which means that the number of characters processed depends on the largest (character) size of the input/output.
		 */
		template<CharsType InputType, CharsType OutputType>
		[[nodiscard]] constexpr static auto read(const typename input_type_of<InputType>::pointer source, const std::size_t length) noexcept -> data_type
		{
			if constexpr (const auto mask = icelake_block::mask_of<InputType>(length);
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				if constexpr (
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
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE or
				InputType == CharsType::UTF16
			)
			{
				// todo
				return {};
			}
			else if constexpr (
				InputType == CharsType::UTF32
			)
			{
				// todo
				return {};
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		// ===============================
		// CHECK

		/**
		 * @brief Check whether the current block is pure ASCII or not.
		 * @code
		 * const auto value = icelake_block::read<chars_type, OutputType>(it_input_current);
		 * if (icelake_block::pure_ascii<chars_type>(value))
		 * { do_something(value); }
		 * else
		 * { do_something(value); }
		 * @endcode 
		 */
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto pure_ascii(const data_type value) noexcept -> bool
		{
			if constexpr (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				const auto ascii = _mm512_set1_epi8(static_cast<char>(0x80));

				return _mm512_cmpge_epu8_mask(value, ascii) == 0;
			}
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE or
				InputType == CharsType::UTF16
			)
			{
				const auto ascii = _mm512_set1_epi16(0x80);

				return _mm512_cmpge_epu16_mask(value, ascii) == 0;
			}
			else if constexpr (
				InputType == CharsType::UTF32
			)
			{
				const auto ascii = _mm512_set1_epi32(0x80);

				return _mm512_cmpge_epu32_mask(value, ascii) == 0;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		/**
		 * @note 8 bits characters only (LATIN/UTF8_CHAR/UTF8)
		 */
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto not_ascii_mask(const data_type value) noexcept -> __mmask64
		{
			if constexpr (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				const auto mask = _mm512_movepi8_mask(value);

				return mask;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("8 bits characters only (LATIN/UTF8_CHAR/UTF8)");
			}
		}

		/**
		 * @note 8 bits characters only (LATIN/UTF8_CHAR/UTF8)
		 */
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto not_ascii_count(const data_type value) noexcept -> std::size_t
		{
			if constexpr (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				const auto mask = not_ascii_mask<InputType>(value);
				const auto count = std::popcount(mask);
				return count;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("8 bits characters only (LATIN/UTF8_CHAR/UTF8)");
			}
		}

		// ===============================
		// WRITE

		/**
		 * @brief Writes a block of data to the target location,
		 * unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @tparam InputType Input Character Type
		 * @tparam OutputType Output Character Type
		 * @param dest Destination of character output 
		 * @param data Block of data
		 * @return The number of input characters used for this conversion and the conversion result
		 * @note Conversion advances the output pointer (based on the actual number of output characters),
		 * the input pointer does not (it returns the number of input pointers to be advanced).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType InputType, CharsType OutputType>
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<OutputType>::pointer& dest,
			const data_type data
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			constexpr auto advance = icelake_block::advance_of<InputType, OutputType>();

			if constexpr (
				InputType == CharsType::LATIN
			)
			{
				if constexpr (
					OutputType == CharsType::UTF16_LE or
					OutputType == CharsType::UTF16_BE
				)
				{
					constexpr auto endian = OutputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big;

					const auto native_data = icelake_block::utf16_to_native<endian>(data);
					_mm512_storeu_si512(dest, native_data);
				}
				else
				{
					_mm512_storeu_si512(dest, data);
				}
				dest += advance;

				return {advance, ErrorCode::NONE};
			}
			else if constexpr (
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				// todo
				return {};
			}
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE
			)
			{
				// todo
				return {};
			}
			else if constexpr (
				InputType == CharsType::UTF32
			)
			{
				// todo
				return {};
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		/**
		 * @brief Writes a block of data to the target location,
		 * unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @tparam InputType Input Character Type
		 * @tparam OutputType Output Character Type
		 * @param dest Destination of character output 
		 * @param data Block of data
		 * @param length Length of the "real" data in the block
		 * @return The number of input characters used for this conversion and the conversion result
		 * @note Conversion advances the output pointer (based on the actual number of output characters),
		 * the input pointer does not (it returns the number of input pointers to be advanced).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType InputType, CharsType OutputType>
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<OutputType>::pointer& dest,
			const data_type data,
			const std::size_t length
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			constexpr auto advance = icelake_block::advance_of<InputType, OutputType>();
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length < advance);

			if constexpr (
				InputType == CharsType::LATIN
			)
			{
				const auto mask = icelake_block::mask_of<OutputType>(length);

				if constexpr (
					OutputType == CharsType::UTF8_CHAR or
					OutputType == CharsType::UTF8
				)
				{
					_mm512_mask_storeu_epi8(dest, mask, data);
				}
				else if constexpr (
					OutputType == CharsType::UTF16_LE or
					OutputType == CharsType::UTF16_BE
				)
				{
					constexpr auto endian = OutputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big;

					const auto native_data = icelake_block::utf16_to_native<endian>(data);
					_mm512_mask_storeu_epi16(dest, mask, native_data);
				}
				else if constexpr (
					OutputType == CharsType::UTF32
				)
				{
					_mm512_mask_storeu_epi32(dest, mask, data);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}

				dest += length;

				return {length, ErrorCode::NONE};
			}
			else if constexpr (
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				// todo
				return {};
			}
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE
			)
			{
				// todo
				return {};
			}
			else if constexpr (
				InputType == CharsType::UTF32
			)
			{
				// todo
				return {};
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		// ===============================
		// UTF16

		template<std::endian SourceEndian>
		[[nodiscard]] constexpr static auto utf16_to_native(const data_type data) noexcept -> data_type
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
	};
}

#endif
