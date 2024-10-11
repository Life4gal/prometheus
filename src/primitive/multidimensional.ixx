// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:primitive.multidimensional;

import std;

import :meta;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>

#include <prometheus/macro.hpp>
#include <meta/meta.ixx>

#endif

namespace gal::prometheus::primitive
{
	namespace multidimensional_detail
	{
		template<typename OtherDerived, typename ThisDerived>
		[[nodiscard]] consteval auto is_convertible_derived() noexcept -> bool
		{
			if constexpr (meta::member_size<OtherDerived>() != meta::member_size<ThisDerived>())
			{
				return false;
			}
			else
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
				{
					const auto f = []<std::size_t I>() noexcept -> bool
					{
						using this_type = meta::member_type_of_index_t<I, ThisDerived>;
						using other_type = meta::member_type_of_index_t<I, OtherDerived>;

						return
								// implicit
								std::is_convertible_v<other_type, this_type> or
								// explicit
								std::is_constructible_v<this_type, other_type>;
					};

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<meta::member_size<ThisDerived>()>{});
			}
		}

		template<typename OtherDerived, typename ThisDerived>
		concept convertible_derived_t = is_convertible_derived<OtherDerived, ThisDerived>();

		template<typename T, typename ThisDerived>
		[[nodiscard]] consteval auto is_convertible_type() noexcept -> bool
		{
			return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
			{
				const auto f = []<std::size_t I>() noexcept -> bool
				{
					using this_type = meta::member_type_of_index_t<I, ThisDerived>;

					return
							// implicit
							std::is_convertible_v<T, this_type> or
							// explicit
							std::is_constructible_v<this_type, T>;
				};

				return (f.template operator()<Index>() and ...);
			}(std::make_index_sequence<meta::member_size<ThisDerived>()>{});
		}

		template<typename T, typename ThisDerived>
		concept convertible_type_t = is_convertible_type<T, ThisDerived>();
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	template<typename Derived>
	struct [[nodiscard]] multidimensional
	{
		template<typename D>
		friend struct multidimensional;

		using derived_type = Derived;

	private:
		[[nodiscard]] constexpr auto rep() noexcept -> Derived& { return *static_cast<Derived*>(this); }
		[[nodiscard]] constexpr auto rep() const noexcept -> const Derived& { return *static_cast<const Derived*>(this); }

	public:
		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived>
		[[nodiscard]] constexpr auto to() const noexcept -> OtherDerived
		{
			if constexpr (std::is_same_v<OtherDerived, derived_type>) { return *this; }
			else
			{
				OtherDerived result;

				meta::member_zip_walk(
					[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
					{
						using type = meta::member_type_of_index_t<Index, derived_type>;
						if constexpr (requires { static_cast<type>(rhs); }) { lhs = static_cast<type>(rhs); }
						else { lhs = type{rhs}; }
					},
					result,
					rep()
				);

				return result;
			}
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator+=(const multidimensional<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); }) { lhs += static_cast<type>(rhs); }
					else { lhs += type{rhs}; }
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator+(const multidimensional<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result += other;

			return result;
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		constexpr auto operator+=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); }) { self += value; }
					else { self += type{value}; }
				},
				rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator+(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result += value;

			return result;
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator-=(const multidimensional<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); }) { lhs -= static_cast<type>(rhs); }
					else { lhs -= type{rhs}; }
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator-(const multidimensional<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result -= other;

			return result;
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		constexpr auto operator-=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); }) { self -= value; }
					else { self -= type{value}; }
				},
				rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator-(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result -= value;

			return result;
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator*=(const multidimensional<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); }) { lhs *= static_cast<type>(rhs); }
					else { lhs *= type{rhs}; }
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
			requires(
				meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
			)
		[[nodiscard]] constexpr auto operator*(const multidimensional<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result *= other;

			return result;
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		constexpr auto operator*=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); }) { self *= value; }
					else { self *= type{value}; }
				},
				rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator*(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result *= value;

			return result;
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator/=(const multidimensional<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); }) { lhs /= static_cast<type>(rhs); }
					else { lhs /= type{rhs}; }
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator/(const multidimensional<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result /= other;

			return result;
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		constexpr auto operator/=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); }) { self /= value; }
					else { self /= type{value}; }
				},
				rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator/(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result /= value;

			return result;
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator%=(const multidimensional<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); }) { lhs %= static_cast<type>(rhs); }
					else { lhs %= type{rhs}; }
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator%(const multidimensional<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result %= other;

			return result;
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		constexpr auto operator%=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); }) { self %= value; }
					else { self %= type{value}; }
				},
				rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator%(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result %= value;

			return result;
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator&=(const multidimensional<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); }) { lhs &= static_cast<type>(rhs); }
					else { lhs &= type{rhs}; }
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator&(const multidimensional<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result &= other;

			return result;
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		constexpr auto operator&=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); }) { self &= value; }
					else { self &= type{value}; }
				},
				rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator&(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result &= value;

			return result;
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator|=(const multidimensional<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); }) { lhs |= static_cast<type>(rhs); }
					else { lhs |= type{rhs}; }
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator|(const multidimensional<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result |= other;

			return result;
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		constexpr auto operator|=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); }) { self |= value; }
					else { self |= type{value}; }
				},
				rep()
			);
			return rep();
		}

		template<multidimensional_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator|(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result |= value;

			return result;
		}

		template<std::size_t Dimension, multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
			requires (Dimension < meta::member_size<derived_type>())
		[[nodiscard]] constexpr auto compare(const multidimensional<OtherDerived>& other) noexcept -> auto
		{
			const auto v = meta::member_of_index<Dimension>(rep());
			const auto other_v = meta::member_of_index<Dimension>(other.rep());

			return v <=> other_v;
		}

		template<typename Comparator, multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto compare(Comparator comparator, const multidimensional<OtherDerived>& other) noexcept -> bool
		{
			// manually meta::member_walk
			return [&]<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
			{
				const auto f = [&]<std::size_t I>() noexcept -> bool
				{
					return comparator(meta::member_of_index<I>(rep()), meta::member_of_index<I>(other.rep()));
				};

				return (f.template operator()<Index>() and ...);
			}();
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto equal(const multidimensional<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::equal_to{}, other);
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto not_equal(const multidimensional<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::not_equal_to{}, other);
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto greater_than(const multidimensional<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::greater{}, other);
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto greater_equal(const multidimensional<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::greater_equal{}, other);
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto less_than(const multidimensional<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::less{}, other);
		}

		template<multidimensional_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto less_equal(const multidimensional<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::less_equal{}, other);
		}
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
