// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:encoding;

import std;
import gal.prometheus.utility;
import gal.prometheus.error;

namespace gal::prometheus::chars
{
	export
	{
		enum class EncodingType
		{
			UNKNOWN = 0b0000'0000,

			// BOM 0xef 0xbb 0xbf
			UTF8 = 0b0000'0001,
			// BOM 0xff 0xfe
			UTF16_LE = 0b0000'0010,
			// BOM 0xfe 0xff
			UTF16_BE = 0b0000'0100,
			// BOM 0xff 0xfe 0x00 0x00
			UTF32_LE = 0b0000'1000,
			// BOM 0x00 0x00 0xfe 0xff
			UTF32_BE,
		};
	}

	namespace encoding_detail
	{
		constexpr utility::Enumerations encoding_type_meta{
				EncodingType::UNKNOWN, "unknown",
				EncodingType::UTF8, "UTF8",
				EncodingType::UTF16_LE, "UTF16 little-endian",
				EncodingType::UTF16_BE, "UTF16 big-endian",
				EncodingType::UTF32_LE, "UTF32 little-endian",
				EncodingType::UTF32_BE, "UTF32 big-endian",
		};
	}

	export
	{
		[[nodiscard]] constexpr auto to_string(const EncodingType type) noexcept -> decltype(auto) { return encoding_detail::encoding_type_meta[type]; }

		[[nodiscard]] constexpr auto to_size(const EncodingType type) noexcept -> std::size_t
		{
			switch (type)
			{
				case EncodingType::UNKNOWN: { return 0; }
				case EncodingType::UTF8: { return 3; }
				case EncodingType::UTF16_LE: { return 2; }
				case EncodingType::UTF16_BE: { return 2; }
				case EncodingType::UTF32_LE: { return 4; }
				case EncodingType::UTF32_BE: { return 4; }
				default: { GAL_PROMETHEUS_DEBUG_UNREACHABLE(); }
			}
		}

		class BomChecker
		{
		public:
			// [[nodiscard]] constexpr static auto operator()(const std::span<const char8_t> byte) noexcept -> EncodingType
			[[nodiscard]] constexpr static auto check(const std::span<const char8_t> byte) noexcept -> EncodingType
			{
				constexpr std::span bom_utf8{u8"\xef\xbb\xbf", to_size(EncodingType::UTF8)};
				constexpr std::span bom_utf16_le{u8"\xff\xfe", to_size(EncodingType::UTF16_LE)};
				constexpr std::span bom_utf16_be{u8"\xfe\xff", to_size(EncodingType::UTF16_BE)};
				constexpr std::span bom_utf32_le{u8"\xff\xfe\x00\x00", to_size(EncodingType::UTF32_LE)};
				constexpr std::span bom_utf32_be{u8"\x00\x00\xfe\xff", to_size(EncodingType::UTF32_BE)};

				if (const auto length = byte.size();
					length >= utility::functor::max(bom_utf8.size(), bom_utf32_le.size(), bom_utf32_be.size()))
				{
					if (std::ranges::equal(bom_utf32_le.begin(), bom_utf32_le.end(), byte.begin(), byte.begin() + bom_utf32_le.size())) { return EncodingType::UTF32_LE; }
					if (std::ranges::equal(bom_utf32_be.begin(), bom_utf32_be.end(), byte.begin(), byte.begin() + bom_utf32_be.size())) { return EncodingType::UTF32_BE; }
					if (std::ranges::equal(bom_utf8.begin(), bom_utf8.end(), byte.begin(), byte.begin() + bom_utf8.size())) { return EncodingType::UTF8; }
				}
				else if (length >= utility::functor::max(bom_utf16_le.size(), bom_utf16_be.size()))
				{
					if (std::ranges::equal(bom_utf16_le.begin(), bom_utf16_le.end(), byte.begin(), byte.begin() + bom_utf16_le.size())) { return EncodingType::UTF16_LE; }
					if (std::ranges::equal(bom_utf16_be.begin(), bom_utf16_be.end(), byte.begin(), byte.begin() + bom_utf16_be.size())) { return EncodingType::UTF16_BE; }
				}

				return EncodingType::UNKNOWN;
			}

			// [[nodiscard]] constexpr static auto operator()(const std::span<const char> byte) noexcept -> EncodingType
			[[nodiscard]] constexpr static auto check(const std::span<const char> byte) noexcept -> EncodingType
			{
				const auto data = std::span{GAL_PROMETHEUS_UNRESTRICTED_CHAR_POINTER_CAST(char8_t, byte.data()), byte.size()};
				// return operator()(data);
				return check(data);
			}
		};
	}
}// namespace gal::prometheus::chars
