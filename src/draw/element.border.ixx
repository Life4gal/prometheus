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
import :element;

#else
#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>
#include <draw/surface.ixx>
#include <draw/element.ixx>

#endif

namespace
{
	using namespace gal::prometheus;
	using namespace draw;

	// todo
	constexpr auto pixel_offset_x = 4;
	constexpr auto pixel_offset_y = 4;

	class Border final : public impl::Element
	{
	public:
		using color_type = Surface::draw_list_type::color_type;
		using point_type = rect_type::point_type;

	private:
		color_type color_;

	public:
		Border(elements_type children, const color_type color) noexcept
			: Element{std::move(children)},
			  color_{color} {}

		auto calculate_requirement(Surface& surface) noexcept -> void override
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not children_.empty());

			Element::calculate_requirement(surface);

			requirement_ = children_[0]->requirement();
			requirement_.min_width += 2 * pixel_offset_x;
			requirement_.min_height += 2 * pixel_offset_y;
			if (children_.size() == 2)
			{
				requirement_.min_width = std::ranges::max(
					requirement_.min_width,
					children_[1]->requirement().min_width + 2 * pixel_offset_x
				);
			}
		}

		auto set_rect(const rect_type& rect) noexcept -> void override
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not children_.empty());

			Element::set_rect(rect);

			if (children_.size() == 2)
			{
				const rect_type title_box
				{
						rect.left_top() + point_type{pixel_offset_x, 0},
						rect.right_bottom() + point_type{-pixel_offset_x, 0}
				};
				children_[1]->set_rect(title_box);
			}

			const rect_type box
			{
					rect.left_top() + point_type{pixel_offset_x, pixel_offset_y},
					rect.right_bottom() + point_type{-pixel_offset_y, -pixel_offset_y}
			};
			children_[0]->set_rect(box);
		}

		auto render(Surface& surface) noexcept -> void override
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not children_.empty());

			children_[0]->render(surface);

			surface.draw_list().rect(rect_, color_, 0, DrawFlag::ROUND_CORNER_ALL, 1);

			if (children_.size() == 2)
			{
				children_[1]->render(surface);
			}
		}
	};
}

namespace gal::prometheus::draw::element::impl
{
	[[nodiscard]] auto border(elements_type elements, const primitive::colors::color_type color) noexcept -> element_type
	{
		return make_element<Border>(std::move(elements), color);
	}

	[[nodiscard]] auto border(element_type element, const primitive::colors::color_type color) noexcept -> element_type
	{
		return border(elements_type{std::move(element)}, color);
	}
}
