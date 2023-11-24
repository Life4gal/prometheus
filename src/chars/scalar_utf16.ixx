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
	using input_category = CharsCategory::UTF16;
	using input_type	 = chars::input_type<input_category>;
	using pointer_type	 = input_type::const_pointer;
	using size_type		 = input_type::size_type;

	template<>
	class Scalar<"utf16">
	{
	public:
		template<std::endian Endian>
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
					(word & 0xF800) == 0xD800)
				{
					const auto count_if_error = static_cast<std::size_t>(it_input_current - it_input_end);

					if (it_input_current + 1 == it_input_end) { return {.error = ErrorCode::SURROGATE, .count = count_if_error}; }

					if (const auto diff = word - 0xD800;
						diff > 0x3FF) { return {.error = ErrorCode::SURROGATE, .count = count_if_error}; }

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

					if (const auto diff = next_word - 0xDC00;
						diff > 0x3FF) { return {.error = ErrorCode::SURROGATE, .count = count_if_error}; }

					it_input_current += 2;
				}
				else { it_input_current += 1; }
			}

			return {.error = ErrorCode::NONE, .count = input_length};
		}

		template<std::endian Endian>
		[[nodiscard]] constexpr auto code_points(const input_type input) const noexcept -> std::size_t
		{
			return std::ranges::count_if(
					input,
					[](auto word) noexcept -> bool
					{
						if constexpr (Endian != std::endian::native) { word = std::byteswap(word); }

						return (word & 0xFC00) != 0xDC00;
					});
		}
	};

	export namespace instance::scalar
	{
		constexpr Scalar<"utf16"> utf16{};
	}
}// namespace gal::prometheus::chars
