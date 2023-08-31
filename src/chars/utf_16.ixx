// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>
#include <emmintrin.h>

export module gal.prometheus.chars:utf_16;

import std;
import gal.prometheus.infrastructure;

import :converter;
import :category;

export namespace gal::prometheus::chars
{
	template<>
	class CharMap<char_map_category_utf_16> : public CharInvoker<CharMap<char_map_category_utf_16>>
	{
		friend CharInvoker;

	public:
		using value_type = char16_t;

	private:
		template<std::contiguous_iterator Iterator>
		[[nodiscard]] constexpr auto do_endian(const Iterator iterator, const std::size_t size, std::endian e) const noexcept -> std::endian
		{
			if (size < 2) { return std::endian::native; }

			using value_type = typename std::iterator_traits<Iterator>::value_type;

			if constexpr (sizeof(value_type) == sizeof(std::uint8_t))
			{
				// Check for BOM.
				{
					const auto first  = infrastructure::char_cast<std::uint8_t>(*iterator);
					const auto second = infrastructure::char_cast<std::uint8_t>(*std::ranges::next(iterator, 1));

					if (first == 0xfe and second == 0xff) { return std::endian::big; }
					if (first == 0xff and second == 0xfe) { return std::endian::little; }
				}

				// Check for sequences of zeros.
				{
					std::array<std::size_t, 2> count{};
					for (std::size_t i = 0; i < size; ++i)
					{
						auto& current = count[i % count.size()];
						if (*std::ranges::next(iterator, i) == 0x00) { current += 1; }
						else { current = 0; }

						if (current >= 8) { return i % 2 == 0 ? std::endian::big : std::endian::little; }
					}
				}
			}
			else if constexpr (sizeof(value_type) == 2 * sizeof(std::uint8_t))
			{
				// Check for BOM.
				{
					const auto value  = *iterator;
					const auto first  = infrastructure::char_cast<std::uint8_t>(value >> 8);
					const auto second = infrastructure::char_cast<std::uint8_t>(value << 8 >> 8);

					if (first == 0xfe and second == 0xff) { return std::endian::big; }
					if (first == 0xff and second == 0xfe) { return std::endian::little; }
				}

				// Check for sequences of zeros.
				{
					std::array<std::size_t, 2> count{};
					for (std::size_t i = 0; i < size; ++i)
					{
						const auto value  = std::ranges::next(iterator, i);
						const auto first  = infrastructure::char_cast<std::uint8_t>(value >> 8);
						const auto second = infrastructure::char_cast<std::uint8_t>(value << 8 >> 8);

						// 0
						{
							auto& current = count[0];
							if (first == 0x00) { current += 1; }
							else { current = 0; }

							if (current >= 8) { return std::endian::big; }
						}
						// 1
						{
							auto& current = count[1];
							if (second == 0x00) { current += 1; }
							else { current = 0; }

							if (current >= 8) { return std::endian::little; }
						}
					}
				}
			}
			else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }

			return e;
		}

		template<std::input_iterator Begin, std::sentinel_for<Begin> End>
		[[nodiscard]] constexpr auto do_read(Begin& begin, const End end) const noexcept -> std::pair<code_point_type, bool>
		{
			const auto first_code_uint = *begin;
			std::ranges::advance(begin, 1);

			if (first_code_uint < 0xd800) { return {infrastructure::char_cast<code_point_type>(first_code_uint), true}; }

			if (first_code_uint < 0xdc00)
			{
				if (begin == end)
				{
					// first surrogate at end of string.
					return {0xfffd, false};
				}

				auto       code_point = infrastructure::char_cast<code_point_type>(first_code_uint & 0x3ff);
				const auto code_uint  = *begin;
				if (code_uint >= 0xdc00 and code_uint < 0xe000)
				{
					std::ranges::advance(begin, 1);
					code_point <<= 10;
					code_point |= code_uint & 0x3ff;
					code_point += 0x1'0000;
					return {code_point, true};
				}
				return {0xfffd, false};
			}

			if (first_code_uint < 0xe000)
			{
				// Invalid low surrogate.
				return {0xfffd, false};
			}

			return {first_code_uint, true};
		}

		template<std::output_iterator<value_type> Iterator>
		constexpr auto do_write(Iterator& dest, const code_point_type code_point) const noexcept -> void
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_ASSUME(code_point < 0x10'ffff);
			GAL_PROMETHEUS_DEBUG_ASSUME(not(code_point >= 0xd800 and code_point < 0xe000));

			if (const auto value = infrastructure::truncate<std::int32_t>(code_point) - 0x1'0000;
				value >= 0)
			{
				*std::ranges::next(dest, 0) = infrastructure::char_cast<value_type>((value >> 10) + 0xd800);
				*std::ranges::next(dest, 1) = infrastructure::char_cast<value_type>((value & 0x3ff) + 0xdc00);
				std::ranges::advance(dest, 2);
			}
			else
			{
				*dest = infrastructure::char_cast<value_type>(code_point);
				std::ranges::advance(dest, 1);
			}
		}

		constexpr auto do_size(const code_point_type code_point) const noexcept -> std::pair<std::uint8_t, bool>
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_ASSUME(code_point < 0x11'0000);
			GAL_PROMETHEUS_DEBUG_ASSUME(not(code_point >= 0xd800 and code_point < 0xe000));

			return {infrastructure::truncate<std::uint8_t>((code_point >= 0x01'0000) + 1), true};
		}

		template<std::contiguous_iterator Iterator>
		[[nodiscard]] constexpr auto do_chunk_read(const Iterator source) const noexcept -> chunk_type
		{
			// Load the UTF-16 data.
			const auto* p         = GAL_PROMETHEUS_START_LIFETIME_AS_ARRAY(chunk_type, std::addressof(*source), 2);
			const auto  low_part  = _mm_loadu_si128(p + 0);
			const auto  high_part = _mm_loadu_si128(p + 1);

			// To get _mm_packus_epi16() to work we need to prepare the data as follows:
			//  - bit 15 must be '0'.
			//  - if bit 15 was originally set than we need to set any of bits [14:8].

			// Positive numbers -> 0b0000'0000
			// Negative numbers -> 0b1000'0000
			const auto sign_low_part  = _mm_srai_epi16(low_part, 15);
			const auto sign_high_part = _mm_srai_epi16(high_part, 15);
			const auto sign           = _mm_packs_epi16(sign_low_part, sign_high_part);

			// ASCII            -> 0b0ccc'cccc
			// positive numbers -> 0b1???'????
			// negative numbers -> 0b0000'0000
			const auto chunk = _mm_packus_epi16(low_part, high_part);

			// ASCII            -> 0b0ccc'cccc
			// positive numbers -> 0b1???'????
			// negative numbers -> 0b1000'0000
			return _mm_or_si128(chunk, sign);
		}

		template<std::contiguous_iterator Iterator>
		constexpr auto do_chunk_write(const Iterator dest, const chunk_type chunk) const noexcept -> void
		{
			const auto zero      = _mm_setzero_si128();
			const auto low_part  = _mm_unpacklo_epi8(chunk, zero);
			const auto high_part = _mm_unpackhi_epi8(chunk, zero);

			auto* p = GAL_PROMETHEUS_START_LIFETIME_AS_ARRAY(chunk_type, std::addressof(*dest), 2);
			_mm_storeu_si128(p + 0, low_part);
			_mm_storeu_si128(p + 1, high_part);
		}
	};
}
