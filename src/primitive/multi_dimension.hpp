// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:multi_dimension;

import std;
import gal.prometheus.meta;

#else

#include <type_traits>
#include <compare>

#include <prometheus/macro.hpp>
#include <meta/meta.hpp>
#endif

namespace gal::prometheus::primitive
{
	namespace multi_dimension_detail
	{
		template<typename>
		struct is_always_equal : std::false_type {};

		template<typename T>
			requires requires { T::is_always_equal; }
		struct is_always_equal<T> : std::bool_constant<T::is_always_equal> {};

		template<typename T>
		constexpr static auto is_always_equal_v = is_always_equal<T>::value;

		template<typename T>
		concept always_equal_t = is_always_equal_v<T>;
	}

	enum class Dimension
	{
		_0 = 0,
		_1 = 1,
		_2 = 2,
		_3 = 3,
	};

	template<typename T, typename Derived>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] multi_dimension
	{
		template<typename U, typename D>
			requires std::is_arithmetic_v<U>
		friend struct multi_dimension;

		using value_type = T;

		using derived_type = Derived;

	private:
		[[nodiscard]] constexpr auto rep() noexcept -> Derived& { return *static_cast<Derived*>(this); }
		[[nodiscard]] constexpr auto rep() const noexcept -> const Derived& { return *static_cast<const Derived*>(this); }

		template<Dimension D>
		[[nodiscard]] constexpr auto get_value() const noexcept -> value_type
		{
			if constexpr (D == Dimension::_0)
			{
				if constexpr (requires { rep().x; }) { return rep().x; }
				else if constexpr (requires { rep().a; }) { return rep().a; }
				else if constexpr (requires { rep().template get<0>(); }) { return rep().template get<0>(); }
				else if constexpr (requires { get<0>(rep()); }) { return get<0>(rep()); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
			else if constexpr (D == Dimension::_1)
			{
				if constexpr (requires { rep().y; }) { return rep().y; }
				else if constexpr (requires { rep().b; }) { return rep().b; }
				else if constexpr (requires { rep().template get<1>(); }) { return rep().template get<1>(); }
				else if constexpr (requires { get<1>(rep()); }) { return get<1>(rep()); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
			else if constexpr (D == Dimension::_2)
			{
				if constexpr (requires { rep().z; }) { return rep().z; }
				else if constexpr (requires { rep().c; }) { return rep().c; }
				else if constexpr (requires { rep().template get<2>(); }) { return rep().template get<2>(); }
				else if constexpr (requires { get<2>(rep()); }) { return get<2>(rep()); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
			else if constexpr (D == Dimension::_3)
			{
				if constexpr (requires { rep().w; }) { return rep().w; }
				else if constexpr (requires { rep().d; }) { return rep().d; }
				else if constexpr (requires { rep().template get<3>(); }) { return rep().template get<3>(); }
				else if constexpr (requires { get<3>(rep()); }) { return get<3>(rep()); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<Dimension D>
		[[nodiscard]] constexpr auto get_reference() noexcept -> value_type&
		{
			if constexpr (D == Dimension::_0)
			{
				if constexpr (requires { rep().x; }) { return rep().x; }
				else if constexpr (requires { rep().a; }) { return rep().a; }
				else if constexpr (requires { rep().template get<0>(); }) { return rep().template get<0>(); }
				else if constexpr (requires { get<0>(rep()); }) { return get<0>(rep()); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
			else if constexpr (D == Dimension::_1)
			{
				if constexpr (requires { rep().y; }) { return rep().y; }
				else if constexpr (requires { rep().b; }) { return rep().b; }
				else if constexpr (requires { rep().template get<1>(); }) { return rep().template get<1>(); }
				else if constexpr (requires { get<1>(rep()); }) { return get<1>(rep()); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
			else if constexpr (D == Dimension::_2)
			{
				if constexpr (requires { rep().z; }) { return rep().z; }
				else if constexpr (requires { rep().c; }) { return rep().c; }
				else if constexpr (requires { rep().template get<2>(); }) { return rep().template get<2>(); }
				else if constexpr (requires { get<2>(rep()); }) { return get<2>(rep()); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
			else if constexpr (D == Dimension::_3)
			{
				if constexpr (requires { rep().w; }) { return rep().w; }
				else if constexpr (requires { rep().d; }) { return rep().d; }
				else if constexpr (requires { rep().template get<3>(); }) { return rep().template get<3>(); }
				else if constexpr (requires { get<3>(rep()); }) { return get<3>(rep()); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

	public:
		template<typename OtherDerived>
			requires(
				multi_dimension_detail::always_equal_t<OtherDerived> and multi_dimension_detail::always_equal_t<derived_type>
			)
		[[nodiscard]] constexpr auto to() const noexcept -> OtherDerived
		{
			using type = typename OtherDerived::value_type;

			OtherDerived result;

			meta::member_for_each(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs = static_cast<type>(rhs); },
					result,
					rep()
					);

			return result;
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator+(const multi_dimension<U, OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			meta::member_for_each(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs += static_cast<value_type>(rhs); },
					result,
					other.rep()
					);

			return result;
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator+=(const multi_dimension<U, OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_for_each(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs += static_cast<value_type>(rhs); },
					rep(),
					other.rep()
					);
			return rep();
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto operator+(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			meta::member_for_each(
					[value]<std::size_t Index>(auto& self) noexcept -> void { self += static_cast<value_type>(value); },
					result
					);

			return result;
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto operator+=(const U value) noexcept -> derived_type&
		{
			meta::member_for_each(
					[value]<std::size_t Index>(auto& self) noexcept -> void { self += static_cast<value_type>(value); },
					rep()
					);
			return rep();
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator-(const multi_dimension<U, OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			meta::member_for_each(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs -= static_cast<value_type>(rhs); },
					result,
					other.rep()
					);

			return result;
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator-=(const multi_dimension<U, OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_for_each(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs -= static_cast<value_type>(rhs); },
					rep(),
					other.rep()
					);
			return rep();
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto operator-(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			meta::member_for_each(
					[value]<std::size_t Index>(auto& self) noexcept -> void { self -= static_cast<value_type>(value); },
					result
					);

			return result;
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto operator-=(const U value) noexcept -> derived_type&
		{
			meta::member_for_each(
					[value]<std::size_t Index>(auto& self) noexcept -> void { self -= static_cast<value_type>(value); },
					rep()
					);
			return rep();
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator*(const multi_dimension<U, OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			meta::member_for_each(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs *= static_cast<value_type>(rhs); },
					result,
					other.rep()
					);

			return result;
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator*=(const multi_dimension<U, OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_for_each(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs *= static_cast<value_type>(rhs); },
					rep(),
					other.rep()
					);
			return rep();
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto operator*(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			meta::member_for_each(
					[value]<std::size_t Index>(auto& self) noexcept -> void { self *= static_cast<value_type>(value); },
					result
					);

			return result;
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto operator*=(const U value) noexcept -> derived_type&
		{
			meta::member_for_each(
					[value]<std::size_t Index>(auto& self) noexcept -> void { self *= static_cast<value_type>(value); },
					rep()
					);
			return rep();
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator/(const multi_dimension<U, OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			meta::member_for_each(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs /= static_cast<value_type>(rhs); },
					result,
					other.rep()
					);

			return result;
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator/=(const multi_dimension<U, OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_for_each(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs /= static_cast<value_type>(rhs); },
					rep(),
					other.rep()
					);
			return rep();
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto operator/(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			meta::member_for_each(
					[value]<std::size_t Index>(auto& self) noexcept -> void { self /= static_cast<value_type>(value); },
					result
					);

			return result;
		}

		template<std::convertible_to<value_type> U>
		[[nodiscard]] constexpr auto operator/=(const U value) noexcept -> derived_type&
		{
			meta::member_for_each(
					[value]<std::size_t Index>(auto& self) noexcept -> void { self /= static_cast<value_type>(value); },
					rep()
					);
			return rep();
		}

		template<Dimension D, std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				        std::is_same_v<OtherDerived, derived_type> or
				        (
					        multi_dimension_detail::always_equal_t<OtherDerived> and
					        multi_dimension_detail::always_equal_t<derived_type> and
					        meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				        )
			        ) and
			        std::three_way_comparable_with<value_type, U>
		[[nodiscard]] constexpr auto compare(const multi_dimension<U, OtherDerived>& other) noexcept -> auto
		{
			const auto v = meta::member_of_index<static_cast<std::size_t>(D)>(rep());
			const auto other_v = meta::member_of_index<static_cast<std::size_t>(D)>(other.rep());

			return v <=> other_v;
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto exact_equal(const multi_dimension<U, OtherDerived>& other) noexcept -> bool
		{
			std::size_t index = 0;
			meta::member_for_each_until(
					[&index]<std::size_t Index>(const auto lhs, const auto rhs) noexcept -> bool
					{
						const auto r = lhs == rhs;
						index = Index + 1;
						return r;
					},
					rep(),
					other.rep()
					);
			return index == meta::member_size<derived_type>();
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto exact_greater_than(const multi_dimension<U, OtherDerived>& other) noexcept -> bool
		{
			std::size_t index = 0;
			meta::member_for_each_until(
					[&index]<std::size_t Index>(const auto lhs, const auto rhs) noexcept -> bool
					{
						const auto r = lhs > rhs;
						index = Index + 1;
						return r;
					},
					rep(),
					other.rep()
					);
			return index == meta::member_size<derived_type>();
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto exact_greater_equal(const multi_dimension<U, OtherDerived>& other) noexcept -> bool
		{
			std::size_t index = 0;
			meta::member_for_each_until(
					[&index]<std::size_t Index>(const auto lhs, const auto rhs) noexcept -> bool
					{
						const auto r = lhs >= rhs;
						index = Index + 1;
						return r;
					},
					rep(),
					other.rep()
					);
			return index == meta::member_size<derived_type>();
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto exact_less_than(const multi_dimension<U, OtherDerived>& other) noexcept -> bool
		{
			std::size_t index = 0;
			meta::member_for_each_until(
					[&index]<std::size_t Index>(const auto lhs, const auto rhs) noexcept -> bool
					{
						const auto r = lhs < rhs;
						index = Index + 1;
						return r;
					},
					rep(),
					other.rep()
					);
			return index == meta::member_size<derived_type>();
		}

		template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multi_dimension_detail::always_equal_t<OtherDerived> and
					multi_dimension_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto exact_less_equal(const multi_dimension<U, OtherDerived>& other) noexcept -> bool
		{
			std::size_t index = 0;
			meta::member_for_each_until(
					[&index]<std::size_t Index>(const auto lhs, const auto rhs) noexcept -> bool
					{
						const auto r = lhs <= rhs;
						index = Index + 1;
						return r;
					},
					rep(),
					other.rep()
					);
			return index == meta::member_size<derived_type>();
		}
	};
}
