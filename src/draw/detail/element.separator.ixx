// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.separator;

import std;

import gal.prometheus.primitive;

import :surface;
import :style;
import :element.element;

#else
#pragma once

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>

#include <draw/surface.ixx>
#include <draw/style.ixx>
#include <draw/detail/element.ixx>

#endif

namespace gal::prometheus::draw
{
	namespace detail
	{
		enum class SeparatorOption
		{
			NONE
		};

		template<typename T>
		concept separator_option_t = std::is_same_v<T, SeparatorOption>;
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	constexpr auto separator = options<detail::SeparatorOption::NONE>{};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		class Separator final : public Element
		{
		public:
			explicit Separator() noexcept = default;

			auto calculate_requirement([[maybe_unused]] Surface& surface) noexcept -> void override
			{
				const auto line_width = Style::instance().line_width;
				requirement_.min_width = line_width;
				requirement_.min_height = line_width;
			}

			auto render(Surface& surface) noexcept -> void override
			{
				if (const auto line_width = Style::instance().line_width;
					rect_.width() == line_width)
				{
					const auto from = rect_.left_top();
					auto to = rect_.right_bottom();
					to.x -= line_width;

					surface.draw_list().line(from, to, Style::instance().separator_color, line_width);
				}
				else
				{
					const auto from = rect_.left_top();
					auto to = rect_.right_bottom();
					to.y -= line_width;

					surface.draw_list().line(from, to, Style::instance().separator_color, line_width);
				}
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<separator_option_t auto... Os>
		struct element_maker<Os...>
		{
			[[nodiscard]] auto operator()() const noexcept -> element_type
			{
				return make_element<Separator>();
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
