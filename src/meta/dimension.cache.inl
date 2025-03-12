// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

namespace gal::prometheus::meta::dimension_detail
{
	// ===========================================================================
	// CODE GEN MACRO
	// ===========================================================================

	#if defined(GAL_PROMETHEUS_COMPILER_GNU)
	// return requires
	// {
	// 	{
	// 		meta::member_of_index<I>(std::declval<const ThisDimension&>()) >
	// 		std::declval<const T&>()
	// 	} -> std::convertible_to<bool>;
	// };
	//
	// error: expected ‘}’ before ‘>’ token
	// meta::member_of_index<I>(std::declval<const ThisDimension&>()) >
	// note: to match this ‘{’
	// {
	// ^
	// error: expected ‘;’ before ‘}’ token
	//         std::declval<const T&>()
	//                                 ^
	//                                 ;
	// } -> std::convertible_to<bool>;
	// ~
	// error: wrong number of template arguments (1, should be 2)
	// } -> std::convertible_to<bool>;
	//           ^~~~~~~~~~~~~~~~~~~~

	// before: requires { L > R } -> std::convertible_to<bool>;
	// after:  requires { (L > R) } -> std::convertible_to<bool>;
	#endif

	#define DIMENSION_CACHE_GEN_FUNCTION_DIMENSION(operation, op) \
	constexpr auto f = []<std::size_t I>() noexcept -> bool\
	{\
		if constexpr (\
			requires\
			{\
				{(\
					meta::member_of_index<I>(std::declval<const ThisDimension&>()) op\
					meta::member_of_index<I>(std::declval<const OtherDimension&>())\
				)} -> std::convertible_to<bool>;\
			}\
		)\
		{\
			return true;\
		}\
		else \
		{\
			if constexpr ([[maybe_unused]] constexpr auto value = dimension_folder<ThisDimension, operation>::value;\
				value == DimensionFoldCategory::ANY)\
			{\
				return requires\
				{\
					{\
						std::ranges::any_of(\
							meta::member_of_index<I>(std::declval<const ThisDimension&>()) op\
							meta::member_of_index<I>(std::declval<const OtherDimension&>()),\
							std::identity{}\
						)\
					} -> std::convertible_to<bool>;\
				};\
			}\
			else if constexpr (value == DimensionFoldCategory::ALL)\
			{\
				return requires\
				{\
					{\
						std::ranges::all_of(\
							meta::member_of_index<I>(std::declval<const ThisDimension&>()) op\
							meta::member_of_index<I>(std::declval<const OtherDimension&>()),\
							std::identity{}\
						)\
					} -> std::convertible_to<bool>;\
				};\
			}\
			else\
			{\
				return false;\
			}\
		}\
	}

	#define DIMENSION_CACHE_GEN_FUNCTION_VALUE(operation, op) \
	constexpr auto f = []<std::size_t I>() noexcept -> bool\
	{\
		if constexpr (\
			requires\
			{\
				{(\
					meta::member_of_index<I>(std::declval<const ThisDimension&>()) op\
					std::declval<const T&>()\
				)} -> std::convertible_to<bool>;\
			}\
		)\
		{\
			return true;\
		}\
		else \
		{\
			if constexpr ([[maybe_unused]] constexpr auto value = dimension_folder<ThisDimension, operation>::value;\
				value == DimensionFoldCategory::ANY)\
			{\
				return requires\
				{\
					{\
						std::ranges::any_of(\
							meta::member_of_index<I>(std::declval<const ThisDimension&>()) op\
							std::declval<const T&>(),\
							std::identity{}\
						)\
					} -> std::convertible_to<bool>;\
				};\
			}\
			else if constexpr (value == DimensionFoldCategory::ALL)\
			{\
				return requires\
				{\
					{\
						std::ranges::all_of(\
							meta::member_of_index<I>(std::declval<const ThisDimension&>()) op\
							std::declval<const T&>(),\
							std::identity{}\
						)\
					} -> std::convertible_to<bool>;\
				};\
			}\
			else\
			{\
				return false;\
			}\
		}\
	}

	#define DIMENSION_CACHE_GEN_FUNCTION_SELF(operation, op) \
	constexpr auto f = []<std::size_t I>() noexcept -> bool\
	{\
		if constexpr (\
			requires\
			{\
				{\
					op meta::member_of_index<I>(std::declval<const ThisDimension&>())\
				} -> std::convertible_to<bool>;\
			}\
		)\
		{\
			return true;\
		}\
		else \
		{\
			if constexpr ([[maybe_unused]] constexpr auto value = dimension_folder<ThisDimension, operation>::value;\
				value == DimensionFoldCategory::ANY)\
			{\
				return requires\
				{\
					{\
						std::ranges::any_of(\
							op meta::member_of_index<I>(std::declval<const ThisDimension&>()),\
							std::identity{}\
						)\
					} -> std::convertible_to<bool>;\
				};\
			}\
			else if constexpr (value == DimensionFoldCategory::ALL)\
			{\
				return requires\
				{\
					{\
						std::ranges::all_of(\
							op meta::member_of_index<I>(std::declval<const ThisDimension&>()),\
							std::identity{}\
						)\
					} -> std::convertible_to<bool>;\
				};\
			}\
			else\
			{\
				return false;\
			}\
		}\
	}

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
					DIMENSION_CACHE_GEN_FUNCTION_DIMENSION(DimensionFoldOperation::LOGICAL_AND, and);

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{})
			> {};

	template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
	struct cache<tag_logical_and, ThisDimension, T> : std::bool_constant<
				// manually member_walk
				[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
				{
					DIMENSION_CACHE_GEN_FUNCTION_VALUE(DimensionFoldOperation::LOGICAL_AND, and);

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
					DIMENSION_CACHE_GEN_FUNCTION_DIMENSION(DimensionFoldOperation::LOGICAL_OR, or);

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{})
			> {};

	template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
	struct cache<tag_logical_or, ThisDimension, T> : std::bool_constant<
				// manually member_walk
				[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
				{
					DIMENSION_CACHE_GEN_FUNCTION_VALUE(DimensionFoldOperation::LOGICAL_OR, or);

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
					DIMENSION_CACHE_GEN_FUNCTION_SELF(DimensionFoldOperation::LOGICAL_NOT, not);

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
					DIMENSION_CACHE_GEN_FUNCTION_DIMENSION(DimensionFoldOperation::EQUAL, ==);

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{})
			> {};

	template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
	struct cache<tag_compare_equal, ThisDimension, T> : std::bool_constant<
				// manually member_walk
				[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
				{
					DIMENSION_CACHE_GEN_FUNCTION_VALUE(DimensionFoldOperation::EQUAL, ==);

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
					DIMENSION_CACHE_GEN_FUNCTION_DIMENSION(DimensionFoldOperation::NOT_EQUAL, !=);

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{})
			> {};

	template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
	struct cache<tag_compare_not_equal, ThisDimension, T> : std::bool_constant<
				// manually member_walk
				[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
				{
					DIMENSION_CACHE_GEN_FUNCTION_VALUE(DimensionFoldOperation::NOT_EQUAL, !=);

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
					DIMENSION_CACHE_GEN_FUNCTION_DIMENSION(DimensionFoldOperation::GREATER_THAN, >);

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{})
			> {};

	template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
	struct cache<tag_compare_greater_than, ThisDimension, T> : std::bool_constant<
				// manually member_walk
				[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
				{
					DIMENSION_CACHE_GEN_FUNCTION_VALUE(DimensionFoldOperation::GREATER_THAN, >);

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
					DIMENSION_CACHE_GEN_FUNCTION_DIMENSION(DimensionFoldOperation::GREATER_EQUAL, >=);

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{})
			> {};

	template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
	struct cache<tag_compare_greater_equal, ThisDimension, T> : std::bool_constant<
				// manually member_walk
				[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
				{
					DIMENSION_CACHE_GEN_FUNCTION_VALUE(DimensionFoldOperation::GREATER_EQUAL, >=);

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
					DIMENSION_CACHE_GEN_FUNCTION_DIMENSION(DimensionFoldOperation::LESS_THAN, <);

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{})
			> {};

	template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
	struct cache<tag_compare_less_than, ThisDimension, T> : std::bool_constant<
				// manually member_walk
				[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
				{
					DIMENSION_CACHE_GEN_FUNCTION_VALUE(DimensionFoldOperation::LESS_THAN, <);

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
					DIMENSION_CACHE_GEN_FUNCTION_DIMENSION(DimensionFoldOperation::LESS_EQUAL, <=);

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{})
			> {};

	template<dimension_t ThisDimension, compatible_value_type_t<ThisDimension> T>
	struct cache<tag_compare_less_equal, ThisDimension, T> : std::bool_constant<
				// manually member_walk
				[]<std::size_t... Index>(std::index_sequence<Index...>) consteval noexcept -> bool
				{
					DIMENSION_CACHE_GEN_FUNCTION_VALUE(DimensionFoldOperation::LESS_EQUAL, <=);

					return (f.template operator()<Index>() and ...);
				}(std::make_index_sequence<member_size<ThisDimension>()>{})
			> {};

	#undef DIMENSION_CACHE_GEN_FUNCTION_DIMENSION
	#undef DIMENSION_CACHE_GEN_FUNCTION_VALUE
	#undef DIMENSION_CACHE_GEN_FUNCTION_SELF
}
