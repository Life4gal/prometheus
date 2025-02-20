// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if defined(GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED)

#include <bit>
#include <functional>
#include <numeric>
#include <string>
#include <algorithm>

#include <prometheus/macro.hpp>

#include <chars/encoding.hpp>
#include <chars/icelake.common.hpp>
#include <chars/scalar.latin.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

// ReSharper disable once CppRedundantNamespaceDefinition
namespace gal::prometheus::chars
{
	template<>
	class Simd<"icelake.latin">
	{
	public:
		using scalar_type = Scalar<"latin">;

		constexpr static auto chars_type = scalar_type::chars_type;

		using input_type = scalar_type::input_type;
		using char_type = scalar_type::char_type;
		using pointer_type = scalar_type::pointer_type;
		using size_type = scalar_type::size_type;

		using block_type = block<category_tag_icelake, chars_type>;
		using data_type = block_type::data_type;

		// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
		[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> result_error_input_type
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			using block_agent_type = block_type::agent_type<chars_type>;

			constexpr auto advance = block_agent_type::advance();

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			while (it_input_current + advance <= it_input_end)
			{
				#if GAL_PROMETHEUS_COMPILER_DEBUG
				[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
				#endif

				const auto data = block_agent_type::read(it_input_current);

				if (const auto sign = block_agent_type::sign_of(data);
					not sign.pure())
				{
					it_input_current += sign.start_count();

					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

					return {.error = ErrorCode::TOO_LARGE, .input = current_input_length};
				}

				it_input_current += advance;
			}

			const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
			GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

			if (remaining != 0)
			{
				#if GAL_PROMETHEUS_COMPILER_DEBUG
				[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
				#endif

				const auto data = block_agent_type::read(it_input_current, remaining);

				if (const auto sign = block_agent_type::sign_of(data);
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

		// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
		[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> auto
		{
			return Simd::validate({input, std::char_traits<char_type>::length(input)});
		}

		// note: we are not BOM aware
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
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
				using block_agent_type = block_type::agent_type<OutputType>;

				constexpr auto advance = block_agent_type::advance();

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

					const auto data = block_agent_type::read(it_input_current);

					if (const auto sign = block_agent_type::sign_of(data);
						not sign.pure())
					{
						output_length += sign.count();
					}

					it_input_current += advance;
				}

				const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
					#endif

					// fallback
					output_length += scalar_type::length<OutputType>({it_input_current, remaining});
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
				using block_agent_type = block_type::agent_type<OutputType>;

				constexpr auto advance = block_agent_type::advance();

				const auto process = [
							// advance only
							&it_input_current,
							// write + advance
							&it_output_current
						]<bool MaskOut>(
					const data_type data,
					const decltype(advance) data_length
				) noexcept -> void
				{
					if constexpr (not MaskOut)
					{
						GAL_PROMETHEUS_ERROR_ASSUME(data_length == advance);
					}

					const auto sign = block_agent_type::sign_of(data);

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

						block_agent_type::write(it_output_current, output_low, out_size_low);
						block_agent_type::write(it_output_current, output_high, out_size_high);
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
							block_agent_type::write(it_output_current, output_low, out_size);
						}
					}
					else
					{
						do_write_full();
					}

					it_input_current += data_length;
				};

				// if there's at least 128 bytes remaining, we don't need to mask the output
				while (it_input_current + 2 * advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
					#endif

					const auto data = block_agent_type::read(it_input_current);

					if (const auto sign = block_agent_type::sign_of(data);
						sign.pure())
					{
						block_agent_type::write(it_output_current, data);

						it_input_current += advance;
					}
					else
					{
						process.template operator()<false>(data, advance);
					}
				}

				// in the last 128 bytes, the first 64 may require masking the output
				if (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
					#endif

					const auto data = block_agent_type::read(it_input_current);
					process.template operator()<true>(data, advance);
				}

				const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
					#endif

					// with the last 64 bytes, the input also needs to be masked
					const auto data = block_agent_type::read(it_input_current, remaining);
					process.template operator()<true>(data, remaining);
				}
			}
			else if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE or
				OutputType == CharsType::UTF32
			)
			{
				using block_agent_type = block_type::agent_type<OutputType>;

				constexpr auto advance = block_agent_type::advance();

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
					#endif

					const auto data = block_agent_type::read(it_input_current);

					block_agent_type::write(it_output_current, data);

					it_input_current += advance;
				}

				const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
					#endif

					const auto data = block_agent_type::read(it_input_current, remaining);

					block_agent_type::write(it_output_current, data, remaining);

					it_input_current += remaining;
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
			return chars::make_result<ProcessPolicy>(
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
