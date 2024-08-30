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
import gal.prometheus.functional;

#else
#pragma once

#include <memory>
#include <vector>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>
#include <functional/functional.ixx>

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

	template<std::derived_from<impl::Element> T>
	[[nodiscard]] constexpr auto cast_element(const element_type element) noexcept -> std::shared_ptr<T>
	{
		return std::static_pointer_cast<T>(element);
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

	namespace element
	{
		template<typename Element, typename Decorator>
			requires (impl::derived_element_t<Element> and impl::derived_element_t<std::invoke_result_t<Decorator, Element>>)
		[[nodiscard]] constexpr auto operator|(Element&& element, Decorator&& decorator) noexcept -> decltype(auto)
		{
			return std::invoke(std::forward<Decorator>(decorator), std::forward<Element>(element));
		}

		template<typename Decorator>
			requires impl::derived_element_t<std::invoke_result_t<Decorator, elements_type::value_type>>
		[[nodiscard]] constexpr auto operator|(elements_type& elements, Decorator&& decorator) noexcept -> decltype(auto)
		{
			return std::invoke(std::forward<Decorator>(decorator), elements);
		}

		template<typename Decorator>
			requires impl::derived_element_t<std::invoke_result_t<Decorator, elements_type::value_type>>
		[[nodiscard]] constexpr auto operator|(elements_type&& elements, Decorator&& decorator) noexcept -> decltype(auto)
		{
			return std::invoke(std::forward<Decorator>(decorator), std::move(elements));
		}

		// ===============================
		// TEXT

		namespace impl
		{
			[[nodiscard]] auto text(std::u32string text) noexcept -> element_type;
			[[nodiscard]] auto text(std::string text) noexcept -> element_type;
		}

		constexpr auto text = functional::overloaded{
				[](std::u32string string) noexcept -> element_type
				{
					return impl::text(std::move(string));
				},
				[](std::string string) noexcept -> element_type
				{
					return impl::text(std::move(string));
				},
		};

		// ===============================
		// BORDER

		namespace impl
		{
			[[nodiscard]] auto border(elements_type elements, primitive::colors::color_type color) noexcept -> element_type;
			[[nodiscard]] auto border(element_type element, primitive::colors::color_type color) noexcept -> element_type;
		}

		constexpr auto border = functional::overloaded{
				[](elements_type elements) noexcept -> element_type
				{
					return impl::border(std::move(elements), primitive::colors::black);
				},
				[](element_type element) noexcept -> element_type
				{
					return impl::border(std::move(element), primitive::colors::black);
				},
				[](primitive::colors::color_type color) noexcept -> auto
				{
					return functional::overloaded{
							[color](elements_type elements) noexcept -> element_type
							{
								return impl::border(std::move(elements), color);
							},
							[color](element_type element) noexcept -> element_type
							{
								return impl::border(std::move(element), color);
							},
					};
				},
		};

		// ===============================
		// LAYOUT

		namespace impl
		{
			[[nodiscard]] auto horizontal_box(elements_type elements) noexcept -> element_type;
			[[nodiscard]] auto vertical_box(elements_type elements) noexcept -> element_type;
		}

		constexpr auto horizontal_box = functional::overloaded{
				[](elements_type elements) noexcept -> element_type
				{
					return impl::horizontal_box(std::move(elements));
				},
				[](draw::impl::derived_element_t auto... elements) noexcept -> element_type
				{
					elements_type es{};
					es.reserve(sizeof...(elements));

					(es.emplace_back(std::move(elements)), ...);
					return impl::horizontal_box(std::move(es));
				},
		};

		constexpr auto vertical_box = functional::overloaded{
				[](elements_type elements) noexcept -> element_type
				{
					return impl::vertical_box(std::move(elements));
				},
				[](draw::impl::derived_element_t auto... elements) noexcept -> element_type
				{
					elements_type es{};
					es.reserve(sizeof...(elements));

					(es.emplace_back(std::move(elements)), ...);
					return impl::vertical_box(std::move(es));
				},
		};
	}
}
