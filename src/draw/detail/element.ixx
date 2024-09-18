// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:element.element;

import std;

import gal.prometheus.primitive;

import :options;

#else
#pragma once

#include <type_traits>
#include <memory>
#include <vector>
#include <span>

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>

#include <draw/detail/options.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw)
{
	class Style;
	class Surface;

	namespace detail
	{
		class Element;
	}

	using element_type = std::shared_ptr<detail::Element>;

	using elements_type = std::vector<element_type>;
	using elements_view_type = std::span<element_type>;

	template<typename T>
	concept derived_element_t = std::is_base_of_v<detail::Element, typename T::element_type>;

	template<typename Range>
	concept derived_elements_t = std::ranges::range<Range> and derived_element_t<typename Range::value_type>;

	template<std::derived_from<detail::Element> T, typename... Args>
	[[nodiscard]] auto make_element(Args&&... args) noexcept -> element_type
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template<std::derived_from<detail::Element> T>
	[[nodiscard]] constexpr auto cast_element(const element_type& element) noexcept -> std::shared_ptr<T>
	{
		return std::dynamic_pointer_cast<T>(element);
	}

	template<std::derived_from<detail::Element> T>
	[[nodiscard]] constexpr auto cast_element_unchecked(const element_type& element) noexcept -> std::shared_ptr<T>
	{
		return std::static_pointer_cast<T>(element);
	}

	namespace detail
	{
		struct requirement_type
		{
			float min_width{0};
			float min_height{0};

			float flex_grow_width{0};
			float flex_grow_height{0};
			float flex_shrink_width{0};
			float flex_shrink_height{0};

			constexpr auto reset() noexcept -> void
			{
				min_width = 0;
				min_height = 0;

				flex_grow_width = 0;
				flex_grow_height = 0;
				flex_shrink_width = 0;
				flex_shrink_height = 0;
			}
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

			explicit Element(derived_element_t auto... child) noexcept
				: children_{std::move(child)...} {}

			virtual ~Element() noexcept = 0;

			[[nodiscard]] auto requirement() const noexcept -> const requirement_type&
			{
				return requirement_;
			}

			virtual auto calculate_requirement(const Style& style, Surface& surface) noexcept -> void
			{
				std::ranges::for_each(
					children_,
					[&style, &surface](auto& child) noexcept -> void
					{
						child->calculate_requirement(style, surface);
					}
				);
			}

			[[nodiscard]] auto rect() const noexcept -> const rect_type&
			{
				return rect_;
			}

			virtual auto set_rect(const Style& style, const rect_type& rect) noexcept -> void
			{
				(void)style;
				rect_ = rect;
			}

			virtual auto render(const Style& style, Surface& surface) noexcept -> void
			{
				std::ranges::for_each(
					children_,
					[&style, &surface](auto& child) noexcept -> void
					{
						child->render(style, surface);
					}
				);
			}
		};
	}
}

// element_type -> std::shared_ptr
GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
{
	// element | decorator
	template<gal::prometheus::draw::derived_element_t Element, typename Decorator>
	[[nodiscard]] constexpr auto operator|(Element&& element, Decorator&& decorator) noexcept -> gal::prometheus::draw::element_type // NOLINT(cert-dcl58-cpp)
		requires (
			gal::prometheus::draw::derived_element_t<std::invoke_result_t<Decorator, Element>>
		)
	{
		return std::invoke(std::forward<Decorator>(decorator), std::forward<Element>(element));
	}

	// element | option => element | decorator
	template<gal::prometheus::draw::derived_element_t Element, gal::prometheus::draw::detail::options_t Option>
	[[nodiscard]] constexpr auto operator|(Element&& element, const Option option) noexcept -> gal::prometheus::draw::element_type // NOLINT(cert-dcl58-cpp)
		requires(
			gal::prometheus::draw::derived_element_t<std::invoke_result_t<std::decay_t<decltype(gal::prometheus::draw::make(option))>, Element>>
		)
	{
		return std::forward<Element>(element) | gal::prometheus::draw::make(option);
	}
}
