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
#include <chars/scalar.latin.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::chars
{
	template<>
	class Simd<"icelake.latin">;

	template<>
	struct descriptor_type<Simd<"icelake.latin">>
	{
	private:
		friend class Simd<"icelake.latin">;

		using scalar_type = Scalar<"latin">;
		using scalar_descriptor_type = descriptor_type<scalar_type>;

	public:
		constexpr static auto chars_type = scalar_descriptor_type::chars_type;

		using input_type = scalar_descriptor_type::input_type;
		using char_type = scalar_descriptor_type::char_type;
		using pointer_type = scalar_descriptor_type::pointer_type;
		using size_type = scalar_descriptor_type::size_type;

		using data_type = __m512i;

		constexpr static std::ptrdiff_t advance_per_step = sizeof(data_type) / sizeof(char_type);
		// zero extend ==> 1 char -> 1 out_char
		template<CharsType Type>
		constexpr static std::ptrdiff_t advance_per_step_with =
				sizeof(data_type) /
				(sizeof(typename output_type_of<Type>::value_type) / sizeof(char_type));
	};

	namespace icelake_latin_detail
	{
		using data_type = descriptor_type<Simd<"icelake.latin">>::data_type;

		template<CharsType Type>
		struct block;

		template<>
		struct block<CharsType::LATIN>
		{
			using mask_type = __mmask64;

			[[nodiscard]] static auto make_mask(const std::size_t length) noexcept -> mask_type
			{
				return _bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(length));
			}

			template<bool MaskOut>
			[[nodiscard]] static auto load_data(const auto source, [[maybe_unused]] const std::size_t length) noexcept -> data_type
			{
				if constexpr (MaskOut)
				{
					const auto mask = make_mask(length);
					return _mm512_maskz_loadu_epi8(mask, source);
				}
				else
				{
					return _mm512_loadu_si512(source);
				}
			}
		};

		template<>
		struct block<CharsType::UTF8_CHAR> : block<CharsType::LATIN> {};

		template<>
		struct block<CharsType::UTF8> : block<CharsType::UTF8_CHAR> {};

		template<>
		struct block<CharsType::UTF16>
		{
			friend struct block<CharsType::UTF16_LE>;
			friend struct block<CharsType::UTF16_BE>;

			using mask_type = __mmask32;

			[[nodiscard]] static auto make_mask(const std::size_t length) noexcept -> mask_type
			{
				return _bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length));
			}

		private:
			[[nodiscard]] static auto byte_flip() noexcept -> data_type
			{
				// clang-format off
				return _mm512_setr_epi64(
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
					0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
				);
				// clang-format on
			}

			template<bool MaskOut>
			[[nodiscard]] static auto load_data(const auto source, [[maybe_unused]] const std::size_t length) noexcept -> data_type
			{
				const auto m256 = [&]() noexcept -> auto
				{
					if constexpr (MaskOut)
					{
						const auto mask = make_mask(length);
						return _mm256_maskz_loadu_epi8(mask, source);
					}
					else
					{
						return _mm256_loadu_si256(
							GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(
								const __m256i *,
								source
							)
						);
					}
				}();

				// Zero extend each set of 8 ascii characters to 32 16-bit integers
				return _mm512_cvtepu8_epi16(m256);
			}
		};

		template<>
		struct block<CharsType::UTF16_LE> : block<CharsType::UTF16>
		{
			template<bool MaskOut>
			[[nodiscard]] static auto load_data(const auto source, [[maybe_unused]] const std::size_t length) noexcept -> data_type
			{
				if constexpr (const auto data = block<CharsType::UTF16>::load_data<MaskOut>(source, length);
					std::endian::native != std::endian::little)
				{
					return _mm512_shuffle_epi8(data, byte_flip());
				}
				else
				{
					return data;
				}
			}
		};

		template<>
		struct block<CharsType::UTF16_BE> : block<CharsType::UTF16>
		{
			template<bool MaskOut>
			[[nodiscard]] static auto load_data(const auto source, [[maybe_unused]] const std::size_t length) noexcept -> data_type
			{
				if constexpr (const auto data = block<CharsType::UTF16>::load_data<MaskOut>(source, length);
					std::endian::native != std::endian::big)
				{
					return _mm512_shuffle_epi8(data, byte_flip());
				}
				else
				{
					return data;
				}
			}
		};

		template<>
		struct block<CharsType::UTF32>
		{
			using mask_type = __mmask16;

			[[nodiscard]] static auto make_mask(const std::size_t length) noexcept -> mask_type
			{
				return static_cast<mask_type>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(length)));
			}

			template<bool MaskOut>
			[[nodiscard]] static auto load_data(const auto source, [[maybe_unused]] const std::size_t length) noexcept -> data_type
			{
				const auto m128 = [&]() noexcept -> auto
				{
					if constexpr (MaskOut)
					{
						const auto mask = make_mask(length);
						return _mm_maskz_loadu_epi8(mask, source);
					}
					else
					{
						return _mm_loadu_si128(
							GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(
								const __m128i *,
								source
							)
						);
					}
				}();

				// Zero extend each set of 8 ascii characters to 16 32-bit integers
				return _mm512_cvtepu8_epi32(m128);
			}
		};
	}

	template<>
	class Simd<"icelake.latin">
	{
	public:
		using descriptor_type = descriptor_type<Simd>;
		using scalar_type = descriptor_type::scalar_type;

		constexpr static auto chars_type = descriptor_type::chars_type;

		using input_type = descriptor_type::input_type;
		using char_type = descriptor_type::char_type;
		using pointer_type = descriptor_type::pointer_type;
		using size_type = descriptor_type::size_type;

		using data_type = descriptor_type::data_type;

		// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
		template<bool Detail = false>
		[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> auto
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			using block_type = icelake_latin_detail::block<CharsType::LATIN>;

			constexpr auto process_policy = Detail ? InputProcessPolicy::DEFAULT : InputProcessPolicy::RESULT;

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const auto ascii = _mm512_set1_epi8(static_cast<char>(0x80));
			// used iff not ReturnResultType
			auto running_or = _mm512_setzero_si512();

			while (it_input_current + descriptor_type::advance_per_step <= it_input_end)
			{
				#if GAL_PROMETHEUS_COMPILER_DEBUG
				[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, descriptor_type::advance_per_step};
				#endif

				if constexpr (const auto in = block_type::load_data<false>(it_input_current, descriptor_type::advance_per_step);
					Detail)
				{
					if (const auto not_ascii = _mm512_cmpge_epu8_mask(in, ascii);
						not_ascii)
					{
						it_input_current += std::countr_zero(not_ascii);

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						constexpr auto current_output_length = static_cast<std::size_t>(0);

						return make_result<process_policy>(
							ErrorCode::TOO_LARGE,
							current_input_length,
							current_output_length
						);
					}
				}
				else
				{
					// running_or | (in & ascii)
					running_or = _mm512_ternarylogic_epi32(running_or, in, ascii, 0xf8);
					if (_mm512_test_epi8_mask(running_or, running_or) != 0)
					{
						return false;
					}
				}

				it_input_current += descriptor_type::advance_per_step;
			}

			const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
			GAL_PROMETHEUS_ERROR_ASSUME(remaining < descriptor_type::advance_per_step);

			if (remaining != 0)
			{
				if constexpr (const auto in = block_type::load_data<true>(it_input_current, remaining);
					Detail)
				{
					if (const auto not_ascii = _mm512_cmpge_epu8_mask(in, ascii);
						not_ascii)
					{
						it_input_current += std::countr_zero(not_ascii);

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						constexpr auto current_output_length = static_cast<std::size_t>(0);

						return make_result<process_policy>(
							ErrorCode::TOO_LARGE,
							current_input_length,
							current_output_length
						);
					}
				}
				else
				{
					// running_or | (in & ascii)
					running_or = _mm512_ternarylogic_epi32(running_or, in, ascii, 0xf8);
					if (_mm512_test_epi8_mask(running_or, running_or) != 0)
					{
						return false;
					}
				}

				it_input_current += remaining;
			}

			// ==================================================
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
			if constexpr (Detail)
			{
				const auto current_input_length = static_cast<std::size_t>(input_length);
				constexpr auto current_output_length = static_cast<std::size_t>(0);
				return make_result<process_policy>(
					ErrorCode::NONE,
					current_input_length,
					current_output_length
				);
			}
			else
			{
				return _mm512_test_epi8_mask(running_or, running_or) == 0;
			}
		}

		// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
		template<bool Detail = false>
		[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> auto
		{
			return Simd::validate<Detail>({input, std::char_traits<char_type>::length(input)});
		}

		// note: we are not BOM aware
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			using block_type = icelake_latin_detail::block<CharsType::LATIN>;

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
				// number of 512-bit chunks that fits into the length
				auto result_length = input_length / descriptor_type::advance_per_step * descriptor_type::advance_per_step;

				if (constexpr size_type long_string_optimization_threshold = 2048;
					input_length >= long_string_optimization_threshold)
				{
					auto eight_64_bits = _mm512_setzero_si512();
					do
					{
						const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
						// avoid overflow
						const auto iterations = std::ranges::min(
							remaining / descriptor_type::advance_per_step,
							static_cast<size_type>(std::numeric_limits<std::uint8_t>::max() - 1)
						);
						const auto this_turn_end = it_input_current + (iterations * descriptor_type::advance_per_step);

						auto sum = _mm512_setzero_si512();
						while (it_input_current < this_turn_end)
						{
							#if GAL_PROMETHEUS_COMPILER_DEBUG
							[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, descriptor_type::advance_per_step};
							#endif

							const auto in = block_type::load_data<false>(it_input_current, descriptor_type::advance_per_step);
							const auto mask = _mm512_movepi8_mask(in);
							// ASCII => 0x00
							// NON-ASCII => 0xFF
							const auto mask_vec = _mm512_movm_epi8(mask);
							// 0x00 => 0x00
							// 0xFF => 0x01
							const auto counts = _mm512_abs_epi8(mask_vec);
							// const auto counts = _mm512_and_si512(mask_vec, _mm512_set1_epi8(1));

							sum = _mm512_add_epi8(sum, counts);
							it_input_current += descriptor_type::advance_per_step;
						}

						const auto abs = _mm512_sad_epu8(sum, _mm512_setzero_si512());
						eight_64_bits = _mm512_add_epi64(eight_64_bits, abs);
					} while (it_input_current + descriptor_type::advance_per_step < it_input_end);

					result_length += _mm512_reduce_add_epi64(eight_64_bits);
				}
				else
				{
					while (it_input_current + descriptor_type::advance_per_step <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, descriptor_type::advance_per_step};
						#endif

						const auto in = block_type::load_data<false>(it_input_current, descriptor_type::advance_per_step);
						const auto not_ascii = _mm512_movepi8_mask(in);

						result_length += std::popcount(not_ascii);
						it_input_current += descriptor_type::advance_per_step;
					}
				}

				const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < descriptor_type::advance_per_step);
				if (remaining != 0)
				{
					// fallback
					result_length += scalar_type::length<OutputType>({it_input_current, remaining});
				}

				return result_length;
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

		// note: we are not BOM aware
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto length(const pointer_type input) noexcept -> size_type
		{
			return Simd::length<OutputType>({input, std::char_traits<char_type>::length(input)});
		}

		template<
			CharsType OutputType,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
		>
		[[nodiscard]] constexpr static auto convert(
			const input_type input,
			typename output_type_of<OutputType>::pointer output
		) noexcept -> auto
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);
			if constexpr (assume_all_correct<ProcessPolicy>())
			{
				// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(validate(input));
			}

			using block_type = icelake_latin_detail::block<OutputType>;

			using output_type = output_type_of<OutputType>;
			using output_pointer_type = typename output_type::pointer;

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const output_pointer_type it_output_begin = output;
			output_pointer_type it_output_current = it_output_begin;

			if constexpr (OutputType == CharsType::LATIN)
			{
				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				it_input_current += input_length;
				it_output_current += input_length;
			}
			else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
			{
				const auto process = [
							// advance only
							&it_input_current,
							// write + advance
							&it_output_current
						]<bool MaskOut>(
					const data_type source,
					const size_type source_length
				) noexcept -> void
				{
					const auto non_ascii = _mm512_movepi8_mask(source);
					const auto non_ascii_high = static_cast<std::uint32_t>(non_ascii >> 32);
					const auto non_ascii_low = static_cast<std::uint32_t>(non_ascii);

					const auto ascii = ~non_ascii;
					const auto ascii_high = static_cast<std::uint64_t>(static_cast<std::uint32_t>(ascii >> 32));
					const auto ascii_low = static_cast<std::uint64_t>(static_cast<std::uint32_t>(ascii));

					// the bits in ascii are inverted and zeros are interspersed in between them
					constexpr auto alternate_bits = 0x5555'5555'5555'5555ull;
					const auto mask_high = ~_pdep_u64(ascii_high, alternate_bits);
					const auto mask_low = ~_pdep_u64(ascii_low, alternate_bits);

					// interleave bytes from top and bottom halves (abcd...ABCD -> aAbBcCdD)
					const auto source_interleaved = _mm512_permutexvar_epi8(
						// clang-format off
						_mm512_set_epi32(
							0x3f1f3e1e, 0x3d1d3c1c, 0x3b1b3a1a, 0x39193818,
							0x37173616, 0x35153414, 0x33133212, 0x31113010,
							0x2f0f2e0e, 0x2d0d2c0c, 0x2b0b2a0a, 0x29092808,
							0x27072606, 0x25052404, 0x23032202, 0x21012000
						),
						// clang-format on
						source
					);

					// Mask to denote whether the byte is a leading byte that is not ascii
					// binary representation of -64: 1100'0000
					const auto sixth = _mm512_cmpge_epu8_mask(source, _mm512_set1_epi8(static_cast<char>(-64)));
					const auto sixth_high = static_cast<__mmask32>(sixth >> 32);
					const auto sixth_low = static_cast<__mmask32>(sixth);

					const auto output_low = [](const data_type interleaved, const __mmask32 s, const __mmask64 mask) noexcept -> auto
					{
						// Upscale the bytes to 16-bit value, adding the 0b1100'0010 leading byte in the process.
						// We adjust for the bytes that have their two most significant bits.
						// This takes care of the first 32 bytes, assuming we interleaved the bytes.
						// binary representation of -62: 1100'0010
						auto v = _mm512_shldi_epi16(interleaved, _mm512_set1_epi8(static_cast<char>(-62)), 8);
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

					const auto output_high = [](const __m512i interleaved, const __mmask32 s, const __mmask64 mask) noexcept -> auto
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

					const auto out_size = static_cast<unsigned int>(source_length + std::popcount(non_ascii));
					const auto out_size_low = 32 + static_cast<unsigned int>(std::popcount(non_ascii_low));

					if constexpr (MaskOut)
					{
						// is the second half of the input vector used?
						if (source_length > 32)
						{
							const auto out_size_high = static_cast<unsigned int>(source_length - 32) + static_cast<unsigned int>(std::popcount(non_ascii_high));

							const auto mask_1 = block_type::make_mask(out_size_low);
							const auto mask_2 = block_type::make_mask(out_size_high);

							_mm512_mask_storeu_epi8(it_output_current + 0, mask_1, output_low);
							_mm512_mask_storeu_epi8(it_output_current + out_size_low, mask_2, output_high);
						}
						else
						{
							const auto mask = block_type::make_mask(out_size);

							_mm512_mask_storeu_epi8(it_output_current, mask, output_low);
						}
					}
					else
					{
						_mm512_storeu_si512(it_output_current + 0, output_low);
						_mm512_storeu_si512(it_output_current + out_size_low, output_high);
					}

					it_input_current += source_length;
					it_output_current += out_size;
				};

				const auto process_or_just_store = [
							process,
							// advance only
							&it_input_current,
							// write + advance
							&it_output_current
						](
					const data_type source,
					const size_type source_length
				) noexcept -> void
				{
					const auto non_ascii = _mm512_movepi8_mask(source);
					const auto count = std::popcount(non_ascii);
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(count >= 0);

					if (count != 0)
					{
						process.template operator()<false>(source, source_length);
					}
					else
					{
						_mm512_storeu_si512(it_output_current, source);

						it_input_current += source_length;
						it_output_current += source_length;
					}
				};

				constexpr auto advance = descriptor_type::advance_per_step_with<OutputType>;

				// if there's at least 128 bytes remaining, we don't need to mask the output
				while (it_input_current + 2 * advance <= it_input_end)
				{
					const auto in = block_type::template load_data<false>(it_input_current, advance);

					process_or_just_store(in, advance);
				}

				// in the last 128 bytes, the first 64 may require masking the output
				while (it_input_current + advance <= it_input_end)
				{
					const auto in = block_type::template load_data<false>(it_input_current, advance);

					process.template operator()<true>(in, advance);
				}

				// with the last 64 bytes, the input also needs to be masked
				if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					remaining != 0)
				{
					const auto in = block_type::template load_data<true>(it_input_current, remaining);

					process.template operator()<true>(in, remaining);
				}
			}
			else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE)
			{
				// round down to nearest multiple of 32
				constexpr auto advance = descriptor_type::advance_per_step_with<OutputType>;
				const auto rounded_input_length = input_length & ~(advance - 1);
				const auto it_rounded_input_end = it_input_begin + rounded_input_length;

				while (it_input_current < it_rounded_input_end)
				{
					const auto in = block_type::template load_data<false>(it_input_current, advance);

					_mm512_storeu_si512(it_output_current, in);

					it_input_current += advance;
					it_output_current += advance;
				}

				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_rounded_input_end);

				if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					remaining != 0)
				{
					const auto in = block_type::template load_data<true>(it_input_current, remaining);
					const auto in_mask = block_type::make_mask(remaining);

					_mm512_mask_storeu_epi16(it_output_current, in_mask, in);

					it_input_current += remaining;
					it_output_current += remaining;
				}
			}
			else if constexpr (OutputType == CharsType::UTF32)
			{
				// Round down to nearest multiple of 16
				constexpr auto advance = descriptor_type::advance_per_step_with<OutputType>;
				const auto rounded_input_length = input_length & ~(advance - 1);
				const auto it_rounded_input_end = it_input_begin + rounded_input_length;

				while (it_input_current < it_rounded_input_end)
				{
					const auto in = block_type::template load_data<false>(it_input_current, advance);

					_mm512_storeu_si512(it_output_current, in);

					it_input_current += advance;
					it_output_current += advance;
				}

				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_rounded_input_end);

				if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					remaining != 0)
				{
					const auto in = block_type::template load_data<true>(it_input_current, remaining);
					const auto in_mask = block_type::make_mask(remaining);

					// Store the results back to memory
					_mm512_mask_storeu_epi32(it_output_current, in_mask, in);

					it_input_current += remaining;
					it_output_current += remaining;
				}
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}

			// ==================================================
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
			const auto current_input_length = static_cast<std::size_t>(input_length);
			const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
			return make_result<ProcessPolicy>(
				ErrorCode::NONE,
				current_input_length,
				current_output_length
			);
		}

		template<
			CharsType OutputType,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
		>
		[[nodiscard]] constexpr static auto convert(
			const pointer_type input,
			typename output_type_of<OutputType>::pointer output
		) noexcept -> auto
		{
			return Simd::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, output);
		}

		template<
			typename StringType,
			CharsType OutputType,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
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
			result.resize(Simd::length<OutputType>(input));

			std::ignore = Simd::convert<OutputType, ProcessPolicy>(input, result.data());
			return result;
		}

		template<
			typename StringType,
			CharsType OutputType,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
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
			result.resize(Simd::length<OutputType>(input));

			return Simd::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
		}

		template<
			CharsType OutputType,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
		>
		[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
		{
			std::basic_string<typename output_type_of<OutputType>::value_type> result{};
			result.resize(Simd::length<OutputType>(input));

			std::ignore = Simd::convert<OutputType, ProcessPolicy>(input, result.data());
			return result;
		}

		template<
			CharsType OutputType,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
		>
		[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
		{
			std::basic_string<typename output_type_of<OutputType>::value_type> result{};
			result.resize(Simd::length<OutputType>(input));

			return Simd::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
		}
	};
}

#endif
