// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <concepts>

#include <unit_test/def.hpp>
#include <unit_test/events.hpp>
#include <unit_test/operands.hpp>
#include <unit_test/executor.hpp>

namespace gal::prometheus::unit_test::dispatcher
{
	template<typename Lhs, typename Dispatcher>
	struct dispatched_expression
	{
		using expression_type = Lhs;
		using dispatcher_type = Dispatcher;

		expression_type expression; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

		// explicit
		[[nodiscard]] constexpr explicit operator bool() const noexcept //
		{
			return static_cast<bool>(expression);
		}
	};

	template<typename>
	struct is_dispatched_expression : std::false_type {};

	template<typename T, typename Dispatcher>
	struct is_dispatched_expression<dispatched_expression<T, Dispatcher>> : std::true_type {};

	template<typename T>
	constexpr auto is_dispatched_expression_v = is_dispatched_expression<T>::value;
	template<typename T>
	concept dispatched_expression_t = is_dispatched_expression_v<T>;

	template<typename T, typename RequiredType>
	struct is_type : std::bool_constant<std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<RequiredType>>> {};

	template<typename T, typename RequiredType>
	constexpr auto is_type_v = is_type<T, RequiredType>::value;

	template<typename T, typename RequiredType>
	concept type_t = is_type_v<T, RequiredType>;

	template<typename T, typename RequiredType>
	struct is_type_or_dispatched_type : std::bool_constant<
				// In most cases the following line will already fulfill the requirement
				is_type_v<T, RequiredType> or
				// The next two lines allow us to support more complex expressions, such as `(xxx == xxx) == "same!"_b)`
				// implicit
				std::is_convertible_v<std::remove_cvref_t<T>, std::remove_cvref_t<RequiredType>> or
				// explicit
				std::is_constructible_v<std::remove_cvref_t<RequiredType>, std::remove_cvref_t<T>>
			> {};

	template<typename T, typename Dispatcher, typename RequiredType>
	struct is_type_or_dispatched_type<dispatched_expression<T, Dispatcher>, RequiredType> : is_type_or_dispatched_type<T, RequiredType> {};

	template<typename T, typename RequiredType>
	constexpr auto is_type_or_dispatched_type_v = is_type_or_dispatched_type<T, RequiredType>::value;

	template<typename T, typename RequiredType>
	concept type_or_dispatched_type_t = is_type_or_dispatched_type_v<T, RequiredType>;

	template<typename T, template<typename> typename Constrain>
	struct is_constrain_satisfied_type_or_dispatched_type : std::bool_constant<Constrain<std::remove_cvref_t<T>>::value> {};

	template<typename T, template<typename> typename Constrain, typename Dispatcher>
	struct is_constrain_satisfied_type_or_dispatched_type<dispatched_expression<T, Dispatcher>, Constrain> :
			is_constrain_satisfied_type_or_dispatched_type<typename dispatched_expression<T, Dispatcher>::expression_type, Constrain> {};

	template<typename T, template<typename> typename Constrain>
	constexpr auto is_constrain_satisfied_type_or_dispatched_type_v = is_constrain_satisfied_type_or_dispatched_type<T, Constrain>::value;

	template<typename T, template<typename> typename Constrain>
	concept constrain_satisfied_type_or_dispatched_type_t = is_constrain_satisfied_type_or_dispatched_type_v<T, Constrain>;

	template<template<typename> typename Constrain>
	struct constrain_against_wrapper
	{
		template<typename T>
		struct rebind
		{
			constexpr static auto value = not Constrain<T>::value;
		};
	};

	template<typename T, template<typename> typename Constrain>
	constexpr auto is_constrain_against_type_or_dispatched_type_v = is_constrain_satisfied_type_or_dispatched_type_v<
		T,
		constrain_against_wrapper<Constrain>::template rebind
	>;

	template<typename T, template<typename> typename Constrain>
	concept constrain_against_type_or_dispatched_type_t = is_constrain_against_type_or_dispatched_type_v<T, Constrain>;

	struct type_or_dispatched_type
	{
		template<typename T>
		[[nodiscard]] constexpr static auto get(T&& v) noexcept -> decltype(auto) //
		{
			if constexpr (is_dispatched_expression_v<std::remove_cvref_t<T>>) //
			{
				return std::forward<T>(v).expression;
			}
			else //
			{
				return std::forward<T>(v);
			}
		}
	};

	template<bool, typename, typename>
	struct lazy_dispatcher_type;

	template<typename L, typename R>
	struct lazy_dispatcher_type<true, L, R>
	{
		using type = typename L::dispatcher_type;
	};

	template<typename L, typename R>
	struct lazy_dispatcher_type<false, L, R>
	{
		using type = typename R::dispatcher_type;
	};

	template<bool C, typename L, typename R>
	using lazy_dispatcher_type_t = typename lazy_dispatcher_type<C, L, R>::type;

	template<bool, typename>
	struct lazy_expression_type;

	template<typename T>
	struct lazy_expression_type<true, T>
	{
		using type = typename T::expression_type;
	};

	template<typename T>
	struct lazy_expression_type<false, T>
	{
		using type = T;
	};

	template<bool C, typename T>
	using lazy_expression_type_t = typename lazy_expression_type<C, T>::type;

	// ============================================
	// operator==
	// ============================================

	// OperandValue

	// floating_point == value{...}
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::APPROX,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value(),
						std::numeric_limits<typename right_expression_type::value_type>::epsilon()
				}
		};
	}

	// value{...} == floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs == lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::APPROX,
				typename left_expression_type::value_type,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs),
						std::numeric_limits<typename left_expression_type::value_type>::epsilon()
				}
		};
	}

	// not(floating_point) == value{...}
	template<
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value()
				}
		};
	}

	// value{...} == not(floating_point)
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs == lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// OperandLiteralXxx

	// character == "xxx"_c
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_c == character
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs == lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// integral == "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_x == integral
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs == lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// floating_point == "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::APPROX,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value,
						right_expression_type::epsilon
				}
		};
	}

	// "xxx"_x == floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs == lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::APPROX,
				typename left_expression_type::value_type,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs),
						left_expression_type::epsilon
				}
		};
	}

	// ? == "xxx"_auto
	template<
		typename L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<R>, R>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<L>, L>
			        >;
		        }
	[[nodiscard]] constexpr auto operator==(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// lhs == dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{}
		return lhs == dispatched_expression<typename right_expression_type::template rebind<left_expression_type>, dispatcher_type>{};
	}

	// "xxx"_auto == ?
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> L,
		typename R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<L>, L>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<R>, R>
			        >;
		        }
	[[nodiscard]] constexpr auto operator==([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs == lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{} == rhs
		return dispatched_expression<typename left_expression_type::template rebind<right_expression_type>, dispatcher_type>{} == rhs;
	}

	// OperandIdentityBoolean

	// bool == operands::OperandIdentityBoolean::value_type{...}
	template<
		type_or_dispatched_type_t<bool> L,
		// not allowed to be a candidate for an expression such as `xxx == "xxx"`, must be `xxx == "xxx"_b`
		type_t<operands::OperandIdentityBoolean::value_type> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		return dispatched_expression<
			operands::OperandIdentityBoolean,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(rhs),
						// (xxx == xxx) == "xxx"_b => (operands::OperandExpression<EQUAL> == operands::OperandIdentityBoolean::value_type)
						// explicit cast(operands::OperandExpression => bool)
						static_cast<bool>(type_or_dispatched_type::get(lhs))
				}
		};
	}

	// operands::OperandIdentityBoolean::value_type{...} == bool
	template<
		// not allowed to be a candidate for an expression such as `"xxx" == xxx`, must be `"xxx"_b == xxx`
		type_t<operands::OperandIdentityBoolean::value_type> L,
		type_or_dispatched_type_t<bool> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		return rhs == lhs;
	}

	// OperandIdentityString

	// string == operands::OperandIdentityString{...}
	template<
		typename L,
		// not allowed to be a candidate for an expression such as `xxx == "xxx"`, must be `xxx == "xxx"_s`
		type_t<operands::OperandIdentityString> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::EQUAL,
				left_expression_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// operands::OperandIdentityString{...} == string
	template<
		// not allowed to be a candidate for an expression such as `"xxx" == xxx`, must be `"xxx"_s == xxx`
		type_t<operands::OperandIdentityString> L,
		typename R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs == lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::EQUAL,
				left_expression_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs)
				}
		};
	}


	// ============================================
	// operator!=
	// ============================================

	// OperandValue

	// floating_point != value{...}
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_APPROX,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value(),
						std::numeric_limits<typename right_expression_type::value_type>::epsilon()
				}
		};
	}

	// value{...} != floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs != lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_APPROX,
				typename left_expression_type::value_type,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs),
						std::numeric_limits<typename left_expression_type::value_type>::epsilon()
				}
		};
	}

	// not(floating_point) != value{...}
	template<
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value()
				}
		};
	}

	// value{...} != not(floating_point)
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs != lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// OperandLiteralXxx

	// character != "xxx"_c
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_c != character
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs != lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// integral != "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_x != integral
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs != lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// floating_point != "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_APPROX,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value,
						right_expression_type::epsilon
				}
		};
	}

	// "xxx"_x != floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs != lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_APPROX,
				typename left_expression_type::value_type,
				right_expression_type,
				typename left_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs),
						left_expression_type::epsilon
				}
		};
	}

	// ? != "xxx"_auto
	template<
		typename L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<R>, R>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<L>, L>
			        >;
		        }
	[[nodiscard]] constexpr auto operator!=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// lhs != dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{}
		return lhs != dispatched_expression<typename right_expression_type::template rebind<left_expression_type>, dispatcher_type>{};
	}

	// "xxx"_auto != ?
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> L,
		typename R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<L>, L>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<R>, R>
			        >;
		        }
	[[nodiscard]] constexpr auto operator!=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs != lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// lhs != dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{}
		return dispatched_expression<typename left_expression_type::template rebind<right_expression_type>, dispatcher_type>{} != rhs;
	}

	// OperandIdentityBoolean

	// bool != operands::OperandIdentityBoolean::value_type{...}
	template<
		type_or_dispatched_type_t<bool> L,
		type_or_dispatched_type_t<operands::OperandIdentityBoolean::value_type> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		return dispatched_expression<
			operands::OperandIdentityBoolean,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(rhs),
						// (xxx == xxx) != "xxx"_b => (operands::OperandExpression<EQUAL> != operands::OperandIdentityBoolean::value_type)
						// explicit cast(operands::OperandExpression => bool)
						not static_cast<bool>(type_or_dispatched_type::get(lhs))
				}
		};
	}

	// operands::OperandIdentityBoolean::value_type{...} != bool
	template<
		type_or_dispatched_type_t<operands::OperandIdentityBoolean::value_type> L,
		type_or_dispatched_type_t<bool> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		return rhs != lhs;
	}

	// OperandIdentityString

	// string != operands::OperandIdentityString{...}
	template<
		typename L,
		// not allowed to be a candidate for an expression such as `xxx != "xxx"`, must be `xxx != "xxx"_s`
		type_t<operands::OperandIdentityString> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_EQUAL,
				left_expression_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// operands::OperandIdentityString{...} != string
	template<
		// not allowed to be a candidate for an expression such as `"xxx" != xxx`, must be `"xxx"_s != xxx`
		type_t<operands::OperandIdentityString> L,
		typename R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs != lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::NOT_EQUAL,
				left_expression_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// ============================================
	// operator>
	// ============================================

	// OperandValue

	// floating_point > value{...}
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value(),
						std::numeric_limits<typename right_expression_type::value_type>::epsilon()
				}
		};
	}

	// value{...} > floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs > lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				typename left_expression_type::value_type,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs),
						std::numeric_limits<typename left_expression_type::value_type>::epsilon()
				}
		};
	}

	// not(floating_point) > value{...}
	template<
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value()
				}
		};
	}

	// value{...} > not(floating_point)
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs > lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// OperandLiteralXxx

	// character > "xxx"_c
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_c > character
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs > lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// integral > "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_x > integral
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs > lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// floating_point > "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value,
						right_expression_type::epsilon
				}
		};
	}

	// "xxx"_x > floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs > lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_THAN,
				typename left_expression_type::value_type,
				right_expression_type,
				typename left_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs),
						left_expression_type::epsilon
				}
		};
	}

	// ? > "xxx"_auto
	template<
		typename L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<R>, R>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<L>, L>
			        >;
		        }
	[[nodiscard]] constexpr auto operator>(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// lhs > dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{}
		return lhs > dispatched_expression<typename right_expression_type::template rebind<left_expression_type>, dispatcher_type>{};
	}

	// "xxx"_auto > ?
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> L,
		typename R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<L>, L>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<R>, R>
			        >;
		        }
	[[nodiscard]] constexpr auto operator>([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs > lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// lhs > dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{}
		return dispatched_expression<typename left_expression_type::template rebind<right_expression_type>, dispatcher_type>{} > rhs;
	}

	// ============================================
	// operator>=
	// ============================================

	// OperandValue

	// floating_point >= value{...}
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value(),
						std::numeric_limits<typename right_expression_type::value_type>::epsilon()
				}
		};
	}

	// value{...} >= floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs >= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				typename left_expression_type::value_type,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs),
						std::numeric_limits<typename left_expression_type::value_type>::epsilon()
				}
		};
	}

	// not(floating_point) >= value{...}
	template<
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value()
				}
		};
	}

	// value{...} >= not(floating_point)
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs >= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// OperandLiteralXxx

	// character >= "xxx"_c
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_c >= character
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs >= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// integral >= "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_x >= integral
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs >= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// floating_point >= "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				left_expression_type, typename
				right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value,
						right_expression_type::epsilon
				}
		};
	}

	// "xxx"_x >= floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator>=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs >= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::GREATER_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type,
				typename left_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs),
						left_expression_type::epsilon
				}
		};
	}

	// ? >= "xxx"_auto
	template<
		typename L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<R>, R>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<L>, L>
			        >;
		        }
	[[nodiscard]] constexpr auto operator>=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// lhs >= dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{}
		return lhs >= dispatched_expression<typename right_expression_type::template rebind<left_expression_type>, dispatcher_type>{};
	}

	// "xxx"_auto >= ?
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> L,
		typename R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<L>, L>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<R>, R>
			        >;
		        }
	[[nodiscard]] constexpr auto operator>=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs >= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// lhs >= dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{}
		return dispatched_expression<typename left_expression_type::template rebind<right_expression_type>, dispatcher_type>{} >= rhs;
	}

	// ============================================
	// operator<
	// ============================================

	// OperandValue

	// floating_point < value{...}
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value(),
						std::numeric_limits<typename right_expression_type::value_type>::epsilon()
				}
		};
	}

	// value{...} < floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs < lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				typename left_expression_type::value_type,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs),
						std::numeric_limits<typename left_expression_type::value_type>::epsilon()
				}
		};
	}

	// not(floating_point) < value{...}
	template<
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value()
				}
		};
	}

	// value{...} < not(floating_point)
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs < lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// OperandLiteralXxx

	// character < "xxx"_c
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_c < character
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs < lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// integral < "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_x < integral
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs < lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// floating_point < "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value,
						right_expression_type::epsilon
				}
		};
	}

	// "xxx"_x < floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs < lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_THAN,
				typename left_expression_type::value_type,
				right_expression_type,
				typename left_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs),
						left_expression_type::epsilon
				}
		};
	}

	// ? < "xxx"_auto
	template<
		typename L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<R>, R>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<L>, L>
			        >;
		        }
	[[nodiscard]] constexpr auto operator<(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// lhs < dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{}
		return lhs < dispatched_expression<typename right_expression_type::template rebind<left_expression_type>, dispatcher_type>{};
	}

	// "xxx"_auto < ?
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> L,
		typename R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<L>, L>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<R>, R>
			        >;
		        }
	[[nodiscard]] constexpr auto operator<([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs < lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		//dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{} < rhs
		return dispatched_expression<typename left_expression_type::template rebind<right_expression_type>, dispatcher_type>{} < rhs;
	}

	// ============================================
	// operator<=
	// ============================================

	// OperandValue

	// floating_point <= value{...}
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value(),
						std::numeric_limits<typename right_expression_type::value_type>::epsilon()
				}
		};
	}

	// value{...} <= floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs <= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				typename left_expression_type::value_type,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs),
						std::numeric_limits<typename left_expression_type::value_type>::epsilon()
				}
		};
	}

	// not(floating_point) <= value{...}
	template<
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs).value()
				}
		};
	}

	// value{...} <= not(floating_point)
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> L,
		constrain_against_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=(const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs <= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs).value(),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// OperandLiteralXxx

	// character <= "xxx"_c
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_c <= character
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs <= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// integral <= "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value
				}
		};
	}

	// "xxx"_x <= integral
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs <= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// floating_point <= "xxx"_x
	template<
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				left_expression_type,
				typename right_expression_type::value_type,
				typename right_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						right_expression_type::value,
						right_expression_type::epsilon
				}
		};
	}

	// "xxx"_x <= floating_point
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> L,
		constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator<=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs <= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LESS_EQUAL,
				typename left_expression_type::value_type,
				right_expression_type,
				typename left_expression_type::value_type
			>,
			dispatcher_type
		>{
				.expression = {
						left_expression_type::value,
						type_or_dispatched_type::get(rhs),
						left_expression_type::epsilon
				}
		};
	}

	// ? <= "xxx"_auto
	template<
		typename L,
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<R>, R>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<L>, L>
			        >;
		        }
	[[nodiscard]] constexpr auto operator<=(const L& lhs, [[maybe_unused]] const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// lhs <= dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{}
		return lhs <= dispatched_expression<typename right_expression_type::template rebind<left_expression_type>, dispatcher_type>{};
	}

	// "xxx"_auto <= ?
	template<
		constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_auto> L,
		typename R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
		        // rebind-able type
		        and requires
		        {
			        typename lazy_expression_type_t<is_dispatched_expression_v<L>, L>::template rebind<
				        lazy_expression_type_t<is_dispatched_expression_v<R>, R>
			        >;
		        }
	[[nodiscard]] constexpr auto operator<=([[maybe_unused]] const L& lhs, const R& rhs) noexcept -> auto //
	{
		// forward
		// return rhs <= lhs;

		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		// forward
		// dispatched_expression<operands::OperandLiteralXxx, dispatcher_type>{} <= rhs
		return dispatched_expression<typename left_expression_type::template rebind<right_expression_type>, dispatcher_type>{} <= rhs;
	}

	// ============================================
	// operator and & operator or
	// ============================================

	// ? and ?
	template<
		constrain_satisfied_type_or_dispatched_type_t<is_expression> L,
		constrain_satisfied_type_or_dispatched_type_t<is_expression> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator and(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LOGICAL_AND,
				left_expression_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	// ? or ?
	template<
		constrain_satisfied_type_or_dispatched_type_t<is_expression> L,
		constrain_satisfied_type_or_dispatched_type_t<is_expression> R
	>
	// can we trust users not to (inadvertently) mess up the ADL? vvv
		requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
	[[nodiscard]] constexpr auto operator or(const L& lhs, const R& rhs) noexcept -> auto //
	{
		using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

		using left_expression_type = lazy_expression_type_t<is_dispatched_expression_v<L>, L>;
		using right_expression_type = lazy_expression_type_t<is_dispatched_expression_v<R>, R>;

		return dispatched_expression<
			operands::OperandExpression<
				operands::ExpressionCategory::LOGICAL_OR,
				left_expression_type,
				right_expression_type
			>,
			dispatcher_type
		>{
				.expression = {
						type_or_dispatched_type::get(lhs),
						type_or_dispatched_type::get(rhs)
				}
		};
	}

	template<events::event_t EventType>
	auto register_event(EventType&& event) noexcept(false) -> decltype(auto)
	{
		if constexpr (std::is_same_v<EventType, events::EventSuite>)
		{
			return
					executor::Executor::instance()
					.on(std::forward<EventType>(event));
		}
		else
		{
			return
					executor::Worker::instance()
					.on(std::forward<EventType>(event));
		}
	}

	template<typename Dispatcher>
	class ExpressionDispatcher // NOLINT(bugprone-crtp-constructor-accessibility)
	{
	public:
		template<typename Lhs>
		[[nodiscard]] constexpr auto operator%(Lhs&& lhs) const noexcept -> dispatched_expression<Lhs, Dispatcher> //
		{
			return {.expression = std::forward<Lhs>(lhs)};
		}
	};

	class DispatcherThat final : public ExpressionDispatcher<DispatcherThat>
	{
	public:
		using ExpressionDispatcher::operator%;
	};

	// more dispatched expression vvv
	// ...

	class ExpectResult final
	{
	public:
		struct fatal {};

	private:
		template<typename T>
		struct with_location
		{
			std::source_location location;

			constexpr explicit(false) with_location(const T&, const std::source_location& l = std::source_location::current()) noexcept
				: location{l} {}
		};

		bool result_;

	public:
		constexpr explicit ExpectResult(const bool result) noexcept
			: result_{result} {}

		template<typename MessageType>
		constexpr auto operator<<(MessageType&& message) noexcept -> ExpectResult& //
			requires requires { events::EventLog{.message = std::forward<MessageType>(message)}; }
		{
			if (not result_)
			{
				dispatcher::register_event(events::EventLog{.message = std::forward<MessageType>(message)});
			}

			return *this;
		}

		constexpr auto operator<<(const with_location<fatal>& location) noexcept(false) -> ExpectResult&
		{
			if (not result_)
			{
				register_event(events::EventAssertionFatal{.location = location.location});
			}

			return *this;
		}
	};

	class DispatcherExpect final
	{
	public:
		template<typename Expression>
			requires(is_expression_v<Expression> or is_dispatched_expression_v<Expression>)
		constexpr auto operator()(
			Expression&& expression,
			const std::source_location& location = std::source_location::current()
		) const noexcept(false) -> ExpectResult
		{
			if constexpr (is_dispatched_expression_v<Expression>)
			{
				// workaround if needed
				// using dispatcher_type = typename Expression::dispatcher_type;

				const auto result = dispatcher::register_event(
					events::EventAssertion<typename Expression::expression_type>{
							.expression = std::forward<Expression>(expression).expression,
							.location = location
					}
				);

				return ExpectResult{result};
			}
			else
			{
				return ExpectResult
				{
						dispatcher::register_event(
							events::EventAssertion<Expression>{
									.expression = std::forward<Expression>(expression),
									.location = location
							}
						)
				};
			}
		}
	};

	namespace dispatcher_detail
	{
		template<typename D>
		class DispatcherTestBase // NOLINT(bugprone-crtp-constructor-accessibility)
		{
		protected:
			test_categories_type categories_;

		public:
			template<std::invocable InvocableType>
			constexpr auto operator=(InvocableType&& invocable) & noexcept(false) -> DispatcherTestBase&
			{
				dispatcher::register_event(
					events::EventTest<InvocableType>{
							.name = static_cast<D&>(*this).name(),
							.categories = categories_,
							.invocable = std::forward<InvocableType>(invocable),
							.arg = {}
					}
				);

				return *this;
			}

			template<std::invocable InvocableType>
			constexpr auto operator=(InvocableType&& invocable) && noexcept(false) -> DispatcherTestBase&
			{
				dispatcher::register_event(
					events::EventTest<InvocableType>{
							.name = static_cast<D&>(*this).name(),
							.categories = std::move(categories_),
							.invocable = std::forward<InvocableType>(invocable),
							.arg = {}
					}
				);

				return *this;
			}

		private:
			// string literal
			template<std::size_t N>
			constexpr auto do_push(const char (&string)[N]) noexcept -> void
			{
				categories_.emplace_back(string);
			}

			// string literal
			constexpr auto do_push(const char* string) noexcept -> void
			{
				categories_.emplace_back(string);
			}

			// string/string_view
			constexpr auto do_push(const std::string_view string) noexcept -> void
			{
				categories_.emplace_back(string);
			}

			// string&&
			constexpr auto do_push(std::string&& string) noexcept -> void
			{
				categories_.emplace_back(std::move(string));
			}

			// const vector<string>&
			constexpr auto do_push(const test_categories_view_type categories) noexcept -> void
			{
				#if __has_cpp_attribute(cpp_lib_containers_ranges) and cpp_lib_containers_ranges >= 202202L
				categories_.append_range(categories.get());
				#else
				categories_.reserve(categories_.size() + categories.get().size());
				categories_.insert(categories_.end(), categories.get().begin(), categories.get().end());
				#endif
			}

			// vector<string>&&
			constexpr auto do_push(test_categories_type&& categories) noexcept -> void
			{
				#if __has_cpp_attribute(cpp_lib_containers_ranges) and cpp_lib_containers_ranges >= 202202L
				categories_.append_range(std::move(categories));
				#else
				categories_.reserve(categories_.size() + categories.size());
				categories_.insert(categories_.end(), std::make_move_iterator(categories.begin()), std::make_move_iterator(categories.end()));
				// silence warning
				(void)std::move(categories);
				#endif
			}

			[[nodiscard]] constexpr auto do_move() && noexcept -> D&&
			{
				return std::move(*static_cast<D*>(this));
			}

		public:
			template<typename... Args>
			[[nodiscard]] constexpr auto operator[](Args&&... args) && noexcept -> D&&
			{
				(this->do_push(std::forward<Args>(args)), ...);

				return std::move(*this).do_move();
			}
		};
	}

	template<meta::basic_fixed_string StringLiteral>
	class DispatcherSuite final
	{
		// fixme: lifetime
		suite_name_view_type name_;

	public:
		template<std::invocable InvocableType>
		constexpr explicit(false) DispatcherSuite(InvocableType invocable) noexcept //
			requires requires { +invocable; }
		{
			register_event(
				events::EventSuite
				{
						.name = StringLiteral.template as<suite_name_view_type>(),
						.suite = +invocable
				}
			);
		}
	};

	template<meta::basic_fixed_string StringLiteral>
	class DispatcherTestLiteral final : public dispatcher_detail::DispatcherTestBase<DispatcherTestLiteral<StringLiteral>>
	{
		friend class dispatcher_detail::DispatcherTestBase<DispatcherTestLiteral>;

		[[nodiscard]] constexpr auto name() const noexcept -> test_name_view_type
		{
			std::ignore = this;
			return StringLiteral.template as<suite_name_view_type>();
		}

	public:
		using dispatcher_detail::DispatcherTestBase<DispatcherTestLiteral>::operator=;
	};

	class DispatcherTest final : public dispatcher_detail::DispatcherTestBase<DispatcherTest>
	{
		friend class DispatcherTestBase;

		[[nodiscard]] constexpr auto name() const noexcept -> test_name_view_type
		{
			return name_;
		}

		test_name_view_type name_;

	public:
		using DispatcherTestBase::operator=;

		constexpr explicit DispatcherTest(const test_name_view_type name) noexcept
			: name_{name} {}
	};
}
