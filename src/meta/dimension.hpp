// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
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
		concept known_member_size_t = requires { member_size<T>(); };

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
						using this_type = member_type_of_index<I, ThisDimension>;
						using other_type = member_type_of_index<I, OtherDimension>;

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
				known_member_size_t<OtherDimension> and
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
				known_member_size_t<DimensionLike> and
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
					using this_type = member_type_of_index<I, ThisDimension>;

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

		struct tag_logical_and {};

		struct tag_logical_or {};

		struct tag_logical_not {};

		struct tag_compare_equal {};

		struct tag_compare_not_equal {};

		struct tag_compare_greater_than {};

		struct tag_compare_greater_equal {};

		struct tag_compare_less_than {};

		struct tag_compare_less_equal {};

		template<typename T, typename ThisDimension, typename Tag>
		constexpr auto is_operation_supported_v = cache<Tag, ThisDimension, T>::value;

		template<typename T, typename ThisDimension, typename Tag>
		concept operation_supported_t = is_operation_supported_v<T, ThisDimension, Tag>;

		template<typename Dimension>
		using boolean_result_type = std::array<bool, member_size<Dimension>()>;

		// Disguises type T as an N-dimensional structure, but actually returns the same object in all dimensions
		template<typename T, std::size_t>
		struct dimension_wrapper
		{
			std::reference_wrapper<T> ref;

			template<std::size_t Index>
			[[nodiscard]] constexpr auto get() const noexcept -> T&
			{
				std::ignore = Index;
				return ref.get();
			}
		};

		// Convert current dimension to target dimension/dimension_like/value_type, member types must be convertible (one-to-one)
		// Generally used to convert a dimension to another compatible type
		template<typename T>
		struct dimension_transformer
		{
			template<std::size_t Index>
			[[nodiscard]] constexpr auto operator()(const auto& self) const noexcept -> decltype(auto)
			{
				if constexpr (known_member_size_t<T>)
				{
					return static_cast<member_type_of_index<Index, T>>(self);
				}
				else
				{
					std::ignore = Index;
					return static_cast<T>(self);
				}
			}
		};

		template<typename Tag, Dimensions D>
		struct dimension_walker
		{
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH

			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING(4244)
			#endif

			// lhs `op`= rhs
			// lhs = `op` rhs
			template<std::size_t Index>
			constexpr auto operator()(auto& lhs, const auto& rhs) const noexcept -> void
			{
				// If the index is the specified one,
				// then the operation will be performed (lhs `op`= rhs or lhs = `op` rhs),
				// otherwise, the binary operations can be handled uniformly (do nothing),
				// but not unary operations, which still require assignment (unless they were assigned before walk).

				if constexpr (D != Dimensions::ALL and static_cast<std::size_t>(D) != Index)
				{
					if constexpr (
						std::is_same_v<Tag, tag_bit_flip> or
						std::is_same_v<Tag, tag_logical_not>
					)
					{
						lhs = rhs;
					}
				}
				else
				{
					std::ignore = Index;
					if constexpr (std::is_same_v<Tag, tag_addition_self>)
					{
						lhs += rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_subtraction_self>)
					{
						lhs -= rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_multiplication_self>)
					{
						lhs *= rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_division_self>)
					{
						lhs /= rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_modulus_self>)
					{
						lhs %= rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_bit_and_self>)
					{
						lhs &= rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_bit_or_self>)
					{
						lhs |= rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_bit_xor_self>)
					{
						lhs ^= rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_bit_flip>)
					{
						lhs = ~rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_logical_not>)
					{
						lhs = not rhs;
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}
			}

			// result = lhs `op` rhs
			template<std::size_t Index>
			constexpr auto operator()(auto& result, const auto& lhs, const auto& rhs) const noexcept -> void
			{
				// If the index is the specified one,
				// then the operation will be performed (result = lhs `op` rhs or result = lhs(rhs)),
				// otherwise, the value will be assigned directly (result = lhs).

				if constexpr (D != Dimensions::ALL and static_cast<std::size_t>(D) != Index)
				{
					if constexpr (requires { result = lhs; })
					{
						result = lhs;
					}
					else if constexpr (requires { result = static_cast<std::remove_cvref_t<decltype(result)>>(lhs); })
					{
						result = static_cast<std::remove_cvref_t<decltype(result)>>(lhs);
					}
					else if constexpr (requires { result = std::remove_cvref_t<decltype(result)>{lhs}; })
					{
						result = std::remove_cvref_t<decltype(result)>{lhs};
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}
				else
				{
					if constexpr (std::is_same_v<Tag, tag_transform>)
					{
						if constexpr (requires { lhs(rhs); })
						{
							result = lhs(rhs);
						}
						else if constexpr (requires { lhs.template operator()<Index>(rhs); })
						{
							result = lhs.template operator()<Index>(rhs);
						}
						else
						{
							GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
						}
					}
					else if constexpr (std::is_same_v<Tag, tag_addition>)
					{
						result = lhs + rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_subtraction>)
					{
						result = lhs - rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_multiplication>)
					{
						result = lhs * rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_division>)
					{
						result = lhs / rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_modulus>)
					{
						result = lhs % rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_bit_and>)
					{
						result = lhs & rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_bit_or>)
					{
						result = lhs | rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_bit_xor>)
					{
						result = lhs ^ rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_logical_and>)
					{
						result = lhs and rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_logical_or>)
					{
						result = lhs or rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_compare_equal>)
					{
						result = lhs == rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_compare_not_equal>)
					{
						result = lhs != rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_compare_greater_than>)
					{
						result = lhs > rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_compare_greater_equal>)
					{
						result = lhs >= rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_compare_less_than>)
					{
						result = lhs < rhs;
					}
					else if constexpr (std::is_same_v<Tag, tag_compare_less_equal>)
					{
						result = lhs <= rhs;
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
					}
				}
			}

			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_POP

			template<typename Lhs, typename Rhs>
			constexpr auto walk(Lhs& lhs, const Rhs& rhs) const noexcept -> void
			{
				meta::member_walk(*this, lhs, rhs);
			}

			template<typename Result, typename Lhs, typename Rhs>
			constexpr auto walk(Result& result, const Lhs& lhs, const Rhs& rhs) const noexcept -> void
			{
				meta::member_walk(*this, result, lhs, rhs);
			}
		};
	}

	template<typename Dimension>
	struct [[nodiscard]] dimension // NOLINT(bugprone-crtp-constructor-accessibility)
	{
		using dimension_type = Dimension;

	private:
		[[nodiscard]] constexpr auto rep() noexcept -> dimension_type& { return *static_cast<dimension_type*>(this); }
		[[nodiscard]] constexpr auto rep() const noexcept -> const dimension_type& { return *static_cast<const dimension_type*>(this); }

		template<typename Tag, Dimensions D>
		constexpr static auto walk(auto& lhs, const auto& rhs) noexcept -> void
		{
			dimension_detail::dimension_walker<Tag, D>{}.walk(lhs, rhs);
		}

		template<typename Tag, Dimensions D>
		constexpr static auto walk(auto& result, const auto& lhs, const auto& rhs) noexcept -> void
		{
			dimension_detail::dimension_walker<Tag, D>{}.walk(result, lhs, rhs);
		}

	public:
		// ===========================================================================
		// from

		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> FromDimension
		>
		[[nodiscard]] constexpr static auto from(const FromDimension& from) noexcept -> dimension_type
		{
			const auto transformer = dimension_detail::dimension_transformer<dimension_type>{};
			const auto wrapper = dimension_detail::dimension_wrapper<const dimension_detail::dimension_transformer<dimension_type>, meta::member_size<dimension_type>()>{.ref = transformer};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_transform, Dimensions::ALL>(result, wrapper, from);

			return result;
		}

		template<
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
		[[nodiscard]] constexpr static auto from(const T& value) noexcept -> dimension_type
		{
			const auto transformer = dimension_detail::dimension_transformer<dimension_type>{};
			const auto wrapper = dimension_detail::dimension_wrapper<const dimension_detail::dimension_transformer<dimension_type>, meta::member_size<dimension_type>()>{.ref = transformer};
			const auto value_wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_transform, Dimensions::ALL>(result, wrapper, value_wrapper);

			return result;
		}

		// ===========================================================================
		// to

		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> TargetDimension = dimension_type,
			typename TransformFunction = dimension_detail::dimension_transformer<TargetDimension>>
		[[nodiscard]] constexpr auto to(const TransformFunction& transform_function = {}) const noexcept -> TargetDimension
		{
			const dimension_detail::dimension_wrapper<const TransformFunction, meta::member_size<dimension_type>()> wrapper{.ref = transform_function};

			TargetDimension result{};
			dimension::walk<dimension_detail::tag_transform, Dimensions::ALL>(result, wrapper, rep());

			return result;
		}

		[[nodiscard]] constexpr auto copy() const noexcept -> dimension_type
		{
			return to();
		}

		// ===========================================================================
		// all / any / none

		[[nodiscard]] constexpr auto all() const noexcept -> bool
		{
			const auto result = to<dimension_detail::boolean_result_type<dimension_type>>();
			return std::ranges::all_of(result, std::identity{});
		}

		[[nodiscard]] constexpr auto any() const noexcept -> bool
		{
			const auto result = to<dimension_detail::boolean_result_type<dimension_type>>();
			return std::ranges::any_of(result, std::identity{});
		}

		[[nodiscard]] constexpr auto none() const noexcept -> bool
		{
			const auto result = to<dimension_detail::boolean_result_type<dimension_type>>();
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
			dimension::walk<dimension_detail::tag_addition_self, D>(rep(), other);

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
			dimension::walk<dimension_detail::tag_addition, D>(result, rep(), other);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension::walk<dimension_detail::tag_addition_self, D>(rep(), wrapper);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_addition, D>(result, rep(), wrapper);

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
			dimension::walk<dimension_detail::tag_subtraction_self, D>(rep(), other);

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
			dimension::walk<dimension_detail::tag_subtraction, D>(result, rep(), other);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension::walk<dimension_detail::tag_subtraction_self, D>(rep(), wrapper);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_subtraction, D>(result, rep(), wrapper);

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
			dimension::walk<dimension_detail::tag_multiplication_self, D>(rep(), other);

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
			dimension::walk<dimension_detail::tag_multiplication, D>(result, rep(), other);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension::walk<dimension_detail::tag_multiplication_self, D>(rep(), wrapper);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_multiplication, D>(result, rep(), wrapper);

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
			dimension::walk<dimension_detail::tag_division_self, D>(rep(), other);

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
			dimension::walk<dimension_detail::tag_division, D>(result, rep(), other);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension::walk<dimension_detail::tag_division_self, D>(rep(), wrapper);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_division, D>(result, rep(), wrapper);

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
			dimension::walk<dimension_detail::tag_modulus_self, D>(rep(), other);

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
			dimension::walk<dimension_detail::tag_modulus, D>(result, rep(), other);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension::walk<dimension_detail::tag_modulus_self, D>(rep(), wrapper);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_modulus, D>(result, rep(), wrapper);

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
			dimension::walk<dimension_detail::tag_bit_and_self, D>(rep(), other);

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
			dimension::walk<dimension_detail::tag_bit_and, D>(result, rep(), other);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension::walk<dimension_detail::tag_bit_and_self, D>(rep(), wrapper);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_bit_and, D>(result, rep(), wrapper);

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
			dimension::walk<dimension_detail::tag_bit_or_self, D>(rep(), other);

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
			dimension::walk<dimension_detail::tag_bit_or, D>(result, rep(), other);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension::walk<dimension_detail::tag_bit_or_self, D>(rep(), wrapper);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_bit_or, D>(result, rep(), wrapper);

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
			dimension::walk<dimension_detail::tag_bit_xor_self, D>(rep(), other);

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
			dimension::walk<dimension_detail::tag_bit_xor, D>(result, rep(), other);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension::walk<dimension_detail::tag_bit_xor_self, D>(rep(), wrapper);

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
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_type result{};
			dimension::walk<dimension_detail::tag_bit_xor, D>(result, rep(), wrapper);

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
			dimension::walk<dimension_detail::tag_bit_flip, D>(result, rep());

			return result;
		}


		// ===========================================================================
		// operator and

		// dimension and dimension / dimension_like
		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_logical_and>
			)
		[[nodiscard]] constexpr auto logical_and(const OtherDimension& other) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_logical_and, Dimensions::ALL>(result, rep(), other);

			return result;
		}

		// dimension and value
		template<
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_logical_and>
			)
		[[nodiscard]] constexpr auto logical_and(const T& value) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_logical_and, Dimensions::ALL>(result, rep(), wrapper);

			return result;
		}

		// ===========================================================================
		// operator or

		// dimension or dimension / dimension_like
		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_logical_or>
			)
		[[nodiscard]] constexpr auto logical_or(const OtherDimension& other) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_logical_or, Dimensions::ALL>(result, rep(), other);

			return result;
		}

		// dimension or value
		template<
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_logical_or>
			)
		[[nodiscard]] constexpr auto logical_or(const T& value) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_logical_or, Dimensions::ALL>(result, rep(), wrapper);

			return result;
		}

		// ===========================================================================
		// operator not

		// not dimension
		[[nodiscard]] constexpr auto logical_not() const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
			requires (
				dimension_detail::operation_supported_t<void, dimension_type, dimension_detail::tag_logical_not>
			)
		{
			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_logical_not, Dimensions::ALL>(result, rep());

			return result;
		}

		// ===========================================================================
		// operator==

		// dimension == dimension / dimension_like
		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_equal>
			)
		[[nodiscard]] constexpr auto equal(const OtherDimension& other) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_equal, Dimensions::ALL>(result, rep(), other);

			return result;
		}

		// dimension == value
		template<
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_equal>
			)
		[[nodiscard]] constexpr auto equal(const T& value) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_equal, Dimensions::ALL>(result, rep(), wrapper);

			return result;
		}

		// ===========================================================================
		// operator!=

		// dimension != dimension / dimension_like
		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_not_equal>
			)
		[[nodiscard]] constexpr auto not_equal(const OtherDimension& other) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_not_equal, Dimensions::ALL>(result, rep(), other);

			return result;
		}

		// dimension != value
		template<
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_not_equal>
			)
		[[nodiscard]] constexpr auto not_equal(const T& value) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_not_equal, Dimensions::ALL>(result, rep(), wrapper);

			return result;
		}

		// ===========================================================================
		// operator>

		// dimension > dimension / dimension_like
		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_greater_than>
			)
		[[nodiscard]] constexpr auto greater_than(const OtherDimension& other) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_greater_than, Dimensions::ALL>(result, rep(), other);

			return result;
		}

		// dimension > value
		template<
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_greater_than>
			)
		[[nodiscard]] constexpr auto greater_than(const T& value) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_greater_than, Dimensions::ALL>(result, rep(), wrapper);

			return result;
		}

		// ===========================================================================
		// operator>=

		// dimension >= dimension / dimension_like
		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_greater_equal>
			)
		[[nodiscard]] constexpr auto greater_equal(const OtherDimension& other) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_greater_equal, Dimensions::ALL>(result, rep(), other);

			return result;
		}

		// dimension >= value
		template<
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_greater_equal>
			)
		[[nodiscard]] constexpr auto greater_equal(const T& value) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_greater_equal, Dimensions::ALL>(result, rep(), wrapper);

			return result;
		}

		// ===========================================================================
		// operator<

		// dimension < dimension / dimension_like
		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_less_than>
			)
		[[nodiscard]] constexpr auto less_than(const OtherDimension& other) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_less_than, Dimensions::ALL>(result, rep(), other);

			return result;
		}

		// dimension < value
		template<
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_less_than>
			)
		[[nodiscard]] constexpr auto less_than(const T& value) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_less_than, Dimensions::ALL>(result, rep(), wrapper);

			return result;
		}

		// ===========================================================================
		// operator<=

		// dimension <= dimension / dimension_like
		template<
			dimension_detail::compatible_dimension_or_dimension_like_t<dimension_type> OtherDimension = dimension_type>
			requires(
				dimension_detail::operation_supported_t<OtherDimension, dimension_type, dimension_detail::tag_compare_less_equal>
			)
		[[nodiscard]] constexpr auto less_equal(const OtherDimension& other) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_less_equal, Dimensions::ALL>(result, rep(), other);

			return result;
		}

		// dimension <= value
		template<
			dimension_detail::compatible_value_type_t<dimension_type> T
		>
			requires(
				dimension_detail::operation_supported_t<T, dimension_type, dimension_detail::tag_compare_less_equal>
			)
		[[nodiscard]] constexpr auto less_equal(const T& value) const noexcept -> auto // dimension_detail::boolean_result_type<dimension_type>
		{
			const auto wrapper = dimension_detail::dimension_wrapper<const T, meta::member_size<dimension_type>()>{.ref = value};

			dimension_detail::boolean_result_type<dimension_type> result{};
			dimension::walk<dimension_detail::tag_compare_less_equal, Dimensions::ALL>(result, rep(), wrapper);

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

namespace std
{
	template<std::size_t Index, typename T, std::size_t N>
	struct tuple_element<Index, gal::prometheus::meta::dimension_detail::dimension_wrapper<T, N>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<typename T, std::size_t N>
	struct tuple_size<gal::prometheus::meta::dimension_detail::dimension_wrapper<T, N>> : std::integral_constant<std::size_t, N> {}; // NOLINT(cert-dcl58-cpp)
}

#include <meta/dimension.cache.inl>
