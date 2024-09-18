// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.border;

import std;

import gal.prometheus.primitive;

import :surface;
import :style;
import :element.element;

#else
#pragma once

#include <prometheus/macro.hpp>

#include <optional>

#include <primitive/primitive.ixx>

#include <draw/surface.ixx>
#include <draw/style.ixx>
#include <draw/detail/element.ixx>

#endif

namespace gal::prometheus::draw
{
	namespace detail::element
	{
		enum class BorderOption
		{
			NONE
		};

		template<typename T>
		concept border_option_t = std::is_same_v<T, BorderOption>;
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace element
	{
		constexpr auto border = detail::options<detail::element::BorderOption::NONE>{};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		namespace element
		{
			class Border final : public Element
			{
			public:
				using color_type = Style::color_type;

			private:
				std::optional<color_type> color_;

				[[nodiscard]] static auto extra_offset(const Style& style) noexcept -> Style::extern_type
				{
					const auto line_width = style.line_width;
					const auto& border_padding = style.border_padding;
					const auto offset_x = line_width + border_padding.width;
					const auto offset_y = line_width + border_padding.height;

					return {offset_x, offset_y};
				}

			public:
				Border(element_type element, const color_type color) noexcept
					: Element{std::move(element)},
					  color_{color} {}

				explicit Border(element_type element) noexcept
					: Element{std::move(element)},
					  color_{std::nullopt} {}

				auto calculate_requirement(const Style& style, Surface& surface) noexcept -> void override
				{
					Element::calculate_requirement(style, surface);

					requirement_ = children_[0]->requirement();

					const auto [offset_x, offset_y] = extra_offset(style);

					requirement_.min_width += 2 * offset_x;
					requirement_.min_height += 2 * offset_y;
				}

				auto set_rect(const Style& style, const rect_type& rect) noexcept -> void override
				{
					Element::set_rect(style, rect);

					const auto [offset_x, offset_y] = extra_offset(style);
					const auto& [point, extent] = rect;

					const rect_type box
					{
							// left
							point.x + offset_x,
							// top
							point.y + offset_y,
							// right
							point.x + extent.width - offset_x,
							// bottom
							point.y + extent.height - offset_y
					};

					children_[0]->set_rect(style, box);
				}

				auto render(const Style& style, Surface& surface) noexcept -> void override
				{
					render(style, surface, DrawFlag::ROUND_CORNER_ALL);
				}

				auto render(const Style& style, Surface& surface, const DrawFlag flag) const noexcept -> void
				{
					surface.draw_list().rect(rect_, color_.value_or(style.border_color), style.border_rounding, flag, style.line_width);
					children_[0]->render(style, surface);
				}
			};

			class TitleBorder final : public Element
			{
			public:
				using color_type = Style::color_type;

			private:
				std::optional<color_type> color_;

			public:
				TitleBorder(element_type title, const color_type title_color, element_type content) noexcept
					: Element{std::move(title), std::move(content)},
					  color_{title_color} {}

				TitleBorder(element_type title, element_type content) noexcept
					: Element{std::move(title), std::move(content)},
					  color_{std::nullopt} {}

				auto calculate_requirement(const Style& style, Surface& surface) noexcept -> void override
				{
					Element::calculate_requirement(style, surface);

					// content
					requirement_ = children_[1]->requirement();
					// title
					requirement_.min_width = std::ranges::max(requirement_.min_width, children_[0]->requirement().min_width);
					requirement_.min_height += children_[0]->requirement().min_height;
				}

				auto set_rect(const Style& style, const rect_type& rect) noexcept -> void override
				{
					Element::set_rect(style, rect);

					const auto& [point, extent] = rect;

					// title
					const rect_type title_box{
							// left
							point.x,
							// top
							point.y,
							// right
							point.x + extent.width,
							// bottom
							point.y + children_[0]->requirement().min_height
					};
					children_[0]->set_rect(style, title_box);

					// content
					const rect_type content_box{
							// left
							point.x,
							// top
							point.y + title_box.height(),
							// right
							point.x + title_box.width(),
							// bottom
							point.y + extent.height
					};
					children_[1]->set_rect(style, content_box);
				}

				auto render(const Style& style, Surface& surface) noexcept -> void override
				{
					// title
					surface.draw_list().rect_filled(children_[0]->rect(), color_.value_or(style.window_title_color), style.border_rounding, DrawFlag::ROUND_CORNER_TOP);
					children_[0]->render(style, surface);

					// content
					const auto content = cast_element_unchecked<Border>(children_[1]);
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(content != nullptr);
					content->render(style, surface, DrawFlag::ROUND_CORNER_BOTTOM);
				}
			};
		}

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<element::border_option_t auto... Os>
		struct maker<Os...>
		{
			[[nodiscard]] auto operator()(element_type element, const element::Border::color_type color) const noexcept -> element_type
			{
				return make_element<element::Border>(std::move(element), color);
			}

			[[nodiscard]] auto operator()(const element::Border::color_type color) const noexcept -> auto
			{
				return [color, this](element_type element) noexcept -> element_type
				{
					return this->operator()(std::move(element), color);
				};
			}

			[[nodiscard]] auto operator()(element_type element) const noexcept -> element_type
			{
				return make_element<element::Border>(std::move(element));
			}

			[[nodiscard]] auto operator()(
				element_type title,
				const element::TitleBorder::color_type title_color,
				element_type content,
				const element::Border::color_type content_color
			) const noexcept -> element_type
			{
				auto content_with_border = this->operator()(std::move(content), content_color);
				return make_element<element::TitleBorder>(std::move(title), title_color, std::move(content_with_border));
			}

			[[nodiscard]] auto operator()(const element::TitleBorder::color_type title_color, const element::Border::color_type content_color) const noexcept -> auto
			{
				return [title_color, content_color, this](element_type title, element_type content) noexcept -> element_type
				{
					return this->operator()(std::move(title), title_color, std::move(content), content_color);
				};
			}

			[[nodiscard]] auto operator()(element_type title, element_type content) const noexcept -> element_type
			{
				auto content_with_border = this->operator()(std::move(content));
				return make_element<element::TitleBorder>(std::move(title), std::move(content_with_border));
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
