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
import :style;
import :element.element;

#else
#pragma once

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>
#include <chars/chars.ixx>

#include <draw/surface.ixx>
#include <draw/style.ixx>
#include <draw/detail/element.ixx>

#endif

namespace gal::prometheus::draw
{
	namespace detail
	{
		enum class TextOption
		{
			NONE,
		};

		template<typename T>
		concept text_option_t = std::is_same_v<T, TextOption>;
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	constexpr auto text = options<detail::TextOption::NONE>{};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		[[nodiscard]] inline auto calculate_text_area(
			const font_type& font,
			const float font_size,
			const std::u32string_view s
		) noexcept -> Element::rect_type::extent_type
		{
			using rect_type = Element::rect_type;
			using point_type = rect_type::point_type;
			using extent_type = rect_type::extent_type;

			const float scale = font_size / font.pixel_height;

			auto it_input_current = s.begin();
			const auto it_input_end = s.end();

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

		class Text final : public Element
		{
		public:
			using text_type = std::u32string;

		private:
			text_type text_;

		public:
			explicit Text(std::u32string&& string) noexcept
				: text_{std::move(string)} {}

			auto calculate_requirement(Surface& surface) noexcept -> void override
			{
				const auto& font = surface.draw_list().shared_data()->get_default_font();
				const auto area = calculate_text_area(font, Style::instance().font_size, text_);
				requirement_.min_width = area.width;
				requirement_.min_height = area.height;
			}

			auto render(Surface& surface) noexcept -> void override
			{
				surface.draw_list().text(Style::instance().font_size, rect_.left_top(), primitive::colors::black, text_, rect_.width());
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<text_option_t auto... Os>
		struct element_maker<Os...>
		{
			[[nodiscard]] auto operator()(Text::text_type string) const noexcept -> element_type
			{
				return make_element<Text>(std::move(string));
			}

			[[nodiscard]] auto operator()(const std::string_view& string) const noexcept -> element_type
			{
				return this->operator()(chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32>(string));
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
