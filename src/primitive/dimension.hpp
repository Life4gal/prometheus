// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <utility>
#include <ranges>
#include <functional>

#include <prometheus/macro.hpp>

#include <meta/member.hpp>

namespace gal::prometheus::primitive
{
	namespace dimension_detail
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

	template<typename Derived>
	struct [[nodiscard]] dimension
	{
		template<typename D>
		friend struct dimension;

		using derived_type = Derived;

	private:
		[[nodiscard]] constexpr auto rep() noexcept -> derived_type& { return *static_cast<derived_type*>(this); }
		[[nodiscard]] constexpr auto rep() const noexcept -> const derived_type& { return *static_cast<const derived_type*>(this); }

	public:
		template<std::size_t Index>
		[[nodiscard]] constexpr auto value() noexcept -> decltype(auto)
		{
			return meta::member_of_index<Index>(rep());
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto value() const noexcept -> decltype(auto)
		{
			return meta::member_of_index<Index>(rep());
		}

		template<std::size_t Index>
		using type = meta::member_type_of_index_t<Index, derived_type>;

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived>
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
						if constexpr (requires { static_cast<type>(rhs); })
						{
							lhs = static_cast<type>(rhs);
						}
						else
						{
							lhs = type{rhs};
						}
					},
					result,
					rep()
				);

				return result;
			}
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator+=(const dimension<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); })
					{
						lhs += static_cast<type>(rhs);
					}
					else
					{
						lhs += type{rhs};
					}
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator+(const dimension<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result += other;

			return result;
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		constexpr auto operator+=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); })
					{
						self += value;
					}
					else
					{
						self += type{value};
					}
				},
				rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator+(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result += value;

			return result;
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator-=(const dimension<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); })
					{
						lhs -= static_cast<type>(rhs);
					}
					else
					{
						lhs -= type{rhs};
					}
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator-(const dimension<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result -= other;

			return result;
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		constexpr auto operator-=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); })
					{
						self -= value;
					}
					else
					{
						self -= type{value};
					}
				},
				rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator-(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result -= value;

			return result;
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator*=(const dimension<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); })
					{
						lhs *= static_cast<type>(rhs);
					}
					else
					{
						lhs *= type{rhs};
					}
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
			requires(
				meta::member_size<OtherDerived>() == meta::member_size<derived_type>()
			)
		[[nodiscard]] constexpr auto operator*(const dimension<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result *= other;

			return result;
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		constexpr auto operator*=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); })
					{
						self *= value;
					}
					else
					{
						self *= type{value};
					}
				},
				rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator*(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result *= value;

			return result;
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator/=(const dimension<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); })
					{
						lhs /= static_cast<type>(rhs);
					}
					else
					{
						lhs /= type{rhs};
					}
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator/(const dimension<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result /= other;

			return result;
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		constexpr auto operator/=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); })
					{
						self /= value;
					}
					else
					{
						self /= type{value};
					}
				},
				rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator/(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result /= value;

			return result;
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator%=(const dimension<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); })
					{
						lhs %= static_cast<type>(rhs);
					}
					else
					{
						lhs %= type{rhs};
					}
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator%(const dimension<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result %= other;

			return result;
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		constexpr auto operator%=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); })
					{
						self %= value;
					}
					else
					{
						self %= type{value};
					}
				},
				rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator%(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result %= value;

			return result;
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator&=(const dimension<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); })
					{
						lhs &= static_cast<type>(rhs);
					}
					else
					{
						lhs &= type{rhs};
					}
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator&(const dimension<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result &= other;

			return result;
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		constexpr auto operator&=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); })
					{
						self &= value;
					}
					else
					{
						self &= type{value};
					}
				},
				rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator&(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result &= value;

			return result;
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		constexpr auto operator|=(const dimension<OtherDerived>& other) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[]<std::size_t Index>(auto& lhs, const auto& rhs) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(rhs); })
					{
						lhs |= static_cast<type>(rhs);
					}
					else
					{
						lhs |= type{rhs};
					}
				},
				rep(),
				other.rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto operator|(const dimension<OtherDerived>& other) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result |= other;

			return result;
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		constexpr auto operator|=(const T& value) noexcept -> derived_type&
		{
			meta::member_zip_walk(
				[&value]<std::size_t Index>(auto& self) noexcept -> void //
				{
					using type = meta::member_type_of_index_t<Index, derived_type>;
					if constexpr (requires { static_cast<type>(value); })
					{
						self |= value;
					}
					else
					{
						self |= type{value};
					}
				},
				rep()
			);
			return rep();
		}

		template<dimension_detail::convertible_type_t<derived_type> T>
		[[nodiscard]] constexpr auto operator|(const T& value) const noexcept -> derived_type
		{
			derived_type result{rep()};

			result |= value;

			return result;
		}

		template<std::size_t Dimension, dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
			requires (Dimension < meta::member_size<derived_type>())
		[[nodiscard]] constexpr auto compare(const dimension<OtherDerived>& other) noexcept -> auto
		{
			const auto v = meta::member_of_index<Dimension>(rep());
			const auto other_v = meta::member_of_index<Dimension>(other.rep());

			return v <=> other_v;
		}

		template<typename Comparator, dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto compare(Comparator comparator, const dimension<OtherDerived>& other) noexcept -> bool
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

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto equal(const dimension<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::equal_to{}, other);
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto not_equal(const dimension<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::not_equal_to{}, other);
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto greater_than(const dimension<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::greater{}, other);
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto greater_equal(const dimension<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::greater_equal{}, other);
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto less_than(const dimension<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::less{}, other);
		}

		template<dimension_detail::convertible_derived_t<derived_type> OtherDerived = derived_type>
		[[nodiscard]] constexpr auto less_equal(const dimension<OtherDerived>& other) noexcept -> bool
		{
			return this->compare(std::ranges::less_equal{}, other);
		}
	};
}
