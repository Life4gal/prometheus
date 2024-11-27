// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <utility>
#include <ranges>
#include <functional>
#include <algorithm>

#include <prometheus/macro.hpp>

#include <meta/member.hpp>

namespace gal::prometheus::meta
{
	enum class Dimensions : std::size_t
	{
		ALL = std::numeric_limits<std::size_t>::max(),
	};

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

		struct tag_transform {};

		struct tag_addition {};

		struct tag_addition_self {};

		struct tag_subtraction {};

		struct tag_subtraction_self {};

		struct tag_multiplication {};

		struct tag_multiplication_self {};

		struct tag_division {};

		struct tag_division_self {};

		struct tag_modulus {};

		struct tag_modulus_self {};

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

		// Compare operations are only supported to return boolean types
		template<typename Dimension>
		using compare_operation_result = std::array<bool, member_size<Dimension>()>;

		struct tag_compare_equal {};

		struct tag_compare_not_equal {};

		struct tag_compare_greater_than {};

		struct tag_compare_greater_equal {};

		struct tag_compare_less_than {};

		struct tag_compare_less_equal {};

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
		struct cache<tag_modulus, ThisDimension, OtherDimension> : std::bool_constant<
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
		struct cache<tag_modulus, ThisDimension, T> : std::bool_constant<
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
		struct cache<tag_modulus_self, ThisDimension, OtherDimension> : std::bool_constant<
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
		struct cache<tag_modulus_self, ThisDimension, T> : std::bool_constant<
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

		// ===========================================================================
		// operator==

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_compare_equal, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs == rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) ==
									meta::member_of_index<I>(std::declval<const OtherDimension&>())
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_compare_equal, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs == rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) ==
									std::declval<const T&>()
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator!=

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_compare_not_equal, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs != rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) !=
									meta::member_of_index<I>(std::declval<const OtherDimension&>())
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_compare_not_equal, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs != rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) !=
									std::declval<const T&>()
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator>

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_compare_greater_than, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs > rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) >
									meta::member_of_index<I>(std::declval<const OtherDimension&>())
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_compare_greater_than, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs > rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) >
									std::declval<const T&>()
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator>=

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_compare_greater_equal, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs >= rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) >=
									meta::member_of_index<I>(std::declval<const OtherDimension&>())
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_compare_greater_equal, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs >= rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) >=
									std::declval<const T&>()
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator<

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_compare_less_than, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs < rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) <
									meta::member_of_index<I>(std::declval<const OtherDimension&>())
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_compare_less_than, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs < rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) <
									std::declval<const T&>()
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		// ===========================================================================
		// operator<=

		template<dimension_t ThisDimension, compatible_dimension_or_dimension_like_t<ThisDimension> OtherDimension>
		struct cache<tag_compare_less_equal, ThisDimension, OtherDimension> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs <= rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) <=
									meta::member_of_index<I>(std::declval<const OtherDimension&>())
								} -> std::convertible_to<bool>;
							};
						};

						return (f.template operator()<Index>() and ...);
					}(std::make_index_sequence<member_size<ThisDimension>()>{})
				> {};

		template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
		struct cache<tag_compare_less_equal, ThisDimension, T> : std::bool_constant<
					// manually member_walk
					[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
					{
						constexpr auto f = []<std::size_t I>() noexcept -> bool
						{
							// lhs <= rhs
							return requires
							{
								{
									meta::member_of_index<I>(std::declval<const ThisDimension&>()) <=
									std::declval<const T&>()
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

		template<typename TargetDimension>
		struct identity_caster
		{
			template<std::size_t Index>
			[[nodiscard]] constexpr auto operator()(const auto& self) const noexcept -> decltype(auto)
			{
				return static_cast<member_type_of_index_t<Index, TargetDimension>>(self);
			}
		};

		struct boolean_caster
		{
			template<std::size_t Index>
			[[nodiscard]] constexpr auto operator()(const auto& self) const noexcept -> bool
			{
				std::ignore = Index;
				return static_cast<bool>(self);
			}
		};

		struct empty {};

		template<typename T>
		using walker_value_holder = std::conditional_t<std::is_same_v<T, void>, empty, std::reference_wrapper<T>>;

		template<typename T>
		using walker_value_parameter = std::conditional_t<std::is_same_v<T, void>, empty, std::add_lvalue_reference_t<T>>;

		template<typename T, typename Tag, typename ThisDimension, Dimensions D>
		struct walker
		{
			walker_value_holder<T> holder;

			constexpr explicit walker() noexcept //
				requires (std::is_same_v<T, void>)
				: holder{} {}

			constexpr explicit walker(walker_value_parameter<T> d) noexcept //
				requires(not std::is_same_v<T, void> and dimension_detail::dimension_t<std::remove_cvref_t<T>>)
				: holder{d.rep()} {}

			constexpr explicit walker(walker_value_parameter<T> d) noexcept //
				requires(not std::is_same_v<T, void> and not dimension_detail::dimension_t<std::remove_cvref_t<T>>)
				: holder{d} {}

		private:
			template<std::size_t Index>
			[[nodiscard]] constexpr auto value_of() const noexcept -> decltype(auto)
			{
				using type = std::remove_cvref_t<T>;
				if constexpr (dimension_detail::compatible_dimension_or_dimension_like_t<type, ThisDimension>)
				{
					return meta::member_of_index<Index>(holder.get());
				}
				else if constexpr (dimension_detail::compatible_value_type_t<type, ThisDimension>)
				{
					return holder.get();
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
			constexpr auto operator()(auto& self) const noexcept -> void
			{
				if constexpr (std::is_same_v<Tag, tag_addition_self>)
				{
					self += value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_subtraction_self>)
				{
					self -= value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_multiplication_self>)
				{
					self *= value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_division_self>)
				{
					self /= value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_modulus_self>)
				{
					self %= value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_bit_and_self>)
				{
					self &= value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_bit_or_self>)
				{
					self |= value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_bit_xor_self>)
				{
					self ^= value_of<Index>();
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			template<std::size_t Index>
			constexpr auto operator()(auto& result, const auto& self) const noexcept -> void
			{
				if constexpr (std::is_same_v<Tag, tag_transform>)
				{
					if constexpr (requires { holder.get()(self); })
					{
						result = holder.get()(self);
					}
					else if constexpr (requires { holder.get().template operator()<Index>(self); })
					{
						result = holder.get().template operator()<Index>(self);
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}
				else if constexpr (std::is_same_v<Tag, tag_addition>)
				{
					result = self + value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_subtraction>)
				{
					result = self - value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_multiplication>)
				{
					result = self * value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_division>)
				{
					result = self / value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_modulus>)
				{
					result = self % value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_bit_and>)
				{
					result = self & value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_bit_or>)
				{
					result = self | value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_bit_xor>)
				{
					result = self ^ value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_bit_flip>)
				{
					result = ~self;
				}
				else if constexpr (std::is_same_v<Tag, tag_logical_and>)
				{
					result = self and value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_logical_or>)
				{
					result = self or value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_logical_not>)
				{
					result = not self;
				}
				else if constexpr (std::is_same_v<Tag, tag_compare_equal>)
				{
					result = self == value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_compare_not_equal>)
				{
					result = self != value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_compare_greater_than>)
				{
					result = self > value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_compare_greater_equal>)
				{
					result = self >= value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_compare_less_than>)
				{
					result = self < value_of<Index>();
				}
				else if constexpr (std::is_same_v<Tag, tag_compare_less_equal>)
				{
					result = self <= value_of<Index>();
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}

			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_POP

			template<typename... Args>
			constexpr auto operator()(Args&&... args) const noexcept -> void
			{
				if constexpr (D == Dimensions::ALL)
				{
					meta::member_zip_walk(*this, std::forward<Args>(args)...);
				}
				else
				{
					this->template operator()<static_cast<std::size_t>(D)>(std::forward<Args>(args)...);
				}
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
		template<typename T, typename Tag, Dimensions D>
		using walker_type = dimension_detail::walker<T, Tag, dimension_type, D>;

		template<typename T, typename Tag, typename ThisDimension, Dimensions D>
		friend struct dimension_detail::walker;

		[[nodiscard]] constexpr auto rep() noexcept -> dimension_type& { return *static_cast<dimension_type*>(this); }
		[[nodiscard]] constexpr auto rep() const noexcept -> const dimension_type& { return *static_cast<const dimension_type*>(this); }

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
		using type = member_type_of_index_t<Index, dimension_type>;

		// ===========================================================================
		// transform

		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> TargetDimension = dimension_type,
			typename TransformFunction = dimension_detail::identity_caster<TargetDimension>>
		[[nodiscard]] constexpr auto transform(const TransformFunction& transform_function = {}) const noexcept -> TargetDimension
		{
			TargetDimension result{};

			walker_type<const TransformFunction, dimension_detail::tag_transform, Dimensions::ALL> w{transform_function};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// all / any / none

		[[nodiscard]] constexpr auto all() const noexcept -> bool
		{
			const auto result = transform<dimension_detail::compare_operation_result<dimension_type>, dimension_detail::boolean_caster>();
			return std::ranges::all_of(result, std::identity{});
		}

		[[nodiscard]] constexpr auto any() const noexcept -> bool
		{
			const auto result = transform<dimension_detail::compare_operation_result<dimension_type>, dimension_detail::boolean_caster>();
			return std::ranges::any_of(result, std::identity{});
		}

		[[nodiscard]] constexpr auto none() const noexcept -> bool
		{
			const auto result = transform<dimension_detail::compare_operation_result<dimension_type>, dimension_detail::boolean_caster>();
			return std::ranges::none_of(result, std::identity{});
		}

		// ===========================================================================
		// operator+= / operator+

		// dimension += dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_addition_self>
			)
		constexpr auto add_equal(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker_type<const OtherDimension, dimension_detail::tag_addition_self, D> w{other};
			w(rep());

			return rep();
		}

		// dimension + dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_addition>
			)
		[[nodiscard]] constexpr auto add(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const OtherDimension, dimension_detail::tag_addition, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension += value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_addition_self>
			)
		constexpr auto add_equal(const T& value) noexcept -> dimension_type&
		{
			walker_type<const T, dimension_detail::tag_addition_self, D> w{value};
			w(rep());

			return rep();
		}

		// dimension + value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_addition>
			)
		[[nodiscard]] constexpr auto add(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const T, dimension_detail::tag_addition, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator-= / operator-

		// dimension -= dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_subtraction_self>
			)
		constexpr auto subtract_equal(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker_type<const OtherDimension, dimension_detail::tag_subtraction_self, D> w{other};
			w(rep());

			return rep();
		}

		// dimension - dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_subtraction>
			)
		[[nodiscard]] constexpr auto subtract(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const OtherDimension, dimension_detail::tag_subtraction, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension -= value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_subtraction_self>
			)
		constexpr auto subtract_equal(const T& value) noexcept -> dimension_type&
		{
			walker_type<const T, dimension_detail::tag_subtraction_self, D> w{value};
			w(rep());

			return rep();
		}

		// dimension - value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_subtraction>
			)
		[[nodiscard]] constexpr auto subtract(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const T, dimension_detail::tag_subtraction, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator*= / operator*

		// dimension *= dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_multiplication_self>
			)
		constexpr auto multiply_equal(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker_type<const OtherDimension, dimension_detail::tag_multiplication_self, D> w{other};
			w(rep());

			return rep();
		}

		// dimension * dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_multiplication>
			)
		[[nodiscard]] constexpr auto multiply(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const OtherDimension, dimension_detail::tag_multiplication, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension *= value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_multiplication_self>
			)
		constexpr auto multiply_equal(const T& value) noexcept -> dimension_type&
		{
			walker_type<const T, dimension_detail::tag_multiplication_self, D> w{value};
			w(rep());

			return rep();
		}

		// dimension * value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_multiplication>
			)
		[[nodiscard]] constexpr auto multiply(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const T, dimension_detail::tag_multiplication, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator/= / operator/

		// dimension /= dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_division_self>
			)
		constexpr auto divide_equal(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker_type<const OtherDimension, dimension_detail::tag_division_self, D> w{other};
			w(rep());

			return rep();
		}

		// dimension / dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_division>
			)
		[[nodiscard]] constexpr auto divide(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const OtherDimension, dimension_detail::tag_division, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension /= value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_division_self>
			)
		constexpr auto divide_equal(const T& value) noexcept -> dimension_type&
		{
			walker_type<const T, dimension_detail::tag_division_self, D> w{value};
			w(rep());

			return rep();
		}

		// dimension / value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_division>
			)
		[[nodiscard]] constexpr auto divide(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const T, dimension_detail::tag_division, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator%= / operator%

		// dimension %= dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_modulus_self>
			)
		constexpr auto mod_equal(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker_type<const OtherDimension, dimension_detail::tag_modulus_self, D> w{other};
			w(rep());

			return rep();
		}

		// dimension % dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_modulus>
			)
		[[nodiscard]] constexpr auto mod(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const OtherDimension, dimension_detail::tag_modulus, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension %= value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_modulus_self>
			)
		constexpr auto mod_equal(const T& value) noexcept -> dimension_type&
		{
			walker_type<const T, dimension_detail::tag_modulus_self, D> w{value};
			w(rep());

			return rep();
		}

		// dimension % value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_modulus>
			)
		[[nodiscard]] constexpr auto mod(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const T, dimension_detail::tag_modulus, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator&= / operator&

		// dimension &= dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_and_self>
			)
		constexpr auto bit_and_equal(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker_type<const OtherDimension, dimension_detail::tag_bit_and_self, D> w{other};
			w(rep());

			return rep();
		}

		// dimension & dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_and>
			)
		[[nodiscard]] constexpr auto bit_and(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const OtherDimension, dimension_detail::tag_bit_and, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension &= value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_and_self>
			)
		constexpr auto bit_and_equal(const T& value) noexcept -> dimension_type&
		{
			walker_type<const T, dimension_detail::tag_bit_and_self, D> w{value};
			w(rep());

			return rep();
		}

		// dimension & value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_and>
			)
		[[nodiscard]] constexpr auto bit_and(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const T, dimension_detail::tag_bit_and, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator|= / operator|

		// dimension |= dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_or_self>
			)
		constexpr auto bit_or_equal(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker_type<const OtherDimension, dimension_detail::tag_bit_or_self, D> w{other};
			w(rep());

			return rep();
		}

		// dimension | dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_or>
			)
		[[nodiscard]] constexpr auto bit_or(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const OtherDimension, dimension_detail::tag_bit_or, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension |= value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_or_self>
			)
		constexpr auto bit_or_equal(const T& value) noexcept -> dimension_type&
		{
			walker_type<const T, dimension_detail::tag_bit_or_self, D> w{value};
			w(rep());

			return rep();
		}

		// dimension | value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_or>
			)
		[[nodiscard]] constexpr auto bit_or(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const T, dimension_detail::tag_bit_or, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator^= / operator^

		// dimension ^= dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_xor_self>
			)
		constexpr auto bit_xor_equal(const OtherDimension& other) noexcept -> dimension_type&
		{
			walker_type<const OtherDimension, dimension_detail::tag_bit_xor_self, D> w{other};
			w(rep());

			return rep();
		}

		// dimension ^ dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_bit_xor>
			)
		[[nodiscard]] constexpr auto bit_xor(const OtherDimension& other) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const OtherDimension, dimension_detail::tag_bit_xor, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension ^= value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_xor_self>
			)
		constexpr auto bit_xor_equal(const T& value) noexcept -> dimension_type&
		{
			walker_type<const T, dimension_detail::tag_bit_xor_self, D> w{value};
			w(rep());

			return rep();
		}

		// dimension ^ value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_bit_xor>
			)
		[[nodiscard]] constexpr auto bit_xor(const T& value) const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<const T, dimension_detail::tag_bit_xor, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator~

		// ~dimension
		template<
			Dimensions D = Dimensions::ALL
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<void, dimension_type, dimension_detail::tag_bit_flip>
			)
		[[nodiscard]] constexpr auto bit_flip() const noexcept -> dimension_type
		{
			dimension_type result{};

			walker_type<void, dimension_detail::tag_bit_flip, D> w{};
			w(result, rep());

			return result;
		}


		// ===========================================================================
		// operator and

		// dimension and dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_logical_and>
			)
		[[nodiscard]] constexpr auto logical_and(const OtherDimension& other) const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker_type<const OtherDimension, dimension_detail::tag_logical_and, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension and value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_logical_and>
			)
		[[nodiscard]] constexpr auto logical_and(const T& value) const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker_type<const T, dimension_detail::tag_logical_and, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator or

		// dimension or dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_logical_or>
			)
		[[nodiscard]] constexpr auto logical_or(const OtherDimension& other) const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker_type<const OtherDimension, dimension_detail::tag_logical_or, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension or value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_logical_or>
			)
		[[nodiscard]] constexpr auto logical_or(const T& value) const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker_type<const T, dimension_detail::tag_logical_or, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator not

		// not dimension
		template<
			Dimensions D = Dimensions::ALL
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<void, dimension_type, dimension_detail::tag_logical_not>
			)
		[[nodiscard]] constexpr auto logical_not() const noexcept -> auto // dimension_detail::logical_operation_result<dimension_type>
		{
			dimension_detail::logical_operation_result<dimension_type> result{};

			walker_type<void, dimension_detail::tag_logical_not, D> w{};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator==

		// dimension == dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_equal>
			)
		[[nodiscard]] constexpr auto equal(const OtherDimension& other) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const OtherDimension, dimension_detail::tag_compare_equal, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension == value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_equal>
			)
		[[nodiscard]] constexpr auto equal(const T& value) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const T, dimension_detail::tag_compare_equal, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator!=

		// dimension != dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_not_equal>
			)
		constexpr auto not_equal(const OtherDimension& other) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const OtherDimension, dimension_detail::tag_compare_not_equal, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension != value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_not_equal>
			)
		[[nodiscard]] constexpr auto not_equal(const T& value) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const T, dimension_detail::tag_compare_not_equal, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator>

		// dimension > dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_greater_than>
			)
		constexpr auto greater_than(const OtherDimension& other) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const OtherDimension, dimension_detail::tag_compare_greater_than, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension > value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_greater_than>
			)
		[[nodiscard]] constexpr auto greater_than(const T& value) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const T, dimension_detail::tag_compare_greater_than, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator>=

		// dimension >= dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_greater_equal>
			)
		constexpr auto greater_equal(const OtherDimension& other) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const OtherDimension, dimension_detail::tag_compare_greater_equal, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension >= value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_greater_equal>
			)
		[[nodiscard]] constexpr auto greater_equal(const T& value) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const T, dimension_detail::tag_compare_greater_equal, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator<

		// dimension < dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_less_than>
			)
		constexpr auto less_than(const OtherDimension& other) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const OtherDimension, dimension_detail::tag_compare_less_than, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension < value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_less_than>
			)
		[[nodiscard]] constexpr auto less_than(const T& value) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const T, dimension_detail::tag_compare_less_than, D> w{value};
			w(result, rep());

			return result;
		}

		// ===========================================================================
		// operator<=

		// dimension <= dimension / dimension_like
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_less_equal>
			)
		constexpr auto less_equal(const OtherDimension& other) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const OtherDimension, dimension_detail::tag_compare_less_equal, D> w{other};
			w(result, rep());

			return result;
		}

		// dimension <= value
		template<
			Dimensions D = Dimensions::ALL,
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				(D == Dimensions::ALL or static_cast<std::size_t>(D) < meta::member_size<dimension_type>()) and
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_less_equal>
			)
		[[nodiscard]] constexpr auto less_equal(const T& value) const noexcept -> auto // dimension_detail::compare_operation_result<dimension_type>
		{
			dimension_detail::compare_operation_result<dimension_type> result{};

			walker_type<const T, dimension_detail::tag_compare_less_equal, D> w{value};
			w(result, rep());

			return result;
		}
	};

	// ===========================================================================
	// operator+= / operator+

	// dimension += dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_addition_self>
		)
	constexpr auto operator+=(LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension&
	{
		return lhs.add_equal(rhs);
	}

	// dimension + dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_addition>
		)
	[[nodiscard]] constexpr auto operator+(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension
	{
		return lhs.add(rhs);
	}

	// dimension += value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_addition_self>
		)
	constexpr auto operator+=(LeftDimension& lhs, const T& value) noexcept -> LeftDimension&
	{
		return lhs.add_equal(value);
	}

	// dimension + value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_addition>
		)
	[[nodiscard]] constexpr auto operator+(const LeftDimension& lhs, const T& value) noexcept -> LeftDimension
	{
		return lhs.add(value);
	}

	// ===========================================================================
	// operator-= / operator-

	// dimension -= dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_subtraction_self>
		)
	constexpr auto operator-=(LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension&
	{
		return lhs.subtract_equal(rhs);
	}

	// dimension - dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_subtraction>
		)
	[[nodiscard]] constexpr auto operator-(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension
	{
		return lhs.subtract(rhs);
	}

	// dimension -= value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_subtraction_self>
		)
	constexpr auto operator-=(LeftDimension& lhs, const T& value) noexcept -> LeftDimension&
	{
		return lhs.subtract_equal(value);
	}

	// dimension - value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_subtraction>
		)
	[[nodiscard]] constexpr auto operator-(const LeftDimension& lhs, const T& value) noexcept -> LeftDimension
	{
		return lhs.subtract(value);
	}

	// ===========================================================================
	// operator*= / operator*

	// dimension *= dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_multiplication_self>
		)
	constexpr auto operator*=(LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension&
	{
		return lhs.multiply_equal(rhs);
	}

	// dimension * dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_multiplication>
		)
	[[nodiscard]] constexpr auto operator*(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension
	{
		return lhs.multiply(rhs);
	}

	// dimension *= value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_multiplication_self>
		)
	constexpr auto operator*=(LeftDimension& lhs, const T& value) noexcept -> LeftDimension&
	{
		return lhs.multiply_equal(value);
	}

	// dimension * value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_multiplication>
		)
	[[nodiscard]] constexpr auto operator*(const LeftDimension& lhs, const T& value) noexcept -> LeftDimension
	{
		return lhs.multiply(value);
	}

	// ===========================================================================
	// operator/= / operator/

	// dimension /= dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_division_self>
		)
	constexpr auto operator/=(LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension&
	{
		return lhs.divide_equal(rhs);
	}

	// dimension / dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_division>
		)
	[[nodiscard]] constexpr auto operator/(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension
	{
		return lhs.divide(rhs);
	}

	// dimension /= value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_division_self>
		)
	constexpr auto operator/=(LeftDimension& lhs, const T& value) noexcept -> LeftDimension&
	{
		return lhs.divide_equal(value);
	}

	// dimension / value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_division>
		)
	[[nodiscard]] constexpr auto operator/(const LeftDimension& lhs, const T& value) noexcept -> LeftDimension
	{
		return lhs.divide(value);
	}

	// ===========================================================================
	// operator%= / operator%

	// dimension %= dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_modulus_self>
		)
	constexpr auto operator%=(LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension&
	{
		return lhs.mod_equal(rhs);
	}

	// dimension % dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_modulus>
		)
	[[nodiscard]] constexpr auto operator%(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension
	{
		return lhs.mod(rhs);
	}

	// dimension %= value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_modulus_self>
		)
	constexpr auto operator%=(LeftDimension& lhs, const T& value) noexcept -> LeftDimension&
	{
		return lhs.mod_equal(value);
	}

	// dimension % value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_modulus>
		)
	[[nodiscard]] constexpr auto operator%(const LeftDimension& lhs, const T& value) noexcept -> LeftDimension
	{
		return lhs.mod(value);
	}

	// ===========================================================================
	// operator&= / operator&

	// dimension &= dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_bit_and_self>
		)
	constexpr auto operator&=(LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension&
	{
		return lhs.bit_and_equal(rhs);
	}

	// dimension & dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_bit_and>
		)
	[[nodiscard]] constexpr auto operator&(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension
	{
		return lhs.bit_and(rhs);
	}

	// dimension &= value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_bit_and_self>
		)
	constexpr auto operator&=(LeftDimension& lhs, const T& value) noexcept -> LeftDimension&
	{
		return lhs.bit_and_equal(value);
	}

	// dimension & value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_bit_and>
		)
	[[nodiscard]] constexpr auto operator&(const LeftDimension& lhs, const T& value) noexcept -> LeftDimension
	{
		return lhs.bit_and(value);
	}

	// ===========================================================================
	// operator|= / operator|

	// dimension |= dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_bit_or_self>
		)
	constexpr auto operator|=(LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension&
	{
		return lhs.bit_or_equal(rhs);
	}

	// dimension | dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_bit_or>
		)
	[[nodiscard]] constexpr auto operator|(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension
	{
		return lhs.bit_or(rhs);
	}

	// dimension |= value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_bit_or_self>
		)
	constexpr auto operator|=(LeftDimension& lhs, const T& value) noexcept -> LeftDimension&
	{
		return lhs.bit_or_equal(value);
	}

	// dimension | value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_bit_or>
		)
	[[nodiscard]] constexpr auto operator|(const LeftDimension& lhs, const T& value) noexcept -> LeftDimension
	{
		return lhs.bit_or(value);
	}

	// ===========================================================================
	// operator^= / operator|

	// dimension ^= dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_bit_xor_self>
		)
	constexpr auto operator^=(LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension&
	{
		return lhs.bit_xor_equal(rhs);
	}

	// dimension ^ dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_bit_xor>
		)
	[[nodiscard]] constexpr auto operator^(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> LeftDimension
	{
		return lhs.bit_xor(rhs);
	}

	// dimension ^= value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_bit_xor_self>
		)
	constexpr auto operator^=(LeftDimension& lhs, const T& value) noexcept -> LeftDimension&
	{
		return lhs.bit_xor_equal(value);
	}

	// dimension ^ value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_bit_xor>
		)
	[[nodiscard]] constexpr auto operator^(const LeftDimension& lhs, const T& value) noexcept -> LeftDimension
	{
		return lhs.bit_xor(value);
	}

	// ===========================================================================
	// operator~

	// ~dimension
	template<
		dimension_detail::dimension_t LeftDimension
	>
		requires(
			dimension_detail::operation_supported_t<void, LeftDimension, dimension_detail::tag_bit_flip>
		)
	[[nodiscard]] constexpr auto operator~(const LeftDimension& lhs) noexcept -> LeftDimension
	{
		return lhs.bit_flip();
	}

	// ===========================================================================
	// operator and

	// dimension and dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_logical_and>
		)
	[[nodiscard]] constexpr auto operator and(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> auto
	{
		return lhs.logical_and(rhs);
	}

	// dimension and value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_logical_and>
		)
	constexpr auto operator and(const LeftDimension& lhs, const T& value) noexcept -> auto
	{
		return lhs.logical_and(value);
	}

	// ===========================================================================
	// operator or

	// dimension or dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_logical_or>
		)
	[[nodiscard]] constexpr auto operator or(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> auto
	{
		return lhs.logical_or(rhs);
	}

	// dimension or value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_logical_or>
		)
	constexpr auto operator or(const LeftDimension& lhs, const T& value) noexcept -> auto
	{
		return lhs.logical_or(value);
	}

	// ===========================================================================
	// operator not

	// not dimension
	template<
		dimension_detail::dimension_t LeftDimension
	>
		requires(
			dimension_detail::operation_supported_t<void, LeftDimension, dimension_detail::tag_logical_not>
		)
	[[nodiscard]] constexpr auto operator not(const LeftDimension& lhs) noexcept -> auto
	{
		return lhs.logical_not();
	}

	// ===========================================================================
	// operator==

	// dimension == dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_compare_equal>
		)
	[[nodiscard]] constexpr auto operator==(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> auto
	{
		return lhs.equal(rhs);
	}

	// dimension == value
	template<
		dimension_detail::dimension_t LeftDimension,
		dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_compare_equal>
		)
	[[nodiscard]] constexpr auto operator==(const LeftDimension& lhs, const T& rhs) noexcept -> auto
	{
		return lhs.equal(rhs);
	}

	// ===========================================================================
	// operator!=

	// dimension != dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_compare_not_equal>
		)
	[[nodiscard]] constexpr auto operator!=(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> auto
	{
		return lhs.not_equal(rhs);
	}

	// dimension != value
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_compare_not_equal>
		)
	[[nodiscard]] constexpr auto operator!=(const LeftDimension& lhs, const T& rhs) noexcept -> auto
	{
		return lhs.not_equal(rhs);
	}

	// ===========================================================================
	// operator>

	// dimension > dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_compare_greater_than>
		)
	[[nodiscard]] constexpr auto operator>(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> auto
	{
		return lhs.greater_than(rhs);
	}

	// dimension > value
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_compare_greater_than>
		)
	[[nodiscard]] constexpr auto operator>(const LeftDimension& lhs, const T& rhs) noexcept -> auto
	{
		return lhs.greater_than(rhs);
	}

	// ===========================================================================
	// operator>=

	// dimension >= dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_compare_greater_equal>
		)
	[[nodiscard]] constexpr auto operator>=(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> auto
	{
		return lhs.greater_equal(rhs);
	}

	// dimension >= value
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_compare_greater_equal>
		)
	[[nodiscard]] constexpr auto operator>=(const LeftDimension& lhs, const T& rhs) noexcept -> auto
	{
		return lhs.greater_equal(rhs);
	}

	// ===========================================================================
	// operator<

	// dimension < dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_compare_less_than>
		)
	[[nodiscard]] constexpr auto operator<(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> auto
	{
		return lhs.less_than(rhs);
	}

	// dimension < value
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_compare_less_than>
		)
	[[nodiscard]] constexpr auto operator<(const LeftDimension& lhs, const T& rhs) noexcept -> auto
	{
		return lhs.less_than(rhs);
	}

	// ===========================================================================
	// operator<=

	// dimension <= dimension / dimension_like
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_dimension_or_dimension_like_t<LeftDimension> RightDimension
	>
		requires(
			dimension_detail::operation_supported_t<RightDimension, LeftDimension, dimension_detail::tag_compare_less_equal>
		)
	[[nodiscard]] constexpr auto operator<=(const LeftDimension& lhs, const RightDimension& rhs) noexcept -> auto
	{
		return lhs.less_equal(rhs);
	}

	// dimension <= value
	template<
		dimension_detail::dimension_t LeftDimension, dimension_detail::compatible_value_type_t<LeftDimension> T
	>
		requires(
			dimension_detail::operation_supported_t<T, LeftDimension, dimension_detail::tag_compare_less_equal>
		)
	[[nodiscard]] constexpr auto operator<=(const LeftDimension& lhs, const T& rhs) noexcept -> auto
	{
		return lhs.less_equal(rhs);
	}

	// fixme(OPT): std::totally_ordered_with => [dimension_like `op` dimension] / [value `op` dimension]
}
