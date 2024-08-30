// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element;

import std;
import gal.prometheus.primitive;

#else
#pragma once

#include <memory>
#include <vector>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw)
{
	class Surface;

	namespace impl
	{
		class Element;
	}

	using element_type = std::shared_ptr<impl::Element>;
	using elements_type = std::vector<element_type>;

	template<std::derived_from<impl::Element> T, typename... Args>
	[[nodiscard]] constexpr auto make_element(Args&&... args) noexcept -> element_type
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	namespace impl
	{
		template<typename T>
		concept derived_element_t = std::derived_from<typename T::element_type, Element>;

		struct requirement_type
		{
			float min_width{0};
			float min_height{0};

			float flex_grow_width{0};
			float flex_grow_height{0};
			float flex_shrink_width{0};
			float flex_shrink_height{0};
		};

		class Element
		{
		public:
			using rect_type = primitive::basic_rect_2d<float>;

		protected:
			elements_type children_;

			requirement_type requirement_;
			rect_type rect_;

		public:
			Element(const Element&) noexcept = default;
			auto operator=(const Element&) noexcept -> Element& = default;
			Element(Element&&) noexcept = default;
			auto operator=(Element&&) noexcept -> Element& = default;

			explicit Element() noexcept = default;

			explicit Element(elements_type children) noexcept
				: children_{std::move(children)} {}

			virtual ~Element() noexcept = 0;

			[[nodiscard]] auto requirement() const noexcept -> const requirement_type&
			{
				return requirement_;
			}

			virtual auto calculate_requirement(Surface& surface) noexcept -> void
			{
				std::ranges::for_each(
					children_,
					[&surface](auto& node) noexcept -> void { node->calculate_requirement(surface); }
				);
			}

			[[nodiscard]] auto rect() const noexcept -> const rect_type&
			{
				return rect_;
			}

			virtual auto set_rect(const rect_type& rect) noexcept -> void
			{
				rect_ = rect;
			}

			virtual auto render(Surface& surface) noexcept -> void
			{
				std::ranges::for_each(
					children_,
					[&surface](auto& element) noexcept -> void { element->render(surface); }
				);
			}
		};
	}

	inline namespace decorator
	{
		// element => element
		template<typename Decorator>
		concept decorator_t = requires(element_type element, Decorator&& decorator)
		{
			{
				std::invoke(std::forward<Decorator>(decorator), element)
			} -> impl::derived_element_t;
		};

		template<decorator_t Decorator>
		[[nodiscard]] constexpr auto operator|(element_type element, Decorator&& decorator) noexcept -> element_type
		{
			return std::invoke(std::forward<Decorator>(decorator), element);
		}
	}

	namespace element
	{
		// ===============================
		// TEXT

		[[nodiscard]] auto text(std::u32string text) noexcept -> element_type;
		[[nodiscard]] auto text(std::string text) noexcept -> element_type;

		// ===============================
		// BORDER

		[[nodiscard]] auto border(element_type element) noexcept -> element_type;

		// ===============================
		// LAYOUT

		[[nodiscard]] auto horizontal_box(elements_type elements) noexcept -> element_type;

		template<impl::derived_element_t... Element>
		[[nodiscard]] auto horizontal_box(Element&&... elements) noexcept -> element_type
		{
			elements_type e{};
			e.reserve(sizeof...(elements));

			(e.emplace_back(std::forward<Element>(elements)), ...);

			return horizontal_box(std::move(e));
		}
	}
}
