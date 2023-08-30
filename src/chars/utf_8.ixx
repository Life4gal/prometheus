// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:utf_8;

import std;
import gal.prometheus.infrastructure;

import :converter;
import :category;
import :cp_1252;

export namespace gal::prometheus::chars
{
	template<>
	class CharMap<char_map_category_utf_8> : public CharInvoker<CharMap<char_map_category_utf_8>>
	{
		friend CharInvoker;

	public:
		using value_type = char;

		constexpr static auto fallback_category = char_map_category_cp_1252;
		using fallback_encoder_type = CharMap<fallback_category>;
		using fallback_value_type = fallback_encoder_type::value_type;

	private:
		template<std::input_iterator Iterator>
		[[nodiscard]] constexpr auto do_endian([[maybe_unused]] const Iterator iterator, [[maybe_unused]] const std::size_t size, [[maybe_unused]] std::endian e) const noexcept -> std::endian
		{
			(void)this;

			return std::endian::native;
		}

		[[nodiscard]] constexpr auto do_fallback_peek(const fallback_value_type code_unit) const noexcept -> std::pair<code_point_type, bool>
		{
			(void)this;

			std::basic_string_view string{&code_unit, 1};

			auto begin = string.begin();
			return fallback_encoder_type{}.read(begin, string.end());
		}

		template<std::input_iterator Begin, std::sentinel_for<Begin> End>
		[[nodiscard]] constexpr auto do_read(Begin& begin, const End end, const char8_t first_code_unit) const noexcept -> std::pair<code_point_type, bool>
		{
			if (begin == end or (first_code_unit & 0xc0) == 0x80)
			{
				// A non-ASCII character at the end of string.
				// Or an unexpected continuation code-unit should be treated as CP-1252.
				return do_fallback_peek(infrastructure::char_cast<fallback_value_type>(first_code_unit));
			}

			const auto length = infrastructure::narrow_cast<std::uint8_t>(std::countl_one(infrastructure::char_cast<std::uint8_t>(first_code_unit)));
			GAL_PROMETHEUS_DEBUG_ASSUME(length >= 2);

			// First part of the code-point.
			auto                     code_point =
					[length](char8_t code_uint) -> code_point_type
					{
						code_uint <<= length;
						code_uint >>= length;
						return infrastructure::char_cast<code_point_type>(code_uint);
					}(first_code_unit);

			// Read the first continuation code-unit which is always here.
			if (const auto code_unit = infrastructure::char_cast<char8_t>(*begin);
				(code_unit & 0xc0) != 0x80)
			{
				// If the second code-unit is not a UTF-8 continuation character, treat the first code-unit as if it was CP-1252.
				return do_fallback_peek(infrastructure::char_cast<fallback_value_type>(first_code_unit));
			}
			else
			{
				code_point <<= 6;
				code_point |= code_unit & char_placeholder;
			}

			// If there are a start and a continuation code-unit in a sequence we consider this to be properly UTF-8 encoded.
			// So from this point any errors are replaced with 0xfffd.
			std::ranges::advance(begin, 1);

			for (std::uint8_t real_length = 2; real_length < length; ++real_length)
			{
				if (begin == end) { return {0xfffd, false}; }

				const auto code_unit = infrastructure::char_cast<char8_t>(*begin);
				if ((code_unit & 0b1100'0000) != 0b1000'0000)
				{
					// Unexpected end of sequence.
					return {0xfffd, false};
				}

				std::ranges::advance(begin, 1);

				// shift in the next 6 bits.
				code_point <<= 6;
				code_point |= code_unit & 0b0011'1111;
			}

			if (const auto valid =
						// range
						(code_point < 0x11'0000) and
						// not a surrogate
						(code_point < 0xd800 or code_point >= 0xe000) and
						// not overlong encoded
						(length == infrastructure::narrow_cast<std::uint8_t>((code_point > 0x7f) + (code_point > 0x7ff) + (code_point > 0xffff) + 1));
				not valid) { return {0xfffd, false}; }
			return {code_point, true};
		}

		template<std::input_iterator Begin, std::sentinel_for<Begin> End>
		[[nodiscard]] constexpr auto do_read(Begin& begin, const End end) const noexcept -> std::pair<code_point_type, bool>
		{
			const auto first_code_uint = infrastructure::char_cast<char8_t>(*begin);
			std::ranges::advance(begin, 1);

			if ((first_code_uint & 0x80) == 0)
			[[likely]]
			{
				// ASCII character.
				return {infrastructure::char_cast<code_point_type>(first_code_uint), true};
			}

			return do_read(begin, end, first_code_uint);
		}

		template<std::output_iterator<value_type> Iterator>
		constexpr auto do_write(Iterator& dest, const code_point_type code_point) const noexcept -> void
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_ASSUME(code_point < 0x11'0000);
			GAL_PROMETHEUS_DEBUG_ASSUME(not(code_point >= 0xd800 and code_point < 0xe000));

			const auto num_code_uint = infrastructure::narrow_cast<std::uint8_t>((code_point > 0x7f) + (code_point > 0x7ff) + (code_point > 0xffff));
			const auto leading_ones  = num_code_uint == 0 ? 0 : infrastructure::char_cast<std::int8_t>(infrastructure::narrow_cast<std::uint8_t>(0x80)) >> num_code_uint;

			auto shift = num_code_uint * 6;

			*dest = infrastructure::char_cast<value_type>(infrastructure::truncate<std::uint8_t>(code_point >> shift) | infrastructure::truncate<std::uint8_t>(leading_ones));
			std::ranges::advance(dest, 1);

			while (shift)
			{
				shift -= 6;

				const auto code_unit = infrastructure::truncate<std::uint8_t>(code_point >> shift) & 0b0011'1111 | 0b1000'0000;

				*dest = infrastructure::char_cast<value_type>(code_unit);
				std::ranges::advance(dest, 1);
			}
		}

		constexpr auto do_size(const code_point_type code_point) const noexcept -> std::pair<std::uint8_t, bool>
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_ASSUME(code_point < 0x11'0000);
			GAL_PROMETHEUS_DEBUG_ASSUME(not(code_point >= 0xd800 and code_point < 0xe000));

			return {infrastructure::narrow_cast<std::uint8_t>((code_point > 0x7f) + (code_point > 0x7ff) + (code_point > 0xffff) + 1), true};
		}
	};
}
