// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <chars/scalar_1.hpp>

#include <ranges>
#include <algorithm>

#include <prometheus/macro.hpp>

#include <memory/rw.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace
{
	using namespace gal::prometheus;
	using namespace chars_1;

	using data_type = scalar::data_type;

	namespace common
	{
		template<std::endian SourceEndian, typename ValueType>
			requires (std::is_same_v<ValueType, std::uint16_t> or std::is_same_v<ValueType, char16_t>)
		[[nodiscard]] constexpr auto to_native_utf16(const ValueType value) noexcept -> std::uint16_t
		{
			if constexpr (SourceEndian != std::endian::native)
			{
				return std::byteswap(value);
			}
			else
			{
				return value;
			}
		}

		template<CharsType Type, typename ValueType>
			requires (Type == CharsType::UTF16_LE or Type == CharsType::UTF16_BE) and
			         (std::is_same_v<ValueType, std::uint16_t> or std::is_same_v<ValueType, char16_t>)
		[[nodiscard]] constexpr auto to_native_utf16(const ValueType value) noexcept -> std::uint16_t
		{
			return common::to_native_utf16<Type == CharsType::UTF16_LE ? std::endian::little : std::endian::big>(value);
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr auto to_char(const auto value) noexcept -> typename output_type_of<OutputType>::value_type
		{
			using char_type = typename output_type_of<OutputType>::value_type;

			if constexpr (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE
			)
			{
				return static_cast<char_type>(common::to_native_utf16<OutputType>(static_cast<std::uint16_t>(value)));
			}
			else
			{
				return static_cast<char_type>(value);
			}
		}

		template<CharsType InputType>
			requires (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
		[[nodiscard]] constexpr auto sign_of(const data_type data) noexcept -> auto
		{
			struct sign_type
			{
			private:
				data_type data_;

			public:
				constexpr explicit sign_type(const data_type d) noexcept
					: data_{d} {}

				/**
				 * @brief Get the underlying mask of the current block.
				 */
				[[nodiscard]] constexpr auto mask() const noexcept -> std::uint8_t
				{
					// MSB => LSB
					const auto msb = (data_ >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);
					const auto packed = msb * static_cast<data_type>(0x01'02'04'08'10'20'40'80);
					const auto mask = static_cast<std::uint8_t>(packed >> 56);

					return mask;
				}

				/**
				 * @brief Whether all sign bits are 0, in other words, whether the current block is all ASCII.
				 */
				[[nodiscard]] constexpr auto pure() const noexcept -> bool
				{
					return (data_ & 0x8080'8080'8080'8080) == 0;
				}

				/**
				 * @brief Get the number of non-ASCII in the current block.
				 */
				[[nodiscard]] constexpr auto count() const noexcept -> std::size_t
				{
					// 	const auto mask = this->mask(data_);
					// const auto count = std::popcount(mask);
					// return count;

					// MSB => LSB
					const auto msb = (data_ >> 7) & static_cast<data_type>(0x01'01'01'01'01'01'01'01);

					// const auto packed = msb * static_cast<data_type>(0x01'01'01'01'01'01'01'01);
					// const auto count = static_cast<std::uint8_t>(packed >> 56);

					const auto count = std::popcount(msb);
					return count;
				}

				/**
				 * @brief Get the number of consecutive ASCII at the beginning.
				 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
				 * ^-----^ start_count
				 *                                                             ^----------^ end_count           
				 */
				[[nodiscard]] constexpr auto start_count() const noexcept -> std::size_t
				{
					const auto mask = this->mask();

					return std::countr_zero(mask);
				}

				/**
				 * @brief Get the number of consecutive ASCII at the ending.
				 * [ascii] [non-ascii] [?] [?] ... Xn ... [?] [?] [ascii] [ascii]
				 * ^-----^ start_count
				 *                                                             ^----------^ end_count       
				 */
				[[nodiscard]] constexpr auto end_count() const noexcept -> std::size_t
				{
					const auto mask = this->mask();

					return std::countl_zero(mask);
				}
			};

			return sign_type{data};
		}
	}

	namespace latin
	{
		using input_type = chars_1::latin::input_type;
		using char_type = chars_1::latin::char_type;
		using size_type = chars_1::latin::size_type;
		using pointer_type = chars_1::latin::pointer_type;

		[[nodiscard]] constexpr auto validate(const pointer_type current, const pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			std::ignore = end;
			constexpr std::ptrdiff_t length = 1;

			if (const auto value = static_cast<std::uint8_t>(*(current + 0));
				static_cast<std::uint8_t>(value) < 0x80)
			{
				return {length, ErrorCode::NONE};
			}

			return {length, ErrorCode::TOO_LARGE};
		}

		// 1 LATIN => 1/2 UTF-8
		template<CharsType OutputType, bool Pure, bool Correct>
			requires (
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
		[[nodiscard]] constexpr auto write_utf8(
			typename output_type_of<OutputType>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			std::ignore = Correct;
			std::ignore = end;

			constexpr std::ptrdiff_t length = 1;

			if constexpr (const auto value = static_cast<std::uint8_t>(*(current + 0));
				Pure)
			{
				*(output + 0) = common::to_char<OutputType>(value);

				output += 1;
				return {length, ErrorCode::NONE};
			}
			else
			{
				if ((value & 0x80) == 0)
				{
					// ASCII
					*(output + 0) = common::to_char<OutputType>(value);

					output += 1;
					return {length, ErrorCode::NONE};
				}

				// 0b110?'???? 0b10??'????
				const auto c1 = static_cast<std::uint8_t>((value >> 6) | 0b1100'0000);
				const auto c2 = static_cast<std::uint8_t>((value & 0b0011'1111) | 0b1000'0000);

				*(output + 0) = common::to_char<OutputType>(c1);
				*(output + 1) = common::to_char<OutputType>(c2);

				output += 2;
				return {length, ErrorCode::NONE};
			}
		}

		// 1 LATIN => 1 UTF-16
		template<CharsType OutputType, bool Pure, bool Correct>
			requires (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE
			)
		[[nodiscard]] constexpr auto write_utf16(
			typename output_type_of<OutputType>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			std::ignore = Pure;
			std::ignore = Correct;
			std::ignore = end;

			constexpr std::ptrdiff_t length = 1;

			const auto value = static_cast<std::uint8_t>(*(current + 0));

			*(output + 0) = common::to_char<OutputType>(value);

			output += 1;
			return {length, ErrorCode::NONE};
		}

		// 1 LATIN => 1 UTF-32
		template<CharsType OutputType, bool Pure, bool Correct>
			requires (
				OutputType == CharsType::UTF32
			)
		[[nodiscard]] constexpr auto write_utf32(
			typename output_type_of<OutputType>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			std::ignore = Pure;
			std::ignore = Correct;
			std::ignore = end;

			constexpr std::ptrdiff_t length = 1;

			const auto value = static_cast<std::uint8_t>(*(current + 0));

			*(output + 0) = common::to_char<OutputType>(value);

			output += 1;
			return {length, ErrorCode::NONE};
		}

		namespace scalar
		{
			template<CharsType OutputType>
			[[nodiscard]] constexpr auto advance_of() noexcept -> std::ptrdiff_t
			{
				std::ignore = OutputType;

				return sizeof(data_type) / sizeof(char_type);
			}

			template<CharsType OutputType>
			[[nodiscard]] constexpr auto read(const pointer_type source) noexcept -> data_type
			{
				std::ignore = OutputType;

				return memory::unaligned_load<data_type>(source);
			}

			[[nodiscard]] constexpr auto validate(const input_type input) noexcept -> result_error_input_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				constexpr auto advance = scalar::advance_of<CharsType::LATIN>();

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = scalar::read<CharsType::LATIN>(it_input_current);

					if (const auto sign = common::sign_of<CharsType::LATIN>(data);
						not sign.pure())
					{
						it_input_current += sign.start_count();

						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

						return {.error = ErrorCode::TOO_LARGE, .input = current_input_length};
					}

					it_input_current += advance;
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					const auto end = it_input_current + remaining;
					while (it_input_current < end)
					{
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

						const auto [length, error] = ::latin::validate(it_input_current, it_input_end);
						GAL_PROMETHEUS_ERROR_ASSUME(length == 1);

						if (error != ErrorCode::NONE)
						{
							return {.error = error, .input = current_input_length};
						}

						it_input_current += length;
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				return {.error = ErrorCode::NONE, .input = current_input_length};
			}

			template<CharsType OutputType>
			[[nodiscard]] constexpr auto length(const input_type input) noexcept -> size_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				if constexpr (
					OutputType == CharsType::UTF8_CHAR or
					OutputType == CharsType::UTF8
				)
				{
					constexpr auto advance = scalar::advance_of<OutputType>();

					const auto input_length = input.size();

					const pointer_type it_input_begin = input.data();
					pointer_type it_input_current = it_input_begin;
					const pointer_type it_input_end = it_input_begin + input_length;

					size_type output_length = input_length;

					while (it_input_current + advance <= it_input_end)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
						#endif

						const auto data = scalar::read<OutputType>(it_input_current);

						if (const auto sign = common::sign_of<CharsType::LATIN>(data);
							not sign.pure())
						{
							output_length += sign.count();
						}

						it_input_current += advance;
					}

					const auto remaining = it_input_end - it_input_current;
					GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

					if (remaining != 0)
					{
						#if GAL_PROMETHEUS_COMPILER_DEBUG
						[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
						#endif

						const auto end = it_input_current + remaining;
						while (it_input_current < end)
						{
							const auto [length, error] = ::latin::validate(it_input_current, it_input_end);
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
				else if constexpr (
					OutputType == CharsType::UTF16_LE or
					OutputType == CharsType::UTF16_BE or
					OutputType == CharsType::UTF16
				)
				{
					return input.size();
				}
				else if constexpr (
					OutputType == CharsType::UTF32
				)
				{
					return input.size();
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::UTF8_CHAR or
					OutputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto write_utf8(
				typename output_type_of<OutputType>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				const auto advance = scalar::advance_of<OutputType>();
				const auto transform = [&]<bool P>(const decltype(advance) n) noexcept -> void
				{
					const auto end = it_input_current + n;
					while (it_input_current < end)
					{
						const auto [length, error] = ::latin::write_utf8<OutputType, P, Correct>(it_output_current, it_input_current, it_input_end);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == 1);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(error == ErrorCode::NONE);

						it_input_current += length;
					}
				};

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = scalar::read<OutputType>(it_input_current);

					if constexpr (Pure)
					{
						transform.template operator()<true>(advance);
					}
					else
					{
						if (const auto sign = common::sign_of<CharsType::LATIN>(data);
							not sign.pure())
						{
							const auto start_count = sign.start_count();
							const auto end_count = sign.end_count();
							const auto unknown_count = advance - start_count - end_count;

							transform.template operator()<true>(static_cast<decltype(advance)>(start_count));
							transform.template operator()<false>(static_cast<decltype(advance)>(unknown_count));
						}
						else
						{
							transform.template operator()<true>(advance);
						}
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					transform.template operator()<false>(remaining);
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::UTF16_LE or
					OutputType == CharsType::UTF16_BE
				)
			[[nodiscard]] constexpr auto write_utf16(
				typename output_type_of<OutputType>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				const auto advance = scalar::advance_of<OutputType>();
				const auto transform = [&]<bool P>(const decltype(advance) n) noexcept -> void
				{
					const auto end = it_input_current + n;
					while (it_input_current < end)
					{
						const auto [length, error] = ::latin::write_utf16<OutputType, P, Correct>(it_output_current, it_input_current, it_input_end);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == 1);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(error == ErrorCode::NONE);

						it_input_current += length;
					}
				};

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = scalar::read<OutputType>(it_input_current);

					if constexpr (Pure)
					{
						transform.template operator()<true>(advance);
					}
					else
					{
						if (const auto sign = common::sign_of<CharsType::LATIN>(data);
							not sign.pure())
						{
							const auto start_count = sign.start_count();
							const auto end_count = sign.end_count();
							const auto unknown_count = advance - start_count - end_count;

							transform.template operator()<true>(static_cast<decltype(advance)>(start_count));
							transform.template operator()<false>(static_cast<decltype(advance)>(unknown_count));
						}
						else
						{
							transform.template operator()<true>(advance);
						}
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					transform.template operator()<false>(remaining);
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::UTF32
				)
			[[nodiscard]] constexpr auto write_utf32(
				typename output_type_of<OutputType>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				const auto advance = scalar::advance_of<OutputType>();
				const auto transform = [&]<bool P>(const decltype(advance) n) noexcept -> void
				{
					const auto end = it_input_current + n;
					while (it_input_current < end)
					{
						const auto [length, error] = ::latin::write_utf32<OutputType, P, Correct>(it_output_current, it_input_current, it_input_end);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == 1);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(error == ErrorCode::NONE);

						it_input_current += length;
					}
				};

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = scalar::read<OutputType>(it_input_current);

					if constexpr (Pure)
					{
						transform.template operator()<true>(advance);
					}
					else
					{
						if (const auto sign = common::sign_of<CharsType::LATIN>(data);
							not sign.pure())
						{
							const auto start_count = sign.start_count();
							const auto end_count = sign.end_count();
							const auto unknown_count = advance - start_count - end_count;

							transform.template operator()<true>(static_cast<decltype(advance)>(start_count));
							transform.template operator()<false>(static_cast<decltype(advance)>(unknown_count));
						}
						else
						{
							transform.template operator()<true>(advance);
						}
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					transform.template operator()<false>(remaining);
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType OutputType, bool Pure, bool Correct>
				requires (
					OutputType == CharsType::LATIN
				)
			[[nodiscard]] constexpr auto write_latin(
				typename output_type_of<OutputType>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				std::ignore = Pure;
				std::ignore = Correct;

				using output_type = output_type_of<CharsType::LATIN>;
				using output_pointer_type = output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				std::memcpy(it_output_current, it_input_current, input_length * sizeof(char_type));
				it_input_current += input_length;
				it_output_current += input_length;

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}
		}
	}

	namespace utf8
	{
		namespace detail
		{
			template<CharsType InputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto check_byte_1(
				const typename input_type_of<InputType>::const_pointer current
			) noexcept -> bool
			{
				const auto value = static_cast<std::uint8_t>(*(current + 0));

				return (value & 0x80) == 0;
			}

			template<CharsType InputType, CharsType OutputType, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::LATIN or
					         OutputType == CharsType::UTF16_LE or
					         OutputType == CharsType::UTF16_BE or
					         OutputType == CharsType::UTF32
				         )
			[[nodiscard]] constexpr auto write_byte_1(
				typename output_type_of<OutputType>::pointer& output,
				const typename input_type_of<InputType>::const_pointer current,
				const typename input_type_of<InputType>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(check_byte_1<InputType>(current));

				std::ignore = Correct;
				std::ignore = end;

				constexpr std::ptrdiff_t length = 1;

				const auto value = static_cast<std::uint8_t>(*(current + 0));

				*(output + 0) = common::to_char<OutputType>(value);

				output += 1;
				return {length, ErrorCode::NONE};
			}

			template<CharsType InputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto check_byte_2(
				const typename input_type_of<InputType>::const_pointer current
			) noexcept -> bool
			{
				const auto value = static_cast<std::uint8_t>(*(current + 0));

				return (value & 0b1110'0000) == 0b1100'0000;
			}

			template<CharsType InputType, CharsType OutputType, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::LATIN or
					         OutputType == CharsType::UTF16_LE or
					         OutputType == CharsType::UTF16_BE or
					         OutputType == CharsType::UTF32
				         )
			[[nodiscard]] constexpr auto write_byte_2(
				typename output_type_of<OutputType>::pointer& output,
				const typename input_type_of<InputType>::const_pointer current,
				const typename input_type_of<InputType>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(check_byte_2<InputType>(current));

				// we have a two-byte UTF-8
				constexpr std::ptrdiff_t length = 2;

				// minimal bound checking
				if (current + 1 >= end)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				const auto leading_byte = static_cast<std::uint8_t>(*(current + 0));
				const auto next_byte = static_cast<std::uint8_t>(*(current + 1));

				// checks if the next byte is a valid continuation byte in UTF-8.
				// A valid continuation byte starts with 10.
				if ((next_byte & 0b1100'0000) != 0b1000'0000)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				// assembles the Unicode code point from the two bytes.
				// It does this by discarding the leading 110 and 10 bits from the two bytes,
				// shifting the remaining bits of the first byte,
				// and then combining the results with a bitwise OR operation.
				const auto code_point = static_cast<std::uint32_t>(
					(leading_byte & 0b0001'1111) << 6 |
					(next_byte & 0b0011'1111)
				);

				if constexpr (not Correct)
				{
					if (code_point < 0x80)
					{
						return {length, ErrorCode::OVERLONG};
					}

					if (code_point >
					    []() noexcept -> std::uint32_t
					    {
						    if constexpr (OutputType == CharsType::LATIN) { return 0xff; }
						    else { return 0x7ff; }
					    }()
					)
					{
						return {length, ErrorCode::TOO_LARGE};
					}
				}

				*(output + 0) = common::to_char<OutputType>(code_point);

				output += 1;
				return {length, ErrorCode::NONE};
			}

			template<CharsType InputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto check_byte_3(
				const typename input_type_of<InputType>::const_pointer current
			) noexcept -> bool
			{
				const auto value = static_cast<std::uint8_t>(*(current + 0));

				return (value & 0b1111'0000) == 0b1110'0000;
			}

			template<CharsType InputType, CharsType OutputType, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::UTF16_LE or
					         OutputType == CharsType::UTF16_BE or
					         OutputType == CharsType::UTF32
				         )
			[[nodiscard]] constexpr auto write_byte_3(
				typename output_type_of<OutputType>::pointer& output,
				const typename input_type_of<InputType>::const_pointer current,
				const typename input_type_of<InputType>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(check_byte_3<InputType>(current));

				// we have a three-byte UTF-8
				constexpr std::ptrdiff_t length = 3;

				// minimal bound checking
				if (current + 2 >= end)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				const auto leading_byte = static_cast<std::uint8_t>(*(current + 0));
				const auto next_byte_1 = static_cast<std::uint8_t>(*(current + 1));
				const auto next_byte_2 = static_cast<std::uint8_t>(*(current + 2));

				if constexpr (not Correct)
				{
					if (
						((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
						((next_byte_2 & 0b1100'0000) != 0b1000'0000)
					)
					{
						return {length, ErrorCode::TOO_SHORT};
					}
				}

				const auto code_point = static_cast<std::uint32_t>(
					(leading_byte & 0b0000'1111) << 12 |
					(next_byte_1 & 0b0011'1111) << 6 |
					(next_byte_2 & 0b0011'1111)
				);

				if constexpr (not Correct)
				{
					if (code_point < 0x800)
					{
						return {length, ErrorCode::OVERLONG};
					}

					if (code_point > 0xffff)
					{
						return {length, ErrorCode::TOO_LARGE};
					}

					if (code_point > 0xd7ff and code_point < 0xe000)
					{
						return {length, ErrorCode::SURROGATE};
					}
				}

				*(output + 0) = common::to_char<OutputType>(code_point);

				output += 1;
				return {length, ErrorCode::NONE};
			}

			template<CharsType InputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto check_byte_4(
				const typename input_type_of<InputType>::const_pointer current
			) noexcept -> bool
			{
				const auto value = static_cast<std::uint8_t>(*(current + 0));

				return (value & 0b1111'1000) == 0b1111'0000;
			}

			template<CharsType InputType, CharsType OutputType, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::UTF16_LE or
					         OutputType == CharsType::UTF16_BE or
					         OutputType == CharsType::UTF32
				         )
			[[nodiscard]] constexpr auto write_byte_4(
				typename output_type_of<OutputType>::pointer& output,
				const typename input_type_of<InputType>::const_pointer current,
				const typename input_type_of<InputType>::const_pointer end
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(check_byte_4<InputType>(current));

				// we have a four-byte UTF-8 word
				constexpr std::ptrdiff_t length = 4;

				// minimal bound checking
				if (current + 3 >= end)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				const auto leading_byte = static_cast<std::uint8_t>(*(current + 0));
				const auto next_byte_1 = static_cast<std::uint8_t>(*(current + 1));
				const auto next_byte_2 = static_cast<std::uint8_t>(*(current + 2));
				const auto next_byte_3 = static_cast<std::uint8_t>(*(current + 3));

				if constexpr (not Correct)
				{
					if (
						((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
						((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
						((next_byte_3 & 0b1100'0000) != 0b1000'0000)
					)
					{
						return {length, ErrorCode::TOO_SHORT};
					}
				}

				const auto code_point = static_cast<std::uint32_t>(
					(leading_byte & 0b0000'0111) << 18 |
					(next_byte_1 & 0b0011'1111) << 12 |
					(next_byte_2 & 0b0011'1111) << 6 |
					(next_byte_3 & 0b0011'1111)
				);

				if constexpr (not Correct)
				{
					if (code_point <= 0xffff)
					{
						return {length, ErrorCode::OVERLONG};
					}

					if (code_point > 0x10'ffff)
					{
						return {length, ErrorCode::TOO_LARGE};
					}
				}


				if constexpr (OutputType == CharsType::UTF32)
				{
					*(output + 0) = common::to_char<OutputType>(code_point);

					output += 1;
				}
				else
				{
					const auto [high_surrogate, low_surrogate] = [cp = code_point - 0x1'0000]() noexcept -> auto
					{
						const auto high = static_cast<std::uint16_t>(0xd800 + (cp >> 10));
						const auto low = static_cast<std::uint16_t>(0xdc00 + (cp & 0x3ff));

						return std::make_pair(high, low);
					}();

					*(output + 0) = common::to_char<OutputType>(high_surrogate);
					*(output + 1) = common::to_char<OutputType>(low_surrogate);

					output += 2;
				}

				return {length, ErrorCode::NONE};
			}

			template<CharsType InputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto invalid_input(
				const typename input_type_of<InputType>::const_pointer current
			) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
			{
				const auto value = static_cast<std::uint8_t>(*(current + 0));

				// we either have too many continuation bytes or an invalid leading byte
				constexpr std::ptrdiff_t length = 0;

				if ((value & 0b1100'0000) == 0b1000'0000)
				{
					// we have too many continuation bytes
					return {length, ErrorCode::TOO_LONG};
				}

				// we have an invalid leading byte
				return {length, ErrorCode::HEADER_BITS};
			}

			template<CharsType InputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto code_points(const input_type_of<InputType> input) noexcept -> std::size_t
			{
				return std::ranges::count_if(
					input,
					[](const auto byte) noexcept -> bool
					{
						const auto b = static_cast<std::int8_t>(byte);

						// -65 is 0b10111111, anything larger in two-complement's should start a new code point.
						return b > -65;
					}
				);
			}
		}

		template<CharsType InputType>
			requires (
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
		[[nodiscard]] constexpr auto validate(
			const typename input_type_of<InputType>::const_pointer current,
			const typename input_type_of<InputType>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			// 1-byte UTF-8
			// 2-bytes UTF-8 
			// 3-bytes UTF-8 
			// 4-bytes UTF-8

			const auto leading_byte = static_cast<std::uint8_t>(*(current + 0));

			if (detail::check_byte_1<InputType>(current))
			{
				// we have a one-byte UTF-8
				constexpr std::ptrdiff_t length = 1;

				return {length, ErrorCode::NONE};
			}

			if (detail::check_byte_2<InputType>(current))
			{
				// we have a two-bytes UTF-8
				constexpr std::ptrdiff_t length = 2;

				// minimal bound checking
				if (current + 1 >= end)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				const auto next_byte = static_cast<std::uint8_t>(*(current + 1));

				if ((next_byte & 0b1100'0000) != 0b1000'0000)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				// range check
				const auto code_point = static_cast<std::uint32_t>(
					(leading_byte & 0b0001'1111) << 6 |
					(next_byte & 0b0011'1111)
				);

				if (code_point < 0x80)
				{
					return {length, ErrorCode::OVERLONG};
				}
				if (code_point > 0x7ff)
				{
					return {length, ErrorCode::TOO_LARGE};
				}

				return {length, ErrorCode::NONE};
			}

			if (detail::check_byte_3<InputType>(current))
			{
				// we have a three-byte UTF-8
				constexpr std::ptrdiff_t length = 3;

				// minimal bound checking
				if (current + 2 >= end)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				const auto next_byte_1 = static_cast<std::uint8_t>(*(current + 1));
				const auto next_byte_2 = static_cast<std::uint8_t>(*(current + 2));

				if (
					((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
					((next_byte_2 & 0b1100'0000) != 0b1000'0000)
				)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				// range check
				const auto code_point = static_cast<std::uint32_t>(
					(leading_byte & 0b0000'1111) << 12 |
					(next_byte_1 & 0b0011'1111) << 6 |
					(next_byte_2 & 0b0011'1111)
				);

				if (code_point < 0x800)
				{
					return {length, ErrorCode::OVERLONG};
				}
				if (code_point > 0xffff)
				{
					return {length, ErrorCode::TOO_LARGE};
				}
				if (code_point > 0xd7ff and code_point < 0xe000)
				{
					return {length, ErrorCode::SURROGATE};
				}

				return {length, ErrorCode::NONE};
			}

			if (detail::check_byte_4<InputType>(current))
			{
				// we have a four-byte UTF-8 word
				constexpr std::ptrdiff_t length = 4;

				// minimal bound checking
				if (current + 3 >= end)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				const auto next_byte_1 = static_cast<std::uint8_t>(*(current + 1));
				const auto next_byte_2 = static_cast<std::uint8_t>(*(current + 2));
				const auto next_byte_3 = static_cast<std::uint8_t>(*(current + 3));

				if (
					((next_byte_1 & 0b1100'0000) != 0b1000'0000) or
					((next_byte_2 & 0b1100'0000) != 0b1000'0000) or
					((next_byte_3 & 0b1100'0000) != 0b1000'0000)
				)
				{
					return {length, ErrorCode::TOO_SHORT};
				}

				// range check
				const auto code_point = static_cast<std::uint32_t>(
					(leading_byte & 0b0000'0111) << 18 |
					(next_byte_1 & 0b0011'1111) << 12 |
					(next_byte_2 & 0b0011'1111) << 6 |
					(next_byte_3 & 0b0011'1111)
				);

				if (code_point <= 0xffff)
				{
					return {length, ErrorCode::OVERLONG};
				}
				if (code_point > 0x10'ffff)
				{
					return {length, ErrorCode::TOO_LARGE};
				}

				return {length, ErrorCode::NONE};
			}

			return detail::invalid_input<InputType>(current);
		}

		// 1-byte UTF-8 => 1 LATIN
		// 2-bytes UTF-8 => 1 LATIN
		template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
			requires (
				         InputType == CharsType::UTF8_CHAR or
				         InputType == CharsType::UTF8
			         ) and
			         (
				         OutputType == CharsType::LATIN
			         )
		[[nodiscard]] constexpr auto write_latin(
			typename output_type_of<OutputType>::pointer& output,
			const typename input_type_of<InputType>::const_pointer current,
			const typename input_type_of<InputType>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			if constexpr (Pure)
			{
				return detail::write_byte_1<InputType, OutputType, Correct>(output, current, end);
			}
			else
			{
				if (detail::check_byte_1<InputType>(current))
				{
					return detail::write_byte_1<InputType, OutputType, Correct>(output, current, end);
				}

				if (detail::check_byte_2<InputType>(current))
				{
					return detail::write_byte_2<InputType, OutputType, Correct>(output, current, end);
				}

				if (detail::check_byte_3<InputType>(current))
				{
					return {3, ErrorCode::TOO_LARGE};
				}

				if (detail::check_byte_4<InputType>(current))
				{
					return {4, ErrorCode::TOO_LARGE};
				}

				return detail::invalid_input<InputType>(current);
			}
		}

		// 1-byte UTF-8 => 1 UTF-16
		// 2-bytes UTF-8 => 1 UTF-16
		// 3-bytes UTF-8 => 1 UTF-16
		// 4-bytes UTF-8 => 2 UTF-16
		template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
			requires (
				         InputType == CharsType::UTF8_CHAR or
				         InputType == CharsType::UTF8
			         ) and
			         (
				         OutputType == CharsType::UTF16_LE or
				         OutputType == CharsType::UTF16_BE
			         )
		[[nodiscard]] constexpr auto write_utf16(
			typename output_type_of<OutputType>::pointer& output,
			const typename input_type_of<InputType>::const_pointer current,
			const typename input_type_of<InputType>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			if constexpr (Pure)
			{
				return detail::write_byte_1<InputType, OutputType, Correct>(output, current, end);
			}
			else
			{
				if (detail::check_byte_1<InputType>(current))
				{
					return detail::write_byte_1<InputType, OutputType, Correct>(output, current, end);
				}

				if (detail::check_byte_2<InputType>(current))
				{
					return detail::write_byte_2<InputType, OutputType, Correct>(output, current, end);
				}

				if (detail::check_byte_3<InputType>(current))
				{
					return detail::write_byte_3<InputType, OutputType, Correct>(output, current, end);
				}

				if (detail::check_byte_4<InputType>(current))
				{
					return detail::write_byte_4<InputType, OutputType, Correct>(output, current, end);
				}

				return detail::invalid_input<InputType>(current);
			}
		}

		// 1-byte UTF-8 => 1 UTF-32
		// 2-bytes UTF-8 => 1 UTF-32
		// 3-bytes UTF-8 => 1 UTF-32
		// 4-bytes UTF-8 => 2 UTF-32
		template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
			requires (
				         InputType == CharsType::UTF8_CHAR or
				         InputType == CharsType::UTF8
			         ) and
			         (
				         OutputType == CharsType::UTF32
			         )
		[[nodiscard]] constexpr auto write_utf32(
			typename output_type_of<OutputType>::pointer& output,
			const typename input_type_of<InputType>::const_pointer current,
			const typename input_type_of<InputType>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			if constexpr (Pure)
			{
				return detail::write_byte_1<InputType, OutputType, Correct>(output, current, end);
			}
			else
			{
				if (detail::check_byte_1<InputType>(current))
				{
					return detail::write_byte_1<InputType, OutputType, Correct>(output, current, end);
				}

				if (detail::check_byte_2<InputType>(current))
				{
					return detail::write_byte_2<InputType, OutputType, Correct>(output, current, end);
				}

				if (detail::check_byte_3<InputType>(current))
				{
					return detail::write_byte_3<InputType, OutputType, Correct>(output, current, end);
				}

				if (detail::check_byte_4<InputType>(current))
				{
					return detail::write_byte_4<InputType, OutputType, Correct>(output, current, end);
				}

				return detail::invalid_input<InputType>(current);
			}
		}

		// UTF8_CHAR => UTF8
		// UTF8 => UTF8_CHAR
		template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
			requires (
				         InputType == CharsType::UTF8_CHAR and
				         OutputType == CharsType::UTF8
			         ) or
			         (
				         InputType == CharsType::UTF8 and
				         OutputType == CharsType::UTF8_CHAR
			         )
		[[nodiscard]] constexpr auto transform(
			typename output_type_of<OutputType>::pointer& output,
			const typename input_type_of<InputType>::const_pointer current,
			const typename input_type_of<InputType>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			if constexpr (Pure)
			{
				std::ranges::transform(
					current,
					current + 1,
					output,
					[](const auto c) noexcept -> auto
					{
						return common::to_char<OutputType>(c);
					}
				);
				return {1, ErrorCode::NONE};
			}
			else
			{
				if (detail::check_byte_1<InputType>(current))
				{
					std::ranges::transform(
						current,
						current + 1,
						output,
						[](const auto c) noexcept -> auto
						{
							return common::to_char<OutputType>(c);
						}
					);
					return {1, ErrorCode::NONE};
				}

				if (detail::check_byte_2<InputType>(current))
				{
					if constexpr (not Correct)
					{
						// minimal bound checking
						if (current + 1 >= end)
						{
							return {2, ErrorCode::TOO_SHORT};
						}
					}

					std::ranges::transform(
						current,
						current + 2,
						output,
						[](const auto c) noexcept -> auto
						{
							return common::to_char<OutputType>(c);
						}
					);
					return {2, ErrorCode::NONE};
				}

				if (detail::check_byte_3<InputType>(current))
				{
					if constexpr (not Correct)
					{
						// minimal bound checking
						if (current + 2 >= end)
						{
							return {3, ErrorCode::TOO_SHORT};
						}
					}

					std::ranges::transform(
						current,
						current + 3,
						output,
						[](const auto c) noexcept -> auto
						{
							return common::to_char<OutputType>(c);
						}
					);
					return {3, ErrorCode::NONE};
				}

				if (detail::check_byte_4<InputType>(current))
				{
					if constexpr (not Correct)
					{
						// minimal bound checking
						if (current + 3 >= end)
						{
							return {4, ErrorCode::TOO_SHORT};
						}
					}

					std::ranges::transform(
						current,
						current + 4,
						output,
						[](const auto c) noexcept -> auto
						{
							return common::to_char<OutputType>(c);
						}
					);
					return {4, ErrorCode::NONE};
				}

				return detail::invalid_input<InputType>(current);
			}
		}

		namespace scalar
		{
			template<CharsType InputType, CharsType OutputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto advance_of() noexcept -> std::ptrdiff_t
			{
				std::ignore = OutputType;

				using char_type = typename input_type_of<InputType>::value_type;

				return sizeof(data_type) / sizeof(char_type);
			}

			template<CharsType InputType, CharsType OutputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto read(const typename input_type_of<InputType>::const_pointer source) noexcept -> data_type
			{
				std::ignore = OutputType;

				return memory::unaligned_load<data_type>(source);
			}

			template<CharsType InputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto validate(const input_type_of<InputType> input) noexcept -> result_error_input_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				using input_type = input_type_of<InputType>;
				using pointer_type = typename input_type::const_pointer;
				using size_type = typename input_type::size_type;

				constexpr auto advance = scalar::advance_of<InputType, InputType>();

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const auto check = [&](const size_type n) noexcept -> result_error_input_type
				{
					const auto end = it_input_current + n;
					while (it_input_current < end)
					{
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);

						const auto [length, error] = ::utf8::validate<InputType>(it_input_current, it_input_end);
						if (error != ErrorCode::NONE)
						{
							return {.error = error, .input = current_input_length};
						}

						it_input_current += length;
					}

					// ==================================================
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current >= end);
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					return {.error = ErrorCode::NONE, .input = current_input_length};
				};

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = scalar::read<InputType, InputType>(it_input_current);

					if (const auto sign = common::sign_of<InputType>(data);
						not sign.pure())
					{
						const auto start_count = sign.start_count();
						const auto end_count = sign.end_count();
						const auto unknown_count = advance - start_count - end_count;

						it_input_current += start_count;
						if (const auto result = check(unknown_count);
							result.has_error())
						{
							return result;
						}
					}
					else
					{
						it_input_current += advance;
					}
				}

				const auto remaining = static_cast<size_type>(it_input_end - it_input_current);
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, remaining};
					#endif

					if (const auto result = check(remaining);
						result.has_error())
					{
						return result;
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				return {.error = ErrorCode::NONE, .input = current_input_length};
			}

			template<CharsType InputType, CharsType OutputType>
				requires (
					InputType == CharsType::UTF8_CHAR or
					InputType == CharsType::UTF8
				)
			[[nodiscard]] constexpr auto length(
				const input_type_of<InputType> input
			) noexcept -> typename input_type_of<InputType>::size_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

				using size_type = typename input_type_of<InputType>::size_type;

				// ReSharper disable CppClangTidyBugproneBranchClone
				if constexpr (
					OutputType == CharsType::LATIN
				)
				{
					return detail::code_points<InputType>(input);
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else if constexpr (
					OutputType == CharsType::UTF16_LE or
					OutputType == CharsType::UTF16_BE or
					OutputType == CharsType::UTF16
				)
				{
					return std::ranges::fold_left(
						input.begin(),
						input.end(),
						size_type{0},
						[](const size_type total, const auto byte) noexcept -> size_type
						{
							// -65 is 0b10111111
							return
									total +
									(static_cast<std::int8_t>(byte) > -65) //
									+
									(static_cast<std::uint8_t>(byte) >= 240) //
									;
						}
					);
				}
				// ReSharper disable CppClangTidyBugproneBranchClone
				else if constexpr (
					OutputType == CharsType::UTF32
				)
				{
					return detail::code_points<InputType>(input);
				}
				// ReSharper restore CppClangTidyBugproneBranchClone
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::LATIN
				         )
			[[nodiscard]] constexpr auto write_latin(
				typename output_type_of<OutputType>::pointer& output,
				const input_type_of<InputType> input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using input_type = input_type_of<InputType>;
				using pointer_type = typename input_type::const_pointer;

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				const auto advance = scalar::advance_of<InputType, OutputType>();
				const auto transform = [&]<bool P>(const decltype(advance) n) noexcept -> result_error_input_output_type
				{
					const auto end = it_input_current + n;
					while (it_input_current < end)
					{
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						const auto [length, error] = ::utf8::write_latin<InputType, OutputType, P, Correct>(it_output_current, it_input_current, it_input_end);
						if (error != ErrorCode::NONE)
						{
							return {.error = error, .input = current_input_length, .output = current_output_length};
						}

						it_input_current += length;
					}

					// ==================================================
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current >= end);
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					return {.error = ErrorCode::NONE, .input = current_input_length, .output = length_ignored};
				};

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = scalar::read<InputType, OutputType>(it_input_current);

					if constexpr (Pure)
					{
						const auto result = transform.template operator()<true>(advance);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
					}
					else
					{
						if (const auto sign = common::sign_of<InputType>(data);
							not sign.pure())
						{
							const auto start_count = sign.start_count();
							const auto end_count = sign.end_count();
							const auto unknown_count = advance - start_count - end_count;

							const auto result1 = transform.template operator()<true>(static_cast<decltype(advance)>(start_count));
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result1.has_error());

							if (const auto result2 = transform.template operator()<false>(static_cast<decltype(advance)>(unknown_count));
								result2.has_error())
							{
								return result2;
							}
						}
						else
						{
							const auto result = transform.template operator()<true>(advance);
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
						}
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					if (const auto result = transform.template operator()<false>(remaining);
						result.has_error())
					{
						return result;
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::UTF16_LE or
					         OutputType == CharsType::UTF16_BE
				         )
			[[nodiscard]] constexpr auto write_utf16(
				typename output_type_of<OutputType>::pointer& output,
				const input_type_of<InputType> input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using input_type = input_type_of<InputType>;
				using pointer_type = typename input_type::const_pointer;

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				const auto advance = scalar::advance_of<InputType, OutputType>();
				const auto transform = [&]<bool P>(const decltype(advance) n) noexcept -> result_error_input_output_type
				{
					const auto end = it_input_current + n;
					while (it_input_current < end)
					{
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						const auto [length, error] = ::utf8::write_utf16<InputType, OutputType, P, Correct>(it_output_current, it_input_current, it_input_end);
						if (error != ErrorCode::NONE)
						{
							return {.error = error, .input = current_input_length, .output = current_output_length};
						}

						it_input_current += length;
					}

					// ==================================================
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current >= end);
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					return {.error = ErrorCode::NONE, .input = current_input_length, .output = length_ignored};
				};

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = scalar::read<InputType, OutputType>(it_input_current);

					if constexpr (Pure)
					{
						const auto result = transform.template operator()<true>(advance);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
					}
					else
					{
						if (const auto sign = common::sign_of<InputType>(data);
							not sign.pure())
						{
							const auto start_count = sign.start_count();
							const auto end_count = sign.end_count();
							const auto unknown_count = advance - start_count - end_count;

							const auto result1 = transform.template operator()<true>(static_cast<decltype(advance)>(start_count));
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result1.has_error());

							if (const auto result2 = transform.template operator()<false>(static_cast<decltype(advance)>(unknown_count));
								result2.has_error())
							{
								return result2;
							}
						}
						else
						{
							const auto result = transform.template operator()<true>(advance);
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
						}
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					if (const auto result = transform.template operator()<false>(remaining);
						result.has_error())
					{
						return result;
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR or
					         InputType == CharsType::UTF8
				         ) and
				         (
					         OutputType == CharsType::UTF32
				         )
			[[nodiscard]] constexpr auto write_utf32(
				typename output_type_of<OutputType>::pointer& output,
				const input_type_of<InputType> input
			) noexcept -> result_error_input_output_type
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(output != nullptr);

				using input_type = input_type_of<InputType>;
				using pointer_type = typename input_type::const_pointer;

				using output_type = output_type_of<OutputType>;
				using output_pointer_type = typename output_type::pointer;

				const auto input_length = input.size();

				const pointer_type it_input_begin = input.data();
				pointer_type it_input_current = it_input_begin;
				const pointer_type it_input_end = it_input_begin + input_length;

				const output_pointer_type it_output_begin = output;
				output_pointer_type it_output_current = it_output_begin;

				const auto advance = scalar::advance_of<InputType, OutputType>();
				const auto transform = [&]<bool P>(const decltype(advance) n) noexcept -> result_error_input_output_type
				{
					const auto end = it_input_current + n;
					while (it_input_current < end)
					{
						const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
						const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);

						const auto [length, error] = ::utf8::write_utf32<InputType, OutputType, P, Correct>(it_output_current, it_input_current, it_input_end);
						if (error != ErrorCode::NONE)
						{
							return {.error = error, .input = current_input_length, .output = current_output_length};
						}

						it_input_current += length;
					}

					// ==================================================
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current >= end);
					const auto current_input_length = static_cast<std::size_t>(it_input_current - it_input_begin);
					return {.error = ErrorCode::NONE, .input = current_input_length, .output = length_ignored};
				};

				while (it_input_current + advance <= it_input_end)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + advance};
					#endif

					const auto data = scalar::read<InputType, OutputType>(it_input_current);

					if constexpr (Pure)
					{
						const auto result = transform.template operator()<true>(advance);
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
					}
					else
					{
						if (const auto sign = common::sign_of<InputType>(data);
							not sign.pure())
						{
							const auto start_count = sign.start_count();
							const auto end_count = sign.end_count();
							const auto unknown_count = advance - start_count - end_count;

							const auto result1 = transform.template operator()<true>(static_cast<decltype(advance)>(start_count));
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result1.has_error());

							if (const auto result2 = transform.template operator()<false>(static_cast<decltype(advance)>(unknown_count));
								result2.has_error())
							{
								return result2;
							}
						}
						else
						{
							const auto result = transform.template operator()<true>(advance);
							GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not result.has_error());
						}
					}
				}

				const auto remaining = it_input_end - it_input_current;
				GAL_PROMETHEUS_ERROR_ASSUME(remaining < advance);

				if (remaining != 0)
				{
					#if GAL_PROMETHEUS_COMPILER_DEBUG
					[[maybe_unused]] const auto debug_input_data = std::span{it_input_current, it_input_current + remaining};
					#endif

					if (const auto result = transform.template operator()<false>(remaining);
						result.has_error())
					{
						return result;
					}
				}

				// ==================================================
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it_input_current == it_input_end);
				const auto current_input_length = static_cast<std::size_t>(input_length);
				const auto current_output_length = static_cast<std::size_t>(it_output_current - it_output_begin);
				return {.error = ErrorCode::NONE, .input = current_input_length, .output = current_output_length};
			}

			// UTF8_CHAR => UTF8
			// UTF8 => UTF8_CHAR
			template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
				requires (
					         InputType == CharsType::UTF8_CHAR and
					         OutputType == CharsType::UTF8
				         ) or
				         (
					         InputType == CharsType::UTF8 and
					         OutputType == CharsType::UTF8_CHAR
				         )
			[[nodiscard]] constexpr auto transform(
				typename output_type_of<OutputType>::pointer& output,
				const input_type_of<InputType> input
			) noexcept -> result_error_input_output_type
			{
				using char_type = typename input_type_of<InputType>::value_type;

				if constexpr (not Pure or not Correct)
				{
					if (const auto result = scalar::validate<InputType>(input);
						result.has_error())
					{
						std::memcpy(output, input.data(), result.input * sizeof(char_type));
						return {.error = result.error, .input = result.input, .output = result.input};
					}
				}

				std::memcpy(output, input.data(), input.size() * sizeof(char_type));
				return {.error = ErrorCode::NONE, .input = input.size(), .output = input.size()};
			}
		}
	}

	namespace utf16
	{
		template<std::endian SourceEndian>
		[[nodiscard]] constexpr auto do_validate(
			const input_type_of<CharsType::UTF16>::const_pointer current,
			const input_type_of<CharsType::UTF16>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			// 1-word UTF-16
			// 2-words UTF-16(surrogate pair)

			if (const auto leading_word = common::to_native_utf16<SourceEndian>(*(current + 0));
				(leading_word & 0xf800) == 0xd800)
			{
				// we have a two-word UTF16
				// must be a surrogate pair
				constexpr std::ptrdiff_t length = 2;

				// minimal bound checking
				if (current + 1 >= end)
				{
					return {length, ErrorCode::SURROGATE};
				}

				if (const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);
					diff > 0x3ff)
				{
					return {length, ErrorCode::SURROGATE};
				}

				const auto next_word = common::to_native_utf16<SourceEndian>(*(current + 1));
				if (const auto diff = static_cast<std::uint16_t>(next_word - 0xdc00);
					diff > 0x3ff)
				{
					return {length, ErrorCode::SURROGATE};
				}

				return {length, ErrorCode::NONE};
			}

			// we have a one-word UTF16
			constexpr std::ptrdiff_t length = 1;

			return {length, ErrorCode::NONE};
		}

		// 1-word UTF-16 => 1 LATIN
		template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
			requires (
				         InputType == CharsType::UTF16_LE or
				         InputType == CharsType::UTF16_BE
			         ) and
			         (
				         OutputType == CharsType::LATIN
			         )
		[[nodiscard]] constexpr auto do_write(
			typename output_type_of<OutputType>::pointer& output,
			const input_type_of<CharsType::UTF16>::const_pointer current,
			const input_type_of<CharsType::UTF16>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			std::ignore = end;

			constexpr std::ptrdiff_t length = 1;

			const auto value = common::to_native_utf16<InputType>(*(current + 0));

			if constexpr (not Pure or not Correct)
			{
				if ((value & 0xff00) != 0)
				{
					return {length, ErrorCode::TOO_LARGE};
				}
			}

			*(output + 0) = common::to_char<OutputType>(value);

			output += 1;
			return {length, ErrorCode::NONE};
		}

		// 1-word UTF-16 => 1/2/3 UTF-8
		// 2-words UTF-16(surrogate pair) => 4 UTF-8
		template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
			requires (
				         InputType == CharsType::UTF16_LE or
				         InputType == CharsType::UTF16_BE
			         ) and
			         (
				         OutputType == CharsType::UTF8_CHAR or
				         OutputType == CharsType::UTF8
			         )
		[[nodiscard]] constexpr auto do_write(
			typename output_type_of<OutputType>::pointer& output,
			const input_type_of<CharsType::UTF16>::const_pointer current,
			const input_type_of<CharsType::UTF16>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			if constexpr (const auto leading_word = common::to_native_utf16<InputType>(*(current + 0));
				Pure)
			{
				constexpr std::ptrdiff_t length = 1;

				*(output + 0) = common::to_char<OutputType>(leading_word);

				output += 1;
				return {length, ErrorCode::NONE};
			}
			else
			{
				if ((leading_word & 0xff80) == 0)
				{
					// 1-word utf16 => 1-byte utf8
					constexpr std::size_t length = 1;

					*(output + 0) = common::to_char<OutputType>(leading_word);

					output += 1;
					return {length, ErrorCode::NONE};
				}

				if ((leading_word & 0xf800) == 0)
				{
					// 1-word utf16 => 2-bytes utf8
					constexpr std::size_t length = 1;

					// 0b110?'???? 0b10??'????
					const auto c1 = static_cast<std::uint16_t>((leading_word >> 6) | 0b1100'0000);
					const auto c2 = static_cast<std::uint16_t>((leading_word & 0b0011'1111) | 0b1000'0000);

					*(output + 0) = common::to_char<OutputType>(c1);
					*(output + 1) = common::to_char<OutputType>(c2);

					output += 2;
					return {length, ErrorCode::NONE};
				}

				if ((leading_word & 0xf800) != 0xd800)
				{
					// 1-word utf16 => 3-bytes utf8
					constexpr std::size_t length = 1;

					// 0b1110'???? 0b10??'???? 0b10??'????
					const auto c1 = static_cast<std::uint16_t>((leading_word >> 12) | 0b1110'0000);
					const auto c2 = static_cast<std::uint16_t>(((leading_word >> 6) & 0b0011'1111) | 0b1000'0000);
					const auto c3 = static_cast<std::uint16_t>((leading_word & 0b0011'1111) | 0b1000'0000);

					*(output + 0) = common::to_char<OutputType>(c1);
					*(output + 1) = common::to_char<OutputType>(c2);
					*(output + 2) = common::to_char<OutputType>(c3);

					output += 3;
					return {length, ErrorCode::NONE};
				}

				// 2-word utf16 => 4-bytes utf8
				// must be a surrogate pair
				constexpr std::size_t length = 2;

				// minimal bound checking
				if (current + 1 >= end)
				{
					return {length, ErrorCode::SURROGATE};
				}

				const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);

				if constexpr (not Correct)
				{
					if (diff > 0x3ff)
					{
						return {length, ErrorCode::SURROGATE};
					}
				}

				const auto next_word = common::to_native_utf16<InputType>(*(current + 1));
				const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

				if constexpr (not Correct)
				{
					if (next_diff > 0x3ff)
					{
						return {length, ErrorCode::SURROGATE};
					}
				}

				const auto value = static_cast<std::uint32_t>((diff << 10) + next_diff + 0x1'0000);

				// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
				const auto c1 = static_cast<std::uint16_t>((value >> 18) | 0b1111'0000);
				const auto c2 = static_cast<std::uint16_t>(((value >> 12) & 0b0011'1111) | 0b1000'0000);
				const auto c3 = static_cast<std::uint16_t>(((value >> 6) & 0b0011'1111) | 0b1000'0000);
				const auto c4 = static_cast<std::uint16_t>((value & 0b0011'1111) | 0b1000'0000);

				*(output + 0) = common::to_char<OutputType>(c1);
				*(output + 1) = common::to_char<OutputType>(c2);
				*(output + 2) = common::to_char<OutputType>(c3);
				*(output + 3) = common::to_char<OutputType>(c4);

				output += 4;
				return {length, ErrorCode::NONE};
			}
		}

		// 1-word UTF-16 => 1 UTF-32
		// 2-words UTF-16(surrogate pair) => 1 UTF-32
		template<CharsType InputType, CharsType OutputType, bool Pure, bool Correct>
			requires (
				         InputType == CharsType::UTF16_LE or
				         InputType == CharsType::UTF16_BE
			         ) and
			         (
				         OutputType == CharsType::UTF32
			         )
		[[nodiscard]] constexpr auto do_write(
			typename output_type_of<OutputType>::pointer& output,
			const input_type_of<CharsType::UTF16>::const_pointer current,
			const input_type_of<CharsType::UTF16>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			if constexpr (const auto leading_word = common::to_native_utf16<InputType>(*(current + 0));
				Pure)
			{
				constexpr std::ptrdiff_t length = 1;

				*(output + 0) = common::to_char<OutputType>(leading_word);

				output += 1;
				return {length, ErrorCode::NONE};
			}
			else
			{
				if ((leading_word & 0xf800) == 0xd800)
				{
					// we have a two-word UTF16
					// must be a surrogate pair
					constexpr std::ptrdiff_t length = 2;

					// minimal bound checking
					if (current + 1 >= end)
					{
						return {length, ErrorCode::SURROGATE};
					}

					const auto diff = static_cast<std::uint16_t>(leading_word - 0xd800);

					if constexpr (not Correct)
					{
						if (diff > 0x3ff)
						{
							return {length, ErrorCode::SURROGATE};
						}
					}

					const auto next_word = common::to_native_utf16<InputType>(*(current + 1));
					const auto next_diff = static_cast<std::uint16_t>(next_word - 0xdc00);

					if constexpr (not Correct)
					{
						if (next_diff > 0x3ff)
						{
							return {length, ErrorCode::SURROGATE};
						}
					}

					const auto value = static_cast<std::uint32_t>((diff << 10) + next_diff + 0x1'0000);

					*(output + 0) = common::to_char<OutputType>(value);

					output += 1;
					return {length, ErrorCode::NONE};
				}

				// we have a one-word UTF16
				constexpr std::ptrdiff_t length = 1;

				*(output + 0) = common::to_char<OutputType>(leading_word);

				output += 1;
				return {length, ErrorCode::NONE};
			}
		}
	}

	namespace utf32
	{
		[[nodiscard]] constexpr auto do_validate(
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			std::ignore = end;
			constexpr std::ptrdiff_t length = 1;

			const auto value = static_cast<std::uint32_t>(*(current + 0));

			if (value > 0x10'ffff)
			{
				return {length, ErrorCode::TOO_LARGE};
			}

			if (value >= 0xd800 and value <= 0xdfff)
			{
				return {length, ErrorCode::SURROGATE};
			}

			return {length, ErrorCode::NONE};
		}

		// 1-dword UTF-32 => 1 LATIN
		template<CharsType OutputType, bool Pure, bool Correct>
			requires (
				OutputType == CharsType::LATIN
			)
		[[nodiscard]] constexpr auto do_write(
			typename output_type_of<OutputType>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			std::ignore = end;

			constexpr std::ptrdiff_t length = 1;

			const auto value = static_cast<std::uint32_t>(*(current + 0));

			if constexpr (not Pure or not Correct)
			{
				if ((value & 0xffff'ff00) != 0)
				{
					return {length, ErrorCode::TOO_LARGE};
				}
			}

			*(output + 0) = common::to_char<OutputType>(value);

			output += 1;
			return {length, ErrorCode::NONE};
		}

		// 1-dword UTF-32 => 1/2/3/4 UTF-8
		template<CharsType OutputType, bool Pure, bool Correct>
			requires (
				OutputType == CharsType::UTF8_CHAR or
				OutputType == CharsType::UTF8
			)
		[[nodiscard]] constexpr auto do_write(
			typename output_type_of<OutputType>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			std::ignore = end;

			constexpr std::ptrdiff_t length = 1;

			if constexpr (const auto value = static_cast<std::uint32_t>(*(current + 0));
				Pure)
			{
				*(output + 0) = common::to_char<OutputType>(value);

				output += 1;
				return {length, ErrorCode::NONE};
			}
			else
			{
				if ((value & 0xffff'ff80) == 0)
				{
					// 1-byte utf8

					*(output + 0) = common::to_char<OutputType>(value);

					output += 1;
					return {length, ErrorCode::NONE};
				}

				if ((value & 0xffff'f800) == 0)
				{
					// 2-bytes utf8

					// 0b110?'???? 0b10??'????
					const auto c1 = static_cast<std::uint32_t>((value >> 6) | 0b1100'0000);
					const auto c2 = static_cast<std::uint32_t>((value & 0b0011'1111) | 0b1000'0000);

					*(output + 0) = common::to_char<OutputType>(c1);
					*(output + 1) = common::to_char<OutputType>(c2);

					output += 2;
					return {length, ErrorCode::NONE};
				}

				if ((value & 0xffff'0000) == 0)
				{
					// 3-bytes utf8

					if constexpr (not Correct)
					{
						if (value >= 0xd800 and value <= 0xdfff)
						{
							return {length, ErrorCode::SURROGATE};
						}
					}

					// 0b1110'???? 0b10??'???? 0b10??'????
					const auto c1 = static_cast<std::uint32_t>((value >> 12) | 0b1110'0000);
					const auto c2 = static_cast<std::uint32_t>(((value >> 6) & 0b0011'1111) | 0b1000'0000);
					const auto c3 = static_cast<std::uint32_t>((value & 0b0011'1111) | 0b1000'0000);

					*(output + 0) = common::to_char<OutputType>(c1);
					*(output + 1) = common::to_char<OutputType>(c2);
					*(output + 2) = common::to_char<OutputType>(c3);

					output += 3;
					return {length, ErrorCode::NONE};
				}

				// 4-bytes utf8

				if constexpr (not Correct)
				{
					if (value > 0x0010'ffff)
					{
						return {length, ErrorCode::TOO_LARGE};
					}
				}

				// 0b1111'0??? 0b10??'???? 0b10??'???? 0b10??'????
				const auto c1 = static_cast<std::uint32_t>((value >> 18) | 0b1111'0000);
				const auto c2 = static_cast<std::uint32_t>(((value >> 12) & 0b0011'1111) | 0b1000'0000);
				const auto c3 = static_cast<std::uint32_t>(((value >> 6) & 0b0011'1111) | 0b1000'0000);
				const auto c4 = static_cast<std::uint32_t>((value & 0b0011'1111) | 0b1000'0000);

				*(output + 0) = common::to_char<OutputType>(c1);
				*(output + 1) = common::to_char<OutputType>(c2);
				*(output + 2) = common::to_char<OutputType>(c3);
				*(output + 3) = common::to_char<OutputType>(c4);

				output += 4;
				return {length, ErrorCode::NONE};
			}
		}

		// 1-dword UTF-32 => 1/2 UTF-16
		template<CharsType OutputType, bool Pure, bool Correct>
			requires (
				OutputType == CharsType::UTF16_LE or
				OutputType == CharsType::UTF16_BE
			)
		[[nodiscard]] constexpr auto do_write(
			typename output_type_of<OutputType>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			std::ignore = end;

			constexpr std::ptrdiff_t length = 1;

			if constexpr (const auto value = static_cast<std::uint32_t>(*(current + 0));
				Pure)
			{
				*(output + 0) = common::to_char<OutputType>(value);

				output += 1;
				return {length, ErrorCode::NONE};
			}
			else
			{
				if ((value & 0xffff'0000) == 0)
				{
					if constexpr (not Correct)
					{
						if (value >= 0xd800 and value <= 0xdfff)
						{
							return {length, ErrorCode::SURROGATE};
						}
					}

					*(output + 0) = common::to_char<OutputType>(value);

					output += 1;
					return {length, ErrorCode::NONE};
				}

				// will generate a surrogate pair

				if constexpr (not Correct)
				{
					if (value > 0x0010'ffff)
					{
						return {length, ErrorCode::TOO_LARGE};
					}
				}

				const auto [high_surrogate, low_surrogate] = [v = value - 0x0001'0000]() noexcept
				{
					const auto high = static_cast<std::uint16_t>(0xd800 + (v >> 10));
					const auto low = static_cast<std::uint16_t>(0xdc00 + (v & 0x3ff));

					return std::make_pair(high, low);
				}();

				*(output + 0) = common::to_char<OutputType>(high_surrogate);
				*(output + 1) = common::to_char<OutputType>(low_surrogate);

				output += 2;
				return {length, ErrorCode::NONE};
			}
		}
	}
}

namespace gal::prometheus::chars_1
{
	namespace latin
	{
		auto validate(const pointer_type current, const pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::validate(current, end);
		}

		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf8<CharsType::UTF8_CHAR, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf8<CharsType::UTF8_CHAR, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf8<CharsType::UTF8_CHAR, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf8<CharsType::UTF8, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf8<CharsType::UTF8, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf8<CharsType::UTF8, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf16<CharsType::UTF16_LE, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf16<CharsType::UTF16_LE, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf16<CharsType::UTF16_LE, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf16<CharsType::UTF16_LE, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf16<CharsType::UTF16_LE, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf16<CharsType::UTF16_BE, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf32<CharsType::UTF32, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf32<CharsType::UTF32, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::latin::write_utf32<CharsType::UTF32, false, false>(output, current, end);
		}

		namespace scalar
		{
			auto validate(const input_type input) noexcept -> result_error_input_type
			{
				return ::latin::scalar::validate(input);
			}

			auto validate(const pointer_type input) noexcept -> result_error_input_type
			{
				return validate({input, std::char_traits<char_type>::length(input)});
			}

			auto length_for_utf8(const input_type input) noexcept -> size_type
			{
				const auto length = ::latin::scalar::length<CharsType::UTF8_CHAR>(input);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == ::latin::scalar::length<CharsType::UTF8>(input));

				return length;
			}

			auto length_for_utf8(const pointer_type input) noexcept -> size_type
			{
				return length_for_utf8({input, std::char_traits<char_type>::length(input)});
			}

			auto length_for_utf16(const input_type input) noexcept -> size_type
			{
				const auto length = ::latin::scalar::length<CharsType::UTF16>(input);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == ::latin::scalar::length<CharsType::UTF16_LE>(input));
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(length == ::latin::scalar::length<CharsType::UTF16_BE>(input));

				return length;
			}

			auto length_for_utf16(const pointer_type input) noexcept -> size_type
			{
				return length_for_utf16({input, std::char_traits<char_type>::length(input)});
			}

			auto length_for_utf32(const input_type input) noexcept -> size_type
			{
				return ::latin::scalar::length<CharsType::UTF32>(input);
			}

			auto length_for_utf32(const pointer_type input) noexcept -> size_type
			{
				return length_for_utf32({input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::latin::scalar::write_utf8<CharsType::UTF8_CHAR, false, false>(output, input);
			}

			auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_pure(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::latin::scalar::write_utf8<CharsType::UTF8_CHAR, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf8_pure(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf8_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::latin::scalar::write_utf8<CharsType::UTF8_CHAR, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf8_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::latin::scalar::write_utf8<CharsType::UTF8, false, false>(output, input);
			}

			auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_pure(
				output_type_of<CharsType::UTF8>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::latin::scalar::write_utf8<CharsType::UTF8, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf8_pure(
				output_type_of<CharsType::UTF8>::pointer& output,
				pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf8_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::latin::scalar::write_utf8<CharsType::UTF8, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
				const pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf8_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::latin::scalar::write_utf16<CharsType::UTF16_LE, false, false>(output, input);
			}

			auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf16_le(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_le_pure(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::latin::scalar::write_utf16<CharsType::UTF16_LE, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf16_le_pure(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf16_le_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::latin::scalar::write_utf16<CharsType::UTF16_LE, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf16_le_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::latin::scalar::write_utf16<CharsType::UTF16_BE, false, false>(output, input);
			}

			auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf16_be(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_be_pure(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::latin::scalar::write_utf16<CharsType::UTF16_BE, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf16_be_pure(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf16_be_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::latin::scalar::write_utf16<CharsType::UTF16_BE, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf16_be_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::latin::scalar::write_utf32<CharsType::UTF32, false, false>(output, input);
			}

			auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf32(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf32_pure(
				output_type_of<CharsType::UTF32>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::latin::scalar::write_utf32<CharsType::UTF32, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf32_pure(
				output_type_of<CharsType::UTF32>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf32_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::latin::scalar::write_utf32<CharsType::UTF32, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf32_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::latin::scalar::write_latin<CharsType::LATIN, false, false>(output, input);
			}

			auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_latin(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_latin_pure(
				output_type_of<CharsType::LATIN>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::latin::scalar::write_latin<CharsType::LATIN, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_latin_pure(
				output_type_of<CharsType::LATIN>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_latin_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::latin::scalar::write_latin<CharsType::LATIN, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				pointer_type input
			) noexcept -> result_output_type
			{
				return write_latin_correct(output, {input, std::char_traits<char_type>::length(input)});
			}
		}
	}

	namespace utf8_char
	{
		[[nodiscard]] auto validate(const pointer_type current, const pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::validate<CharsType::UTF8_CHAR>(current, end);
		}

		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_latin<CharsType::UTF8_CHAR, CharsType::LATIN, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_latin<CharsType::UTF8_CHAR, CharsType::LATIN, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_latin<CharsType::UTF8_CHAR, CharsType::LATIN, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_LE, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_LE, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_LE, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_BE, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_BE, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_BE, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf32<CharsType::UTF8_CHAR, CharsType::UTF32, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf32<CharsType::UTF8_CHAR, CharsType::UTF32, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf32<CharsType::UTF8_CHAR, CharsType::UTF32, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::transform<CharsType::UTF8_CHAR, CharsType::UTF8, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::transform<CharsType::UTF8_CHAR, CharsType::UTF8, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::transform<CharsType::UTF8_CHAR, CharsType::UTF8, false, true>(output, current, end);
		}

		namespace scalar
		{
			auto validate(const input_type input) noexcept -> result_error_input_type
			{
				return ::utf8::scalar::validate<CharsType::UTF8_CHAR>(input);
			}

			auto validate(const pointer_type input) noexcept -> result_error_input_type
			{
				return validate({input, std::char_traits<char_type>::length(input)});
			}

			auto length_for_latin(const input_type input) noexcept -> size_type
			{
				return ::utf8::scalar::length<CharsType::UTF8_CHAR, CharsType::LATIN>(input);
			}

			auto length_for_latin(const pointer_type input) noexcept -> size_type
			{
				return length_for_latin({input, std::char_traits<char_type>::length(input)});
			}

			auto length_for_utf16(const input_type input) noexcept -> size_type
			{
				const auto length = ::utf8::scalar::length<CharsType::UTF8_CHAR, CharsType::UTF16>(input);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((length == ::utf8::scalar::length<CharsType::UTF8_CHAR, CharsType::UTF16_LE>(input)));
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((length == ::utf8::scalar::length<CharsType::UTF8_CHAR, CharsType::UTF16_BE>(input)));

				return length;
			}

			auto length_for_utf16(const pointer_type input) noexcept -> size_type
			{
				return length_for_utf16({input, std::char_traits<char_type>::length(input)});
			}

			auto length_for_utf32(const input_type input) noexcept -> size_type
			{
				return ::utf8::scalar::length<CharsType::UTF8_CHAR, CharsType::UTF32>(input);
			}

			auto length_for_utf32(pointer_type input) noexcept -> size_type
			{
				return length_for_utf32({input, std::char_traits<char_type>::length(input)});
			}

			auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::write_latin<CharsType::UTF8_CHAR, CharsType::LATIN, false, false>(output, input);
			}

			auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_latin(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_latin_pure(
				output_type_of<CharsType::LATIN>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::write_latin<CharsType::UTF8_CHAR, CharsType::LATIN, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_latin_pure(
				output_type_of<CharsType::LATIN>::pointer& output,
				pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_latin_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::write_latin<CharsType::UTF8_CHAR, CharsType::LATIN, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				const pointer_type input
			) noexcept -> result_output_type
			{
				return write_latin_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_LE, false, false>(output, input);
			}

			auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf16_le(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_le_pure(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_LE, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf16_le_pure(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf16_le_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_LE, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf16_le_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_BE, false, false>(output, input);
			}

			auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf16_be(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_be_pure(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_BE, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf16_be_pure(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf16_be_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::write_utf16<CharsType::UTF8_CHAR, CharsType::UTF16_BE, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf16_be_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::write_utf32<CharsType::UTF8_CHAR, CharsType::UTF32, false, false>(output, input);
			}

			auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf32(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf32_pure(
				output_type_of<CharsType::UTF32>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::write_utf32<CharsType::UTF8_CHAR, CharsType::UTF32, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf32_pure(
				output_type_of<CharsType::UTF32>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf32_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::write_utf32<CharsType::UTF8_CHAR, CharsType::UTF32, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				const pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf32_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::transform<CharsType::UTF8_CHAR, CharsType::UTF8, false, false>(output, input);
			}

			auto write_utf8(
				output_type_of<CharsType::UTF8>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_pure(
				output_type_of<CharsType::UTF8>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::transform<CharsType::UTF8_CHAR, CharsType::UTF8, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf8_pure(
				output_type_of<CharsType::UTF8>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf8_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
				const pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf8_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_correct(
				output_type_of<CharsType::UTF8>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::transform<CharsType::UTF8_CHAR, CharsType::UTF8, false, true>(output, input);
				return {.output = result.output};
			}
		}
	}

	namespace utf8
	{
		[[nodiscard]] auto validate(const pointer_type current, const pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::validate<CharsType::UTF8>(current, end);
		}

		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_latin<CharsType::UTF8, CharsType::LATIN, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_latin<CharsType::UTF8, CharsType::LATIN, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_latin<CharsType::UTF8, CharsType::LATIN, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8, CharsType::UTF16_LE, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8, CharsType::UTF16_LE, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8, CharsType::UTF16_LE, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8, CharsType::UTF16_BE, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8, CharsType::UTF16_BE, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf16<CharsType::UTF8, CharsType::UTF16_BE, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf32(
			output_type_of<CharsType::UTF32>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf32<CharsType::UTF8, CharsType::UTF32, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_pure(
			output_type_of<CharsType::UTF32>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf32<CharsType::UTF8, CharsType::UTF32, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_correct(
			output_type_of<CharsType::UTF32>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::write_utf32<CharsType::UTF8, CharsType::UTF32, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::transform<CharsType::UTF8, CharsType::UTF8_CHAR, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::transform<CharsType::UTF8, CharsType::UTF8_CHAR, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const pointer_type current,
			const pointer_type end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf8::transform<CharsType::UTF8, CharsType::UTF8_CHAR, false, true>(output, current, end);
		}

		namespace scalar
		{
			auto validate(const input_type input) noexcept -> result_error_input_type
			{
				return ::utf8::scalar::validate<CharsType::UTF8>(input);
			}

			auto validate(const pointer_type input) noexcept -> result_error_input_type
			{
				return validate({input, std::char_traits<char_type>::length(input)});
			}

			auto length_for_latin(const input_type input) noexcept -> size_type
			{
				return ::utf8::scalar::length<CharsType::UTF8, CharsType::LATIN>(input);
			}

			auto length_for_latin(const pointer_type input) noexcept -> size_type
			{
				return length_for_latin({input, std::char_traits<char_type>::length(input)});
			}

			auto length_for_utf16(const input_type input) noexcept -> size_type
			{
				const auto length = ::utf8::scalar::length<CharsType::UTF8, CharsType::UTF16>(input);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((length == ::utf8::scalar::length<CharsType::UTF8, CharsType::UTF16_LE>(input)));
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME((length == ::utf8::scalar::length<CharsType::UTF8, CharsType::UTF16_BE>(input)));

				return length;
			}

			auto length_for_utf16(const pointer_type input) noexcept -> size_type
			{
				return length_for_utf16({input, std::char_traits<char_type>::length(input)});
			}

			auto length_for_utf32(const input_type input) noexcept -> size_type
			{
				return ::utf8::scalar::length<CharsType::UTF8, CharsType::UTF32>(input);
			}

			auto length_for_utf32(pointer_type input) noexcept -> size_type
			{
				return length_for_utf32({input, std::char_traits<char_type>::length(input)});
			}

			auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::write_latin<CharsType::UTF8, CharsType::LATIN, false, false>(output, input);
			}

			auto write_latin(
				output_type_of<CharsType::LATIN>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_latin(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_latin_pure(
				output_type_of<CharsType::LATIN>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::write_latin<CharsType::UTF8, CharsType::LATIN, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_latin_pure(
				output_type_of<CharsType::LATIN>::pointer& output,
				pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_latin_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::write_latin<CharsType::UTF8, CharsType::LATIN, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_latin_correct(
				output_type_of<CharsType::LATIN>::pointer& output,
				const pointer_type input
			) noexcept -> result_output_type
			{
				return write_latin_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::write_utf16<CharsType::UTF8, CharsType::UTF16_LE, false, false>(output, input);
			}

			auto write_utf16_le(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf16_le(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_le_pure(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::write_utf16<CharsType::UTF8, CharsType::UTF16_LE, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf16_le_pure(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf16_le_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::write_utf16<CharsType::UTF8, CharsType::UTF16_LE, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf16_le_correct(
				output_type_of<CharsType::UTF16_LE>::pointer& output,
				pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf16_le_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::write_utf16<CharsType::UTF8, CharsType::UTF16_BE, false, false>(output, input);
			}

			auto write_utf16_be(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf16_be(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_be_pure(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::write_utf16<CharsType::UTF8, CharsType::UTF16_BE, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf16_be_pure(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf16_be_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::write_utf16<CharsType::UTF8, CharsType::UTF16_BE, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf16_be_correct(
				output_type_of<CharsType::UTF16_BE>::pointer& output,
				pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf16_be_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::write_utf32<CharsType::UTF8, CharsType::UTF32, false, false>(output, input);
			}

			auto write_utf32(
				output_type_of<CharsType::UTF32>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf32(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf32_pure(
				output_type_of<CharsType::UTF32>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::write_utf32<CharsType::UTF8, CharsType::UTF32, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf32_pure(
				output_type_of<CharsType::UTF32>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf32_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::write_utf32<CharsType::UTF8, CharsType::UTF32, false, true>(output, input);
				return {.output = result.output};
			}

			auto write_utf32_correct(
				output_type_of<CharsType::UTF32>::pointer& output,
				const pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf32_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_output_type
			{
				return ::utf8::scalar::transform<CharsType::UTF8, CharsType::UTF8_CHAR, false, false>(output, input);
			}

			auto write_utf8(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_output_type
			{
				return write_utf8(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_pure(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const input_type input
			) noexcept -> result_error_input_type
			{
				const auto result = ::utf8::scalar::transform<CharsType::UTF8, CharsType::UTF8_CHAR, true, false>(output, input);
				return {.error = result.error, .input = result.input};
			}

			auto write_utf8_pure(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const pointer_type input
			) noexcept -> result_error_input_type
			{
				return write_utf8_pure(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const pointer_type input
			) noexcept -> result_output_type
			{
				return write_utf8_correct(output, {input, std::char_traits<char_type>::length(input)});
			}

			auto write_utf8_correct(
				output_type_of<CharsType::UTF8_CHAR>::pointer& output,
				const input_type input
			) noexcept -> result_output_type
			{
				const auto result = ::utf8::scalar::transform<CharsType::UTF8, CharsType::UTF8_CHAR, false, true>(output, input);
				return {.output = result.output};
			}
		}
	}

	namespace utf16
	{
		[[nodiscard]] auto validate_le(const pointer_type current, const pointer_type end) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_validate<std::endian::little>(current, end);
		}
	}

	namespace utf32
	{
		//
	}
}

namespace gal::prometheus::chars_1::scalar
{
	namespace utf16
	{
		[[nodiscard]] auto write_latin_le(
			output_type_of<CharsType::LATIN>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::LATIN, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_pure_le(
			output_type_of<CharsType::LATIN>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::LATIN, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_correct_le(
			output_type_of<CharsType::LATIN>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::LATIN, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::UTF8_CHAR, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::UTF8_CHAR, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct_le(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::UTF8_CHAR, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_le(
			output_type_of<CharsType::UTF8>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::UTF8, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure_le(
			output_type_of<CharsType::UTF8>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::UTF8, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct_le(
			output_type_of<CharsType::UTF8>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::UTF8, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_le(
			output_type_of<CharsType::UTF32>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::UTF32, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_pure_le(
			output_type_of<CharsType::UTF32>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::UTF32, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_correct_le(
			output_type_of<CharsType::UTF32>::pointer& output,
			const input_type_of<CharsType::UTF16_LE>::const_pointer current,
			const input_type_of<CharsType::UTF16_LE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_LE, CharsType::UTF32, false, true>(output, current, end);
		}

		[[nodiscard]] auto validate_be(
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_validate<std::endian::big>(current, end);
		}

		[[nodiscard]] auto write_latin_be(
			output_type_of<CharsType::LATIN>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::LATIN, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_pure_be(
			output_type_of<CharsType::LATIN>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::LATIN, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_correct_be(
			output_type_of<CharsType::LATIN>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::LATIN, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::UTF8_CHAR, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::UTF8_CHAR, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct_be(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::UTF8_CHAR, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_be(
			output_type_of<CharsType::UTF8>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::UTF8, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure_be(
			output_type_of<CharsType::UTF8>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::UTF8, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct_be(
			output_type_of<CharsType::UTF8>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::UTF8, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_be(
			output_type_of<CharsType::UTF32>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::UTF32, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_pure_be(
			output_type_of<CharsType::UTF32>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::UTF32, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf32_correct_be(
			output_type_of<CharsType::UTF32>::pointer& output,
			const input_type_of<CharsType::UTF16_BE>::const_pointer current,
			const input_type_of<CharsType::UTF16_BE>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf16::do_write<CharsType::UTF16_BE, CharsType::UTF32, false, true>(output, current, end);
		}
	}

	namespace utf32
	{
		[[nodiscard]] auto validate(
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_validate(current, end);
		}

		[[nodiscard]] auto write_latin(
			output_type_of<CharsType::LATIN>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::LATIN, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_pure(
			output_type_of<CharsType::LATIN>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::LATIN, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_latin_correct(
			output_type_of<CharsType::LATIN>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::LATIN, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF8_CHAR, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF8_CHAR, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8_CHAR>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF8_CHAR, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf8(
			output_type_of<CharsType::UTF8>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF8, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_pure(
			output_type_of<CharsType::UTF8>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF8, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf8_correct(
			output_type_of<CharsType::UTF8>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF8, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF16_LE, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le_pure(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF16_LE, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_le_correct(
			output_type_of<CharsType::UTF16_LE>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF16_LE, false, true>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF16_BE, false, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be_pure(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF16_BE, true, false>(output, current, end);
		}

		[[nodiscard]] auto write_utf16_be_correct(
			output_type_of<CharsType::UTF16_BE>::pointer& output,
			const input_type_of<CharsType::UTF32>::const_pointer current,
			const input_type_of<CharsType::UTF32>::const_pointer end
		) noexcept -> std::pair<std::ptrdiff_t, ErrorCode>
		{
			return ::utf32::do_write<CharsType::UTF16_BE, false, true>(output, current, end);
		}
	}
}
