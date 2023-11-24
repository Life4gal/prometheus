// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars:scalar.utf32;

import std;
import gal.prometheus.string;
import gal.prometheus.error;
import gal.prometheus.utility;

import :encoding;
import :converter;

namespace gal::prometheus::chars
{
	using input_category = CharsCategory::UTF32;
	using input_type	 = chars::input_type<input_category>;
	using pointer_type	 = input_type::const_pointer;
	using size_type		 = input_type::size_type;

	template<>
	class Scalar<"utf32">
	{
	public:
		[[nodiscard]] constexpr auto validate(const input_type input) const noexcept -> result_type
		{
			(void)this;

			GAL_PROMETHEUS_DEBUG_NOT_NULL(input.data());

			const auto		   input_length		= input.size();

			const pointer_type it_input_begin	= input.data();
			pointer_type	   it_input_current = it_input_begin;
			const pointer_type it_input_end		= it_input_begin + input_length;

			while (it_input_current != it_input_end)
			{
				const auto counf_if_error = static_cast<std::size_t>(it_input_current - it_input_begin);

				if (const auto word = *it_input_current;
					word > 0x10FFFF) { return result_type{.error = ErrorCode::TOO_LARGE, .count = counf_if_error}; }
				else if (word >= 0xD800 and word <= 0xDFFF) { return result_type{.error = ErrorCode::SURROGATE, .count = counf_if_error}; }
			}

			return {.error = ErrorCode::NONE, .count = input_length};
		}
	};

	export namespace instance::scalar
	{
		constexpr Scalar<"utf32"> utf32{};
	}
}// namespace gal::prometheus::chars
