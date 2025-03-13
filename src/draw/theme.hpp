// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <draw/def.hpp>
#include <draw/flag.hpp>

namespace gal::prometheus::draw
{
	class [[nodiscard]] Theme final
	{
	public:
		using rect_type = DrawListDef::rect_type;
		using point_type = DrawListDef::point_type;
		using extent_type = DrawListDef::extent_type;

		using value_type = point_type::value_type;

		using color_type = DrawListDef::color_type;
		using colors_type = std::array<color_type, theme_category_count>;

		std::string font_path;
		value_type font_size;
		value_type title_bar_height;
		value_type window_rounding;
		extent_type window_padding;
		extent_type window_min_size;
		extent_type resize_grip_size;
		extent_type item_spacing;
		extent_type item_inner_spacing;
		colors_type colors;

		template<ThemeCategory Category>
			requires (std::to_underlying(Category) < theme_category_count)
		[[nodiscard]] constexpr auto color() const noexcept -> color_type
		{
			return colors[static_cast<std::size_t>(Category)];
		}

		[[nodiscard]] static auto default_theme() noexcept -> Theme;

		[[nodiscard]] static auto another_theme_for_test() noexcept -> Theme;
	};
}
