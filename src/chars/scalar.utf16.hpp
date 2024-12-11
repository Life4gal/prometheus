// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
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

#include <memory/rw.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

template<>
class gal::prometheus::chars::Scalar<"utf16">
{
public:
	constexpr static auto chars_type = CharsType::UTF16;

	using input_type = input_type_of<chars_type>;
	using char_type = input_type::value_type;
	using pointer_type = input_type::const_pointer;
	using size_type = input_type::size_type;

	constexpr static std::size_t char_size = sizeof(char_type);

	template<bool ReturnResultType = false, std::endian SourceEndian = std::endian::native>
	[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		while (it_input_current != it_input_end)
		{
			if (const auto word = [w = static_cast<std::uint16_t>(*it_input_current)]() noexcept -> auto
				{
					if constexpr (SourceEndian == std::endian::native)
					{
						return w;
					}
					else
					{
						return std::byteswap(w);
					}
				}();
				(word & 0xf800) == 0xd800)
			{
				const auto count_if_error = static_cast<std::size_t>(it_input_current - it_input_end);

				// minimal bound checking
				if (it_input_current + 1 >= it_input_end)
				{
					if constexpr (ReturnResultType)
					{
						return {.error = ErrorCode::SURROGATE, .count = count_if_error};
					}
					else
					{
						return false;
					}
				}

				if (const auto diff = static_cast<std::uint16_t>(word - 0xd800);
					diff > 0x3ff)
				{
					if constexpr (ReturnResultType)
					{
						return {.error = ErrorCode::SURROGATE, .count = count_if_error};
					}
					else
					{
						return false;
					}
				}

				const auto next_word = [w = static_cast<std::uint16_t>(*(it_input_current + 1))]() noexcept -> auto
				{
					if constexpr (SourceEndian == std::endian::native)
					{
						return w;
					}
					else
					{
						return std::byteswap(w);
					}
				}();

				if (const auto diff = static_cast<std::uint16_t>(next_word - 0xdc00);
					diff > 0x3ff)
				{
					if constexpr (ReturnResultType)
					{
						return {.error = ErrorCode::SURROGATE, .count = count_if_error};
					}
					else
					{
						return false;
					}
				}

				it_input_current += 2;
			}
			else
			{
				it_input_current += 1;
			}
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

	template<bool ReturnResultType = false, std::endian SourceEndian = std::endian::native>
	[[nodiscard]] constexpr static auto validate(const pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		return Scalar::validate<ReturnResultType, SourceEndian>({input, std::char_traits<char_type>::length(input)});
	}

	// note: we are not BOM aware
	template<CharsType OutputType, std::endian SourceEndian = std::endian::native>
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
			return std::transform_reduce(
				input.begin(),
				input.end(),
				static_cast<size_type>(0),
				std::plus<>{},
				[](const auto word) noexcept
				{
					const auto native_word = [w = static_cast<std::uint16_t>(word)]() noexcept
					{
						if constexpr (SourceEndian == std::endian::native)
						{
							return w;
						}
						else
						{
							return std::byteswap(w);
						}
					}();

					return
							// ASCII
							1
							+
							// non-ASCII is at least 2 bytes, surrogates are 2*2 == 4 bytes
							(native_word > 0x7f)
							+
							(native_word > 0x7ff && native_word <= 0xd7ff)
							+
							(native_word >= 0xe000);
				}
			);
		}
		// ReSharper disable CppClangTidyBugproneBranchClone
		else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
		{
			return input.size();
		}
		// ReSharper restore CppClangTidyBugproneBranchClone
		else if constexpr (OutputType == CharsType::UTF32)
		{
			return std::transform_reduce(
				input.begin(),
				input.end(),
				static_cast<size_type>(0),
				std::plus<>{},
				[](const auto word) noexcept
				{
					const auto native_word = [w = static_cast<std::uint16_t>(word)]() noexcept
					{
						if constexpr (SourceEndian == std::endian::native)
						{
							return w;
						}
						else
						{
							return std::byteswap(w);
						}
					}();

					return +((native_word & 0xfc00) != 0xdc00);
				}
			);
		}
		else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
	}

	// note: we are not BOM aware
	template<CharsType OutputType, std::endian SourceEndian = std::endian::native>
	[[nodiscard]] constexpr static auto length(const pointer_type input) noexcept -> size_type
	{
		return Scalar::length<OutputType, SourceEndian>({input, std::char_traits<char_type>::length(input)});
	}

	template<
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
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
			// fixme: error C2187
			// GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(Scalar::validate<false, SourceEndian>(input));
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((Scalar::validate<false, SourceEndian>(input)));
		}

		using output_type = output_type_of<OutputType>;
		using output_pointer_type = typename output_type::pointer;
		using output_char_type = typename output_type::value_type;

		const auto input_length = input.size();

		const pointer_type it_input_begin = input.data();
		pointer_type it_input_current = it_input_begin;
		const pointer_type it_input_end = it_input_begin + input_length;

		const output_pointer_type it_output_begin = output;
		output_pointer_type it_output_current = it_output_begin;

		if constexpr (
			const auto to_native = [](const std::uint64_t value) noexcept -> std::uint64_t
			{
				if constexpr (SourceEndian != std::endian::native)
				{
					return (value >> 8) | (value << (64 - 8));
				}
				else
				{
					return value;
				}
			};
			OutputType == CharsType::LATIN)
		{
			while (it_input_current < it_input_end)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				// if it is safe to read 32 more bytes, check that they are ascii
				if (constexpr auto step = 4 * sizeof(std::uint64_t) / char_size;
					it_input_current + step <= it_input_end)
				{
					constexpr auto offset = sizeof(std::uint64_t) / char_size;

					const auto v1 = to_native(memory::unaligned_load<std::uint64_t>(it_input_current + 0 * offset));
					const auto v2 = to_native(memory::unaligned_load<std::uint64_t>(it_input_current + 1 * offset));
					const auto v3 = to_native(memory::unaligned_load<std::uint64_t>(it_input_current + 2 * offset));
					const auto v4 = to_native(memory::unaligned_load<std::uint64_t>(it_input_current + 3 * offset));

					if (const auto value = v1 | v2 | v3 | v4;
						(value & 0xff00'ff00'ff00'ff00) == 0)
					{
						std::ranges::transform(
							it_input_current,
							it_input_current + step,
							it_output_current,
							[](const auto word) noexcept
							{
								if constexpr (const auto w = static_cast<std::uint16_t>(word);
									SourceEndian != std::endian::native)
								{
									return static_cast<output_char_type>(std::byteswap(w));
								}
								else
								{
									return static_cast<output_char_type>(w);
								}
							}
						);

						it_input_current += step;
						it_output_current += step;
						continue;
					}
				}

				const auto word = [w = static_cast<std::uint16_t>(*it_input_current)]() noexcept
				{
					if constexpr (SourceEndian != std::endian::native)
					{
						return std::byteswap(w);
					}
					else
					{
						return w;
					}
				}();

				if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
				{
					if ((word & 0xff00) != 0)
					{
						if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
						{
							return 0;
						}
						else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
						{
							return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
						}
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
					}
				}

				*(it_output_current + 0) = static_cast<output_char_type>(word & 0xff);

				it_input_current += 1;
				it_output_current += 1;
			}
		}
		else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
		{
			while (it_input_current < it_input_end)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				// if it is safe to read 8 more bytes, check that they are ascii
				if (constexpr auto step = 1 * sizeof(std::uint64_t) / char_size;
					it_input_current + step <= it_input_end)
				{
					constexpr auto offset = sizeof(std::uint64_t) / char_size;

					const auto v1 = to_native(memory::unaligned_load<std::uint64_t>(it_input_current + 0 * offset));

					if (const auto value = v1;
						(value & 0xff80'ff80'ff80'ff80) == 0)
					{
						std::ranges::transform(
							it_input_current,
							it_input_current + step,
							it_output_current,
							[](const auto word) noexcept
							{
								if constexpr (const auto w = static_cast<std::uint16_t>(word);
									SourceEndian != std::endian::native)
								{
									return static_cast<output_char_type>(std::byteswap(w));
								}
								else
								{
									return static_cast<output_char_type>(w);
								}
							}
						);

						it_input_current += step;
						it_output_current += step;
						continue;
					}
				}

				if (const auto word = [w = static_cast<std::uint16_t>(*it_input_current)]() noexcept
					{
						if constexpr (SourceEndian != std::endian::native)
						{
							return std::byteswap(w);
						}
						else
						{
							return w;
						}
					}();
					(word & 0xff80) == 0)
				{
					// 1-byte utf8
					*(it_output_current + 0) = static_cast<output_char_type>(word);

					it_input_current += 1;
					it_output_current += 1;
				}
				else if ((word & 0xf800) == 0)
				{
					// 2-bytes utf8
					// 0b110?'???? 0b10??'????
					*(it_output_current + 0) = static_cast<output_char_type>((word >> 6) | 0b1100'0000);
					*(it_output_current + 1) = static_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);

					it_input_current += 1;
					it_output_current += 2;
				}
				else if ((word & 0xf800) != 0xd800)
				{
					// 3-bytes utf8
					// 0b1110'???? 0b10??'???? 0b10??'????
					*(it_output_current + 0) = static_cast<output_char_type>((word >> 12) | 0b1110'0000);
					*(it_output_current + 1) = static_cast<output_char_type>(((word >> 6) & 0b0011'1111) | 0b1000'0000);
					*(it_output_current + 2) = static_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);

					it_input_current += 1;
					it_output_current += 3;
				}
				else
				{
					// 4-bytes utf8
					// must be a surrogate pair

					// minimal bound checking
					if (it_input_current + 1 >= it_input_end)
					{
						if constexpr (
							ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
							ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT
						)
						{
							return 0;
						}
						else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
						{
							return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
						}
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
					}

					const auto diff = static_cast<std::uint16_t>(word - 0xd800);

					if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
					{
						if (diff > 0x3ff)
						{
							if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
							{
								return 0;
							}
							else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
							{
								return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
					}

					const auto next_word = [w = static_cast<std::uint16_t>(*(it_input_current + 1))]() noexcept
					{
						if constexpr (SourceEndian != std::endian::native)
						{
							return std::byteswap(w);
						}
						else
						{
							return w;
						}
					}();
					const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

					if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
					{
						if (next_diff > 0x3ff)
						{
							if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
							{
								return 0;
							}
							else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
							{
								return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
					}

					const auto value = static_cast<std::uint32_t>((diff << 10) + next_diff + 0x1'0000);

					// will generate four UTF-8 bytes
					// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
					*(it_output_current + 0) = static_cast<output_char_type>((value >> 18) | 0b1111'0000);
					*(it_output_current + 1) = static_cast<output_char_type>(((value >> 12) & 0b0011'1111) | 0b1000'0000);
					*(it_output_current + 2) = static_cast<output_char_type>(((value >> 6) & 0b0011'1111) | 0b1000'0000);
					*(it_output_current + 3) = static_cast<output_char_type>((value & 0b0011'1111) | 0b1000'0000);

					it_input_current += 2;
					it_output_current += 4;
				}
			}
		}
		else if constexpr (
			OutputType == CharsType::UTF16_LE or
			OutputType == CharsType::UTF16_BE or
			OutputType == CharsType::UTF16
		)
		{
			if constexpr ((OutputType == CharsType::UTF16) or ((SourceEndian == std::endian::little) == (OutputType == CharsType::UTF16_LE)))
			{
				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
			}
			else
			{
				Scalar::flip_endian(input, output);
			}

			it_input_current += input_length;
			it_output_current += input_length;
		}
		else if constexpr (OutputType == CharsType::UTF32)
		{
			while (it_input_current < it_input_end)
			{
				const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				if (const auto word = [w = static_cast<std::uint16_t>(*(it_input_current + 0))]() noexcept
					{
						if constexpr (SourceEndian != std::endian::native)
						{
							return std::byteswap(w);
						}
						else
						{
							return w;
						}
					}();
					(word & 0xf800) != 0xd800)
				{
					// no surrogate pair, extend 16-bit word to 32-bit word
					*(it_output_current + 0) = static_cast<output_char_type>(word);

					it_input_current += 1;
					it_output_current += 1;
				}
				else
				{
					// must be a surrogate pair

					// minimal bound checking
					if (it_input_current + 1 >= it_input_end)
					{
						if constexpr (
							ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or
							ProcessPolicy == InputProcessPolicy::ASSUME_VALID_INPUT)
						{
							return 0;
						}
						else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
						{
							return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
						}
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
					}

					const auto diff = static_cast<std::uint16_t>(word - 0xd800);

					if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
					{
						if (diff > 0x3ff)
						{
							if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
							{
								return 0;
							}
							else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
							{
								return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
					}

					const auto next_word = [w = static_cast<std::uint16_t>(*(it_input_current + 1))]() noexcept
					{
						if constexpr (SourceEndian != std::endian::native) { return std::byteswap(w); }
						else { return w; }
					}();
					const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

					if constexpr (ProcessPolicy != InputProcessPolicy::ASSUME_VALID_INPUT)
					{
						if (next_diff > 0x3ff)
						{
							if constexpr (ProcessPolicy == InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
							{
								return 0;
							}
							else if constexpr (ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE)
							{
								return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
							}
							else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
						}
					}

					const auto value = static_cast<std::uint32_t>((diff << 10) + next_diff + 0x1'0000);

					*(it_output_current + 0) = static_cast<output_char_type>(value);

					it_input_current += 2;
					it_output_current += 1;
				}
			}
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
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
		std::endian SourceEndian = std::endian::native,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(
		const pointer_type input,
		typename output_type_of<OutputType>::pointer output
	) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
	{
		return Scalar::convert<OutputType, SourceEndian, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, output);
	}

	template<
		typename StringType,
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
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
		result.resize(length<OutputType, SourceEndian>(input));

		std::ignore = Scalar::convert<OutputType, SourceEndian, ProcessPolicy>(input, result.data());
		return result;
	}

	template<
		typename StringType,
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
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
		result.resize(length<OutputType, SourceEndian>(input));

		return Scalar::convert<OutputType, SourceEndian, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
	}

	template<
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(const input_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
	{
		std::basic_string<typename output_type_of<OutputType>::value_type> result{};
		result.resize(length<OutputType, SourceEndian>(input));

		std::ignore = Scalar::convert<OutputType, SourceEndian, ProcessPolicy>(input, result.data());
		return result;
	}

	template<
		CharsType OutputType,
		std::endian SourceEndian = std::endian::native,
		InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE
	>
	[[nodiscard]] constexpr static auto convert(const pointer_type input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
	{
		std::basic_string<typename output_type_of<OutputType>::value_type> result{};
		result.resize(length<OutputType, SourceEndian>(input));

		return Scalar::convert<OutputType, SourceEndian, ProcessPolicy>({input, std::char_traits<char_type>::length(input)}, result.data());
	}

	template<std::endian SourceEndian = std::endian::native>
	[[nodiscard]] constexpr auto code_points(const input_type input) const noexcept -> size_type
	{
		(void)this;

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

		return std::ranges::count_if(
			input,
			[](auto word) noexcept -> bool
			{
				if constexpr (SourceEndian != std::endian::native)
				{
					word = std::byteswap(word);
				}

				return (word & 0xfc00) != 0xdc00;
			}
		);
	}

	constexpr auto static flip_endian(const input_type input, const output_type_of<chars_type>::pointer output) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

		std::ranges::transform(
			input,
			output,
			[](const auto word) noexcept
			{
				return static_cast<output_type_of<chars_type>::value_type>(std::byteswap(word));
			}
		);
	}

	template<typename StringType>
		requires requires(StringType& string)
		{
			string.resize(std::declval<size_type>());
			{
				string.data()
			} -> std::convertible_to<output_type_of<chars_type>::pointer>;
		}
	[[nodiscard]] constexpr static auto flip_endian(const input_type input) noexcept -> StringType
	{
		StringType result{};
		result.resize(length<chars_type>(input));

		Scalar::flip_endian(input, result.data());
		return result;
	}

	[[nodiscard]] constexpr static auto flip_endian(const input_type input) noexcept -> std::basic_string<output_type_of<chars_type>::value_type>
	{
		std::basic_string<output_type_of<chars_type>::value_type> result{};
		result.resize(length<chars_type>(input));

		flip_endian(input, result.data());
		return result;
	}
};
