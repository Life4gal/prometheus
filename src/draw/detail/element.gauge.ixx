// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.gauge;

import std;

import gal.prometheus.primitive;
GAL_PROMETHEUS_ERROR_IMPORT_DEBUG_MODULE

import :style;
import :element.element;

#else
#pragma once

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <draw/style.ixx>
#include <draw/detail/element.ixx>

#endif

namespace gal::prometheus::draw
{
	namespace detail
	{
		enum class GaugeDirectionOption
		{
			NONE,

			UP,
			DOWN,
			LEFT,
			RIGHT,
		};

		template<typename T>
		concept gauge_direction_option_t = std::is_same_v<T, GaugeDirectionOption>;

		struct gauge_options
		{
			options<GaugeDirectionOption::UP> up{};
			options<GaugeDirectionOption::DOWN> down{};
			options<GaugeDirectionOption::LEFT> left{};
			options<GaugeDirectionOption::RIGHT> right{};
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace element
	{
		constexpr auto gauge = detail::gauge_options{};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		template<GaugeDirectionOption Option>
		class Gauge final : public Element
		{
		public:
			using value_type = float;

			constexpr static value_type min = 0;
			constexpr static value_type max = 1;

			constexpr static auto option = Option;

		private:
			value_type value_;

			template<bool Horizontal, bool Invert>
			auto render(const Style& style, Surface& surface) noexcept -> void
			{
				const auto& [point, extent] = rect_;

				if constexpr (const auto current_value = Invert ? max - value_ : value_;
					Horizontal)
				{
					if (current_value == 0 or current_value == max) // NOLINT(clang-diagnostic-float-equal)
					{
						const rect_type rect{
								point.x,
								point.y,
								point.x + extent.width,
								point.y + style.gauge_size
						};
						if (current_value == 0) // NOLINT(clang-diagnostic-float-equal)
						{
							surface.draw_list().rect(rect, style.border_color, style.border_rounding, DrawFlag::ROUND_CORNER_ALL, style.line_width);
						}
						else
						{
							surface.draw_list().rect_filled(rect, style.gauge_color, style.border_rounding, DrawFlag::ROUND_CORNER_ALL);
						}
					}
					else
					{
						const auto width = current_value * rect_.width();
						const auto height = style.gauge_size;

						const rect_type rect1
						{
								point.x,
								point.y,
								point.x + width,
								point.y + height
						};
						const rect_type rect2
						{
								point.x + width,
								point.y,
								point.x + extent.width,
								point.y + height
						};

						if constexpr (Invert)
						{
							surface.draw_list().rect(rect1, style.border_color, style.border_rounding, DrawFlag::ROUND_CORNER_LEFT, style.line_width);
							surface.draw_list().rect_filled(rect2, style.gauge_color, style.border_rounding, DrawFlag::ROUND_CORNER_RIGHT);
						}
						else
						{
							surface.draw_list().rect_filled(rect1, style.gauge_color, style.border_rounding, DrawFlag::ROUND_CORNER_LEFT);
							surface.draw_list().rect(rect2, style.border_color, style.border_rounding, DrawFlag::ROUND_CORNER_RIGHT, style.line_width);
						}
					}
				}
				else
				{
					if (current_value == 0 or current_value == max) // NOLINT(clang-diagnostic-float-equal)
					{
						const rect_type rect{
								point.x,
								point.y,
								point.x + style.gauge_size,
								point.y + extent.height
						};
						if (current_value == 0) // NOLINT(clang-diagnostic-float-equal)
						{
							surface.draw_list().rect(rect, style.border_color, style.border_rounding, DrawFlag::ROUND_CORNER_ALL, style.line_width);
						}
						else
						{
							surface.draw_list().rect_filled(rect, style.gauge_color, style.border_rounding, DrawFlag::ROUND_CORNER_ALL);
						}
					}
					else
					{
						const auto width = style.gauge_size;
						const auto height = current_value * rect_.height();

						const rect_type rect1
						{
								point.x,
								point.y,
								point.x + width,
								point.y + height
						};
						const rect_type rect2
						{
								point.x,
								point.y + height,
								point.x + width,
								point.y + extent.height
						};

						if constexpr (Invert)
						{
							surface.draw_list().rect(rect1, style.border_color, style.border_rounding, DrawFlag::ROUND_CORNER_TOP, style.line_width);
							surface.draw_list().rect_filled(rect2, style.gauge_color, style.border_rounding, DrawFlag::ROUND_CORNER_BOTTOM);
						}
						else
						{
							surface.draw_list().rect_filled(rect1, style.gauge_color, style.border_rounding, DrawFlag::ROUND_CORNER_TOP);
							surface.draw_list().rect(rect2, style.border_color, style.border_rounding, DrawFlag::ROUND_CORNER_BOTTOM, style.line_width);
						}
					}
				}
			}

		public:
			explicit Gauge(const value_type value) noexcept
				: Element{},
				  value_{std::ranges::clamp(value, min, max)} {}

			Gauge(const Gauge& other) noexcept = default;
			Gauge& operator=(const Gauge& other) noexcept = default;
			Gauge(Gauge&& other) noexcept = default;
			Gauge& operator=(Gauge&& other) noexcept = default;
			~Gauge() noexcept override = default;

			auto calculate_requirement(const Style& style, [[maybe_unused]] Surface& surface) noexcept -> void override
			{
				if constexpr (option == GaugeDirectionOption::UP or option == GaugeDirectionOption::DOWN)
				{
					requirement_.min_width = style.gauge_size;
					requirement_.flex_grow_width = 0;
					requirement_.flex_shrink_width = 0;

					requirement_.min_height = 1;
					requirement_.flex_grow_height = style.flex_y;
					requirement_.flex_shrink_height = style.flex_y;
				}
				else if constexpr (option == GaugeDirectionOption::LEFT or option == GaugeDirectionOption::RIGHT)
				{
					requirement_.min_width = 1;
					requirement_.flex_grow_width = style.flex_x;
					requirement_.flex_shrink_width = style.flex_x;

					requirement_.min_height = style.gauge_size;
					requirement_.flex_grow_height = 0;
					requirement_.flex_shrink_height = 0;
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			auto render(const Style& style, Surface& surface) noexcept -> void override
			{
				render<
					option == GaugeDirectionOption::LEFT or option == GaugeDirectionOption::RIGHT,
					option == GaugeDirectionOption::LEFT or option == GaugeDirectionOption::DOWN
				>(style, surface);
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<gauge_direction_option_t auto... Os>
		struct element_maker<Os...>
		{
			template<GaugeDirectionOption Option>
			[[nodiscard]] constexpr auto operator()(options<Option>) const noexcept -> auto
			{
				return [](const typename Gauge<Option>::value_type value) noexcept -> element_type
				{
					return make_element<Gauge<Option>>(value);
				};
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
