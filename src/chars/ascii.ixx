// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:ascii;

import std;
import gal.prometheus.utility;

import :converter;
import :category;

export namespace gal::prometheus::chars
{
	template<>
	class CharMap<char_map_category_ascii> : public CharInvoker<CharMap<char_map_category_ascii>>
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
			const auto code_point = utility::char_cast<code_point_type>(*begin);
			std::ranges::advance(begin, 1);

			if (code_point < 0x80) { return {code_point, true}; }
			return {0xfffd, false};
		}

		template<std::output_iterator<value_type> Iterator>
		constexpr auto do_write(Iterator& dest, const code_point_type code_point) const noexcept -> void
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_ASSUME(code_point < 0x11'0000);
			GAL_PROMETHEUS_DEBUG_ASSUME(not(code_point >= 0xd800 and code_point < 0xe000));

			if (code_point < 0x80) { *dest = utility::char_cast<value_type>(code_point); }
			else { *dest = utility::char_cast<value_type>(char_placeholder); }

			std::ranges::advance(dest, 1);
		}

		constexpr auto do_size(const code_point_type code_point) const noexcept -> std::pair<std::uint8_t, bool>
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_ASSUME(code_point < 0x11'0000);
			GAL_PROMETHEUS_DEBUG_ASSUME(not(code_point >= 0xd800 and code_point < 0xe000));

			if (code_point < 0x80) { return {utility::narrow_cast<std::uint8_t>(1), true}; }

			return {utility::narrow_cast<std::uint8_t>(1), false};
		}
	};
}
