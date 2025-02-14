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
	namespace icelake_common_detail
	{
		struct category_tag_type_icelake {};
	}

	constexpr icelake_common_detail::category_tag_type_icelake category_tag_icelake{};

	namespace icelake_common_detail
	{
		using data_64_type = __m512i;
		using data_32_type = __m256i;
		using data_16_type = __m128i;

		using data_type = data_64_type;

		template<CharsType InputType>
		struct native_data_type_selector;

		template<>
		struct native_data_type_selector<CharsType::LATIN>
		{
			using type = data_64_type;
		};

		template<>
		struct native_data_type_selector<CharsType::UTF8_CHAR> : native_data_type_selector<CharsType::LATIN> {};

		template<>
		struct native_data_type_selector<CharsType::UTF8> : native_data_type_selector<CharsType::UTF8_CHAR> {};

		template<>
		struct native_data_type_selector<CharsType::UTF16_LE>
		{
			using type = data_32_type;
		};

		template<>
		struct native_data_type_selector<CharsType::UTF16_BE> : native_data_type_selector<CharsType::UTF16_LE> {};

		template<>
		struct native_data_type_selector<CharsType::UTF16> : native_data_type_selector<CharsType::UTF16_LE> {};

		template<>
		struct native_data_type_selector<CharsType::UTF32>
		{
			using type = data_16_type;
		};

		template<CharsType InputType>
		using native_data_type = typename native_data_type_selector<InputType>::type;
	}

	// ==================================================
	// LATIN

	template<>
	struct block<category_tag_icelake, CharsType::LATIN>
	{
		constexpr static auto chars_type = CharsType::LATIN;

		template<CharsType OutputType>
		using agent_type = block_agent<category_tag_icelake, chars_type, OutputType>;

		using data_type = icelake_common_detail::data_type;
		using native_data_type = icelake_common_detail::native_data_type<chars_type>;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		template<CharsType OutputType>
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			return sizeof(data_type) / sizeof(typename input_type_of<OutputType>::value_type);
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
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

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		template<CharsType OutputType>
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
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

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
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

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			if constexpr (const auto mask = block::mask_of<OutputType>(length);
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

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data
		) noexcept -> void
		{
			constexpr auto advance = block::advance<OutputType>();

			if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE
			)
			{
				constexpr auto endian = OutputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big;

				const auto native_data = block::shuffle<endian>(data);
				_mm512_storeu_si512(output, native_data);
			}
			else
			{
				_mm512_storeu_si512(output, data);
			}

			output += advance;
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> void
		{
			constexpr auto advance = block::advance<OutputType>();
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length < advance);

			const auto mask = block::mask_of<OutputType>(length);

			if constexpr (
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
				constexpr auto endian = OutputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big;

				const auto native_data = block::shuffle<endian>(data);
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

		// /**
		//  * @brief Writes a complete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data
		// ) noexcept -> void;

		// /**
		//  * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data,
		// 	const std::size_t length
		// ) noexcept -> void;

		/**
		 * @brief On little-endian systems, no shuffle is applied by default. When SourceEndian differs, this function reverses the byte order of the entire 512-bit block.
		 */
		template<std::endian SourceEndian>
		[[nodiscard]] constexpr static auto shuffle(const data_type data) noexcept -> data_type
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

	template<CharsType OutputType>
	struct block_agent<category_tag_icelake, CharsType::LATIN, OutputType>
	{
		using agent_block = block<category_tag_icelake, CharsType::LATIN>;

		constexpr static auto chars_type = agent_block::chars_type;
		constexpr static auto output_chars_type = OutputType;

		using data_type = agent_block::data_type;
		using native_data_type = agent_block::native_data_type;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			return agent_block::advance<output_chars_type>();
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
		{
			return agent_block::sign_of(data);
		}

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			return agent_block::mask_of<output_chars_type>(length);
		}

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source);
		}

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source, length);
		}

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const data_type data
		) noexcept -> void
		{
			return agent_block::write<output_chars_type>(output, data);
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> void
		{
			return agent_block::write<output_chars_type>(output, data, length);
		}

		// /**
		//  * @brief Writes a complete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data
		// ) noexcept -> void;

		// /**
		//  * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data,
		// 	const std::size_t length
		// ) noexcept -> void;

		/**
		 * @brief On little-endian systems, no shuffle is applied by default. When SourceEndian differs, this function reverses the byte order of the entire 512-bit block.
		 */
		template<std::endian SourceEndian>
		[[nodiscard]] constexpr static auto shuffle(const data_type data) noexcept -> data_type
		{
			return agent_block::shuffle<SourceEndian>(data);
		}
	};

	// ==================================================
	// UTF8_CHAR

	template<>
	struct block<category_tag_icelake, CharsType::UTF8_CHAR>
	{
	private:
		using agent_block = block<category_tag_icelake, CharsType::LATIN>;

	public:
		constexpr static auto chars_type = CharsType::UTF8_CHAR;

		using data_type = agent_block::data_type;
		using native_data_type = agent_block::native_data_type;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		template<CharsType OutputType>
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			return agent_block::advance<OutputType>();
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
		{
			return agent_block::sign_of(data);
		}

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		template<CharsType OutputType>
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			return agent_block::mask_of<OutputType>(length);
		}

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
		{
			return agent_block::read<OutputType>(source);
		}

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			return agent_block::read<OutputType>(source, length);
		}

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<OutputType>(output, data);
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<OutputType>(output, data, length);
		}

		// /**
		//  * @brief Writes a complete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data
		// ) noexcept -> void;

		// /**
		//  * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data,
		// 	const std::size_t length
		// ) noexcept -> void;

		/**
		 * @brief On little-endian systems, no shuffle is applied by default. When SourceEndian differs, this function reverses the byte order of the entire 512-bit block.
		 */
		template<std::endian SourceEndian>
		[[nodiscard]] constexpr static auto shuffle(const data_type data) noexcept -> data_type
		{
			return agent_block::shuffle<SourceEndian>(data);
		}
	};

	template<CharsType OutputType>
	struct block_agent<category_tag_icelake, CharsType::UTF8_CHAR, OutputType>
	{
		using agent_block = block<category_tag_icelake, CharsType::UTF8_CHAR>;

		constexpr static auto chars_type = agent_block::chars_type;
		constexpr static auto output_chars_type = OutputType;

		using data_type = agent_block::data_type;
		using native_data_type = agent_block::native_data_type;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			return agent_block::advance<output_chars_type>();
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
		{
			return agent_block::sign_of(data);
		}

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			return agent_block::mask_of<output_chars_type>(length);
		}

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source);
		}

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source, length);
		}

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const data_type data
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<output_chars_type>(output, data);
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<output_chars_type>(output, data, length);
		}

		// /**
		//  * @brief Writes a complete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data
		// ) noexcept -> void;

		// /**
		//  * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data,
		// 	const std::size_t length
		// ) noexcept -> void;

		/**
		 * @brief On little-endian systems, no shuffle is applied by default. When SourceEndian differs, this function reverses the byte order of the entire 512-bit block.
		 */
		template<std::endian SourceEndian>
		[[nodiscard]] constexpr static auto shuffle(const data_type data) noexcept -> data_type
		{
			return agent_block::shuffle<SourceEndian>(data);
		}
	};

	// ==================================================
	// UTF8

	template<>
	struct block<category_tag_icelake, CharsType::UTF8>
	{
	private:
		using agent_block = block<category_tag_icelake, CharsType::UTF8_CHAR>;

	public:
		constexpr static auto chars_type = CharsType::UTF8;

		template<CharsType OutputType>
		using agent_type = block_agent<category_tag_icelake, chars_type, OutputType>;

		using data_type = agent_block::data_type;
		using native_data_type = agent_block::native_data_type;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		template<CharsType OutputType>
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			return agent_block::advance<OutputType>();
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
		{
			return agent_block::sign_of(data);
		}

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		template<CharsType OutputType>
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			return agent_block::mask_of<OutputType>(length);
		}

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
		{
			return agent_block::read<OutputType>(source);
		}

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			return agent_block::read<OutputType>(source, length);
		}

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<OutputType>(output, data);
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<OutputType>(output, data, length);
		}

		// /**
		//  * @brief Writes a complete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data
		// ) noexcept -> void;

		// /**
		//  * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data,
		// 	const std::size_t length
		// ) noexcept -> void;

		/**
		 * @brief On little-endian systems, no shuffle is applied by default. When SourceEndian differs, this function reverses the byte order of the entire 512-bit block.
		 */
		template<std::endian SourceEndian>
		[[nodiscard]] constexpr static auto shuffle(const data_type data) noexcept -> data_type
		{
			return agent_block::shuffle<SourceEndian>(data);
		}
	};

	template<CharsType OutputType>
	struct block_agent<category_tag_icelake, CharsType::UTF8, OutputType>
	{
		using agent_block = block<category_tag_icelake, CharsType::UTF8>;

		constexpr static auto chars_type = agent_block::chars_type;
		constexpr static auto output_chars_type = OutputType;

		using data_type = agent_block::data_type;
		using native_data_type = agent_block::native_data_type;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			return agent_block::advance<output_chars_type>();
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
		{
			return agent_block::sign_of(data);
		}

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			return agent_block::mask_of<output_chars_type>(length);
		}

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source);
		}

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source, length);
		}

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const data_type data
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<output_chars_type>(output, data);
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<output_chars_type>(output, data, length);
		}

		// /**
		//  * @brief Writes a complete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data
		// ) noexcept -> void;

		// /**
		//  * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		//  * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		//  * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		//  */
		// template<CharsType OutputType>
		// constexpr static auto write(
		// 	typename output_type_of<OutputType>::pointer& output,
		// 	const native_data_type data,
		// 	const std::size_t length
		// ) noexcept -> void;

		/**
		 * @brief On little-endian systems, no shuffle is applied by default. When SourceEndian differs, this function reverses the byte order of the entire 512-bit block.
		 */
		template<std::endian SourceEndian>
		[[nodiscard]] constexpr static auto shuffle(const data_type data) noexcept -> data_type
		{
			return agent_block::shuffle<SourceEndian>(data);
		}
	};

	// ==================================================
	// UTF16

	template<>
	struct block<category_tag_icelake, CharsType::UTF16_LE>
	{
		constexpr static auto chars_type = CharsType::UTF16_LE;

		template<CharsType OutputType>
		using agent_type = block_agent<category_tag_icelake, chars_type, OutputType>;

		using data_type = icelake_common_detail::data_type;
		using native_data_type = icelake_common_detail::native_data_type<chars_type>;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		template<CharsType OutputType>
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			return sizeof(data_type) / std::ranges::max(
				       sizeof(input_type_of<chars_type>::value_type),
				       sizeof(typename input_type_of<OutputType>::value_type)
			       );
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
		{
			struct sign_type
			{
				__mmask32 value;

				/**
				 * @brief Whether all sign bits are 0, in other words, whether the current block is all ASCII.
				 */
				[[nodiscard]] constexpr auto pure() const noexcept -> bool
				{
					return value == 0;
				}

				/**
				 * @brief Get the number of non-ASCII in the current block.
				 */
				[[nodiscard]] constexpr auto count() const noexcept -> std::size_t
				{
					return std::popcount(value);
				}

				/**
				 * @brief Get the number of consecutive ASCII at the beginning.
				 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
				 * ^-----^ start_count
				 *                                                             ^----------^ end_count           
				 */
				[[nodiscard]] constexpr auto start_count() const noexcept -> std::size_t
				{
					return std::countr_zero(value);
				}

				/**
				 * @brief Get the number of consecutive ASCII at the ending.
				 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
				 * ^-----^ start_count
				 *                                                             ^----------^ end_count       
				 */
				[[nodiscard]] constexpr auto end_count() const noexcept -> std::size_t
				{
					return std::countl_zero(value);
				}
			};

			return sign_type{.value = _mm512_movepi16_mask(data)};
		}

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		template<CharsType OutputType>
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			// ReSharper disable once CppRedundantCastExpression
			const auto m_32 = static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));
			const auto m_16 = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));

			if constexpr (
				OutputType == CharsType::LATIN or
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8 or
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

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
		{
			// todo
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

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			// todo
			if constexpr (const auto mask = block::mask_of<OutputType>(length);
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

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data
		) noexcept -> void
		{
			// todo
			return;
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> void
		{
			// todo
			return;
		}

		/**
		 * @brief Writes a complete native-type block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const native_data_type data
		) noexcept -> void
		{
			constexpr auto advance = block::advance<OutputType>();

			// todo
			_mm256_storeu_epi16(output, data);
			output += advance;
		}

		/**
		 * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const native_data_type data,
			const std::size_t length
		) noexcept -> void
		{
			constexpr auto advance = block::advance<OutputType>();
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length < advance);

			const auto mask = block::mask_of<OutputType>(length);

			// todo
			_mm256_mask_storeu_epi16(output, mask, data);
			output += length;
		}
	};

	template<CharsType OutputType>
	struct block_agent<category_tag_icelake, CharsType::UTF16_LE, OutputType>
	{
		using agent_block = block<category_tag_icelake, CharsType::UTF16_LE>;

		constexpr static auto chars_type = agent_block::chars_type;
		constexpr static auto output_chars_type = OutputType;

		using data_type = agent_block::data_type;
		using native_data_type = agent_block::native_data_type;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			return agent_block::advance<output_chars_type>();
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
		{
			return agent_block::sign_of(data);
		}

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			return agent_block::mask_of<output_chars_type>(length);
		}

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source);
		}

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source, length);
		}

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& dest,
			const data_type data
		) noexcept -> void
		{
			return agent_block::write<output_chars_type>(dest, data);
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& dest,
			const data_type data,
			const std::size_t length
		) noexcept -> void
		{
			return agent_block::write<output_chars_type>(dest, data, length);
		}

		/**
		 * @brief Writes a complete native-type block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& dest,
			const native_data_type data
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<output_chars_type>(dest, data);
		}

		/**
		 * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		[[nodiscard]] constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& dest,
			const native_data_type data,
			const std::size_t length
		) noexcept -> std::pair<std::size_t, ErrorCode>
		{
			return agent_block::write<output_chars_type>(dest, data, length);
		}
	};

	template<>
	struct block<category_tag_icelake, CharsType::UTF16_BE> :
			block<category_tag_icelake, CharsType::UTF16_LE> {};

	template<CharsType OutputType>
	struct block_agent<category_tag_icelake, CharsType::UTF16_BE, OutputType> :
			block_agent<category_tag_icelake, CharsType::UTF16_LE, OutputType> {};

	// ==================================================
	// UTF32

	template<>
	struct block<category_tag_icelake, CharsType::UTF32>
	{
		constexpr static auto chars_type = CharsType::UTF32;

		template<CharsType OutputType>
		using agent_type = block_agent<category_tag_icelake, chars_type, OutputType>;

		using data_type = icelake_common_detail::data_type;
		using native_data_type = icelake_common_detail::native_data_type<chars_type>;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		template<CharsType OutputType>
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			std::ignore = OutputType;
			return sizeof(data_type) / sizeof(input_type_of<chars_type>::value_type);
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
		{
			struct sign_type
			{
				__mmask16 value;

				/**
				 * @brief Whether all sign bits are 0, in other words, whether the current block is all ASCII.
				 */
				[[nodiscard]] constexpr auto pure() const noexcept -> bool
				{
					return value == 0;
				}

				/**
				 * @brief Get the number of non-ASCII in the current block.
				 */
				[[nodiscard]] constexpr auto count() const noexcept -> std::size_t
				{
					return std::popcount(value);
				}

				/**
				 * @brief Get the number of consecutive ASCII at the beginning.
				 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
				 * ^-----^ start_count
				 *                                                             ^----------^ end_count           
				 */
				[[nodiscard]] constexpr auto start_count() const noexcept -> std::size_t
				{
					return std::countr_zero(value);
				}

				/**
				 * @brief Get the number of consecutive ASCII at the ending.
				 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
				 * ^-----^ start_count
				 *                                                             ^----------^ end_count       
				 */
				[[nodiscard]] constexpr auto end_count() const noexcept -> std::size_t
				{
					return std::countl_zero(value);
				}
			};

			return sign_type{.value = _mm512_movepi32_mask(data)};
		}

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		template<CharsType OutputType>
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			std::ignore = OutputType;

			const auto m_16 = static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));

			return m_16;
		}

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
		{
			std::ignore = OutputType;

			return _mm512_loadu_si512(source);
		}

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		template<CharsType OutputType>
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			const auto mask = block::mask_of<OutputType>(length);

			return _mm512_maskz_loadu_epi32(mask, source);
		}

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data
		) noexcept -> void
		{
			constexpr auto advance = block::advance<OutputType>();

			if constexpr (
				OutputType == CharsType::LATIN
			)
			{
				// Gathers the lowest byte of each 32-bit code point in 'data' into a contiguous 16-byte __m128i register.
				// clang-format off
				const auto shuffle_mask = _mm512_set_epi8(
					00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
					00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
					00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
					60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8u, 04, 00
				);
				// clang-format on

				const auto out = _mm512_castsi512_si128(_mm512_permutexvar_epi8(shuffle_mask, data));
				return block::write<OutputType>(output, out);
			}
			else if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE
			)
			{
				// todo
			}
			else if constexpr (
				OutputType == CharsType::UTF32
			)
			{
				// todo
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> void
		{
			constexpr auto advance = block::advance<OutputType>();
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length < advance);

			if constexpr (OutputType == CharsType::LATIN)
			{
				// clang-format off
				const auto shuffle_mask = _mm512_set_epi8(
					00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
					00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
					00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 
					60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8u, 04, 00
				);
				// clang-format on

				const auto out = _mm512_castsi512_si128(_mm512_permutexvar_epi8(shuffle_mask, data));
				return block::write<OutputType>(output, out, length);
			}
			else if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE
			)
			{
				// todo
			}
			else if constexpr (
				OutputType == CharsType::UTF32
			)
			{
				// todo
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		/**
		 * @brief Writes a complete native-type block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const native_data_type data
		) noexcept -> void
		{
			constexpr auto advance = block::advance<OutputType>();

			auto* p = GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(native_data_type*, output);

			_mm_storeu_si128(p, data);
			output += advance;
		}

		/**
		 * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		template<CharsType OutputType>
		constexpr static auto write(
			typename output_type_of<OutputType>::pointer& output,
			const native_data_type data,
			const std::size_t length
		) noexcept -> void
		{
			constexpr auto advance = block::advance<OutputType>();
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length < advance);

			const auto mask = block::mask_of<OutputType>(length);

			_mm_mask_storeu_epi8(output, mask, data);
			output += length;
		}
	};

	template<CharsType OutputType>
	struct block_agent<category_tag_icelake, CharsType::UTF32, OutputType>
	{
		using agent_block = block<category_tag_icelake, CharsType::UTF32>;

		constexpr static auto chars_type = agent_block::chars_type;
		constexpr static auto output_chars_type = OutputType;

		using data_type = agent_block::data_type;
		using native_data_type = agent_block::native_data_type;

		/**
		 * @brief Gets the number of input pointer iterations (based on type input_type_of<chars_type>::pointer) for each block processed.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If an input block will produce an output block, then the number of characters processed depends on the largest (character) size of the input/output.
		 */
		constexpr static auto advance() noexcept -> std::ptrdiff_t
		{
			return agent_block::advance<output_chars_type>();
		}

		/**
		 * @brief Gets the sign bits of all characters in a block.
		 */
		[[nodiscard]] static auto sign_of(const data_type data) noexcept -> auto
		{
			return agent_block::sign_of(data);
		}

		/**
		 * @brief Basically equivalent to `(1 << length) - 1`, but different OutputType return different types of masks.
		 */
		constexpr static auto mask_of(const std::size_t length) noexcept -> auto
		{
			return agent_block::mask_of<output_chars_type>(length);
		}

		/**
		 * @brief Reads a complete block of characters, assuming there are at least `advance<OutputType>()` characters to read.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source);
		}

		/**
		 * @brief Reads an incomplete block of characters, depending on `length`.
		 * @note When processing in SIMD mode, data `block` is used for reading and writing.
		 * @note If the output characters are larger than the input characters, the number of characters read will be reduced accordingly, then zero-extend them to fill the whole block.
		 */
		constexpr static auto read(const input_type_of<chars_type>::pointer source, const std::size_t length) noexcept -> data_type
		{
			return agent_block::read<output_chars_type>(source, length);
		}

		/**
		 * @brief Writes a complete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const data_type data
		) noexcept -> void
		{
			return agent_block::write<output_chars_type>(output, data);
		}

		/**
		 * @brief Writes an incomplete block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const data_type data,
			const std::size_t length
		) noexcept -> void
		{
			return agent_block::write<output_chars_type>(output, data, length);
		}

		/**
		 * @brief Writes a complete native-type block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const native_data_type data
		) noexcept -> void
		{
			return agent_block::write<output_chars_type>(output, data);
		}

		/**
		 * @brief Writes an incomplete native-type block of characters, then advances the output iterator.
		 * @note Unlike scalar mode, SIMD mode always assumes that the block being read is legal (this also means that writes are always successful).
		 * @note If the input type is UTF16, the endian depends on the InputType (InputType == CharsType::UTF16_LE ? std::endian::little : std::endian::big).
		 */
		constexpr static auto write(
			typename output_type_of<output_chars_type>::pointer& output,
			const native_data_type data,
			const std::size_t length
		) noexcept -> void
		{
			return agent_block::write<output_chars_type>(output, data, length);
		}
	};
}

#endif
