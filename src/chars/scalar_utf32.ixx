// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:scalar.utf32;

import std;
import gal.prometheus.meta;
import gal.prometheus.memory;
GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

import :encoding;

#else
#include <functional>
#include <numeric>

#include <prometheus/macro.hpp>
#include <chars/encoding.ixx>
#include <meta/meta.ixx>
#include <memory/memory.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

// ReSharper disable once CppRedundantNamespaceDefinition
GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::chars)
{
	template<>
	class Scalar<"utf32">
	{
	public:
		constexpr static auto input_category = CharsCategory::UTF32;
		using input_type = chars::input_type<input_category>;
		using char_type = input_type::value_type;
		using pointer_type = input_type::const_pointer;
		using size_type = input_type::size_type;

		template<bool ReturnResultType = false>
		[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			while (it_input_current != it_input_end)
			{
				const auto count_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				if (const auto word = *it_input_current;
					word > 0x10'ffff)
				{
					if constexpr (ReturnResultType)
					{
						return result_type{.error = ErrorCode::TOO_LARGE, .count = count_if_error};
					}
					else
					{
						return false;
					}
				}
				else if (word >= 0xd800 and word <= 0xdfff)
				{
					if constexpr (ReturnResultType)
					{
						return result_type{.error = ErrorCode::SURROGATE, .count = count_if_error};
					}
					else
					{
						return false;
					}
				}

				it_input_current += 1;
			}

			if constexpr (ReturnResultType)
			{
				return {.error = ErrorCode::NONE, .count = input_length};
			}
			else
			{
				return true;
			}
		}

		template<bool ReturnResultType = false>
		[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
		{
			return validate<ReturnResultType>({input, std::char_traits<char_type>::length(input)});
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory>
		[[nodiscard]] constexpr static auto length(const input_type input) noexcept -> size_type
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			if constexpr (OutputCategory == CharsCategory::ASCII) { return input.size(); } // NOLINT(bugprone-branch-clone)
			else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
			{
				return std::transform_reduce(
					input.begin(),
					input.end(),
					static_cast<size_type>(0),
					std::plus<>{},
					[](const auto data) noexcept
					{
						return 1 // ascii
						       +
						       (data > 0x7f) // two-byte
						       +
						       (data > 0x7ff) // three-byte
						       +
						       (data > 0xffff) // four-byte
								;
					});
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE or OutputCategory == CharsCategory::UTF16)
			{
				return std::transform_reduce(
					input.begin(),
					input.end(),
					static_cast<size_type>(0),
					std::plus<>{},
					[](const auto data) noexcept
					{
						return 1 // non-surrogate word
						       +
						       (data > 0xffff) // surrogate pair
								;
					});
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32) { return input.size(); }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory>
		[[nodiscard]] constexpr static auto length(const pointer_type input) noexcept -> size_type
		{
			return length<OutputCategory>({input, std::char_traits<char_type>::length(input)});
		}

		template<
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			bool CheckNextBlock = true
		>
		[[nodiscard]] constexpr static auto convert(
			const input_type input,
			typename output_type<OutputCategory>::pointer output
		) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);
			if constexpr (ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(validate(input));
			}

			using output_pointer_type = typename output_type<OutputCategory>::pointer;
			using output_char_type = typename output_type<OutputCategory>::value_type;

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const output_pointer_type it_output_begin = output;
			output_pointer_type it_output_current = it_output_begin;

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if constexpr (CheckNextBlock)
					{
						// try to convert the next block of 2 ASCII characters
						// if it is safe to read 8 more bytes, check that they are ascii
						if (it_input_current + 2 <= it_input_end)
						{
							if (const auto value = memory::unaligned_load<std::uint64_t>(it_input_current);
								(value & 0xffff'ff00'ffff'ff00) == 0)
							{
								*(it_output_current + 0) = static_cast<output_char_type>(*(it_input_current + 0));
								*(it_output_current + 1) = static_cast<output_char_type>(*(it_input_current + 1));

								// one more step, see `for (; it_input_current < it_input_end; ++it_input_current)`
								it_input_current += 1;
								it_output_current += 2;
								continue;
							}
						}
					}

					const auto word = *it_input_current;
					if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
					{
						if ((word & 0xffff'ff00) != 0)
						{
							if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
							else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
							{
								return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
					}

					*(it_output_current + 0) = static_cast<output_char_type>(word & 0xff);
					it_output_current += 1;
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8_CHAR or OutputCategory == CharsCategory::UTF8)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if constexpr (CheckNextBlock)
					{
						// try to convert the next block of 2 ASCII characters
						// if it is safe to read 8 more bytes, check that they are ascii
						if (it_input_current + 2 <= it_input_end)
						{
							if (const auto value = memory::unaligned_load<std::uint64_t>(it_input_current);
								(value & 0xffff'ff80'ffff'ff80) == 0)
							{
								*(it_output_current + 0) = static_cast<output_char_type>(*(it_input_current + 0));
								*(it_output_current + 1) = static_cast<output_char_type>(*(it_input_current + 1));

								// one more step, see `for (; it_input_current < it_input_end; ++it_input_current)`
								it_input_current += 1;
								it_output_current += 2;
								continue;
							}
						}
					}

					if (const auto word = *it_input_current;
						(word & 0xffff'ff80) == 0)
					{
						// 1-byte ascii
						*(it_output_current + 0) = static_cast<output_char_type>(word);
						it_output_current += 1;
					}
					else if ((word & 0xffff'f800) == 0)
					{
						// 2-bytes utf8
						// 0b110?'???? 0b10??'????
						*(it_output_current + 0) = static_cast<output_char_type>((word >> 6) | 0b1100'0000);
						*(it_output_current + 1) = static_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);
						it_output_current += 2;
					}
					else if ((word & 0xffff'0000) == 0)
					{
						// 3-bytes utf8
						if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
						{
							if (word >= 0xd800 and word <= 0xdfff)
							{
								if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}
						}
						// 0b1110'???? 0b10??'???? 0b10??'????
						*(it_output_current + 0) = static_cast<output_char_type>((word >> 12) | 0b1110'0000);
						*(it_output_current + 1) = static_cast<output_char_type>(((word >> 6) & 0b0011'1111) | 0b1000'0000);
						*(it_output_current + 2) = static_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);
						it_output_current += 3;
					}
					else
					{
						// 4-bytes utf8
						if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
						{
							if (word > 0x0010'ffff)
							{
								if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
								}
								else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
							}
						}
						// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
						*(it_output_current + 0) = static_cast<output_char_type>((word >> 18) | 0b1111'0000);
						*(it_output_current + 1) = static_cast<output_char_type>(((word >> 12) & 0b0011'1111) | 0b1000'0000);
						*(it_output_current + 2) = static_cast<output_char_type>(((word >> 6) & 0b0011'1111) | 0b1000'0000);
						*(it_output_current + 3) = static_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);
						it_output_current += 4;
					}
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if (const auto word = *it_input_current;
						(word & 0xffff'0000) == 0)
					{
						if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
						{
							if (word >= 0xd800 and word <= 0xdfff)
							{
								if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
							}
						}
						const auto real_word = [w = static_cast<output_char_type>(word)]() noexcept
						{
							if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
							{
								return std::byteswap(w);
							}
							else { return w; }
						}();

						*(it_output_current + 0) = static_cast<output_char_type>(real_word);
						it_output_current += 1;
					}
					else
					{
						if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
						{
							if (word > 0x0010'ffff)
							{
								if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT) { return 0; }
								else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
								}
							}
							const auto [high_surrogate, low_surrogate] = [real_word = word - 0x0001'0000]() noexcept
							{
								const auto high = 0xd800 + (real_word >> 10);
								const auto low = 0xdc00 + (real_word & 0x3ff);
								if constexpr ((OutputCategory == CharsCategory::UTF16_LE) != (std::endian::native == std::endian::little))
								{
									return std::make_pair(std::byteswap(high), std::byteswap(low));
								}
								else { return std::make_pair(high, low); }
							}();

							*(it_output_current + 0) = static_cast<output_char_type>(high_surrogate);
							*(it_output_current + 1) = static_cast<output_char_type>(low_surrogate);
							it_output_current += 2;
						}
					}
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				it_input_current += input_length;
				it_output_current += input_length;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("Unknown or unsupported `OutputCategory` (we don't know the `endian` by UTF16, so it's not allowed to use it here).");
			}

			if constexpr (
				ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
				ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
			) { return static_cast<std::size_t>(it_output_current - it_output_begin); }
			else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
			{
				return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			bool CheckNextBlock = true
		>
		[[nodiscard]] constexpr static auto convert(
			const pointer_type input,
			typename output_type<OutputCategory>::pointer output
		) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
		{
			return convert<OutputCategory, ProcessPolicy, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, output);
		}

		template<
			typename StringType,
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			bool CheckNextBlock = true
		>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<typename output_type<OutputCategory>::pointer>;
			}
		[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> StringType
		{
			StringType result{};
			result.resize(length<OutputCategory>(input));

			(void)convert<OutputCategory, ProcessPolicy, CheckNextBlock>(input, result.data());
			return result;
		}

		template<
			typename StringType,
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			bool CheckNextBlock = true
		>
			requires requires(StringType& string)
			{
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<typename output_type<OutputCategory>::pointer>;
			}
		[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> StringType
		{
			StringType result{};
			result.resize(length<OutputCategory>(input));

			return convert<OutputCategory, ProcessPolicy, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, result.data());
		}

		template<
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			bool CheckNextBlock = true
		>
		[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
		{
			std::basic_string<typename output_type<OutputCategory>::value_type> result{};
			result.resize(length<OutputCategory>(input));

			(void)convert<OutputCategory, ProcessPolicy, CheckNextBlock>(input, result.data());
			return result;
		}

		template<
			CharsCategory OutputCategory,
			InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE,
			bool CheckNextBlock = true
		>
		[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
		{
			std::basic_string<typename output_type<OutputCategory>::value_type> result{};
			result.resize(length<OutputCategory>(input));

			return convert<OutputCategory, ProcessPolicy, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, result.data());
		}
	};

	template<>
	struct scalar_processor_of<CharsCategory::UTF32>
	{
		using type = Scalar<"utf32">;
	};
} // namespace gal::prometheus::chars
