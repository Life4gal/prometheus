// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <vector>
#include <cstdint>

#include <prometheus/macro.hpp>

// ReSharper disable once CppInconsistentNaming
namespace gal::prometheus::i18n
{
	class RangeBuilder final
	{
	public:
		using value_type = std::uint32_t;

		struct range_type
		{
			value_type from;
			value_type to;
		};

		using ranges_type = std::vector<range_type>;

	private:
		ranges_type ranges_;

	public:
		template<typename Self>
		[[nodiscard]] auto range(this Self&& self) noexcept -> decltype(auto)
		{
			return std::forward<Self>(self).ranges_;
		}

		// Latin
		auto latin() & noexcept -> RangeBuilder&;
		auto latin() && noexcept -> RangeBuilder&&;

		// Latin + Greek and Coptic
		auto greek() & noexcept -> RangeBuilder&;
		auto greek() && noexcept -> RangeBuilder&&;

		// Latin + Korean characters
		auto korean() & noexcept -> RangeBuilder&;
		auto korean() && noexcept -> RangeBuilder&&;

		// Latin + Hiragana, Katakana, Half-Width, Selection of 2999 Ideographs
		auto japanese() & noexcept -> RangeBuilder&;
		auto japanese() && noexcept -> RangeBuilder&&;

		// Latin + Half-Width + Japanese Hiragana/Katakana + set of 2500 CJK Unified Ideographs for common simplified Chinese
		auto simplified_chinese_common() & noexcept -> RangeBuilder&;
		auto simplified_chinese_common() && noexcept -> RangeBuilder&&;

		// Latin + Half-Width + Japanese Hiragana/Katakana + full set of about 21000 CJK Unified Ideographs
		auto simplified_chinese_all() & noexcept -> RangeBuilder&;
		auto simplified_chinese_all() && noexcept -> RangeBuilder&&;
	};
}
