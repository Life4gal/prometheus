// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:scalar.utf16;

import std;
import gal.prometheus.string;
import gal.prometheus.error;
import gal.prometheus.utility;

import :encoding;
import :converter;

namespace gal::prometheus::chars
{
	template<>
	class Scalar<"utf16">
	{
	public:
		constexpr static auto input_category = CharsCategory::UTF16;
		using input_type					 = chars::input_type<input_category>;
		using char_type						 = input_type::value_type;
		using pointer_type					 = input_type::const_pointer;
		using size_type						 = input_type::size_type;

		template<std::endian Endian = std::endian::native>
		[[nodiscard]] constexpr auto validate(const input_type input) const noexcept -> result_type
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			const auto		   input_length		= input.size();

			const pointer_type it_input_begin	= input.data();
			pointer_type	   it_input_current = it_input_begin;
			const pointer_type it_input_end		= it_input_begin + input_length;

			while (it_input_current != it_input_end)
			{
				if (const auto word = [it_input_current]() noexcept -> auto
					{
						if constexpr (Endian == std::endian::native)
						{
							return *it_input_current;
						}
						else
						{
							return std::byteswap(*it_input_current);
						}
					}();
					(word & 0xf800) == 0xd800)
				{
					const auto count_if_error = static_cast<std::size_t>(it_input_current - it_input_end);

					if (it_input_current + 1 == it_input_end) { return {.error = ErrorCode::SURROGATE, .count = count_if_error}; }

					if (const auto diff = word - 0xd800;
						diff > 0x3ff) { return {.error = ErrorCode::SURROGATE, .count = count_if_error}; }

					const auto next_word = [it_input_current]() noexcept -> auto
					{
						if constexpr (Endian == std::endian::native)
						{
							return *(it_input_current + 1);
						}
						else
						{
							return std::byteswap(*(it_input_current + 1));
						}
					}();

					if (const auto diff = next_word - 0xdc00;
						diff > 0x3ff) { return {.error = ErrorCode::SURROGATE, .count = count_if_error}; }

					it_input_current += 2;
				}
				else { it_input_current += 1; }
			}

			return {.error = ErrorCode::NONE, .count = input_length};
		}

		template<std::endian Endian = std::endian::native>
		[[nodiscard]] constexpr auto validate(const pointer_type input) const noexcept -> result_type
		{
			return this->validate<Endian>({input, std::char_traits<char_type>::length(input)});
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory, std::endian Endian = std::endian::native>
		[[nodiscard]] constexpr auto length(const input_type input) const noexcept -> size_type
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				return input.size();
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				return std::transform_reduce(
						input.begin(),
						input.end(),
						static_cast<size_type>(0),
						std::plus<>{},
						[](const auto word) noexcept
						{
							const auto native_word = [word]() noexcept
							{
								if constexpr (Endian != std::endian::native)
								{
									return std::byteswap(word);
								}
								else
								{
									return word;
								}
							}();

							return 1// ascii
								   +
								   (native_word > 0x7f)// non-ASCII is at least 2 bytes, surrogates are 2*2 == 4 bytes
								   +
								   ((native_word > 0x7ff && native_word <= 0xd7ff) || (native_word >= 0xe000))// three-byte
									;
						});
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				return input.size();
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				return std::transform_reduce(
						input.begin(),
						input.end(),
						static_cast<size_type>(0),
						std::plus<>{},
						[](const auto word) noexcept
						{
							const auto native_word = [word]() noexcept
							{
								if constexpr (Endian != std::endian::native)
								{
									return std::byteswap(word);
								}
								else
								{
									return word;
								}
							}();

							return +((native_word & 0xfc00) != 0xdc00);
						});
			}
			else
			{
				GAL_PROMETHEUS_STATIC_UNREACHABLE();
			}
		}

		// note: we are not BOM aware
		template<CharsCategory OutputCategory, std::endian Endian = std::endian::native>
		[[nodiscard]] constexpr auto length(const pointer_type input) const noexcept -> size_type
		{
			return this->length<OutputCategory, Endian>({input, std::char_traits<char_type>::length(input)});
		}

		template<CharsCategory OutputCategory, std::endian Endian = std::endian::native, InputProcessCriterion Criterion = InputProcessCriterion::RETURN_RESULT_TYPE, bool CheckNextBlock = true>
		[[nodiscard]] constexpr auto convert(const input_type input, typename output_type<OutputCategory>::pointer output) const noexcept -> std::conditional_t<Criterion == InputProcessCriterion::RETURN_RESULT_TYPE, result_type, std::size_t>
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
			GAL_PROMETHEUS_DEBUG_NOT_NULL(output);

			using output_pointer_type					= typename output_type<OutputCategory>::pointer;
			using output_char_type						= typename output_type<OutputCategory>::value_type;

			const auto				  input_length		= input.size();

			const pointer_type		  it_input_begin	= input.data();
			pointer_type			  it_input_current	= it_input_begin;
			const pointer_type		  it_input_end		= it_input_begin + input_length;

			const output_pointer_type it_output_begin	= output;
			output_pointer_type		  it_output_current = it_output_begin;

			if constexpr (OutputCategory == CharsCategory::ASCII)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if constexpr (CheckNextBlock)
					{
						struct pack
						{
							std::uint64_t v1;
							std::uint64_t v2;
							std::uint64_t v3;
							std::uint64_t v4;
						};

						const auto to_native_word = [](const pack word) noexcept -> pack
						{
							if constexpr (Endian != std::endian::native)
							{
								const auto [v1, v2, v3, v4] = word;
								return {
										.v1 = (v1 >> 8) | (v1 << (64 - 8)),
										.v2 = (v2 >> 8) | (v2 << (64 - 8)),
										.v3 = (v3 >> 8) | (v3 << (64 - 8)),
										.v4 = (v1 >> 8) | (v4 << (64 - 8))};
							}
							else
							{
								return word;
							}
						};

						// if it is safe to read 32 more bytes, check that they are ascii
						if (it_input_current + 16 <= it_input_end)
						{
							const auto [v1, v2, v3, v4] = to_native_word(
									{.v1 = utility::unaligned_load<std::uint64_t>(it_input_current + 0),
									 .v2 = utility::unaligned_load<std::uint64_t>(it_input_current + 4),
									 .v3 = utility::unaligned_load<std::uint64_t>(it_input_current + 8),
									 .v4 = utility::unaligned_load<std::uint64_t>(it_input_current + 12)});

							if (const auto value = (v1 | v2 | v3 | v4);
								(value & 0xff00'ff00'ff00'ff00) == 0)
							{
								std::ranges::transform(
										it_input_current,
										it_input_current + 16,
										it_output_current,
										[](const auto word) noexcept
										{
											if constexpr (Endian != std::endian::native)
											{
												return utility::char_cast<output_char_type>(std::byteswap(word));
											}
											else
											{
												return utility::char_cast<output_char_type>(word);
											}
										});

								// 15 more steps
								it_input_current += 15;
								it_output_current += 16;
								continue;
							}
						}
					}

					const auto word = [w = *it_input_current]() noexcept
					{
						if constexpr (Endian != std::endian::native)
						{
							return std::byteswap(w);
						}
						else
						{
							return w;
						}
					}();
					if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
					{
						if ((word & 0xff00) != 0)
						{
							if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
							{
								return 0;
							}
							else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
							{
								return result_type{.error = ErrorCode::TOO_LARGE, .count = length_if_error};
							}
							else
							{
								GAL_PROMETHEUS_STATIC_UNREACHABLE();
							}
						}
					}

					*(it_output_current + 0) = utility::char_cast<output_char_type>(word & 0xff);
					it_output_current += 1;
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF8)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if constexpr (CheckNextBlock)
					{
						const auto to_native_word = [](const auto word) noexcept -> auto
						{
							if constexpr (Endian != std::endian::native)
							{
								return (word >> 8) | (word << (64 - 8));
							}
							else
							{
								return word;
							}
						};

						// if it is safe to read 8 more bytes, check that they are ascii
						if (it_input_current + 4 <= it_input_end)
						{
							if (const auto value = to_native_word(utility::unaligned_load<std::uint64_t>(it_input_current));
								(value & 0xffff'ff80'ffff'ff80) == 0)
							{
								std::ranges::transform(
										it_input_current,
										it_input_current + 4,
										it_output_current,
										[](const auto word) noexcept
										{
											if constexpr (Endian != std::endian::native)
											{
												return utility::char_cast<output_char_type>(std::byteswap(word));
											}
											else
											{
												return utility::char_cast<output_char_type>(word);
											}
										});

								// 3 more step
								it_input_current += 3;
								it_output_current += 4;
								continue;
							}
						}
					}

					if (const auto word = [w = *it_input_current]() noexcept
						{
						if constexpr (Endian != std::endian::native)
						{
							return std::byteswap(w);
						}
						else
						{
							return w;
						} }();
						(word & 0xff80) == 0)
					{
						// 1-byte utf8
						*(it_output_current + 0) = utility::char_cast<output_char_type>(word);
						it_output_current += 1;
					}
					else if ((word & 0xf800) == 0)
					{
						// 2-bytes utf8
						// 0b110?'???? 0b10??'????
						*(it_output_current + 0) = utility::char_cast<output_char_type>((word >> 6) | 0b1100'0000);
						*(it_output_current + 1) = utility::char_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);
						it_output_current += 2;
					}
					else if ((word & 0xf800) != 0xd800)
					{
						// 3-bytes utf8
						// 0b1110'???? 0b10??'???? 0b10??'????
						*(it_output_current + 0) = utility::char_cast<output_char_type>((word >> 12) | 0b1110'0000);
						*(it_output_current + 1) = utility::char_cast<output_char_type>(((word >> 6) & 0b0011'1111) | 0b1000'0000);
						*(it_output_current + 2) = utility::char_cast<output_char_type>((word & 0b0011'1111) | 0b1000'0000);
						it_output_current += 3;
					}
					else
					{
						// 4-bytes utf8
						// must be a surrogate pair
						const auto diff = word - 0xd800;
						if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							if (diff > 0x3ff)
							{
								if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
								else
								{
									GAL_PROMETHEUS_STATIC_UNREACHABLE();
								}
							}
						}

						// minimal bound checking
						if (it_input_current + 1 >= it_input_end)
						{
							if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
							{
								return 0;
							}
							else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
							{
								return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
							}
							else
							{
								GAL_PROMETHEUS_STATIC_UNREACHABLE();
							}
						}

						const auto next_word = [w = *(it_input_current + 1)]() noexcept
						{
							if constexpr (Endian != std::endian::native)
							{
								return std::byteswap(w);
							}
							else
							{
								return w;
							}
						}();
						const auto next_diff = next_word - 0xdc00;
						if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							if (next_diff > 0x3ff)
							{
								// minimal bound checking
								if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
								else
								{
									GAL_PROMETHEUS_STATIC_UNREACHABLE();
								}
							}
						}

						const auto value		 = (diff << 10) + next_diff + 0x1'0000;

						// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
						*(it_output_current + 0) = utility::char_cast<output_char_type>((value >> 18) | 0b1111'0000);
						*(it_output_current + 1) = utility::char_cast<output_char_type>(((value >> 12) & 0b0011'1111) | 0b1000'0000);
						*(it_output_current + 2) = utility::char_cast<output_char_type>(((value >> 6) & 0b0011'1111) | 0b1000'0000);
						*(it_output_current + 3) = utility::char_cast<output_char_type>((value & 0b0011'1111) | 0b1000'0000);

						// one more step
						it_input_current += 1;
						it_output_current += 4;
					}
				}
			}
			else if constexpr (OutputCategory == CharsCategory::UTF16_LE or OutputCategory == CharsCategory::UTF16_BE)
			{
				// fixme: endian?
				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				it_input_current += input_length;
				it_output_current += input_length;
			}
			else if constexpr (OutputCategory == CharsCategory::UTF32)
			{
				for (; it_input_current < it_input_end; ++it_input_current)
				{
					const auto length_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

					if (const auto word = [w = *(it_input_current + 0)]() noexcept
						{
						if constexpr (Endian != std::endian::native)
						{
							return std::byteswap(w);
						}
						else
						{
							return w;
						} }();
						(word & 0xf800) != 0xd800)
					{
						// no surrogate pair, extend 16-bit word to 32-bit word
						*(it_output_current + 0) = utility::char_cast<output_char_type>(word);
						it_output_current += 1;
					}
					else
					{
						// must be a surrogate pair
						const auto diff = word - 0xd800;
						if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							if (diff > 0x3ff)
							{
								if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
								else
								{
									GAL_PROMETHEUS_STATIC_UNREACHABLE();
								}
							}
						}

						// minimal bound checking
						if (it_input_current + 1 >= it_input_end)
						{
							if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
							{
								return 0;
							}
							else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
							{
								return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
							}
							else
							{
								GAL_PROMETHEUS_STATIC_UNREACHABLE();
							}
						}

						const auto next_word = [w = *(it_input_current + 1)]() noexcept
						{
							if constexpr (Endian != std::endian::native)
							{
								return std::byteswap(w);
							}
							else
							{
								return w;
							}
						}();
						const auto next_diff = next_word - 0xdc00;
						if constexpr (Criterion != InputProcessCriterion::ASSUME_VALID_INPUT)
						{
							if (next_diff > 0x3ff)
							{
								// minimal bound checking
								if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT)
								{
									return 0;
								}
								else if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
								{
									return result_type{.error = ErrorCode::SURROGATE, .count = length_if_error};
								}
								else
								{
									GAL_PROMETHEUS_STATIC_UNREACHABLE();
								}
							}
						}

						const auto value		 = (diff << 10) + next_diff + 0x1'0000;

						*(it_output_current + 0) = utility::char_cast<output_char_type>(value);

						// one more step
						it_input_current += 1;
						it_output_current += 1;
					}
				}
			}
			else
			{
				GAL_PROMETHEUS_UNREACHABLE();
			}

			if constexpr (Criterion == InputProcessCriterion::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT or Criterion == InputProcessCriterion::ASSUME_VALID_INPUT)
			{
				return static_cast<std::size_t>(it_output_current - it_output_begin);
			}
			if constexpr (Criterion == InputProcessCriterion::RETURN_RESULT_TYPE)
			{
				return result_type{.error = ErrorCode::NONE, .count = static_cast<std::size_t>(it_input_current - it_input_begin)};
			}
			else
			{
				GAL_PROMETHEUS_STATIC_UNREACHABLE();
			}
		}

		template<CharsCategory OutputCategory, std::endian Endian = std::endian::native, InputProcessCriterion Criterion = InputProcessCriterion::RETURN_RESULT_TYPE, bool CheckNextBlock = true>
		[[nodiscard]] constexpr auto convert(const pointer_type input, typename output_type<OutputCategory>::pointer output) const noexcept -> std::conditional_t<Criterion == InputProcessCriterion::RETURN_RESULT_TYPE, result_type, std::size_t>
		{
			return this->convert<OutputCategory, Endian, Criterion, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, output);
		}

		template<typename StringType, CharsCategory OutputCategory, std::endian Endian = std::endian::native, InputProcessCriterion Criterion = InputProcessCriterion::RETURN_RESULT_TYPE, bool CheckNextBlock = true>
			requires requires(StringType& string) {
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<typename output_type<OutputCategory>::pointer>;
			}
		[[nodiscard]] constexpr auto convert(const input_type input) const noexcept -> StringType
		{
			StringType result{};
			result.resize(this->length<OutputCategory>(input));

			(void)this->convert<OutputCategory, Endian, Criterion, CheckNextBlock>(input, result.data());
			return result;
		}

		template<typename StringType, CharsCategory OutputCategory, std::endian Endian = std::endian::native, InputProcessCriterion Criterion = InputProcessCriterion::RETURN_RESULT_TYPE, bool CheckNextBlock = true>
			requires requires(StringType& string) {
				string.resize(std::declval<size_type>());
				{
					string.data()
				} -> std::convertible_to<typename output_type<OutputCategory>::pointer>;
			}
		[[nodiscard]] constexpr auto convert(const pointer_type input) const noexcept -> StringType
		{
			StringType result{};
			result.resize(this->length<OutputCategory, Endian>(input));

			return this->convert<OutputCategory, Endian, Criterion, CheckNextBlock>({input, std::char_traits<char_type>::length(input)}, result.data());
		}

		template<std::endian Endian = std::endian::native>
		[[nodiscard]] constexpr auto code_points(const input_type input) const noexcept -> std::size_t
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			return std::ranges::count_if(
					input,
					[](auto word) noexcept -> bool
					{
						if constexpr (Endian != std::endian::native) { word = std::byteswap(word); }

						return (word & 0xfc00) != 0xdc00;
					});
		}

		constexpr auto flip_endian(const input_type input, const output_type<input_category>::pointer output) const noexcept -> void
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());
			GAL_PROMETHEUS_DEBUG_NOT_NULL(output);

			std::ranges::transform(
					input,
					output,
					[](const auto word) noexcept
					{
						return utility::char_cast<output_type<input_category>::value_type>(word >> 8 | word << 8);
					});
		}
	};

	export namespace instance::scalar
	{
		constexpr Scalar<"utf16"> utf16{};
	}
}// namespace gal::prometheus::chars
