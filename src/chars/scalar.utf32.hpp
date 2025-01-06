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

namespace gal::prometheus::chars
{
	template<>
	class Scalar<"utf32">;

	template<>
	struct descriptor_type<Scalar<"utf32">>
	{
		constexpr static auto chars_type = CharsType::UTF32;

		using input_type = input_type_of<chars_type>;
		using char_type = input_type::value_type;
		using pointer_type = input_type::const_pointer;
		using size_type = input_type::size_type;

		using data_type = std::uint64_t;

		constexpr static std::ptrdiff_t advance_per_step = sizeof(data_type) / sizeof(char_type);
		// zero extend ==> 1 char -> 1 out_char
		template<CharsType Type>
		constexpr static std::ptrdiff_t advance_per_step_with = advance_per_step;
	};

	template<>
	class Scalar<"utf32">
	{
	public:
		using descriptor_type = descriptor_type<Scalar>;

		constexpr static auto chars_type = descriptor_type::chars_type;

		using input_type = descriptor_type::input_type;
		using char_type = descriptor_type::char_type;
		using pointer_type = descriptor_type::pointer_type;
		using size_type = descriptor_type::size_type;

		using data_type = descriptor_type::data_type;

		template<bool Detail = false>
		[[nodiscard]] constexpr static auto validate(const input_type input) noexcept -> auto
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			constexpr auto process_policy = Detail ? InputProcessPolicy::DEFAULT : InputProcessPolicy::RESULT;

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			while (it_input_current != it_input_end)
			{
				const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
				constexpr auto current_output_length = static_cast<std::size_t>(0);

				if (const auto word = static_cast<std::uint32_t>(*(it_input_current + 0));
					word > 0x10'ffff)
				{
					return make_result<process_policy>(
						ErrorCode::TOO_LARGE,
						current_input_length,
						current_output_length
					);
				}
				else if (word >= 0xd800 and word <= 0xdfff)
				{
					return make_result<process_policy>(
						ErrorCode::SURROGATE,
						current_input_length,
						current_output_length
					);
				}

				it_input_current += 1;
			}

			// ==================================================
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
			const auto current_input_length = static_cast<std::size_t>(input_length);
			constexpr auto current_output_length = static_cast<std::size_t>(0);
			return make_result<process_policy>(
				ErrorCode::NONE,
				current_input_length,
				current_output_length
			);
		}

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
					[](const auto data) noexcept
					{
						const auto v = static_cast<std::uint32_t>(data);
						return
								1 // ascii
								+
								(v > 0x7f) // two-byte
								+
								(v > 0x7ff) // three-byte
								+
								(v > 0xffff) // four-byte
								;
					}
				);
			}
			else if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE or OutputType == CharsType::UTF16)
			{
				return std::transform_reduce(
					input.begin(),
					input.end(),
					static_cast<size_type>(0),
					std::plus<>{},
					[](const auto data) noexcept
					{
						const auto v = static_cast<std::uint32_t>(data);
						return
								1 // non-surrogate word
								+
								(v > 0xffff) // surrogate pair
								;
					}
				);
			}
			// ReSharper disable CppClangTidyBugproneBranchClone
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
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(Scalar::validate(input));
			}

			using output_type = output_type_of<OutputType>;
			using output_pointer_type = typename output_type::pointer;

			const auto input_length = input.size();

			const pointer_type it_input_begin = input.data();
			pointer_type it_input_current = it_input_begin;
			const pointer_type it_input_end = it_input_begin + input_length;

			const output_pointer_type it_output_begin = output;
			output_pointer_type it_output_current = it_output_begin;

			if constexpr (OutputType == CharsType::UTF32)
			{
				if constexpr (not assume_all_correct<ProcessPolicy>())
				{
					if (const auto result = Scalar::validate<true>(input);
						result.has_error())
					{
						if constexpr (write_all_correct<ProcessPolicy>())
						{
							std::memcpy(it_output_current, it_input_current, result.input * sizeof(char_type));
							it_input_current += result.input;
							it_output_current += result.input;
						}

						return make_result<ProcessPolicy>(
							result.error,
							result.input,
							result.input
						);
					}
				}

				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				it_input_current += input_length;
				it_output_current += input_length;
			}
			else
			{
				const auto transform = [
							// Used to calculate the processed input length
							it_input_begin,
							&it_input_current,
							// Used to calculate the processed output length
							it_output_begin,
							&it_output_current
						]<bool Pure>(const size_type n) noexcept -> auto
				{
					// [error/input/output]
					constexpr auto process_policy_keep_all_result = InputProcessPolicy::WRITE_ALL_CORRECT_2;

					const auto end = it_input_current + n;

					while (it_input_current < end)
					{
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						if constexpr (const auto data = static_cast<std::uint32_t>(*(it_input_current + 0));
							Pure)
						{
							*(it_output_current + 0) = to_output_type<OutputType>(data);

							it_output_current += 1;
							it_input_current += 1;
						}
						else
						{
							if constexpr (OutputType == CharsType::LATIN)
							{
								if constexpr (not assume_all_correct<ProcessPolicy>())
								{
									if ((data & 0xffff'ff00) != 0)
									{
										return make_result<process_policy_keep_all_result>(
											ErrorCode::TOO_LARGE,
											current_input_length,
											current_output_length
										);
									}
								}

								*(it_output_current + 0) = to_output_type<OutputType>(data & 0xff);

								it_output_current += 1;
								it_input_current += 1;
							}
							else if constexpr (OutputType == CharsType::UTF8_CHAR or OutputType == CharsType::UTF8)
							{
								if ((data & 0xffff'ff80) == 0)
								{
									// 1-byte utf8

									*(it_output_current + 0) = to_output_type<OutputType>(data);

									it_input_current += 1;
									it_output_current += 1;
								}
								else if ((data & 0xffff'f800) == 0)
								{
									// 2-bytes utf8

									// 0b110?'???? 0b10??'????
									*(it_output_current + 0) = to_output_type<OutputType>((data >> 6) | 0b1100'0000);
									*(it_output_current + 1) = to_output_type<OutputType>((data & 0b0011'1111) | 0b1000'0000);

									it_input_current += 1;
									it_output_current += 2;
								}
								else if ((data & 0xffff'0000) == 0)
								{
									// 3-bytes utf8

									if constexpr (not assume_all_correct<ProcessPolicy>())
									{
										if (data >= 0xd800 and data <= 0xdfff)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::SURROGATE,
												current_input_length,
												current_output_length
											);
										}
									}

									// 0b1110'???? 0b10??'???? 0b10??'????
									*(it_output_current + 0) = to_output_type<OutputType>((data >> 12) | 0b1110'0000);
									*(it_output_current + 1) = to_output_type<OutputType>(((data >> 6) & 0b0011'1111) | 0b1000'0000);
									*(it_output_current + 2) = to_output_type<OutputType>((data & 0b0011'1111) | 0b1000'0000);

									it_input_current += 1;
									it_output_current += 3;
								}
								else
								{
									// 4-bytes utf8

									if constexpr (not assume_all_correct<ProcessPolicy>())
									{
										if (data > 0x0010'ffff)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::TOO_LARGE,
												current_input_length,
												current_output_length
											);
										}
									}

									// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
									*(it_output_current + 0) = to_output_type<OutputType>((data >> 18) | 0b1111'0000);
									*(it_output_current + 1) = to_output_type<OutputType>(((data >> 12) & 0b0011'1111) | 0b1000'0000);
									*(it_output_current + 2) = to_output_type<OutputType>(((data >> 6) & 0b0011'1111) | 0b1000'0000);
									*(it_output_current + 3) = to_output_type<OutputType>((data & 0b0011'1111) | 0b1000'0000);

									it_input_current += 1;
									it_output_current += 4;
								}
							}
							else if constexpr (
								OutputType == CharsType::UTF16_LE or
								OutputType == CharsType::UTF16_BE
							)
							{
								if ((data & 0xffff'0000) == 0)
								{
									if constexpr (not assume_all_correct<ProcessPolicy>())
									{
										if (data >= 0xd800 and data <= 0xdfff)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::SURROGATE,
												current_input_length,
												current_output_length
											);
										}
									}

									*(it_output_current + 0) = to_output_type<OutputType>(data);

									it_input_current += 1;
									it_output_current += 1;
								}
								else
								{
									// will generate a surrogate pair

									if constexpr (not assume_all_correct<ProcessPolicy>())
									{
										if (data > 0x0010'ffff)
										{
											return make_result<process_policy_keep_all_result>(
												ErrorCode::TOO_LARGE,
												current_input_length,
												current_output_length
											);
										}
									}

									const auto [high_surrogate, low_surrogate] = [d = data - 0x0001'0000]() noexcept
									{
										const auto high = static_cast<std::uint16_t>(0xd800 + (d >> 10));
										const auto low = static_cast<std::uint16_t>(0xdc00 + (d & 0x3ff));

										return std::make_pair(high, low);
									}();

									*(it_output_current + 0) = to_output_type<OutputType>(high_surrogate);
									*(it_output_current + 1) = to_output_type<OutputType>(low_surrogate);

									it_input_current += 1;
									it_output_current += 2;
								}
							}
							else
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
							}
						}
					}

					// ==================================================
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current >= end);
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					constexpr auto current_output_length = static_cast<std::size_t>(0);
					return make_result<process_policy_keep_all_result>(
						ErrorCode::NONE,
						current_input_length,
						current_output_length
					);
				};

				while (it_input_current + descriptor_type::advance_per_step <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, descriptor_type::advance_per_step};
					#endif

					const auto value = memory::unaligned_load<std::uint64_t>(it_input_current + 0);
					const auto pure = [value]() noexcept -> bool
					{
						if constexpr (OutputType == CharsType::LATIN)
						{
							return (value & static_cast<data_type>(0xffff'ff00'ffff'ff00)) == 0;
						}
						else
						{
							return (value & static_cast<data_type>(0xff80'ff80'ff80'ff80)) == 0;
						}
					}();

					if (not pure)
					{
						if (const auto result = transform.template operator()<false>(descriptor_type::advance_per_step);
							result.has_error())
						{
							return make_result<ProcessPolicy>(
								result.error,
								result.input,
								result.output
							);
						}
					}
					else
					{
						const auto result = transform.template operator()<true>(descriptor_type::advance_per_step);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
					}
				}

				const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < descriptor_type::advance_per_step);

				#if GAL_PROMETHEUS_COMPILER_DEBUG
				[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
				#endif

				if (const auto result = transform.template operator()<false>(remaining);
					result.has_error())
				{
					return make_result<ProcessPolicy>(
						result.error,
						result.input,
						result.output
					);
				}
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
			return Scalar::convert<string_type, OutputType, ProcessPolicy>({input, std::char_traits<char_type>::length(input)});
		}
	};
}
