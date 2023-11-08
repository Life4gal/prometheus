// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:cp_1252;

import std;
import gal.prometheus.utility;

import :converter;
import :category;

namespace gal::prometheus::chars
{
	constexpr auto cp_1252_database_0000_02dc = []
	{
		using data_type = std::array<std::uint8_t, 0x02dd>;

		data_type data{};
		std::ranges::fill(data, char_placeholder);

		std::ranges::generate_n(
				data.begin(),
				0x80,
				[i = 0]() mutable noexcept -> std::uint8_t { return utility::char_cast<std::uint8_t>(i++); });

		data[0x81] = 0x81;
		data[0x8d] = 0x8d;
		data[0x8f] = 0x8f;
		data[0x90] = 0x90;
		data[0x9d] = 0x9d;

		std::ranges::generate_n(
				data.begin() + 0xa0,
				0x100 - 0xa0,
				[i = 0xa0]() mutable noexcept -> std::uint8_t { return utility::char_cast<std::uint8_t>(i++); });

		data[0x192] = 0x83;
		data[0x2c6] = 0x88;
		data[0x160] = 0x8a;
		data[0x152] = 0x8c;
		data[0x17d] = 0x8e;

		data[0x2dc] = 0x98;
		data[0x161] = 0x9a;
		data[0x153] = 0x9c;
		data[0x17e] = 0x9e;
		data[0x178] = 0x9f;

		return data;
	}();

	constexpr auto cp_1252_database_2000_2122 = []
	{
		using data_type = std::array<std::uint8_t, 0x123>;

		data_type data{};
		std::ranges::fill(data, char_placeholder);

		data[0xac] = 0x80;
		data[0x1a] = 0x82;
		data[0x1e] = 0x84;
		data[0x26] = 0x85;
		data[0x20] = 0x86;
		data[0x21] = 0x87;
		data[0x30] = 0x89;
		data[0x39] = 0x8b;

		data[0x18]  = 0x91;
		data[0x19]  = 0x92;
		data[0x1c]  = 0x93;
		data[0x1d]  = 0x94;
		data[0x22]  = 0x95;
		data[0x13]  = 0x96;
		data[0x14]  = 0x97;
		data[0x122] = 0x99;
		data[0x3a]  = 0x9b;

		return data;
	}();

	export
	{
		template<>
		class CharMap<char_map_category_cp_1252> : public CharInvoker<CharMap<char_map_category_cp_1252>>
		{
			friend CharInvoker;

		public:
			using value_type = char;

		private:
			template<std::input_iterator Iterator>
			[[nodiscard]] constexpr auto do_endian([[maybe_unused]] const Iterator iterator, [[maybe_unused]] const std::size_t size, [[maybe_unused]] std::endian e) const noexcept -> std::endian
			{
				(void)this;

				return std::endian::native;
			}

			template<std::input_iterator Begin, std::sentinel_for<Begin> End>
			[[nodiscard]] constexpr auto do_read(Begin& begin, [[maybe_unused]] const End end) const noexcept -> std::pair<code_point_type, bool>
			{
				const auto result = [begin]() noexcept -> std::pair<code_point_type, bool>
				{
					// https://en.wikipedia.org/wiki/Windows-1252#Codepage_layout
					switch (const auto code_point = utility::char_cast<code_point_type>(*begin))
					{
						case 0x80: { return {0x20ac, true}; }
						case 0x81: { return {0x81, true}; }
						case 0x82: { return {0x201a, true}; }
						case 0x83: { return {0x0192, true}; }
						case 0x84: { return {0x201e, true}; }
						case 0x85: { return {0x2026, true}; }
						case 0x86: { return {0x2020, true}; }
						case 0x87: { return {0x2021, true}; }
						case 0x88: { return {0x02c6, true}; }
						case 0x89: { return {0x2030, true}; }
						case 0x8a: { return {0x0160, true}; }
						case 0x8b: { return {0x2039, true}; }
						case 0x8c: { return {0x0152, true}; }
						case 0x8d: { return {0x8d, true}; }
						case 0x8e: { return {0x017d, true}; }
						case 0x8f: { return {0x8f, true}; }
						case 0x90: { return {0x90, true}; }
						case 0x91: { return {0x2018, true}; }
						case 0x92: { return {0x2019, true}; }
						case 0x93: { return {0x201c, true}; }
						case 0x94: { return {0x201d, true}; }
						case 0x95: { return {0x2022, true}; }
						case 0x96: { return {0x2013, true}; }
						case 0x97: { return {0x2014, true}; }
						case 0x98: { return {0x02dc, true}; }
						case 0x99: { return {0x2122, true}; }
						case 0x9a: { return {0x0161, true}; }
						case 0x9b: { return {0x203a, true}; }
						case 0x9c: { return {0x0153, true}; }
						case 0x9d: { return {0x9d, true}; }
						case 0x9e: { return {0x017e, true}; }
						case 0x9f: { return {0x0178, true}; }
						default: { return {code_point, true}; }
					}
				}();
				std::ranges::advance(begin, 1);
				return result;
			}

			template<std::output_iterator<value_type> Iterator>
			constexpr auto do_write(Iterator& dest, const code_point_type code_point) const noexcept -> void
			{
				(void)this;

				GAL_PROMETHEUS_DEBUG_ASSUME(code_point < 0x11'0000);
				GAL_PROMETHEUS_DEBUG_ASSUME(not(code_point >= 0xd800 and code_point < 0xe000));

				if (code_point < 0x2dd) { *dest = utility::char_cast<value_type>(cp_1252_database_0000_02dc[code_point]); }
				else if (code_point < 0x2000) { *dest = utility::char_cast<value_type>(char_placeholder); }
				else if (code_point < 0x2123) { *dest = utility::char_cast<value_type>(cp_1252_database_2000_2122[code_point - 0x2000]); }
				else { *dest = utility::char_cast<value_type>(char_placeholder); }

				std::ranges::advance(dest, 1);
			}

			constexpr auto do_size(const code_point_type code_point) const noexcept -> std::pair<std::uint8_t, bool>
			{
				(void)this;

				GAL_PROMETHEUS_DEBUG_ASSUME(code_point < 0x11'0000);
				GAL_PROMETHEUS_DEBUG_ASSUME(not(code_point >= 0xd800 and code_point < 0xe000));

				if (code_point < 0x2dd)
				{
					if (code_point == char_placeholder) { return {utility::narrow_cast<std::uint8_t>(1), true}; }
					return {utility::narrow_cast<std::uint8_t>(1), cp_1252_database_0000_02dc[code_point] != char_placeholder};
				}

				if (code_point < 0x2000) { return {utility::narrow_cast<std::uint8_t>(1), false}; }

				if (code_point < 0x2123) { return {utility::narrow_cast<std::uint8_t>(1), cp_1252_database_2000_2122[utility::wide_cast<std::size_t>(code_point) - 0x2000] != char_placeholder}; }

				return {utility::narrow_cast<std::uint8_t>(1), false};
			}
		};
	}
}
