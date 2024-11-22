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

namespace gal::prometheus::meta
{
	template<typename Dimension>
	struct dimension;

	namespace dimension_detail
	{
		template<typename>
		constexpr auto is_dimension_v = false;

		// template<typename Dimension>
		// constexpr auto is_dimension_v<dimension<Dimension>> = true;

		template<typename Dimension>
			requires std::is_base_of_v<dimension<Dimension>, Dimension>
		constexpr auto is_dimension_v<Dimension> = true;

		template<typename T>
		concept dimension_t = is_dimension_v<T>;

		// fixme:
		//	`dimension + T{value}` will preferentially try to match `dimension + dimension`,
		//	and then generate a compilation error (T in member_size<T> does not satisfy the constraints of member_size)
		template<typename T>
		concept maybe_dimension_t = requires { member_size<T>(); };

		template<typename OtherDimension, typename ThisDimension>
		[[nodiscard]] consteval auto is_compatible_dimension() noexcept -> bool
		{
			if constexpr (member_size<OtherDimension>() != member_size<ThisDimension>())
			{
				return false;
			}
			else
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
				{
					const auto f = []<std::size_t I>() noexcept -> bool
					{
						using this_type = member_type_of_index_t<I, ThisDimension>;
						using other_type = member_type_of_index_t<I, OtherDimension>;

						return
								// implicit
								std::is_convertible_v<other_type, this_type> or
								// explicit
								std::is_constructible_v<this_type, other_type>;
					};

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{});
			}
		}

		template<typename OtherDimension, typename ThisDimension>
		concept compatible_dimension_t =
				maybe_dimension_t<OtherDimension> and
				is_compatible_dimension<OtherDimension, ThisDimension>();

		template<typename DimensionLike, typename Dimension>
		[[nodiscard]] consteval auto is_compatible_dimension_like() noexcept -> bool
		{
			if constexpr (dimension_t<DimensionLike>)
			{
				return false;
			}
			else
			{
				return is_compatible_dimension<DimensionLike, Dimension>();
			}
		}

		template<typename DimensionLike, typename Dimension>
		concept compatible_dimension_like_t =
				maybe_dimension_t<DimensionLike> and
				is_compatible_dimension_like<DimensionLike, Dimension>();

		template<typename Dimension, typename ThisDimension>
		concept compatible_dimension_or_dimension_like_t =
				compatible_dimension_t<Dimension, ThisDimension> or
				compatible_dimension_like_t<Dimension, ThisDimension>;

		template<typename T, typename ThisDimension>
		[[nodiscard]] consteval auto is_compatible_value_type() noexcept -> bool
		{
			return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> bool
			{
				const auto f = []<std::size_t I>() noexcept -> bool
				{
					using this_type = member_type_of_index_t<I, ThisDimension>;

					return
							// implicit
							std::is_convertible_v<T, this_type> or
							// explicit
							std::is_constructible_v<this_type, T>;
				};

				return (f.template operator()<Index>() and ...);
			}(std::make_index_sequence<member_size<ThisDimension>()>{});
		}

		template<typename T, typename ThisDimension>
		concept compatible_value_type_t = is_compatible_value_type<T, ThisDimension>();

		template<typename, typename, typename>
		struct cache : std::false_type {};

		struct tag_assign {};

		struct tag_addition {};

		struct tag_addition_self {};

		struct tag_subtraction {};

		struct tag_subtraction_self {};

		struct tag_multiplication {};

		struct tag_multiplication_self {};

		struct tag_division {};

		struct tag_division_self {};

		struct tag_modulo {};

		struct tag_modulo_self {};

		struct tag_bit_and {};

		struct tag_bit_and_self {};

		struct tag_bit_or {};

		struct tag_bit_or_self {};

		struct tag_bit_xor {};

		struct tag_bit_xor_self {};

		struct tag_bit_flip {};

		// Logical operations are only supported to return boolean types
		template<typename Dimension>
		using logical_operation_result = std::array<bool, member_size<Dimension>()>;

		struct tag_logical_and {};

		struct tag_logical_or {};

		struct tag_logical_not {};

		// ===========================================================================
		// operator+= / operator+

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_addition, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs + rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) +
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_addition, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs + rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) +
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_addition_self, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs += rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) +=
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_addition_self, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs += rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) +=
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator-= / operator-

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_subtraction, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs - rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) -
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_subtraction, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs - rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) -
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_subtraction_self, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs -= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) -=
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_subtraction_self, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs -= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) -=
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator*= / operator*

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_multiplication, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs * rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) *
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_multiplication, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs * rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) *
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_multiplication_self, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs *= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) *=
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_multiplication_self, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs *= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) *=
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator/= / operator/

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_division, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs / rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) /
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_division, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs / rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) /
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_division_self, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs /= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) /=
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_division_self, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs /= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) /=
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator%= / operator%

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_modulo, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs % rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) %
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_modulo, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs % rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) %
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_modulo_self, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs %= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) %=
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_modulo_self, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs %= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) %=
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator&= / operator&

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_bit_and, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs & rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) &
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_bit_and, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs & rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) &
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_bit_and_self, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs &= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) &=
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_bit_and_self, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs &= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) &=
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator|= / operator|

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_bit_or, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs | rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) |
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_bit_or, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs | rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) |
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_bit_or_self, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs |= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) |=
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_bit_or_self, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs |= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) |=
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator^= / operator^

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_bit_xor, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs ^ rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) ^
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_bit_xor, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs = lhs ^ rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								meta::member_of_index<I>(std::declval<const ThisDimension&>()) ^
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_bit_xor_self, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs ^= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) ^=
								meta::member_of_index<I>(std::declval<const OtherDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_bit_xor_self, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs ^= rhs
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) ^=
								std::declval<const T&>();
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator~= / operator~

		template<dimension_t ThisDimension>
		struct cache<tag_bit_flip, ThisDimension, void> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// ~self
							return requires
							{
								meta::member_of_index<I>(std::declval<ThisDimension&>()) =
								~meta::member_of_index<I>(std::declval<const ThisDimension&>());
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator and

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_logical_and, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs and rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) and
									meta::member_of_index<I>(std::declval<const OtherDimension&>())
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_logical_and, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs and rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) and
									std::declval<const T&>()
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator or

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_logical_or, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs or rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) or
									meta::member_of_index<I>(std::declval<const OtherDimension&>())
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_logical_or, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs or rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) or
									std::declval<const T&>()
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator not

		template<dimension_t ThisDimension>
		struct cache<tag_logical_not, ThisDimension, void> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// not (self)
							return requires
							{
								{
									not meta::member_of_index<I>(std::declval<const ThisDimension&>())
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<typename T, typename ThisDimension, typename Tag>
		constexpr auto is_operation_supported_v = cache<Tag, ThisDimension, T>::value;

		template<typename T, typename ThisDimension, typename Tag>
		concept operation_supported_t = is_operation_supported_v<T, ThisDimension, Tag>;

		constexpr auto comparator_equal_to = []<typename L, typename R>(L&& l, R&& r) noexcept -> bool
		{
			if constexpr (std::equality_comparable_with<L, R>)
			{
				return std::ranges::equal_to{}(std::forward<L>(l), std::forward<R>(r));
			}
			else
			{
				return std::equal_to{}(std::forward<L>(l), std::forward<R>(r));
			}
		};
	}

	template<typename Dimension>
	struct [[nodiscard]] dimension // NOLINT(bugprone-crtp-constructor-accessibility)
	{
		template<typename D>
		friend struct dimension;

		using dimension_type = Dimension;

	private:
		[[nodiscard]] constexpr auto rep() noexcept -> dimension_type& { return *static_cast<dimension_type*>(this); }
		[[nodiscard]] constexpr auto rep() const noexcept -> const dimension_type& { return *static_cast<const dimension_type*>(this); }

	public:
		template<std::size_t Index>
		[[nodiscard]] constexpr auto value() noexcept -> decltype(auto)
		{
			return member_of_index<Index>(rep());
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto value() const noexcept -> decltype(auto)
		{
			return member_of_index<Index>(rep());
		}

		template<std::size_t Index>
		using type = member_type_of_index_t<Index, dimension_type>;

	private:
		template<typename Ref, typename Tag>
		struct walker
		{
			Ref& ref; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

			constexpr explicit walker(Ref& d) noexcept //
				requires(dimension_detail::dimension_t<Ref>)
				: ref{d.rep()} {}

			constexpr explicit walker(Ref& d) noexcept //
				requires(not dimension_detail::dimension_t<Ref>)
				: ref{d} {}

		private:
			template<std::size_t Index>
			[[nodiscard]] constexpr auto value_of() noexcept -> decltype(auto)
			{
				using type = std::remove_cvref_t<Ref>;
				if constexpr (dimension_detail::compatible_dimension_or_dimension_like_t<type, dimension_type>)
				{
					return meta::member_of_index<Index>(ref);
				}
				else if constexpr (dimension_detail::compatible_value_type_t<type, dimension_type>)
				{
					return ref;
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

		public:
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH

			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING(4244)
			#endif

			template<std::size_t Index>
			constexpr auto operator()(auto& self) noexcept -> void
			{
				if constexpr (std::is_same_v<Tag, dimension_detail::tag_assign>)
				{
					// to
					value_of<Index>() = self;
				}
				else
				{
					if constexpr (std::is_same_v<Tag, dimension_detail::tag_addition_self>)
					{
						self += value_of<Index>();
					}
					else if constexpr (std::is_same_v<Tag, dimension_detail::tag_subtraction_self>)
					{
						self -= value_of<Index>();
					}
					else if constexpr (std::is_same_v<Tag, dimension_detail::tag_multiplication_self>)
					{
						self *= value_of<Index>();
					}
					else if constexpr (std::is_same_v<Tag, dimension_detail::tag_division_self>)
					{
						self /= value_of<Index>();
					}
					else if constexpr (std::is_same_v<Tag, dimension_detail::tag_modulo_self>)
					{
						self %= value_of<Index>();
					}
					else if constexpr (std::is_same_v<Tag, dimension_detail::tag_bit_and_self>)
					{
						self &= value_of<Index>();
					}
					else if constexpr (std::is_same_v<Tag, dimension_detail::tag_bit_or_self>)
					{
						self |= value_of<Index>();
					}
					else if constexpr (std::is_same_v<Tag, dimension_detail::tag_bit_xor_self>)
					{
						self ^= value_of<Index>();
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}
			}

			template<std::size_t Index>
			constexpr auto operator()(auto& result, const auto& self) noexcept -> void
			{
				if constexpr (std::is_same_v<Tag, dimension_detail::tag_addition>)
				{
					result = self + value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_subtraction>)
				{
					result = self - value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_multiplication>)
				{
					result = self * value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_division>)
				{
					result = self / value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_modulo>)
				{
					result = self % value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_bit_and>)
				{
					result = self & value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_bit_or>)
				{
					result = self | value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_bit_xor>)
				{
					result = self ^ value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_bit_flip>)
				{
					result = ~self;
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_logical_and>)
				{
					result = self and value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_logical_or>)
				{
					result = self or value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, dimension_detail::tag_logical_not>)
				{
					result = not self;
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_POP

			template<typename... Args>
			constexpr auto operator()(Args&&... args) noexcept -> void
			{
				meta::member_zip_walk(*this, std::forward<Args>(args)...);
			}
		};

	public:
		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> TargetDimension = dimension_type>
		[[nodiscard]] constexpr auto to() const noexcept -> TargetDimension
		{
			if constexpr (std::is_same_v<TargetDimension, dimension_type>) { return *this; }
			else
			{
				TargetDimension result;

				walker<TargetDimension, dimension_detail::tag_assign> w{result};
				w(rep());

				return result;
			}
		}

		// ===========================================================================
		// operator+= / operator+

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_addition_self>
		constexpr auto operator+=(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker<const OtherDimension, dimension_detail::tag_addition_self> w{other};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_addition>
		[[nodiscard]] constexpr auto operator+(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const OtherDimension, dimension_detail::tag_addition> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_addition_self>
		constexpr auto operator+=(const T& value) noexcept -> dimension_type&
		{
			walker<const T, dimension_detail::tag_addition_self> w{value};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_addition>
		[[nodiscard]] constexpr auto operator+(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const T, dimension_detail::tag_addition> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator-= / operator-

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_subtraction_self>
		constexpr auto operator-=(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker<const OtherDimension, dimension_detail::tag_subtraction_self> w{other};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_subtraction>
		[[nodiscard]] constexpr auto operator-(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const OtherDimension, dimension_detail::tag_subtraction> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_subtraction_self>
		constexpr auto operator-=(const T& value) noexcept -> dimension_type&
		{
			walker<const T, dimension_detail::tag_subtraction_self> w{value};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_subtraction>
		[[nodiscard]] constexpr auto operator-(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const T, dimension_detail::tag_subtraction> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator*= / operator*

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_multiplication_self>
		constexpr auto operator*=(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker<const OtherDimension, dimension_detail::tag_multiplication_self> w{other};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_multiplication>
		[[nodiscard]] constexpr auto operator*(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const OtherDimension, dimension_detail::tag_multiplication> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_multiplication_self>
		constexpr auto operator*=(const T& value) noexcept -> dimension_type&
		{
			walker<const T, dimension_detail::tag_multiplication_self> w{value};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_multiplication>
		[[nodiscard]] constexpr auto operator*(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const T, dimension_detail::tag_multiplication> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator/= / operator/

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_division_self>
		constexpr auto operator/=(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker<const OtherDimension, dimension_detail::tag_division_self> w{other};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_division>
		[[nodiscard]] constexpr auto operator/(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const OtherDimension, dimension_detail::tag_division> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_division_self>
		constexpr auto operator/=(const T& value) noexcept -> dimension_type&
		{
			walker<const T, dimension_detail::tag_division_self> w{value};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_division>
		[[nodiscard]] constexpr auto operator/(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const T, dimension_detail::tag_division> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator%= / operator%

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_modulo_self>
		constexpr auto operator%=(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker<const OtherDimension, dimension_detail::tag_modulo_self> w{other};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_modulo>
		[[nodiscard]] constexpr auto operator%(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const OtherDimension, dimension_detail::tag_modulo> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_modulo_self>
		constexpr auto operator%=(const T& value) noexcept -> dimension_type&
		{
			walker<const T, dimension_detail::tag_modulo_self> w{value};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_modulo>
		[[nodiscard]] constexpr auto operator%(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const T, dimension_detail::tag_modulo> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator&= / operator&

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_and_self>
		constexpr auto operator&=(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker<const OtherDimension, dimension_detail::tag_bit_and_self> w{other};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_and>
		[[nodiscard]] constexpr auto operator&(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const OtherDimension, dimension_detail::tag_bit_and> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_and_self>
		constexpr auto operator&=(const T& value) noexcept -> dimension_type&
		{
			walker<const T, dimension_detail::tag_bit_and_self> w{value};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_and>
		[[nodiscard]] constexpr auto operator&(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const T, dimension_detail::tag_bit_and> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator|= / operator|

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_or_self>
		constexpr auto operator|=(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker<const OtherDimension, dimension_detail::tag_bit_or_self> w{other};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_or>
		[[nodiscard]] constexpr auto operator|(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const OtherDimension, dimension_detail::tag_bit_or> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_or_self>
		constexpr auto operator|=(const T& value) noexcept -> dimension_type&
		{
			walker<const T, dimension_detail::tag_bit_or_self> w{value};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_or>
		[[nodiscard]] constexpr auto operator|(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const T, dimension_detail::tag_bit_or> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator^= / operator^

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_xor_self>
		constexpr auto operator^=(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker<const OtherDimension, dimension_detail::tag_bit_xor_self> w{other};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_xor>
		[[nodiscard]] constexpr auto operator^(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const OtherDimension, dimension_detail::tag_bit_xor> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_xor_self>
		constexpr auto operator^=(const T& value) noexcept -> dimension_type&
		{
			walker<const T, dimension_detail::tag_bit_xor_self> w{value};
			w(rep());

			return rep();
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_xor>
		[[nodiscard]] constexpr auto operator^(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker<const T, dimension_detail::tag_bit_xor> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator~

		[[nodiscard]] constexpr auto operator~() const noexcept -> dimension_type //
			requires dimension_detail::operation_supported_t<void, dimension_type, dimension_detail::tag_bit_flip>
		{
			dimension_type result{};

			walker<const dimension_type, dimension_detail::tag_bit_flip> w{rep()};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator and

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_logical_and>
		[[nodiscard]] constexpr auto operator and(const OtherDimension& other) const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker<const OtherDimension, dimension_detail::tag_logical_and> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_logical_and>
		constexpr auto operator and(const T& value) const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker<const T, dimension_detail::tag_logical_and> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator or

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_logical_or>
		[[nodiscard]] constexpr auto operator or(const OtherDimension& other) const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker<const OtherDimension, dimension_detail::tag_logical_or> w{other};
			w(result, rep());

			return result;
		}

		template<dimension_detail::compatible_value_type_t<dimension_type> T>
			requires dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_logical_or>
		constexpr auto operator or(const T& value) const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker<const T, dimension_detail::tag_logical_or> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator not

		[[nodiscard]] constexpr auto operator not() const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
			requires dimension_detail::operation_supported_t<void, dimension_type, dimension_detail::tag_logical_not>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker<const dimension_type, dimension_detail::tag_logical_not> w{rep()};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// compare

		template<
			std::size_t Index,
			typename Comparator,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires (Index < member_size<dimension_type>())
		[[nodiscard]] constexpr auto compare(const Comparator& comparator, const OtherDimension& other) const noexcept -> decltype(auto)
		{
			const auto c = [comparator](const auto& self, const auto& o) noexcept -> decltype(auto)
			{
				const auto& lhs = meta::member_of_index<Index>(self);
				const auto& rhs = meta::member_of_index<Index>(o);

				return std::invoke(comparator, lhs, rhs);
			};

			if constexpr (dimension_detail::dimension_t<OtherDimension>)
			{
				return c(rep(), other.rep());
			}
			else
			{
				return c(rep(), other);
			}
		}

		template<
			typename Comparator,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
		[[nodiscard]] constexpr auto compare(const Comparator& comparator, const OtherDimension& other) const noexcept -> auto
		{
			// manually member_walk
			const auto c = [comparator](const auto& self, const auto& o) noexcept -> auto
			{
				const auto cc = [comparator]<std::size_t I>(const auto& cc_self, const auto& cc_o) noexcept -> decltype(auto)
				{
					const auto& lhs = meta::member_of_index<I>(cc_self);
					const auto& rhs = meta::member_of_index<I>(cc_o);

					return std::invoke(comparator, lhs, rhs);
				};

				return [cc]<std::size_t... Index>(std::index_sequence<Index...>, const auto& r_self, const auto& r_o) noexcept -> auto
				{
					return std::array{cc.template operator()<Index>(r_self, r_o)...};
				}(std::make_index_sequence<member_size<dimension_type>()>{}, self, o);
			};

			if constexpr (dimension_detail::dimension_t<OtherDimension>)
			{
				return c(rep(), other.rep());
			}
			else
			{
				return c(rep(), other);
			}
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
		[[nodiscard]] constexpr auto equal(const OtherDimension& other) const noexcept -> bool
		{
			const auto result = this->compare(dimension_detail::comparator_equal_to, other);
			return std::ranges::all_of(result, std::identity{});
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
		[[nodiscard]] constexpr auto not_equal(const OtherDimension& other) const noexcept -> bool
		{
			const auto result = this->compare(std::ranges::not_equal_to{}, other);
			return std::ranges::all_of(result, std::identity{});
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
		[[nodiscard]] constexpr auto greater_than(const OtherDimension& other) const noexcept -> bool
		{
			const auto result = this->compare(std::ranges::greater{}, other);
			return std::ranges::all_of(result, std::identity{});
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
		[[nodiscard]] constexpr auto greater_equal(const OtherDimension& other) const noexcept -> bool
		{
			const auto result = this->compare(std::ranges::greater_equal{}, other);
			return std::ranges::all_of(result, std::identity{});
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
		[[nodiscard]] constexpr auto less_than(const OtherDimension& other) const noexcept -> bool
		{
			const auto result = this->compare(std::ranges::less{}, other);
			return std::ranges::all_of(result, std::identity{});
		}

		template<dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
		[[nodiscard]] constexpr auto less_equal(const OtherDimension& other) const noexcept -> bool
		{
			const auto result = this->compare(std::ranges::less_equal{}, other);
			return std::ranges::all_of(result, std::identity{});
		}
	};

	template<dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension>
	[[nodiscard]] constexpr auto operator==(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> bool
	{
		return lhs.equal(rhs);
	}

	template<dimension_detail::dimension_t RightDimension, dimension_detail::compatible_dimension_like_t<RightDimension> LeftDimension>
	[[nodiscard]] constexpr auto operator==(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> bool
	{
		return rhs.equal(lhs);
	}
}
