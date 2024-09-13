// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.fixed;

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
		enum class BoundaryTypeOption
		{
			NONE = 0x0000,

			WIDTH = 0x0001,
			HEIGHT = 0x0010,
		};

		enum class BoundaryComparatorOption
		{
			NONE = 0x0000,

			LESS_THAN = 0x0001,
			EQUAL = 0x0010,
			GREATER_THAN = 0x0100,
		};

		template<typename T>
		concept boundary_type_option_t = std::is_same_v<T, BoundaryTypeOption>;

		template<typename T>
		concept boundary_comparator_option_t = std::is_same_v<T, BoundaryComparatorOption>;

		template<typename T>
		concept boundary_type_or_comparator_option_t = boundary_type_option_t<T> or boundary_comparator_option_t<T>;

		struct boundary_options
		{
			options<BoundaryTypeOption::WIDTH> width{};
			options<BoundaryTypeOption::HEIGHT> height{};
			options<BoundaryTypeOption::WIDTH, BoundaryTypeOption::HEIGHT> all{};

			options<BoundaryComparatorOption::LESS_THAN> less_than{};
			options<BoundaryComparatorOption::EQUAL> equal{};
			options<BoundaryComparatorOption::GREATER_THAN> greater_than{};
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	namespace element
	{
		constexpr auto boundary = detail::boundary_options{};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	namespace detail
	{
		template<BoundaryComparatorOption Width, BoundaryComparatorOption Height>
		struct boundary_value;

		template<BoundaryComparatorOption Width, BoundaryComparatorOption Height>
			requires (
				(Width != BoundaryComparatorOption::NONE) and
				(Height == BoundaryComparatorOption::NONE)
			)
		struct boundary_value<Width, Height>
		{
			float width;
		};

		template<BoundaryComparatorOption Width, BoundaryComparatorOption Height>
			requires (
				(Width == BoundaryComparatorOption::NONE) and
				(Height != BoundaryComparatorOption::NONE)
			)
		struct boundary_value<Width, Height>
		{
			float height;
		};

		template<BoundaryComparatorOption Width, BoundaryComparatorOption Height>
			requires (
				(Width != BoundaryComparatorOption::NONE) and
				(Height != BoundaryComparatorOption::NONE)
			)
		struct boundary_value<Width, Height>
		{
			float width;
			float height;
		};

		// placeholder
		template<>
		struct boundary_value<BoundaryComparatorOption::NONE, BoundaryComparatorOption::NONE> {};

		template<
			BoundaryComparatorOption WidthComparator,
			BoundaryComparatorOption HeightComparator
		>
		class Boundary final : public Element
		{
		public:
			using value_type = boundary_value<WidthComparator, HeightComparator>;

		private:
			value_type value_;

		public:
			Boundary(const value_type value, element_type element) noexcept
				: Element{elements_type{std::move(element)}},
				  value_{value} {}

			Boundary(const Boundary& other) noexcept = default;
			Boundary& operator=(const Boundary& other) noexcept = default;
			Boundary(Boundary&& other) noexcept = default;
			Boundary& operator=(Boundary&& other) noexcept = default;
			~Boundary() noexcept override = default;

			auto calculate_requirement(Surface& surface) noexcept -> void override
			{
				Element::calculate_requirement(surface);

				requirement_ = children_[0]->requirement();
				if constexpr (WidthComparator != BoundaryComparatorOption::NONE)
				{
					requirement_.flex_grow_width = 0;
					requirement_.flex_shrink_width = 0;

					if constexpr (WidthComparator == BoundaryComparatorOption::LESS_THAN)
					{
						requirement_.min_width = std::ranges::min(requirement_.min_width, value_.width);
					}
					else if constexpr (WidthComparator == BoundaryComparatorOption::EQUAL)
					{
						requirement_.min_width = value_.width;
					}
					else if constexpr (WidthComparator == BoundaryComparatorOption::GREATER_THAN)
					{
						requirement_.min_width = std::ranges::max(requirement_.min_width, value_.width);
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}

				if constexpr (HeightComparator != BoundaryComparatorOption::NONE)
				{
					requirement_.flex_grow_height = 0;
					requirement_.flex_shrink_height = 0;

					if constexpr (HeightComparator == BoundaryComparatorOption::LESS_THAN)
					{
						requirement_.min_height = std::ranges::min(requirement_.min_height, value_.height);
					}
					else if constexpr (HeightComparator == BoundaryComparatorOption::EQUAL)
					{
						requirement_.min_height = value_.height;
					}
					else if constexpr (HeightComparator == BoundaryComparatorOption::GREATER_THAN)
					{
						requirement_.min_height = std::ranges::max(requirement_.min_height, value_.height);
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}
			}

			auto set_rect(const rect_type& rect) noexcept -> void override
			{
				Element::set_rect(rect);

				auto box = rect;

				if constexpr (
					WidthComparator == BoundaryComparatorOption::LESS_THAN or
					WidthComparator == BoundaryComparatorOption::EQUAL
				)
				{
					box.extent.width = std::ranges::min(box.extent.width, value_.width);
				}

				if constexpr (
					HeightComparator == BoundaryComparatorOption::LESS_THAN or
					HeightComparator == BoundaryComparatorOption::EQUAL
				)
				{
					box.extent.height = std::ranges::min(box.extent.height, value_.height);
				}

				children_[0]->set_rect(box);
			}
		};

		template<
			BoundaryComparatorOption Width = BoundaryComparatorOption::NONE,
			BoundaryComparatorOption Height = BoundaryComparatorOption::NONE,
			BoundaryTypeOption Last = BoundaryTypeOption::NONE
		>
		struct boundary_maker
		{
			using value_type = boundary_value<Width, Height>;

			value_type value;

			[[nodiscard]] constexpr auto operator()(element_type element) const noexcept -> element_type //
				requires(
					(Width != BoundaryComparatorOption::NONE or Height != BoundaryComparatorOption::NONE) and
					Last == BoundaryTypeOption::NONE
				)
			{
				return make_element<Boundary<Width, Height>>(value, std::move(element));
			}

			template<BoundaryTypeOption T>
				requires (Last == BoundaryTypeOption::NONE)
			[[nodiscard]] constexpr auto operator()(const options<T>) const noexcept -> auto
			{
				return boundary_maker<Width, Height, T>{.value = value};
			}

			template<BoundaryComparatorOption C>
				requires (Last != BoundaryTypeOption::NONE and std::to_underlying(Last) != element::boundary.all)
			[[nodiscard]] constexpr auto operator()(const options<C>, const float v) const noexcept -> auto
			{
				if constexpr (Last == BoundaryTypeOption::WIDTH)
				{
					if constexpr (Height == BoundaryComparatorOption::NONE)
					{
						return boundary_maker<C, Height>{.value = {.width = v}};
					}
					else
					{
						return boundary_maker<C, Height>{.value = {.width = v, .height = value.height}};
					}
				}
				else if constexpr (Last == BoundaryTypeOption::HEIGHT)
				{
					if constexpr (Width == BoundaryComparatorOption::NONE)
					{
						return boundary_maker<Width, C>{.value = {.height = v}};
					}
					else
					{
						return boundary_maker<Width, C>{.value = {.width = value.width, .height = v}};
					}
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			template<BoundaryComparatorOption C>
				requires (std::to_underlying(Last) != element::boundary.all)
			[[nodiscard]] constexpr auto operator()(const options<C>, const float width, const float height) const noexcept -> auto
			{
				return boundary_maker<C, C>{.value = {.width = width, .height = height}};
			}
		};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

		template<boundary_type_or_comparator_option_t auto... Os>
		struct element_maker<Os...> : boundary_maker<> {};

		GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
	}
}
