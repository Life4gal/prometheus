// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <bit>
#include <functional>
#include <numeric>
#include <string>
#include <algorithm>

#include <prometheus/macro.hpp>

#include <chars/encoding.hpp>
#include <chars/scalar.common.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

// ReSharper disable once CppRedundantNamespaceDefinition
namespace gal::prometheus::chars
{
	template<>
	class Scalar<"latin">
	{
	public:
		constexpr static auto chars_type = CharsType::LATIN;

		using input_type = input_type_of<chars_type>;
		using char_type = input_type::value_type;
		using pointer_type = input_type::const_pointer;
		using size_type = input_type::size_type;

		using data_type = scalar_block::data_type;

		// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
		template<bool Detail = false>
		[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> auto
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			constexpr auto process_policy = Detail ? InputProcessPolicy::DEFAULT : InputProcessPolicy::RESULT;
			constexpr auto advance = scalar_block::advance_of<chars_type, chars_type>();

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			while (it_input_current + advance <= it_input_end)
			{
				#if GAL_PROMETHEUS_COMPILER_DEBUG
				[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
				#endif

				if (const auto value = scalar_block::read<chars_type, chars_type>(it_input_current);
					not scalar_block::pure_ascii<chars_type>(value))
				{
					if constexpr (Detail)
					{
						const auto mask = scalar_block::not_ascii_mask<chars_type>(value);

						// [ascii] [non-ascii] [?] [?] [?] [?] [ascii] [ascii]
						//           ^ n_ascii
						//                                                 ^ n_next_possible_ascii_chunk_begin
						const auto n_ascii = std::countr_zero(mask);

						it_input_current += n_ascii;

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

						return chars::make_result<process_policy>(
							ErrorCode::TOO_LARGE,
							current_input_length,
							length_ignored
						);
					}
					else
					{
						return chars::make_result<process_policy>(
							ErrorCode::TOO_LARGE,
							length_ignored,
							length_ignored
						);
					}
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

				const auto end = it_input_current + remaining;
				while (it_input_current < end)
				{
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

					const auto [length, error] = scalar_block::validate<chars_type>(it_input_current, it_input_end);
					GAL_PROMETHEUS_ERROR_ASSUME(length == 1);

					if (error != ErrorCode::NONE)
					{
						return chars::make_result<process_policy>(
							error,
							current_input_length,
							length_ignored
						);
					}

					it_input_current += length;
				}
			}

			// ==================================================
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
			const auto current_input_length = static_cast<std::size_t>(input_length);
			return chars::make_result<process_policy>(
				ErrorCode::NONE,
				current_input_length,
				length_ignored
			);
		}

		// note: only used to detect pure ASCII strings, otherwise there is no point in using this function
		template<bool Detail = false>
		[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> auto
		{
			return Scalar::validate<Detail>({input, std::char_traits<char_type>::length(input)});
		}

		// note: we are not BOM aware
		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			constexpr auto advance = scalar_block::advance_of<chars_type, chars_type>();

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

				size_type output_length = input_length;

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
					#endif

					if (const auto value = scalar_block::read<chars_type, OutputType>(it_input_current);
						not scalar_block::pure_ascii<chars_type>(value))
					{
						const auto count = scalar_block::not_ascii_count<chars_type>(value);

						output_length += count;
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

					const auto end = it_input_current + remaining;
					while (it_input_current < end)
					{
						const auto [length, error] = scalar_block::validate<chars_type>(it_input_current, it_input_end);
						GAL_PROMETHEUS_ERROR_ASSUME(length == 1);

						if (error != ErrorCode::NONE)
						{
							output_length += 1;
						}

						it_input_current += length;
					}
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
			return Scalar::length<OutputType>({input, std::char_traits<char_type>::length(input)});
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
				// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(Scalar::validate(input));
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
			else if constexpr (
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8 or
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE or
				OutputType == CharsType::UTF32
			)
			{
				constexpr auto advance = scalar_block::advance_of<chars_type, OutputType>();

				const auto transform = [
							&it_input_current,
							it_input_end,
							&it_output_current
						]<bool Pure>(const decltype(advance) n) noexcept -> void
				{
					const auto end = it_input_current + n;
					while (it_input_current < end)
					{
						const auto [length, error] = scalar_block::write<chars_type, OutputType, Pure, false>(it_output_current, it_input_current, it_input_end);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == 1);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(error == ErrorCode::NONE);

						it_input_current += length;
					}
				};

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, advance};
					#endif

					if (const auto value = scalar_block::read<chars_type, OutputType>(it_input_current);
						not scalar_block::pure_ascii<chars_type>(value))
					{
						const auto mask = scalar_block::not_ascii_mask<chars_type>(value);

						// [ascii] [non-ascii] [?] [?] [?] [?] [ascii] [ascii]
						//           ^ n_ascii
						//                                                 ^ n_next_possible_ascii_chunk_begin
						const auto n_ascii = std::countr_zero(mask);
						const auto n_next_possible_ascii_chunk_begin = advance - std::countl_zero(mask) - n_ascii;

						transform.template operator()<true>(n_ascii);
						transform.template operator()<false>(n_next_possible_ascii_chunk_begin);
					}
					else
					{
						transform.template operator()<true>(advance);
					}
				}

				const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
					#endif

					transform.template operator()<false>(remaining);
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
			return Scalar::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, output);
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
			result.resize(Scalar::length<OutputType>(input));

			std::ignore = Scalar::convert<OutputType, ProcessPolicy>(input, result.data());
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
			return Scalar::convert<OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)});
		}

		template<
			CharsType OutputType,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
		>
		[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
		{
			using string_type = std::basic_string<typename output_type_of<OutputType>::value_type>;
			return Scalar::convert<string_type, OutputType, ProcessPolicy>(input);
		}

		template<
			CharsType OutputType,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::DEFAULT
		>
		[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
		{
			using string_type = std::basic_string<typename output_type_of<OutputType>::value_type>;
			return Scalar::convert<string_type, OutputType, ProcessPolicy>(input);
		}
	};
}
