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
GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

import :surface;
import :style;
import :element;

#else
#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>
#include <draw/surface.ixx>
#include <draw/style.ixx>
#include <draw/element.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

namespace
{
	using namespace gal::prometheus;
	using namespace draw;

	class Border final : public impl::Element
	{
	public:
		using color_type = Style::color_type;

	private:
		color_type color_;

	public:
		Border(element_type element, const color_type color) noexcept
			: Element{elements_type{std::move(element)}},
			  color_{color} {}

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			Element::calculate_requirement(surface);

			requirement_ = children_[0]->requirement();

			const auto line_pixel_width = Style::instance().line_pixel_width;
			requirement_.min_width += 2 * line_pixel_width;
			requirement_.min_height += 2 * line_pixel_width;
		}

		auto set_rect(const rect_type& rect) noexcept -> void override
		{
			Element::set_rect(rect);

			const auto line_pixel_width = Style::instance().line_pixel_width;
			const auto& [point, extent] = rect;

			const rect_type box
			{
					// left
					point.x + line_pixel_width,
					// top
					point.y + line_pixel_width,
					// right
					point.x + extent.width - 2 * line_pixel_width,
					// bottom
					point.y + extent.height - 2 * line_pixel_width
			};

			children_[0]->set_rect(box);
		}

		auto render(Surface& surface) noexcept -> void override
		{
			render(surface, DrawFlag::ROUND_CORNER_ALL);
		}

		auto render(Surface& surface, const DrawFlag flag) const noexcept -> void
		{
			surface.draw_list().rect(rect_, color_, Style::instance().border_round, flag, Style::instance().line_pixel_width);
			children_[0]->render(surface);
		}
	};

	class Window final : public impl::Element
	{
	public:
		using color_type = Style::color_type;

	private:
		color_type color_;

	public:
		// title + content(border)
		Window(elements_type children, const color_type color) noexcept
			: Element{std::move(children)},
			  color_{color} {}

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			Element::calculate_requirement(surface);

			// border
			requirement_ = children_[1]->requirement();
			// title
			requirement_.min_width = std::ranges::max(requirement_.min_width, children_[0]->requirement().min_width);
			requirement_.min_height += children_[0]->requirement().min_height;
		}

		auto set_rect(const rect_type& rect) noexcept -> void override
		{
			Element::set_rect(rect);

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
			children_[0]->set_rect(title_box);

			// border
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
			children_[1]->set_rect(content_box);
		}

		auto render(Surface& surface) noexcept -> void override
		{
			// title
			surface.draw_list().rect_filled(children_[0]->rect(), color_, Style::instance().border_round, DrawFlag::ROUND_CORNER_TOP);
			children_[0]->render(surface);

			// border
			const auto border = cast_element_unchecked<Border>(children_[1]);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(border != nullptr);
			border->render(surface, DrawFlag::ROUND_CORNER_BOTTOM);
		}
	};
}

namespace gal::prometheus::draw::element::impl
{
	[[nodiscard]] auto border(element_type element, const Style::color_type color) noexcept -> element_type
	{
		return make_element<Border>(std::move(element), color);
	}

	[[nodiscard]] auto window(element_type title, const Style::color_type title_color, element_type content, const Style::color_type content_color) noexcept -> element_type
	{
		auto b = border(std::move(content), content_color);
		return make_element<Window>(elements_type{std::move(title), std::move(b)}, title_color);
	}
}
