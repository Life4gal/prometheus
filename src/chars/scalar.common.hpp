// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <chars/encoding.hpp>

#include <memory/rw.hpp>

namespace gal::prometheus::chars
{
	struct scalar_block
	{
		using data_type = std::uint64_t;

		template<CharsType Source, CharsType Dest>
		[[nodiscard]] constexpr static auto advance_of() noexcept -> std::ptrdiff_t
		{
			if constexpr (Source == CharsType::LATIN)
			{
				// zero extend ==> 1 char -> 1 out_char
				std::ignore = Dest;
				return sizeof(data_type) / sizeof(input_type_of<CharsType::LATIN>::value_type);
			}
			else if constexpr (
				Source == CharsType::UTF8_CHAR or
				Source == CharsType::UTF8
			)
			{
				// zero extend ==> 1 char -> 1 out_char
				std::ignore = Dest;
				return sizeof(data_type) / sizeof(input_type_of<CharsType::UTF8>::value_type);
			}
			else if constexpr (
				Source == CharsType::UTF16_LE or
				Source == CharsType::UTF16_BE or
				Source == CharsType::UTF16
			)
			{
				// zero-extend/truncate ==> 1 char -> 1 out_char
				std::ignore = Dest;
				return sizeof(data_type) / sizeof(input_type_of<CharsType::UTF16>::value_type);
			}
			else if constexpr (Source == CharsType::UTF32)
			{
				// truncate ==> 1 char -> 1 out_char
				std::ignore = Dest;
				return sizeof(data_type) / sizeof(input_type_of<CharsType::UTF32>::value_type);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		template<CharsType InputType>
		[[nodiscard]] constexpr static auto read(const typename input_type_of<InputType>::pointer source) noexcept -> data_type
		{
			std::ignore = InputType;
			return memory::unaligned_load<data_type>(source);
		}

		// const auto value = read(pointer); if (pure_ascii(value)) { ... } else { ... }
		template<CharsType InputType>
		[[nodiscard]] constexpr static auto pure_ascii(const data_type value) noexcept -> bool
		{
			if constexpr (
				InputType == CharsType::LATIN or
				InputType == CharsType::UTF8_CHAR or
				InputType == CharsType::UTF8
			)
			{
				return (value & 0x8080'8080'8080'8080) == 0;
			}
			else if constexpr (
				InputType == CharsType::UTF16_LE or
				InputType == CharsType::UTF16_BE or
				InputType == CharsType::UTF16
			)
			{
				return (value & 0xff80'ff80'ff80'ff80) == 0;
			}
			else if constexpr (InputType == CharsType::UTF32)
			{
				return (value & 0xffff'ff80'ffff'ff80) == 0;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		template<CharsType OutputType>
		[[nodiscard]] constexpr static auto char_of(const auto native_value) noexcept -> typename output_type_of<OutputType>::value_type
		{
			using out_type = typename output_type_of<OutputType>::value_type;

			if constexpr (OutputType == CharsType::UTF16_LE or OutputType == CharsType::UTF16_BE)
			{
				if constexpr (const auto v16 = static_cast<std::uint16_t>(native_value);
					(OutputType == CharsType::UTF16_LE) != (std::endian::native == std::endian::little))
				{
					return static_cast<out_type>(std::byteswap(v16));
				}
				else
				{
					return static_cast<out_type>(v16);
				}
			}
			else
			{
				return static_cast<out_type>(native_value);
			}
		}
	};
}
