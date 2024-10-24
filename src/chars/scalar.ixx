// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:chars.scalar;

import std;

#if GAL_PROMETHEUS_COMPILER_DEBUG
import :error;
#endif

import :chars.encoding;
export import :chars.scalar.ascii;
export import :chars.scalar.utf8;
export import :chars.scalar.utf16;
export import :chars.scalar.utf32;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <bit>
#include <type_traits>
#include <span>

#include <prometheus/macro.hpp>
#include <chars/encoding.ixx>
#include <chars/scalar_ascii.ixx>
#include <chars/scalar_utf8.ixx>
#include <chars/scalar_utf16.ixx>
#include <chars/scalar_utf32.ixx>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

// ReSharper disable once CppRedundantNamespaceDefinition
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(chars)
{
	template<>
	class Encoding<"scalar">
	{
	public:
		[[nodiscard]] constexpr static auto encoding_of(const std::span<const char8_t> input) noexcept -> EncodingType
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(input.data() != nullptr);

			if (const auto bom = bom_of(input); bom != EncodingType::UNKNOWN)
			{
				return bom;
			}

			auto all_possible = std::to_underlying(EncodingType::UNKNOWN);
			if (Scalar<"utf8">::validate(input))
			{
				all_possible |= std::to_underlying(EncodingType::UTF8);
			}

			if ((input.size() % 2) == 0 and
			    Scalar<"utf16">::validate<std::endian::little>({GAL_PROMETHEUS_SEMANTIC_UNRESTRICTED_CHAR_POINTER_CAST(Scalar<"utf16">::char_type, input.data()), input.size() / 2}))
			{
				all_possible |= std::to_underlying(EncodingType::UTF16_LE);
			}

			if ((input.size() % 4) == 0 and
			    Scalar<"utf32">::validate({GAL_PROMETHEUS_SEMANTIC_UNRESTRICTED_CHAR_POINTER_CAST(Scalar<"utf32">::char_type, input.data()), input.size() / 4}))
			{
				all_possible |= std::to_underlying(EncodingType::UTF32_LE);
			}

			return static_cast<EncodingType>(all_possible);
		}

		[[nodiscard]] constexpr static auto encoding_of(const std::span<const char> input) noexcept -> EncodingType
		{
			static_assert(sizeof(char) == sizeof(char8_t));

			const auto* char8_string = GAL_PROMETHEUS_SEMANTIC_UNRESTRICTED_CHAR_POINTER_CAST(char8_t, input.data());
			return encoding_of({char8_string, input.size()});
		}
	};
}
