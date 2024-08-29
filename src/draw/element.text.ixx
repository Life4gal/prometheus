// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.text;

import std;

import gal.prometheus.primitive;
import gal.prometheus.chars;

import :surface;
import :element;

#else
#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>
#include <chars/chars.ixx>
#include <draw/surface.ixx>
#include <draw/element.ixx>

#endif

namespace
{
	using namespace gal::prometheus;
	using namespace draw;

	// todo
	constexpr auto default_font_size = 18.f;

	[[nodiscard]] auto calculate_text_area(
		const font_type& font,
		const float font_size,
		const std::string_view text
	) noexcept -> impl::Element::rect_type::extent_type
	{
		using rect_type = impl::Element::rect_type;
		using point_type = rect_type::point_type;
		using extent_type = rect_type::extent_type;

		const auto utf32_text = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32>(text);

		const float scale = font_size / font.pixel_height;

		auto it_input_current = utf32_text.begin();
		const auto it_input_end = utf32_text.end();

		auto area = extent_type{0, 0};
		auto cursor = point_type{0, font_size};

		while (it_input_current != it_input_end)
		{
			const auto c = *it_input_current;
			it_input_current += 1;

			if (c == U'\n')
			{
				area.width = std::ranges::max(area.width, cursor.x);
				cursor.x = 0;
				cursor.y += font.pixel_height * scale;
			}

			const auto& [glyph_rect, glyph_uv, glyph_advance_x] = [&]
			{
				if (const auto it = font.glyphs.find(c);
					it != font.glyphs.end())
				{
					return it->second;
				}

				return font.fallback_glyph;
			}();

			const auto advance_x = glyph_advance_x * scale;
			cursor.x += advance_x;
		}

		area.width = std::ranges::max(area.width, cursor.x);
		area.height = cursor.y;
		return area;
	}

	class Text final : public impl::Element
	{
	public:
		using text_type = std::string;

	private:
		text_type text_;

	public:
		explicit Text(text_type&& text) noexcept
			: text_{std::move(text)} {}

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			const auto& font = surface.draw_list().shared_data()->get_default_font();
			const auto area = calculate_text_area(font, default_font_size, text_);
			requirement_.min_width = area.width;
			requirement_.min_height = area.height;
		}

		auto render(Surface& surface) noexcept -> void override
		{
			// todo
			surface.draw_list().text(default_font_size, rect_.left_top(), primitive::colors::black, text_);
		}
	};
}

namespace gal::prometheus::draw::element
{
	[[nodiscard]] auto text(std::string text) noexcept -> element_type
	{
		return make_element<Text>(std::move(text));
	}
}
