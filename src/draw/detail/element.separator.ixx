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
	namespace detail::element
	{
		enum class SeparatorOption
		{
			NONE
		};

		template<typename T>
		concept separator_option_t = std::is_same_v<T, SeparatorOption>;
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace element
	{
		constexpr auto separator = detail::options<detail::element::SeparatorOption::NONE>{};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		namespace element
		{
			class Separator final : public Element
			{
			public:
				explicit Separator() noexcept = default;

				auto calculate_requirement(const Style& style, [[maybe_unused]] Surface& surface) noexcept -> void override
				{
					const auto line_width = style.line_width;
					requirement_.min_width = line_width;
					requirement_.min_height = line_width;
				}

				auto render(const Style& style, Surface& surface) noexcept -> void override
				{
					if (const auto line_width = style.line_width;
						rect_.width() == line_width) // NOLINT(clang-diagnostic-float-equal)
					{
						const auto from = rect_.left_top();
						auto to = rect_.right_bottom();
						to.x -= line_width;

						surface.draw_list().line(from, to, style.separator_color, line_width);
					}
					else
					{
						const auto from = rect_.left_top();
						auto to = rect_.right_bottom();
						to.y -= line_width;

						surface.draw_list().line(from, to, style.separator_color, line_width);
					}
				}
			};
		}

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<element::separator_option_t auto... Os>
		struct maker<Os...>
		{
			[[nodiscard]] auto operator()() const noexcept -> element_type
			{
				return make_element<element::Separator>();
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
