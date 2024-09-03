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

import :style;

#else
#pragma once

#include <memory>
#include <vector>
#include <algorithm>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>
#include <functional/functional.ixx>
#include <draw/style.ixx>

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
		template<typename T>
		concept derived_element_t = std::derived_from<typename T::element_type, impl::Element>;

		template<typename Element, typename Decorator>
			requires (derived_element_t<Element> and derived_element_t<std::invoke_result_t<Decorator, Element>>)
		[[nodiscard]] constexpr auto operator|(Element&& element, Decorator&& decorator) noexcept -> element_type //
		{
			return std::invoke(std::forward<Decorator>(decorator), std::forward<Element>(element));
		}

		template<typename Decorator>
			requires derived_element_t<std::invoke_result_t<Decorator, elements_type::value_type>>
		[[nodiscard]] constexpr auto operator|(elements_type& elements, Decorator&& decorator) noexcept -> decltype(auto)
		{
			return std::invoke(std::forward<Decorator>(decorator), elements);
		}

		template<typename Decorator>
			requires derived_element_t<std::invoke_result_t<Decorator, elements_type::value_type>>
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
		// SEPARATOR

		namespace impl
		{
			[[nodiscard]] auto separator() noexcept -> element_type;
		}

		// ===============================
		// BORDER & WINDOW

		namespace impl
		{
			[[nodiscard]] auto border(element_type element, Style::color_type color) noexcept -> element_type;

			[[nodiscard]] auto window(element_type title, Style::color_type title_color, element_type content, Style::color_type content_color) noexcept -> element_type;
		}

		constexpr auto border = functional::overloaded{
				[](element_type element) noexcept -> element_type
				{
					return impl::border(std::move(element), Style::instance().border_default_color);
				},
				[](const Style::color_type color) noexcept -> auto
				{
					return functional::overloaded{
							[color](element_type element) noexcept -> element_type
							{
								return impl::border(std::move(element), color);
							},
					};
				},
		};

		constexpr auto window = functional::overloaded{
				[](element_type title, element_type content) noexcept -> element_type
				{
					return impl::window(std::move(title), Style::instance().window_title_default_color, std::move(content), Style::instance().border_default_color);
				},
				[](const Style::color_type title_color, const Style::color_type content_color) noexcept -> auto
				{
					return functional::overloaded{
							[title_color, content_color](element_type title, element_type content) noexcept -> element_type
							{
								return impl::window(std::move(title), title_color, std::move(content), content_color);
							},
					};
				}
		};

		// ===============================
		// LAYOUT

		namespace impl
		{
			// todo
			#define LAMBDA_DEDUCING_THIS_WORKAROUND 1

			enum class FlexOption : std::uint32_t
			{
				NONE = 0b0000'0000,
				ALL = 0b0001'0000'0000,

				GROW = 0b0000'0001,
				SHRINK = 0b0000'0010,

				HORIZONTAL = 0b0001'0000,
				VERTICAL = 0b0010'0000,
			};

			template<FlexOption... Os>
			struct flex_option : std::integral_constant<std::underlying_type_t<FlexOption>, (... + std::to_underlying(Os))> {};

			#if LAMBDA_DEDUCING_THIS_WORKAROUND
			template<>
			struct flex_option<> : std::integral_constant<std::underlying_type_t<FlexOption>, std::to_underlying(FlexOption::NONE)> {};
			#endif

			enum class CenterOption : std::uint32_t
			{
				ALL = 0b0000,
				HORIZONTAL = 0b0001,
				VERTICAL = 0b0010,
			};

			template<CenterOption... Os>
			struct center_option : std::integral_constant<std::underlying_type_t<CenterOption>, (... + std::to_underlying(Os))> {};

			#if LAMBDA_DEDUCING_THIS_WORKAROUND
			template<>
			struct center_option<> : std::integral_constant<std::underlying_type_t<CenterOption>, std::to_underlying(CenterOption::ALL)> {};
			#endif

			[[nodiscard]] auto filler() noexcept -> element_type;
			[[nodiscard]] auto no_flex(element_type element) noexcept -> element_type;

			[[nodiscard]] auto flex(element_type element) noexcept -> element_type;
			[[nodiscard]] auto flex_grow(element_type element) noexcept -> element_type;
			[[nodiscard]] auto flex_shrink(element_type element) noexcept -> element_type;

			[[nodiscard]] auto horizontal_flex(element_type element) noexcept -> element_type;
			[[nodiscard]] auto horizontal_flex_grow(element_type element) noexcept -> element_type;
			[[nodiscard]] auto horizontal_flex_shrink(element_type element) noexcept -> element_type;

			[[nodiscard]] auto vertical_flex(element_type element) noexcept -> element_type;
			[[nodiscard]] auto vertical_flex_grow(element_type element) noexcept -> element_type;
			[[nodiscard]] auto vertical_flex_shrink(element_type element) noexcept -> element_type;

			[[nodiscard]] auto horizontal_box(elements_type elements) noexcept -> element_type;
			[[nodiscard]] auto vertical_box(elements_type elements) noexcept -> element_type;
			[[nodiscard]] auto stack_box(elements_type elements) noexcept -> element_type;

			[[nodiscard]] auto horizontal_center(element_type element) noexcept -> element_type;
			[[nodiscard]] auto vertical_center(element_type element) noexcept -> element_type;
			[[nodiscard]] auto center(element_type element) noexcept -> element_type;
		}

		#if LAMBDA_DEDUCING_THIS_WORKAROUND
		constexpr auto flex_option_none = impl::flex_option{};
		#else
		constexpr auto flex_option_none = impl::flex_option<impl::FlexOption::NONE>{};
		#endif
		constexpr auto flex_option_all = impl::flex_option<impl::FlexOption::ALL>{};
		constexpr auto flex_option_grow = impl::flex_option<impl::FlexOption::GROW>{};
		constexpr auto flex_option_shrink = impl::flex_option<impl::FlexOption::SHRINK>{};
		constexpr auto flex_option_horizontal_grow = impl::flex_option<impl::FlexOption::HORIZONTAL, impl::FlexOption::GROW>{};
		constexpr auto flex_option_horizontal_shrink = impl::flex_option<impl::FlexOption::HORIZONTAL, impl::FlexOption::SHRINK>{};
		constexpr auto flex_option_vertical_grow = impl::flex_option<impl::FlexOption::VERTICAL, impl::FlexOption::GROW>{};
		constexpr auto flex_option_vertical_shrink = impl::flex_option<impl::FlexOption::VERTICAL, impl::FlexOption::SHRINK>{};

		#if LAMBDA_DEDUCING_THIS_WORKAROUND
		constexpr auto center_option_all = impl::center_option{};
		#else
		constexpr auto center_option_all = impl::center_option<impl::CenterOption::ALL>{};
		#endif
		constexpr auto center_option_horizontal = impl::center_option<impl::CenterOption::HORIZONTAL>{};
		constexpr auto center_option_vertical = impl::center_option<impl::CenterOption::VERTICAL>{};

		constexpr auto flex = functional::overloaded{
				[]<impl::FlexOption... Os>(
			element_type element,
			const impl::flex_option<Os...>
			#if LAMBDA_DEDUCING_THIS_WORKAROUND
			 = flex_option_none
			#endif
		) noexcept -> element_type
				{
					#if LAMBDA_DEDUCING_THIS_WORKAROUND
					constexpr auto value = std::conditional_t<sizeof...(Os) == 0, impl::flex_option<impl::FlexOption::NONE>, impl::flex_option<Os...>>::value;
					#else
					constexpr auto value = impl::flex_option<Os...>::value;
					#endif
					if constexpr (value & std::to_underlying(impl::FlexOption::GROW))
					{
						if constexpr (value & std::to_underlying(impl::FlexOption::HORIZONTAL))
						{
							return impl::horizontal_flex_grow(std::move(element));
						}
						else if constexpr (value & std::to_underlying(impl::FlexOption::VERTICAL))
						{
							return impl::vertical_flex_grow(std::move(element));
						}
						else
						{
							static_assert(value == std::to_underlying(impl::FlexOption::GROW));
							return impl::flex_grow(std::move(element));
						}
					}
					else if constexpr (value & std::to_underlying(impl::FlexOption::SHRINK))
					{
						if constexpr (value & std::to_underlying(impl::FlexOption::HORIZONTAL))
						{
							return impl::horizontal_flex_shrink(std::move(element));
						}
						else if constexpr (value & std::to_underlying(impl::FlexOption::VERTICAL))
						{
							return impl::vertical_flex_shrink(std::move(element));
						}
						else
						{
							static_assert(value == std::to_underlying(impl::FlexOption::SHRINK));
							return impl::flex_shrink(std::move(element));
						}
					}
					else if constexpr (value == std::to_underlying(impl::FlexOption::ALL))
					{
						return impl::flex(std::move(element));
					}
					else
					{
						static_assert(value == std::to_underlying(impl::FlexOption::NONE));
						return impl::no_flex(std::move(element));
					}
				},
				#if not LAMBDA_DEDUCING_THIS_WORKAROUND
				[]<typename Self>(this Self&& self, element_type element) noexcept -> element_type
				{
					return std::forward<Self>(self)(std::move(element), flex_option_none);
				},
				#endif
				[]<typename Self, impl::FlexOption... Os>(this Self&& self, const impl::flex_option<Os...> options) noexcept -> auto
				{
					return [s = std::forward<Self>(self), options](element_type element) noexcept -> element_type
					{
						return s(std::move(element), options);
					};
				},
		};

		constexpr auto horizontal_box = functional::overloaded{
				[](elements_type elements) noexcept -> element_type
				{
					return impl::horizontal_box(std::move(elements));
				},
				[](derived_element_t auto... elements) noexcept -> element_type
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
				[](derived_element_t auto... elements) noexcept -> element_type
				{
					elements_type es{};
					es.reserve(sizeof...(elements));

					(es.emplace_back(std::move(elements)), ...);
					return impl::vertical_box(std::move(es));
				},
		};

		constexpr auto stack_box = functional::overloaded{
				[](elements_type elements) noexcept -> element_type
				{
					return impl::stack_box(std::move(elements));
				},
				[](derived_element_t auto... elements) noexcept -> element_type
				{
					elements_type es{};
					es.reserve(sizeof...(elements));

					(es.emplace_back(std::move(elements)), ...);
					return impl::stack_box(std::move(es));
				},
		};

		constexpr auto center = functional::overloaded
		{
				[]<impl::CenterOption... Os>(
			element_type element,
			const impl::center_option<Os...>
			#if LAMBDA_DEDUCING_THIS_WORKAROUND
			 = center_option_all
			#endif
		) noexcept -> element_type
				{
					constexpr auto value = std::conditional_t<sizeof...(Os) == 0, impl::center_option<impl::CenterOption::ALL>, impl::center_option<Os...>>::value;
					if constexpr (value == std::to_underlying(impl::CenterOption::ALL))
					{
						return impl::center(std::move(element));
					}
					else if constexpr (value == std::to_underlying(impl::CenterOption::HORIZONTAL))
					{
						return impl::horizontal_center(std::move(element));
					}
					else
					{
						static_assert(value == std::to_underlying(impl::CenterOption::VERTICAL));
						return impl::vertical_center(std::move(element));
					}
				},
				#if not LAMBDA_DEDUCING_THIS_WORKAROUND
				[]<typename Self>(this Self&& self, element_type element) noexcept -> element_type
				{
					return std::forward<Self>(self)(std::move(element), center_option_all);
				},
				#endif
				[]<typename Self, impl::CenterOption... Os>(this Self&& self, const impl::center_option<Os...> options) noexcept -> auto
				{
					return [s = std::forward<Self>(self), options](element_type element) noexcept -> element_type
					{
						return s(std::move(element), options);
					};
				},
		};
	}
}

#undef LAMBDA_DEDUCING_THIS_WORKAROUND
