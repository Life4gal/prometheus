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

template<>
class gal::prometheus::chars::Simd<"icelake.latin">
{
public:
	using scalar_type = Scalar<"latin">;

	constexpr static auto chars_type = scalar_type::chars_type;

	using input_type = scalar_type::input_type;
	using char_type = scalar_type::char_type;
	using pointer_type = scalar_type::pointer_type;
	using size_type = scalar_type::size_type;

private:
	using data_type = __m512i;
	using mask_type = __mmask64;

	constexpr static std::size_t size_per_char = sizeof(char_type);
	constexpr static std::size_t advance_per_step = sizeof(data_type) / size_per_char;

	template<typename OutChar>
	constexpr static std::size_t advance_per_step_with =
			// zero extend each set of 8 bit latin1 characters to N x-bit integers
			// 1 => 1
			sizeof(data_type) / (sizeof(OutChar) / size_per_char);

	template<CharsType Type>
	[[nodiscard]] static auto make_mask(const size_type source_length) noexcept -> auto
	{
		if constexpr (Type == CharsType::LATIN or Type == CharsType::UTF8_CHAR or Type == CharsType::UTF8)
		{
			return static_cast<mask_type>(_bzhi_u64(~static_cast<unsigned long long>(0), static_cast<unsigned int>(source_length)));
		}
		else if constexpr (Type == CharsType::UTF16 or Type == CharsType::UTF16_LE or Type == CharsType::UTF16_BE)
		{
			return static_cast<__mmask32>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(source_length)));
		}
		else if constexpr (Type == CharsType::UTF32)
		{
			return static_cast<__mmask16>(_bzhi_u32(~static_cast<unsigned int>(0), static_cast<unsigned int>(source_length)));
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<CharsType Type, bool MaskOut>
	[[nodiscard]] static auto load_from_memory(const pointer_type source, const size_type source_length) noexcept -> auto
	{
		if constexpr (Type == CharsType::LATIN or Type == CharsType::UTF8_CHAR or Type == CharsType::UTF8)
		{
			if constexpr (MaskOut)
			{
				const auto source_mask = make_mask<Type>(source_length);
				return _mm512_maskz_loadu_epi8(source_mask, source);
			}
			else
			{
				return _mm512_loadu_si512(source);
			}
		}
		else if constexpr (Type == CharsType::UTF16 or Type == CharsType::UTF16_LE or Type == CharsType::UTF16_BE)
		{
			if constexpr (MaskOut)
			{
				const auto source_mask = make_mask<Type>(source_length);
				return _mm256_maskz_loadu_epi8(source_length, source);
			}
			else
			{
				// Load 32 ascii characters into a 256-bit register
				return _mm256_loadu_si256(
					GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(
						const __m256i *,
						source
					)
				);
			}
		}
		else if constexpr (Type == CharsType::UTF32)
		{
			if constexpr (MaskOut)
			{
				const auto source_mask = make_mask<Type>(source_length);
				return _mm_maskz_loadu_epi8(source_mask, source);
			}
			else
			{
				// Load 16 ascii characters into a 128-bit register
				return _mm_loadu_si128(
					GAL_PROMETHEUS_SEMANTIC_TRIVIAL_REINTERPRET_CAST(
						const __m128i *,
						source
					)
				);
			}
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<CharsType Type>
	static auto load_from_register(const data_type source, const size_type source_length) noexcept -> data_type
	{
		const auto source_mask = make_mask<Type>(source_length);

		return _mm512_maskz_mov_epi8(source_mask, source);
	}

public:
	// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
	template<bool ReturnResultType = false>
	[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		constexpr auto step = 1 * advance_per_step;

		const auto ascii = _mm512_set1_epi8(static_cast<char>(0x80));
		// used iff not ReturnResultType
		auto running_or = _mm512_setzero_si512();

		while (it_input_current + step <= it_input_end)
		{
			if constexpr (const auto in = Simd::load_from_memory<CharsType::LATIN, false>(it_input_current, step);
				ReturnResultType)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				if (const auto not_ascii = _mm512_cmp_epu8_mask(in, ascii, _MM_CMPINT_NLT);
					not_ascii)
				{
					return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + std::countr_zero(not_ascii)};
				}
			}
			else
			{
				// running_or | (in & ascii)
				running_or = _mm512_ternarylogic_epi32(running_or, in, ascii, 0xf8);
			}

			it_input_current += step;
		}

		if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
			remaining != 0)
		{
			if constexpr (const auto in = Simd::load_from_memory<CharsType::LATIN, true>(it_input_current, remaining);
				ReturnResultType)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				if (const auto not_ascii = _mm512_cmp_epu8_mask(in, ascii, _MM_CMPINT_NLT);
					not_ascii)
				{
					return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error + std::countr_zero(not_ascii)};
				}
			}
			else
			{
				// running_or | (in & ascii)
				running_or = _mm512_ternarylogic_epi32(running_or, in, ascii, 0xf8);
			}
		}

		if constexpr (ReturnResultType)
		{
			return result_type{.error = ErrorCode::NONE, .count = input_length};
		}
		else
		{
			return _mm512_test_epi8_mask(running_or, running_or) == 0;
		}
	}

	// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
	template<bool ReturnResultType = false>
	[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		return Simd::validate<ReturnResultType>({input, std::char_traits<char_type>::length(input)});
	}

	// note: we are not BOM aware
	template<CharsType OutputType>
	[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
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
			constexpr auto step = 1 * advance_per_step;
			constexpr size_type long_string_optimization_threshold = 2048;

			// number of 512-bit chunks that fits into the length
			auto result_length = input_length / step * step;

			if (input_length >= long_string_optimization_threshold)
			{
				auto eight_64_bits = _mm512_setzero_si512();
				do
				{
					const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
					// avoid overflow
					const auto iterations = std::ranges::min(
						remaining / step,
						static_cast<size_type>(std::numeric_limits<std::uint8_t>::max() - 1)
					);
					const auto this_turn_end = it_input_current + (iterations * step);

					auto sum = _mm512_setzero_si512();
					while (it_input_current < this_turn_end)
					{
						const auto in = Simd::load_from_memory<CharsType::LATIN, false>(it_input_current, step);
						const auto mask = _mm512_movepi8_mask(in);
						// ASCII => 0x00
						// NON-ASCII => 0xFF
						const auto mask_vec = _mm512_movm_epi8(mask);
						// 0x00 => 0x00
						// 0xFF => 0x01
						const auto counts = _mm512_abs_epi8(mask_vec);
						// const auto counts = _mm512_and_si512(mask_vec, _mm512_set1_epi8(1));

						sum = _mm512_add_epi8(sum, counts);
						it_input_current += step;
					}

					const auto abs = _mm512_sad_epu8(sum, _mm512_setzero_si512());
					eight_64_bits = _mm512_add_epi64(eight_64_bits, abs);
				} while (it_input_current + step < it_input_end);

				result_length += _mm512_reduce_add_epi64(eight_64_bits);
			}
			else
			{
				while (it_input_current + step <= it_input_end)
				{
					const auto in = Simd::load_from_memory<CharsType::LATIN, false>(it_input_current, step);
					const auto not_ascii = _mm512_movepi8_mask(in);

					result_length += std::popcount(not_ascii);
					it_input_current += step;
				}
			}

			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				result_length += scalar_type::length<OutputType>({it_input_current, remaining});
			}

			return result_length;
		}
		// ReSharper disable CppClangTidyBugproneBranchClone
		else if constexpr (
			OutputType == CharsType::UTF16_LE or
			OutputType == CharsType::UTF16_BE or
			OutputType == CharsType::UTF16 or
			OutputType == CharsType::UTF32
		)
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
			// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(validate(input));
		}

		using output_pointer_type = typename output_type_of<OutputType>::pointer;
		using output_char_type = typename output_type_of<OutputType>::value_type;

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
			const auto process = []<bool MaskOut>(
				const data_type source,
				const size_type source_length,
				output_pointer_type dest
			) noexcept -> size_type
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

						const auto mask_1 = make_mask<OutputType>(out_size_low);
						const auto mask_2 = make_mask<OutputType>(out_size_high);

						_mm512_mask_storeu_epi8(dest + 0, mask_1, output_low);
						_mm512_mask_storeu_epi8(dest + out_size_low, mask_2, output_high);
					}
					else
					{
						const auto mask = make_mask<OutputType>(out_size);

						_mm512_mask_storeu_epi8(dest, mask, output_low);
					}
				}
				else
				{
					_mm512_storeu_si512(dest + 0, output_low);
					_mm512_storeu_si512(dest + out_size_low, output_high);
				}

				return out_size;
			};

			const auto process_or_just_store = [process](const data_type source, const size_type source_length, output_pointer_type dest) noexcept -> size_type
			{
				const auto non_ascii = _mm512_movepi8_mask(source);
				const auto count = std::popcount(non_ascii);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(count >= 0);

				if (count != 0)
				{
					return process.template operator()<false>(source, source_length, dest);
				}

				_mm512_storeu_si512(dest, source);
				return source_length;
			};

			constexpr auto step = 1 * advance_per_step_with<output_char_type>;

			// if there's at least 128 bytes remaining, we don't need to mask the output
			while (it_input_current + 2 * step <= it_input_end)
			{
				const auto in = Simd::load_from_memory<OutputType, false>(it_input_current, step);

				it_output_current += process_or_just_store(in, step, it_output_current);
				it_input_current += step;
			}

			// in the last 128 bytes, the first 64 may require masking the output
			while (it_input_current + step <= it_input_end)
			{
				const auto in = Simd::load_from_memory<OutputType, false>(it_input_current, step);

				it_output_current += process.template operator()<true>(in, step, it_output_current);
				it_input_current += step;
			}

			// with the last 64 bytes, the input also needs to be masked
			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				const auto in = Simd::load_from_memory<OutputType, true>(it_input_current, remaining);

				it_output_current += process.template operator()<true>(in, remaining, it_output_current);
				it_input_current += remaining;
			}
		}
		else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE)
		{
			constexpr auto not_native_endian = (OutputType == CharsType::UTF16_LE) != (std::endian::native == std::endian::little);

			// clang-format off
			const auto byte_flip = _mm512_setr_epi64(
				0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809,
				0x0607'0405'0203'0001, 0x0e0f'0c0d'0a0b'0809
			);
			// clang-format on

			// round down to nearest multiple of 32
			constexpr auto step = 1 * advance_per_step_with<output_char_type>;
			const auto rounded_input_length = input_length & ~(step - 1);
			const auto it_rounded_input_end = it_input_begin + rounded_input_length;

			while (it_input_current < it_rounded_input_end)
			{
				// Load 32 ascii characters into a 256-bit register
				const auto in = Simd::load_from_memory<OutputType, false>(it_input_current, step);
				// Zero extend each set of 8 ascii characters to 32 16-bit integers
				const auto out = [](const auto i, [[maybe_unused]] const auto bf) noexcept -> auto
				{
					if constexpr (const auto v = _mm512_cvtepu8_epi16(i);
						not_native_endian)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(in, byte_flip);

				// Store the results back to memory
				_mm512_storeu_si512(it_output_current, out);

				it_input_current += step;
				it_output_current += step;
			}

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_rounded_input_end);

			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				const auto in = Simd::load_from_memory<OutputType, true>(it_input_current, remaining);
				const auto in_mask = make_mask<OutputType>(remaining);

				// Zero extend each set of 8 ascii characters to 32 16-bit integers
				const auto out = [](const auto i, [[maybe_unused]] const auto bf) noexcept -> auto
				{
					if constexpr (const auto v = _mm512_cvtepu8_epi16(i);
						not_native_endian)
					{
						return _mm512_shuffle_epi8(v, bf);
					}
					else
					{
						return v;
					}
				}(in, byte_flip);

				// Store the results back to memory
				_mm512_mask_storeu_epi16(it_output_current, in_mask, out);

				it_input_current += remaining;
				it_output_current += remaining;
			}
		}
		else if constexpr (OutputType == CharsType::UTF32)
		{
			// Round down to nearest multiple of 16
			constexpr auto step = 1 * advance_per_step_with<output_char_type>;
			const auto rounded_input_length = input_length & ~(step - 1);
			const auto it_rounded_input_end = it_input_begin + rounded_input_length;

			while (it_input_current < it_rounded_input_end)
			{
				// Load 16 ascii characters into a 128-bit register
				const auto in = Simd::load_from_memory<OutputType, false>(it_input_current, step);

				// Zero extend each set of 8 ascii characters to 16 32-bit integers
				const auto out = _mm512_cvtepu8_epi32(in);

				// Store the results back to memory
				_mm512_storeu_si512(it_output_current, out);

				it_input_current += step;
				it_output_current += step;
			}

			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_rounded_input_end);

			if (const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				remaining != 0)
			{
				const auto in = Simd::load_from_memory<OutputType, true>(it_input_current, remaining);
				const auto in_mask = make_mask<OutputType>(remaining);

				// Zero extend each set of 8 ascii characters to 16 32-bit integers
				const auto out = _mm512_cvtepu8_epi32(in);

				// Store the results back to memory
				_mm512_mask_storeu_epi32(it_output_current, in_mask, out);

				it_input_current += remaining;
				it_output_current += remaining;
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
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(
		const pointer_type input,
		typename output_type_of<OutputType>::pointer output
	) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t> //
	{
		return Simd::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, output);
	}

	template<
		typename StringType,
		CharsType OutputType,
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
		result.resize(length<OutputType>(input));

		std::ignore = Simd::convert<OutputType, ProcessPolicy>(input, result.data());
		return result;
	}

	template<
		typename StringType,
		CharsType OutputType,
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
		result.resize(length<OutputType>(input));

		return Simd::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
	}

	template<
		CharsType OutputType,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
	{
		std::basic_string<typename output_type_of<OutputType>::value_type> result{};
		result.resize(length<OutputType>(input));

		std::ignore = Simd::convert<OutputType, ProcessPolicy>(input, result.data());
		return result;
	}

	template<
		CharsType OutputType,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
	{
		std::basic_string<typename output_type_of<OutputType>::value_type> result{};
		result.resize(length<OutputType>(input));

		return Simd::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
	}
};

#endif
