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
import gal.prometheus.functional;

#else
#pragma once

#include <type_traits>
#include <memory>
#include <vector>
#include <span>

#include <prometheus/macro.hpp>

#include <primitive/primitive.ixx>
#include <functional/functional.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw)
{
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

			virtual auto calculate_requirement(Surface& surface) noexcept -> void
			{
				std::ranges::for_each(
					children_,
					[&surface](auto& child) noexcept -> void
					{
						child->calculate_requirement(surface);
					}
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
					[&surface](auto& child) noexcept -> void
					{
						child->render(surface);
					}
				);
			}
		};
	}

	template<auto... Os>
	struct options : std::integral_constant<std::common_type_t<std::underlying_type_t<std::decay_t<decltype(Os)>>...>, (0 | ... | std::to_underlying(Os))> {};

	namespace detail
	{
		template<auto... Os>
		struct element_maker;

		template<typename>
		struct is_options : std::false_type {};

		template<auto... Os>
		struct is_options<options<Os...>> : std::true_type {};

		template<typename T>
		constexpr auto is_options_v = is_options<T>::value;

		template<typename T>
		concept options_t = is_options_v<T>;

		template<typename...>
		struct options_builder;

		template<>
		struct options_builder<>
		{
			constexpr static auto value = functional::value_list<>;
		};

		template<auto... Os>
		struct options_builder<options<Os...>>
		{
			constexpr static auto value = functional::value_list<Os...>;
		};

		template<auto... Os1, auto... Os2>
		struct options_builder<options<Os1...>, options<Os2...>>
		{
			constexpr static auto value = functional::value_list<Os1..., Os2...>;
		};

		template<auto... Os1, auto... Os2, options_t... O>
		struct options_builder<options<Os1...>, options<Os2...>, O...>
		{
			constexpr static auto value = options_builder<options<Os1...>, options<Os2...>>::value.template push_back<options_builder<O...>::value>();
		};

		template<auto... Os>
		struct maker_selector
		{
			using type = element_maker<Os...>;

			constexpr explicit maker_selector(functional::value_list_type<Os...>) noexcept {}
		};

		template<auto... Os>
		maker_selector(functional::value_list_type<Os...>) -> maker_selector<Os...>;

		template<options_t... Os>
		[[nodiscard]] constexpr auto select_maker() noexcept -> auto
		{
			[[maybe_unused]] constexpr auto value = options_builder<Os...>::value;
			using selector_type = decltype(maker_selector{value});

			return typename selector_type::type{};
		}
	}
}
