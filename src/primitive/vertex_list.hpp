// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:vertex_list;

import std;
import :vertex;
import :rect;
import gal.prometheus.functional;

#else

#include <type_traits>
#include <vector>
#include <array>
#include <numbers>

#include <prometheus/macro.hpp>
#include <primitive/vertex.hpp>
#include <primitive/rect.hpp>
#include <functional/functional.hpp>
#endif

namespace gal::prometheus::primitive
{
	template<typename PointValueType, typename ColorValueType, template<typename> typename Container = std::vector>
		requires std::is_arithmetic_v<PointValueType> and std::is_arithmetic_v<ColorValueType>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_vertex_list final
	{
		using vertex_type = basic_vertex<PointValueType, ColorValueType>;
		using vertexes_type = Container<vertex_type>;

		using point_value_type = typename vertex_type::point_value_type;

		using point_type = typename vertex_type::point_type;
		using color_type = typename vertex_type::color_type;

		using rect_type = basic_rect<point_value_type>;
		using circle_type = basic_circle<point_value_type>;

		vertexes_type vertexes;

	private:
		constexpr auto emplace_back(const point_type& position, const color_type& color) noexcept -> void //
		{
			vertexes.emplace_back(position, color);
		}

		constexpr auto emplace_back(const point_type& position, const point_type& uv, const color_type& color) noexcept -> void //
		{
			vertexes.emplace_back(position, uv, color);
		}

		constexpr auto push_back(const vertex_type& vertex) noexcept -> void //
		{
			vertexes.push_back(vertex);
		}

	public:
		constexpr auto triangle(const point_type& a, const point_type& b, const point_type& c, const color_type color) noexcept -> void
		{
			if (color.alpha == 0) { return; }

			emplace_back(a, color);
			emplace_back(b, color);
			emplace_back(c, color);
		}

		constexpr auto line(const point_type& from, const point_type& to, const color_type& color) noexcept -> void
		{
			if (color.alpha == 0) { return; }

			const auto half_normal = (to - from) * (.5f / to.distance(from));
			const auto half_perpendicular_0 = point_type{half_normal.y, -half_normal.x};
			const auto half_perpendicular_1 = point_type{-half_normal.y, half_normal.x};

			// two triangles
			triangle(from + half_perpendicular_0, to + half_perpendicular_0, from + half_perpendicular_1, color);
			triangle(to + half_perpendicular_0, to + half_perpendicular_1, from + half_perpendicular_1, color);
		}

		enum class ArcType
		{
			LINE,
			TRIANGLE,
		};

		enum class ArcQuadrant
		{
			// [0~3)
			Q1 = 0x0001,
			// [3~6)
			Q2 = 0x0010,
			// [6~9)
			Q3 = 0x0100,
			// [9~12)
			Q4 = 0x1000,

			TOP = Q1 | Q2,
			BOTTOM = Q3 | Q4,
			LEFT = Q2 | Q3,
			RIGHT = Q1 | Q4,
		};

		template<ArcType Type, ArcQuadrant Quadrant>
		constexpr auto arc(const circle_type& circle, const color_type& color) noexcept -> void
		{
			if (color.alpha == 0) { return; }

			constexpr std::size_t circle_vertex_count = 12;
			constexpr auto make_circle_vertex_point = [](const std::size_t i) noexcept -> point_type
			{
				const auto a =
						static_cast<float>(i) /
						static_cast<float>(circle_vertex_count) *
						2 *
						std::numbers::pi_v<float> +
						std::numbers::pi_v<float>;
				return {functional::cos(a), functional::sin(a)};
			};

			constexpr static auto circle_vertex = []
			{
				std::array<point_type, circle_vertex_count> result{};
				for (decltype(result.size()) i = 0; i < circle_vertex_count; ++i) { result[i] = make_circle_vertex_point(i); }
				return result;
			}();

			constexpr auto min_max = []
			{
				if constexpr (Quadrant == ArcQuadrant::Q1) { return std::make_pair(0, 3); }
				else if constexpr (Quadrant == ArcQuadrant::Q2) { return std::make_pair(3, 6); }
				else if constexpr (Quadrant == ArcQuadrant::Q3) { return std::make_pair(6, 9); }
				else if constexpr (Quadrant == ArcQuadrant::Q4) { return std::make_pair(9, 12); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}();

			for (auto i = min_max.first; i < min_max.second; ++i)
			{
				const auto p1 = circle.center + circle_vertex[i] * circle.radius;
				const auto p2 = circle.center + circle_vertex[i + 1] * circle.radius;

				if constexpr (Type == ArcType::LINE) //
				{
					line(p1, p2, color);
				}
				else if constexpr (Type == ArcType::TRIANGLE) //
				{
					triangle(p1, p2, circle.center, color);
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
		}

		template<ArcType Type, ArcQuadrant Quadrant>
		constexpr auto arc(const point_type& center, const point_value_type radius, const color_type& color) noexcept -> void //
		{
			return arc<Type, Quadrant>({center, radius}, color);
		}

		constexpr auto rect(const rect_type& rect, const color_type& color) noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());

			if (color.alpha == 0) { return; }

			line(rect.left_top(), rect.right_top(), color);
			line(rect.right_top(), rect.right_bottom(), color);
			line(rect.right_bottom(), rect.left_bottom(), color);
			line(rect.left_bottom(), rect.left_top(), color);
		}

		constexpr auto rect(const point_type& left_top, const point_type& right_bottom, const color_type& color) noexcept -> void //
		{
			return rect({left_top, right_bottom}, color);
		}

		constexpr auto rect_filled(const rect_type& rect, const color_type& color) noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());

			if (color.alpha == 0) { return; }

			triangle(rect.left_top(), rect.right_top(), rect.right_bottom(), color);
			triangle(rect.left_top(), rect.right_bottom(), rect.left_bottom(), color);
		}

		constexpr auto rect_filled(const point_type& left_top, const point_type& right_bottom, const color_type& color) noexcept -> void //
		{
			return rect_filled({left_top, right_bottom}, color);
		}

		template<ArcQuadrant Quadrant = ArcQuadrant::Q1 | ArcQuadrant::Q2 | ArcQuadrant::Q3 | ArcQuadrant::Q4>
		constexpr auto rect_rounded(
				const rect_type& rect,
				const color_type& color,
				const float rounding
				) noexcept -> void //
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());

			if (color.alpha == 0) { return; }

			constexpr auto quadrant = std::to_underlying(Quadrant);
			constexpr auto quadrant_top = quadrant & std::to_underlying(ArcQuadrant::TOP) == std::to_underlying(ArcQuadrant::TOP);
			constexpr auto quadrant_bottom = quadrant & std::to_underlying(ArcQuadrant::BOTTOM) == std::to_underlying(ArcQuadrant::BOTTOM);
			constexpr auto quadrant_left = quadrant & std::to_underlying(ArcQuadrant::LEFT) == std::to_underlying(ArcQuadrant::LEFT);
			constexpr auto quadrant_right = quadrant & std::to_underlying(ArcQuadrant::RIGHT) == std::to_underlying(ArcQuadrant::RIGHT);
			constexpr auto quadrant_q1 = quadrant & std::to_underlying(ArcQuadrant::Q1);
			constexpr auto quadrant_q2 = quadrant & std::to_underlying(ArcQuadrant::Q2);
			constexpr auto quadrant_q3 = quadrant & std::to_underlying(ArcQuadrant::Q3);
			constexpr auto quadrant_q4 = quadrant & std::to_underlying(ArcQuadrant::Q4);

			const auto radius = functional::functor::max(
					rounding,
					static_cast<float>(rect.width()) * ((quadrant_top or quadrant_bottom) ? .5f : 1.f),
					static_cast<float>(rect.height()) * ((quadrant_left or quadrant_right) ? .5f : 1.f)
					);

			if (radius == .0f)
			[[unlikely]]
			{
				return rect(rect, color);
			}

			const auto [left, top, right, bottom] = rect;

			line(
					{left + static_cast<point_value_type>(quadrant_q2 ? radius : .0f), top},
					{right - static_cast<point_value_type>(quadrant_q1 ? radius : .0f), top},
					color
					);
			line(
					{right, top + static_cast<point_value_type>(quadrant_q1 ? radius : .0f)},
					{right, bottom - static_cast<point_value_type>(quadrant_q4 ? radius : .0f)},
					color
					);
			line(
					{right - static_cast<point_value_type>(quadrant_q4 ? radius : .0f), bottom},
					{left + static_cast<point_value_type>(quadrant_q3 ? radius : .0f), bottom},
					color
					);
			line(
					{left, bottom - static_cast<point_value_type>(quadrant_q3 ? radius : .0f)},
					{left, top + static_cast<point_value_type>(quadrant_q2 ? radius : .0f)},
					color
					);

			if (quadrant_q2)
			{
				const point_type center{left + radius, top + radius};
				arc<ArcType::LINE, ArcQuadrant::Q2>({center, radius}, color);
			}
			if (quadrant_q1)
			{
				const point_type center{right - radius, top + radius};
				arc<ArcType::LINE, ArcQuadrant::Q1>({center, radius}, color);
			}
			if (quadrant_q4)
			{
				const point_type center{right - radius, bottom - radius};
				arc<ArcType::LINE, ArcQuadrant::Q4>({center, radius}, color);
			}
			if (quadrant_q3)
			{
				const point_type center{left + radius, bottom - radius};
				arc<ArcType::LINE, ArcQuadrant::Q3>({center, radius}, color);
			}
		}

		template<ArcQuadrant Quadrant = ArcQuadrant::Q1 | ArcQuadrant::Q2 | ArcQuadrant::Q3 | ArcQuadrant::Q4>
		constexpr auto rect_rounded(
				const point_type& left_top,
				const point_type& right_bottom,
				const color_type& color,
				const float rounding
				) noexcept -> void //
		{
			return rect_rounded<Quadrant>({left_top, right_bottom}, color, rounding);
		}

		template<ArcQuadrant Quadrant = ArcQuadrant::Q1 | ArcQuadrant::Q2 | ArcQuadrant::Q3 | ArcQuadrant::Q4>
		constexpr auto rect_rounded_filled(
				const rect_type& rect,
				const color_type& color,
				const float rounding
				) noexcept -> void //
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not rect.empty() and rect.valid());

			if (color.alpha == 0) { return; }

			constexpr auto quadrant = std::to_underlying(Quadrant);
			constexpr auto quadrant_top = quadrant & std::to_underlying(ArcQuadrant::TOP) == std::to_underlying(ArcQuadrant::TOP);
			constexpr auto quadrant_bottom = quadrant & std::to_underlying(ArcQuadrant::BOTTOM) == std::to_underlying(ArcQuadrant::BOTTOM);
			constexpr auto quadrant_left = quadrant & std::to_underlying(ArcQuadrant::LEFT) == std::to_underlying(ArcQuadrant::LEFT);
			constexpr auto quadrant_right = quadrant & std::to_underlying(ArcQuadrant::RIGHT) == std::to_underlying(ArcQuadrant::RIGHT);
			constexpr auto quadrant_q1 = quadrant & std::to_underlying(ArcQuadrant::Q1);
			constexpr auto quadrant_q2 = quadrant & std::to_underlying(ArcQuadrant::Q2);
			constexpr auto quadrant_q3 = quadrant & std::to_underlying(ArcQuadrant::Q3);
			constexpr auto quadrant_q4 = quadrant & std::to_underlying(ArcQuadrant::Q4);

			const auto radius = functional::functor::max(
					rounding,
					static_cast<float>(rect.width()) * ((quadrant_top or quadrant_bottom) ? .5f : 1.f),
					static_cast<float>(rect.height()) * ((quadrant_left or quadrant_right) ? .5f : 1.f)
					);

			if (radius == .0f)
			[[unlikely]]
			{
				return rect_filled(rect, color);
			}

			const auto [left, top, right, bottom] = rect;

			triangle({left + radius, top}, {right - radius, top}, {right - radius, bottom}, color);
			triangle({left + radius, top}, {right - radius, bottom}, {left + radius, bottom}, color);

			{
				const auto t = top + static_cast<point_value_type>(quadrant_q2 ? radius : .0f);
				const auto b = bottom - static_cast<point_value_type>(quadrant_q3 ? radius : .0f);
				triangle({left, t}, {left + radius, t}, {left + radius, b}, color);
				triangle({left, t}, {left + radius, b}, {left, b}, color);
			}
			{
				const auto t = top + static_cast<point_value_type>(quadrant_q1 ? radius : .0f);
				const auto b = bottom - static_cast<point_value_type>(quadrant_q4 ? radius : .0f);
				triangle({right - radius, t}, {right, t}, {right, b}, color);
				triangle({right - radius, t}, {right, b}, {right - radius, b}, color);
			}

			if (quadrant_q2)
			{
				const point_type center{left + radius, top + radius};
				arc<ArcType::TRIANGLE, ArcQuadrant::Q2>({center, radius}, color);
			}
			if (quadrant_q1)
			{
				const point_type center{right - radius, top + radius};
				arc<ArcType::TRIANGLE, ArcQuadrant::Q1>({center, radius}, color);
			}
			if (quadrant_q4)
			{
				const point_type center{right - radius, bottom - radius};
				arc<ArcType::TRIANGLE, ArcQuadrant::Q4>({center, radius}, color);
			}
			if (quadrant_q3)
			{
				const point_type center{left + radius, bottom - radius};
				arc<ArcType::TRIANGLE, ArcQuadrant::Q3>({center, radius}, color);
			}
		}

		template<ArcQuadrant Quadrant = ArcQuadrant::Q1 | ArcQuadrant::Q2 | ArcQuadrant::Q3 | ArcQuadrant::Q4>
		constexpr auto rect_rounded_filled(
				const point_type& left_top,
				const point_type& right_bottom,
				const color_type& color,
				const float rounding
				) noexcept -> void //
		{
			return rect_rounded_filled<Quadrant>({left_top, right_bottom}, color, rounding);
		}

		constexpr auto circle(const circle_type& circle, const color_type& color, const int segments = 12) noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(not circle.empty());
			GAL_PROMETHEUS_DEBUG_AXIOM(segments > 0);

			if (color.alpha == 0)
			{
				return;
			}

			// todo
		}
	};
} // namespace gal::prometheus::primitive
