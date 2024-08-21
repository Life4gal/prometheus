// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:multidimensional;

import std;
import gal.prometheus.meta;

#else
#pragma once

#include <type_traits>

#include <prometheus/macro.hpp>
#include <meta/meta.ixx>

#endif

namespace gal::prometheus::primitive
{
	namespace multidimensional_detail
	{
		template<std::size_t Index, typename Derived, bool>
		struct lazy_value_type;

		template<std::size_t Index, typename Derived>
		struct lazy_value_type<Index, Derived, true>
		{
			using type = std::tuple_element_t<Index, typename Derived::value_type>;
		};

		template<std::size_t Index, typename Derived>
		struct lazy_value_type<Index, Derived, false>
		{
			using type = typename Derived::value_type;
		};

		template<std::size_t Index, typename Derived>
		using lazy_value_type_t = typename lazy_value_type<
			Index,
			Derived,
			requires
			{
				// ReSharper disable once CppUseTypeTraitAlias
				typename std::tuple_element<Index, typename Derived::value_type>::type;
			}>::type;

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

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	enum class Dimension
	{
		_0 = 0,
		_1 = 1,
		_2 = 2,
		_3 = 3,
	};

	template<typename Derived, typename... Ts>
		requires (std::is_arithmetic_v<Ts> and ...)
	struct [[nodiscard]] multidimensional
	{
		template<typename D, typename... Us>
			requires (std::is_arithmetic_v<Us> and ...)
		friend struct multidimensional;

		using value_type = std::tuple<Ts...>;

		using derived_type = Derived;

	private:
		[[nodiscard]] constexpr auto rep() noexcept -> Derived& { return *static_cast<Derived*>(this); }
		[[nodiscard]] constexpr auto rep() const noexcept -> const Derived& { return *static_cast<const Derived*>(this); }

	public:
		template<typename OtherDerived>
			requires(
				multidimensional_detail::always_equal_t<OtherDerived> and
				multidimensional_detail::always_equal_t<derived_type> and
				meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
			)
		[[nodiscard]] constexpr auto to() const noexcept -> OtherDerived
		{
			if constexpr (std::is_same_v<OtherDerived, derived_type>) { return *this; }
			else
			{
				OtherDerived result;

				meta::member_zip_walk(
					[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void //
					{
						using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
						lhs = static_cast<type>(rhs);
					},
					result,
					rep()
				);

				return result;
			}
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator+=(const multidimensional<OtherDerived, Us...>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					lhs += static_cast<type>(rhs);
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator+(const multidimensional<OtherDerived, Us...>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result += other;

			return result;
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		constexpr auto operator+=(const U value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					self += static_cast<type>(static_cast<std::common_type_t<Ts...>>(value));
				},
				rep()
			);
			return rep();
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		[[nodiscard]] constexpr auto operator+(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result += value;

			return result;
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator-=(const multidimensional<OtherDerived, Us...>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					lhs -= static_cast<type>(rhs);
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator-(const multidimensional<OtherDerived, Us...>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result -= other;

			return result;
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		constexpr auto operator-=(const U value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					self -= static_cast<type>(static_cast<std::common_type_t<Ts...>>(value));
				},
				rep()
			);
			return rep();
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		[[nodiscard]] constexpr auto operator-(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result -= value;

			return result;
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator*=(const multidimensional<OtherDerived, Us...>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					lhs *= static_cast<type>(rhs);
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator*(const multidimensional<OtherDerived, Us...>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result *= other;

			return result;
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		constexpr auto operator*=(const U value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					self *= static_cast<type>(static_cast<std::common_type_t<Ts...>>(value));
				},
				rep()
			);
			return rep();
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		[[nodiscard]] constexpr auto operator*(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result *= value;

			return result;
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator/=(const multidimensional<OtherDerived, Us...>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					lhs /= static_cast<type>(rhs);
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator/(const multidimensional<OtherDerived, Us...>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result /= other;

			return result;
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		constexpr auto operator/=(const U value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					self /= static_cast<type>(static_cast<std::common_type_t<Ts...>>(value));
				},
				rep()
			);
			return rep();
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		[[nodiscard]] constexpr auto operator/(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result /= value;

			return result;
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator%=(const multidimensional<OtherDerived, Us...>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					lhs %= static_cast<type>(rhs);
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator%(const multidimensional<OtherDerived, Us...>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result %= other;

			return result;
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		constexpr auto operator%=(const U value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					self %= static_cast<type>(static_cast<std::common_type_t<Ts...>>(value));
				},
				rep()
			);
			return rep();
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		[[nodiscard]] constexpr auto operator%(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result %= value;

			return result;
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator&=(const multidimensional<OtherDerived, Us...>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					lhs &= static_cast<type>(rhs);
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator&(const multidimensional<OtherDerived, Us...>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result &= other;

			return result;
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		constexpr auto operator&=(const U value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					self &= static_cast<type>(static_cast<std::common_type_t<Ts...>>(value));
				},
				rep()
			);
			return rep();
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		[[nodiscard]] constexpr auto operator&(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result &= value;

			return result;
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		constexpr auto operator|=(const multidimensional<OtherDerived, Us...>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					lhs |= static_cast<type>(rhs);
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto operator|(const multidimensional<OtherDerived, Us...>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result |= other;

			return result;
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		constexpr auto operator|=(const U value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = multidimensional_detail::lazy_value_type_t<Index, derived_type>;
					self |= static_cast<type>(static_cast<std::common_type_t<Ts...>>(value));
				},
				rep()
			);
			return rep();
		}

		template<std::convertible_to<std::common_type_t<Ts...>> U>
		[[nodiscard]] constexpr auto operator|(const U value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result |= value;

			return result;
		}

		template<Dimension D, typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				        std::is_same_v<OtherDerived, derived_type> or
				        (
					        multidimensional_detail::always_equal_t<OtherDerived> and
					        multidimensional_detail::always_equal_t<derived_type> and
					        meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				        )
			        ) and
			        (std::three_way_comparable_with<Ts, Us> and ...)
		[[nodiscard]] constexpr auto compare(const multidimensional<OtherDerived, Us...>& other) noexcept -> auto
		{
			const auto v = meta::member_of_index<static_cast<std::size_t>(D)>(rep());
			const auto other_v = meta::member_of_index<static_cast<std::size_t>(D)>(other.rep());

			return v <=> other_v;
		}

		template<typename Comparator, typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				        std::is_same_v<OtherDerived, derived_type> or
				        (
					        multidimensional_detail::always_equal_t<OtherDerived> and
					        multidimensional_detail::always_equal_t<derived_type> and
					        meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				        )
			        ) and
			        (std::declval<Comparator>(std::declval<Ts>(), std::declval<Us>()) and ...)
		[[nodiscard]] constexpr auto compare(Comparator comparator, const multidimensional<OtherDerived, Us...>& other) noexcept -> bool
		{
			return [&]<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
			{
				const auto f = [&]<std::size_t I>() noexcept -> bool
				{
					return comparator(meta::member_of_index<I>(rep()), meta::member_of_index<I>(other.rep()));
				};

				return (f.template operator()<Index>() and ...);
			}();
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto equal(const multidimensional<OtherDerived, Us...>& other) noexcept -> bool
		{
			return this->compare(std::ranges::equal_to{}, other);
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto not_equal(const multidimensional<OtherDerived, Us...>& other) noexcept -> bool
		{
			return this->compare(std::ranges::not_equal_to{}, other);
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto greater_than(const multidimensional<OtherDerived, Us...>& other) noexcept -> bool
		{
			return this->compare(std::ranges::greater{}, other);
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto greater_equal(const multidimensional<OtherDerived, Us...>& other) noexcept -> bool
		{
			return this->compare(std::ranges::greater_equal{}, other);
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto less_than(const multidimensional<OtherDerived, Us...>& other) noexcept -> bool
		{
			return this->compare(std::ranges::less{}, other);
		}

		template<typename OtherDerived = derived_type, std::convertible_to<Ts>... Us>
			requires(
				std::is_same_v<OtherDerived, derived_type> or
				(
					multidimensional_detail::always_equal_t<OtherDerived> and
					multidimensional_detail::always_equal_t<derived_type> and
					meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
				)
			)
		[[nodiscard]] constexpr auto less_equal(const multidimensional<OtherDerived, Us...>& other) noexcept -> bool
		{
			return this->compare(std::ranges::less_equal{}, other);
		}
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
