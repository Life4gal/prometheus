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
#include <span>
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
	using elements_view_type = std::span<element_type>;

	using element_matrix_type = std::vector<elements_type>;
	using element_matrix_view_type = std::span<elements_type>;

	template<typename T>
	concept derived_element_t = std::derived_from<typename T::element_type, impl::Element>;

	template<typename Range>
	concept derived_elements_t = std::ranges::range<Range> and std::derived_from<typename Range::value_type::element_type, impl::Element>;

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
			enum class BoxOption : std::uint32_t
			{
				HORIZONTAL = 0b0000'0001,
				VERTICAL = 0b0000'0010,
				GRID = 0b0000'0100,
			};

			enum class FlexOption : std::uint32_t
			{
				NONE = 0b0000'0000,

				GROW = 0b0000'0001,
				SHRINK = 0b0000'0010,

				HORIZONTAL = 0b0001'0000,
				VERTICAL = 0b0010'0000,
			};

			enum class CenterOption : std::uint32_t
			{
				HORIZONTAL = 0b0000'0001,
				VERTICAL = 0b0000'0010,
			};

			template<auto... Os>
			struct options : std::integral_constant<std::common_type_t<std::underlying_type_t<std::decay_t<decltype(Os)>>...>, (0 | ... | std::to_underlying(Os))> {};

			[[nodiscard]] auto box_horizontal(elements_type elements) noexcept -> element_type;
			[[nodiscard]] auto box_vertical(elements_type elements) noexcept -> element_type;
			[[nodiscard]] auto box_grid(element_matrix_type elements_grid) noexcept -> element_type;

			[[nodiscard]] auto flex_filler() noexcept -> element_type;
			[[nodiscard]] auto flex_none(element_type element) noexcept -> element_type;

			[[nodiscard]] auto flex(element_type element) noexcept -> element_type;
			[[nodiscard]] auto flex_grow(element_type element) noexcept -> element_type;
			[[nodiscard]] auto flex_shrink(element_type element) noexcept -> element_type;

			[[nodiscard]] auto flex_horizontal(element_type element) noexcept -> element_type;
			[[nodiscard]] auto flex_horizontal_grow(element_type element) noexcept -> element_type;
			[[nodiscard]] auto flex_horizontal_shrink(element_type element) noexcept -> element_type;

			[[nodiscard]] auto flex_vertical(element_type element) noexcept -> element_type;
			[[nodiscard]] auto flex_vertical_grow(element_type element) noexcept -> element_type;
			[[nodiscard]] auto flex_vertical_shrink(element_type element) noexcept -> element_type;

			[[nodiscard]] auto center_horizontal(element_type element) noexcept -> element_type;
			[[nodiscard]] auto center_vertical(element_type element) noexcept -> element_type;
			[[nodiscard]] auto center(element_type element) noexcept -> element_type;
		}

		constexpr auto option_box_horizontal = impl::options<impl::BoxOption::HORIZONTAL>{};
		constexpr auto option_box_vertical = impl::options<impl::BoxOption::VERTICAL>{};
		constexpr auto option_box_grid = impl::options<impl::BoxOption::GRID>{};

		constexpr auto option_flex_none = impl::options<impl::FlexOption::NONE>{};
		constexpr auto option_flex_grow = impl::options<impl::FlexOption::GROW>{};
		constexpr auto option_flex_shrink = impl::options<impl::FlexOption::SHRINK>{};
		constexpr auto option_flex_horizontal_grow = impl::options<impl::FlexOption::GROW, impl::FlexOption::HORIZONTAL>{};
		constexpr auto option_flex_horizontal_shrink = impl::options<impl::FlexOption::SHRINK, impl::FlexOption::HORIZONTAL>{};
		constexpr auto option_flex_vertical_grow = impl::options<impl::FlexOption::GROW, impl::FlexOption::VERTICAL>{};
		constexpr auto option_flex_vertical_shrink = impl::options<impl::FlexOption::SHRINK, impl::FlexOption::VERTICAL>{};
		constexpr auto option_flex_all = impl::options<impl::FlexOption::GROW, impl::FlexOption::SHRINK, impl::FlexOption::HORIZONTAL, impl::FlexOption::VERTICAL>{};

		constexpr auto option_center_horizontal = impl::options<impl::CenterOption::HORIZONTAL>{};
		constexpr auto option_center_vertical = impl::options<impl::CenterOption::VERTICAL>{};
		constexpr auto option_center_all = impl::options<impl::CenterOption::HORIZONTAL, impl::CenterOption::VERTICAL>{};

		constexpr auto layout = functional::overloaded{
				// BOX
				[]<impl::BoxOption... Os>(impl::options<Os...>, elements_type elements) noexcept -> element_type
				{
					if constexpr (constexpr auto value = impl::options<Os...>::value;
						value == option_box_horizontal)
					{
						return impl::box_horizontal(std::move(elements));
					}
					else if constexpr (value == option_box_vertical)
					{
						return impl::box_vertical(std::move(elements));
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				},
				[]<impl::BoxOption... Os>(impl::options<Os...>, element_matrix_type elements_matrix) noexcept -> element_type
				{
					if constexpr (constexpr auto value = impl::options<Os...>::value;
						value == option_box_grid)
					{
						return impl::box_grid(std::move(elements_matrix));
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				},
				[]<typename Self, impl::BoxOption... Os>(this const Self& self, impl::options<Os...> options, derived_element_t auto... elements) noexcept -> element_type
				{
					elements_type es{};
					es.reserve(sizeof...(elements));

					(es.emplace_back(std::move(elements)), ...);

					return self(options, std::move(es));
				},
				// (options, element...) => (options, elements)
				// (options, element...) !=> (options, elements...)
				[]<typename Self, impl::BoxOption... Os>(this const Self& self, impl::options<Os...> options, derived_elements_t auto... elements) noexcept -> element_type //
					requires (sizeof...(elements) != 1)
				{
					element_matrix_type es{};
					es.reserve(sizeof...(elements));

					(es.emplace_back(std::move(elements)), ...);

					return self(options, std::move(es));
				},
				[]<typename Self, impl::BoxOption... Os>(this const Self& self, impl::options<Os...> options) noexcept -> auto
				{
					return functional::overloaded{
							[self, options](elements_type elements) noexcept -> element_type
							{
								return self(std::move(elements), options);
							},
							[self, options](derived_element_t auto... elements) noexcept -> element_type
							{
								return self(options, std::move(elements)...);
							},
							[self, options](derived_elements_t auto... elements) noexcept -> element_type
							{
								return self(options, std::move(elements)...);
							}
					};
				},
				// FLEX
				[]<impl::FlexOption... Os>(impl::options<Os...>, element_type element) noexcept -> element_type
				{
					if constexpr (constexpr auto value = impl::options<Os...>::value;
						value == option_flex_none)
					{
						return impl::flex_none(std::move(element));
					}
					else if constexpr (value == option_flex_grow)
					{
						return impl::flex_grow(std::move(element));
					}
					else if constexpr (value == option_flex_shrink)
					{
						return impl::flex_shrink(std::move(element));
					}
					else if constexpr (value == option_flex_horizontal_grow)
					{
						return impl::flex_horizontal_grow(std::move(element));
					}
					else if constexpr (value == option_flex_horizontal_shrink)
					{
						return impl::flex_horizontal_shrink(std::move(element));
					}
					else if constexpr (value == option_flex_vertical_grow)
					{
						return impl::flex_vertical_grow(std::move(element));
					}
					else if constexpr (value == option_flex_vertical_shrink)
					{
						return impl::flex_vertical_shrink(std::move(element));
					}
					else if constexpr (value == option_flex_all)
					{
						return impl::flex(std::move(element));
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				},
				[]<typename Self, impl::FlexOption... Os>(this const Self& self, impl::options<Os...> options) noexcept -> auto
				{
					return [self, options](element_type element) noexcept -> element_type
					{
						return self(options, std::move(element));
					};
				},
				// CENTER
				[]<impl::CenterOption... Os>(impl::options<Os...>, element_type element) noexcept -> element_type
				{
					if constexpr (constexpr auto value = impl::options<Os...>::value;
						value == option_center_horizontal)
					{
						return impl::center_horizontal(std::move(element));
					}
					else if constexpr (value == option_center_vertical)
					{
						return impl::center_vertical(std::move(element));
					}
					else if constexpr (value == option_center_all)
					{
						return impl::center(std::move(element));
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				},
				[]<typename Self, impl::CenterOption... Os>(this const Self& self, impl::options<Os...> options) noexcept -> auto
				{
					return [self, options](element_type element) noexcept -> element_type
					{
						return self(options, std::move(element));
					};
				},
		};

		// ----------------------
		// ALIAS

		constexpr auto box_horizontal = functional::overloaded{
				[](elements_type elements) noexcept -> element_type
				{
					return layout(option_box_horizontal, std::move(elements));
				},
				[](derived_element_t auto... elements) noexcept -> element_type
				{
					return layout(option_box_horizontal, std::move(elements)...);
				},
		};

		constexpr auto box_vertical = functional::overloaded{
				[](elements_type elements) noexcept -> element_type
				{
					return layout(option_box_vertical, std::move(elements));
				},
				[](derived_element_t auto... elements) noexcept -> element_type
				{
					return layout(option_box_vertical, std::move(elements)...);
				},
		};

		constexpr auto box_grid = functional::overloaded{
				[](element_matrix_type elements) noexcept -> element_type
				{
					return layout(option_box_grid, std::move(elements));
				},
				[](derived_elements_t auto... elements) noexcept -> element_type
				{
					return layout(option_box_grid, std::move(elements)...);
				},
		};

		constexpr auto flex_none = [](element_type element) noexcept -> element_type
		{
			return layout(option_flex_none, std::move(element));
		};

		constexpr auto flex_grow = [](element_type element) noexcept -> element_type
		{
			return layout(option_flex_grow, std::move(element));
		};

		constexpr auto flex_shrink = [](element_type element) noexcept -> element_type
		{
			return layout(option_flex_shrink, std::move(element));
		};

		constexpr auto flex_horizontal_grow = [](element_type element) noexcept -> element_type
		{
			return layout(option_flex_horizontal_grow, std::move(element));
		};

		constexpr auto flex_horizontal_shrink = [](element_type element) noexcept -> element_type
		{
			return layout(option_flex_horizontal_shrink, std::move(element));
		};

		constexpr auto flex_vertical_grow = [](element_type element) noexcept -> element_type
		{
			return layout(option_flex_vertical_grow, std::move(element));
		};

		constexpr auto flex_vertical_shrink = [](element_type element) noexcept -> element_type
		{
			return layout(option_flex_vertical_shrink, std::move(element));
		};

		constexpr auto flex_all = [](element_type element) noexcept -> element_type
		{
			return layout(option_flex_all, std::move(element));
		};

		constexpr auto center_horizontal = [](element_type element) noexcept -> element_type
		{
			return layout(option_center_horizontal, std::move(element));
		};

		constexpr auto center_vertical = [](element_type element) noexcept -> element_type
		{
			return layout(option_center_vertical, std::move(element));
		};

		constexpr auto center_all = [](element_type element) noexcept -> element_type
		{
			return layout(option_center_all, std::move(element));
		};
	}
}

#undef LAMBDA_DEDUCING_THIS_WORKAROUND
