// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:multidimensional;

import std;
import gal.prometheus.meta;

namespace gal::prometheus::primitive
{
	namespace multidimensional_detail
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

	export
	{
		enum class Dimension
		{
			_0 = 0,
			_1 = 1,
			_2 = 2,
			_3 = 3,
		};

		template<typename T, typename Derived>
			requires std::is_arithmetic_v<T>
		struct [[nodiscard]] multidimensional
		{
			template<typename U, typename D>
				requires std::is_arithmetic_v<U>
			friend struct multidimensional;

			using value_type = T;

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
					using type = typename OtherDerived::value_type;

					OtherDerived result;

					meta::member_for_each(
							[]<std::size_t Index>(auto& lhs, const auto rhs) noexcept -> void { lhs = static_cast<type>(rhs); },
							result,
							rep()
							);

					return result;
				}
			}

			template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
				requires(
					std::is_same_v<OtherDerived, derived_type> or
					(
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			[[nodiscard]] constexpr auto operator+(const multidimensional<U, OtherDerived>& other) const noexcept -> derived_type
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			constexpr auto operator+=(const multidimensional<U, OtherDerived>& other) noexcept -> derived_type&
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			[[nodiscard]] constexpr auto operator-(const multidimensional<U, OtherDerived>& other) const noexcept -> derived_type
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			constexpr auto operator-=(const multidimensional<U, OtherDerived>& other) noexcept -> derived_type&
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			[[nodiscard]] constexpr auto operator*(const multidimensional<U, OtherDerived>& other) const noexcept -> derived_type
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			constexpr auto operator*=(const multidimensional<U, OtherDerived>& other) noexcept -> derived_type&
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			[[nodiscard]] constexpr auto operator/(const multidimensional<U, OtherDerived>& other) const noexcept -> derived_type
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			constexpr auto operator/=(const multidimensional<U, OtherDerived>& other) noexcept -> derived_type&
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
						        multidimensional_detail::always_equal_t<OtherDerived> and
						        multidimensional_detail::always_equal_t<derived_type> and
						        meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					        )
				        ) and
				        std::three_way_comparable_with<value_type, U>
			[[nodiscard]] constexpr auto compare(const multidimensional<U, OtherDerived>& other) noexcept -> auto
			{
				const auto v = meta::member_of_index<static_cast<std::size_t>(D)>(rep());
				const auto other_v = meta::member_of_index<static_cast<std::size_t>(D)>(other.rep());

				return v <=> other_v;
			}

			template<std::convertible_to<value_type> U = value_type, typename OtherDerived = derived_type>
				requires(
					std::is_same_v<OtherDerived, derived_type> or
					(
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			[[nodiscard]] constexpr auto exact_equal(const multidimensional<U, OtherDerived>& other) noexcept -> bool
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			[[nodiscard]] constexpr auto exact_greater_than(const multidimensional<U, OtherDerived>& other) noexcept -> bool
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			[[nodiscard]] constexpr auto exact_greater_equal(const multidimensional<U, OtherDerived>& other) noexcept -> bool
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			[[nodiscard]] constexpr auto exact_less_than(const multidimensional<U, OtherDerived>& other) noexcept -> bool
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
						multidimensional_detail::always_equal_t<OtherDerived> and
						multidimensional_detail::always_equal_t<derived_type> and
						meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
					)
				)
			[[nodiscard]] constexpr auto exact_less_equal(const multidimensional<U, OtherDerived>& other) noexcept -> bool
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
}
