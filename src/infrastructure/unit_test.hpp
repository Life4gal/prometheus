// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// The code below is based on boost-ext/ut(https://github.com/boost-ext/ut)(http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.infrastructure:unit_test;

import std;
import gal.prometheus.functional;
import gal.prometheus.error;
import gal.prometheus.string;
import gal.prometheus.meta;

#else
#include <print>
#include <iostream>
#include <chrono>
#include <source_location>
#include <vector>

#include <prometheus/macro.hpp>
#include <functional/functional.hpp>
#include <error/error.hpp>
#include <string/string.hpp>
#include <meta/meta.hpp>
#endif

namespace gal::prometheus::unit_test
{
	namespace unit_test_detail
	{
		// The color used to print the results of test case execution.
		struct color_type
		{
			std::string_view none = "\033[0m";

			std::string_view fail  = "\033[31m\033[7m";
			std::string_view pass  = "\033[32m\033[7m";
			std::string_view skip  = "\033[33m\033[7m";
			std::string_view fatal = "\033[35m\033[7m";

			std::string_view suite      = "\033[34m\033[7m";
			std::string_view test       = "\033[36m\033[7m";
			std::string_view expression = "\033[38;5;207m\033[7m";
			std::string_view message    = "\033[38;5;27m\033[7m";
		};

		struct config
		{
			// Terminate the program immediately if the assertion fails.
			inline static bool fast_fail = false;
			// Terminate the program after n failed assertions (per suite).
			inline static auto abort_after_n_failures = std::numeric_limits<std::size_t>::max();
		};

		// Default Reporter
		class Reporter;
		// Executor, user can specify a customized reporter
		template<typename ReporterType = Reporter>
		class Executor;

		// Specify the category of test cases, used to filter test cases that do not need to be executed.
		using categories_type = std::vector<std::string_view>;

		struct tag_fatal {};

		struct tag_skip
		{
			constexpr static categories_type::value_type value = "@@__internal-skip__@@";
		};

		struct tag_silence
		{
			constexpr static categories_type::value_type value = "@@__internal-silence__@@";
		};

		struct tag_ignore_pass
		{
			constexpr static categories_type::value_type value = "@@__internal-ignore_pass__@@";
		};

		// namespace operand
		// OperandXXX
		namespace operand
		{
			template<typename>
			constexpr auto is_expression_v = false;

			template<typename Expression>
			// implicit
				requires std::is_convertible_v<Expression, bool>
			constexpr auto is_expression_v<Expression> = true;

			template<typename Expression>
			// explicit
				requires std::is_constructible_v<bool, Expression>
			constexpr auto is_expression_v<Expression> = true;

			template<typename Expression>
			concept expression_t = is_expression_v<Expression>;
		}

		// namespace events
		// EventXXX
		namespace events
		{
			class Event {};

			template<typename E>
			constexpr auto is_event = std::is_base_of_v<Event, E>;

			template<typename E>
			concept event_t = is_event<E>;
		}

		// namespace dispatcher
		// DispatcherXXX

		// namespace literals
		// xxx_operator

		// namespace operators
		// lhs [== / != / > / >= / < / <= / and / or / not] rhs

		namespace events
		{
			class EventSuiteBegin : public Event
			{
			public:
				std::string_view name;
			};

			class EventSuiteEnd : public Event
			{
			public:
				std::string_view name;
			};

			class EventTestBegin : public Event
			{
			public:
				std::string_view name;
			};

			class EventTestSkip : public Event
			{
			public:
				std::string_view name;
			};

			class EventTestEnd : public Event
			{
			public:
				std::string_view name;
			};

			class EventSilenceBegin : public Event {};

			class EventSilenceEnd : public Event {};

			class EventIgnorePassBegin : public Event {};

			class EventIgnorePassEnd : public Event {};

			template<operand::expression_t Expression>
			class EventAssertionPass : public Event
			{
			public:
				Expression           expression;
				std::source_location location;
			};

			template<operand::expression_t Expression>
			class EventAssertionFail : public Event
			{
			public:
				Expression           expression;
				std::source_location location;
			};

			class EventAssertionFatal : public Event
			{
			public:
				std::source_location location;
			};

			template<operand::expression_t Expression>
			class EventAssertionFatalSkip : public Event
			{
			public:
				Expression expression;

				std::source_location location;
			};

			template<typename MessageType>
				requires requires
				{
					std::declval<std::string>().append_range(std::declval<MessageType>());
				}
			class EventLog : public Event
			{
			public:
				using message_type = MessageType;

				message_type message;
			};

			EventLog(const char*) -> EventLog<std::basic_string_view<char>>;
			template<std::size_t N>
			EventLog(const char (&)[N]) -> EventLog<std::basic_string_view<char>>;

			class EventException : public Event
			{
			public:
				std::string_view message;

				[[nodiscard]] constexpr auto what() const noexcept -> std::string_view { return message; }
			};

			class EventSummary : public Event {};

			template<operand::expression_t Expression>
			class EventAssertion : public Event
			{
			public:
				Expression           expression;
				std::source_location location;

				[[nodiscard]] constexpr explicit operator EventAssertionPass<Expression>() const noexcept { return {.expression = expression, .location = location}; }

				[[nodiscard]] constexpr explicit operator EventAssertionFail<Expression>() const noexcept { return {.expression = expression, .location = location}; }

				[[nodiscard]] constexpr explicit operator EventAssertionFatalSkip<Expression>() const noexcept { return {.expression = expression, .location = location}; }
			};

			struct none {};

			template<typename InvocableType, typename Arg = none>
				requires std::is_invocable_v<InvocableType> or std::is_invocable_v<InvocableType, Arg>
			class EventTest : public Event
			{
			public:
				std::string_view name;

				categories_type categories;

				mutable InvocableType invocable;
				mutable Arg           arg;

				constexpr auto operator()() const -> void
				{
					return []<typename I, typename A>(I&& i, A&& a) -> void
					{
						if constexpr (requires { std::invoke(std::forward<I>(i)); }) { std::invoke(std::forward<I>(i)); }
						else if constexpr (requires { std::invoke(std::forward<I>(i), std::forward<A>(a)); }) { std::invoke(std::forward<I>(i), std::forward<A>(a)); }
						else if constexpr (requires { std::invoke(i.template operator()<A>()); }) { std::invoke(i.template operator()<A>()); }
						else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
					}(invocable, arg);
				}

				[[nodiscard]] constexpr explicit operator EventTestBegin() const noexcept { return {.name = name}; }

				[[nodiscard]] constexpr explicit operator EventTestEnd() const noexcept { return {.name = name}; }

				[[nodiscard]] constexpr explicit operator EventTestSkip() const noexcept { return {.name = name}; }
			};

			class EventSuite : public Event
			{
			public:
				std::string_view name;

				void (*function)();

				constexpr auto operator()() -> void { std::invoke(function); }
				constexpr auto operator()() const -> void { std::invoke(function); }

				[[nodiscard]] constexpr explicit operator EventSuiteBegin() const noexcept { return {.name = name}; }

				[[nodiscard]] constexpr explicit operator EventSuiteEnd() const noexcept { return {.name = name}; }
			};
		}

		namespace operand
		{
			constexpr auto pointer_to_string = []<typename Pointer>(const Pointer pointer) noexcept -> auto//
			{
				if constexpr (std::is_same_v<Pointer, std::nullptr_t>) { return std::string_view{"nullptr"}; }
				else
				{
					if (pointer) { return std::format("{}(0x{:x})", meta::name_of<Pointer>(), reinterpret_cast<std::uintptr_t>(pointer)); }
					return std::format("{}(0x00000000)", meta::name_of<Pointer>());
				}
			};

			constexpr auto range_to_string = []<std::ranges::range Range>(const Range& r) noexcept -> std::string//
			{
				if constexpr (std::is_default_constructible_v<std::formatter<std::ranges::range_value_t<Range>>>)
				{
					std::string result{};

					result.append_range(meta::name_of<Range>());
					result.push_back('{');
					std::ranges::for_each(
							r,
							[&result, done = false](const auto& v) mutable -> void
							{
								if (done) { result.push_back(','); }
								done = true;

								std::format_to(
										std::back_inserter(result),
										"{}",
										v);
							});
					result.push_back('}');

					return result;
				}
				else
				{
					if constexpr (requires { std::ranges::data(r); std::ranges::size(r); }) { return std::format("`{}(data: 0x{:x}, size: {})`", meta::name_of<Range>(), reinterpret_cast<std::uintptr_t>(std::ranges::data(r)), std::ranges::size(r)); }
					else { return std::format("`({})`", meta::name_of<Range>()); }
				}
			};

			constexpr auto expression_to_string = []<typename T>(const T& expression) noexcept -> decltype(auto)
			{
				if constexpr (std::is_constructible_v<std::string_view, T> or std::is_convertible_v<T, std::string_view>) { return std::string_view{expression}; }
				else if constexpr (requires { expression.to_string(); }) { return expression.to_string(); }
				// workaround vvv: dispatched_expression
				else if constexpr (requires { expression.expression.to_string(); }) { return expression.expression.to_string(); }
				else if constexpr (requires { std::format("{}", expression); }) { return std::format("{}", expression); }
				else { return std::format("`({})`", meta::name_of<T>()); }
			};

			template<typename T>
			[[nodiscard]] constexpr auto wrap_abs(const T value) noexcept -> T//
				requires requires { std::abs(value); }
			{
				GAL_PROMETHEUS_IF_CONSTANT_EVALUATED { return value > 0 ? value : -value; }

				return std::abs(value);
			}

			template<typename T>
			class OperandType
			{
			public:
				using operand_type_no_alias = T;

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string_view
				{
					(void)this;
					return meta::name_of<T>();
				}
			};

			template<typename T>
			constexpr auto is_operand_type_v = false;
			template<typename T>
			constexpr auto is_operand_type_v<OperandType<T>> = true;
			template<typename T>
			concept operand_type_t = is_operand_type_v<T>;

			template<typename T>
			class OperandValue
			{
			public:
				using value_type = T;

			private:
				value_type value_;

			public:
				constexpr explicit(false) OperandValue(const value_type value) noexcept(std::is_nothrow_copy_constructible_v<value_type>)//
					requires std::is_trivially_copy_constructible_v<value_type>
					: value_{value} {}

				constexpr explicit(false) OperandValue(const value_type& value) noexcept(std::is_nothrow_copy_constructible_v<value_type>)//
					requires(not std::is_trivially_copy_constructible_v<value_type>) and std::is_copy_constructible_v<value_type>
					: value_{value} {}

				constexpr explicit(false) OperandValue(value_type&& value) noexcept(std::is_nothrow_move_constructible_v<value_type>)//
					requires(not std::is_trivially_copy_constructible_v<value_type>) and std::is_move_constructible_v<value_type>
					: value_{std::move(value)} {}

				template<typename U>
					requires std::is_trivially_constructible_v<value_type, U>
				constexpr explicit(false) OperandValue(const value_type value) noexcept(std::is_nothrow_constructible_v<value_type, U>)//
					: value_{value} {}

				template<typename U>
					requires(not std::is_trivially_constructible_v<value_type, const U&>) and std::is_constructible_v<value_type, const U&>
				constexpr explicit(false) OperandValue(const U& value) noexcept(std::is_nothrow_constructible_v<value_type, const U&>)//
					: value_{value} {}

				template<typename U>
					requires(not std::is_trivially_constructible_v<value_type, U&&>) and std::is_constructible_v<value_type, U&&>
				constexpr explicit(false) OperandValue(U&& value) noexcept(std::is_nothrow_constructible_v<value_type, U&&>)//
					: value_{std::forward<U>(value)} {}

				template<typename... Args>
					requires(sizeof...(Args) != 1) and std::is_constructible_v<value_type, Args...>
				constexpr explicit OperandValue(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>)
					: value_{std::forward<Args>(args)...} {}

				[[nodiscard]] constexpr auto value() const noexcept -> const value_type& { return value_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string { return std::format("{}", value_); }
			};

			template<typename T>
			OperandValue(T) -> OperandValue<T>;

			template<>
			class OperandValue<bool>
			{
			public:
				using value_type = std::string_view;

				value_type message;

				// force construct from string_view
				constexpr explicit OperandValue(const value_type m) noexcept
					: message{m} {}

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string_view { return message; }
			};

			template<std::floating_point T>
			class OperandValue<T>
			{
			public:
				using value_type = T;

			private:
				value_type value_;
				value_type epsilon_;

			public:
				constexpr explicit(false) OperandValue(const value_type value, const value_type epsilon = std::numeric_limits<T>::epsilon()) noexcept
					: value_{value},
					  epsilon_{epsilon} {}

				[[nodiscard]] constexpr auto value() const noexcept -> value_type { return value_; }

				[[nodiscard]] constexpr auto epsilon() const noexcept -> value_type { return epsilon_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string { return std::format("{:.6g}", value_); }
			};

			template<typename>
			constexpr auto is_operand_value_v = false;
			template<typename T>
			constexpr auto is_operand_value_v<OperandValue<T>> = true;
			template<typename T>
			concept operand_value_t = is_operand_value_v<T>;
			template<typename>
			constexpr auto is_operand_value_boolean_v = false;
			template<>
			constexpr auto is_operand_value_boolean_v<OperandValue<bool>> = true;
			template<typename T>
			concept operand_value_boolean_t = is_operand_value_boolean_v<T>;
			template<typename>
			constexpr auto is_operand_value_floating_point_v = false;
			template<std::floating_point T>
			constexpr auto is_operand_value_floating_point_v<OperandValue<T>> = true;
			template<typename T>
			concept operand_value_floating_point_t = is_operand_value_floating_point_v<T>;

			template<char Value>
			class OperandConstantCharacter
			{
			public:
				using value_type = char;

				constexpr static value_type value = Value;

				[[nodiscard]] constexpr static auto to_string() noexcept -> std::string { return std::format("{}", value); }
			};

			template<std::integral auto Value>
			class OperandConstantIntegral
			{
			public:
				using value_type = std::remove_cvref_t<decltype(Value)>;

				constexpr static value_type value = Value;

				[[nodiscard]] constexpr auto operator-() const noexcept -> OperandConstantIntegral<-static_cast<std::make_signed_t<value_type>>(value)> { return {}; }

				[[nodiscard]] constexpr static auto to_string() noexcept -> std::string { return std::format("{}", value); }
			};

			template<std::floating_point auto Value, std::size_t DenominatorSize>
			class OperandConstantFloatingPoint
			{
			public:
				using value_type = std::remove_cvref_t<decltype(Value)>;

				constexpr static value_type  value            = Value;
				constexpr static std::size_t denominator_size = DenominatorSize;
				constexpr static value_type  epsilon          = [](std::size_t n) noexcept -> value_type
				{
					auto epsilon = static_cast<value_type>(1);
					while (n--) { epsilon /= static_cast<value_type>(10); }
					return epsilon;
				}(DenominatorSize);

				[[nodiscard]] constexpr auto operator-() const noexcept -> OperandConstantFloatingPoint<-value, DenominatorSize> { return {}; }

				[[nodiscard]] constexpr static auto to_string() noexcept -> std::string { return std::format("{:.6g}", value); }
			};

			template<char... Cs>
			class OperandConstantAuto
			{
			public:
				constexpr static auto char_list = functional::char_list<Cs...>;

				template<typename T>
				struct representation;

				template<char C>
				struct representation<OperandConstantCharacter<C>>
				{
					using type = OperandConstantCharacter<char_list.template nth_value<0>()>;
				};

				template<std::integral T>
				struct representation<T>
				{
					using type = std::conditional_t<std::is_same_v<T, char>, OperandConstantCharacter<char_list.template nth_value<0>()>, OperandConstantIntegral<char_list.template to_integral<T>()>>;
				};

				template<std::integral auto Value>
				struct representation<OperandConstantIntegral<Value>>
				{
					using type = OperandConstantIntegral<char_list.template to_integral<OperandConstantIntegral<Value>::value_type>()>;
				};

				template<std::floating_point T>
				struct representation<T>
				{
					using type = OperandConstantFloatingPoint<char_list.template to_floating_point<T>(), char_list.denominator_length()>;
				};

				template<std::floating_point auto Value, std::size_t DenominatorSize>
				struct representation<OperandConstantFloatingPoint<Value, DenominatorSize>>
				{
					using type = OperandConstantFloatingPoint<char_list.template to_floating_point<OperandConstantFloatingPoint<Value, DenominatorSize>::value_type>(), char_list.denominator_length()>;
				};

				template<typename T>
				using rebind = typename representation<T>::type;
			};

			template<typename>
			constexpr auto is_operand_constant_v = false;
			template<char Value>
			constexpr auto is_operand_constant_v<OperandConstantCharacter<Value>> = true;
			template<std::integral auto Value>
			constexpr auto is_operand_constant_v<OperandConstantIntegral<Value>> = true;
			template<std::floating_point auto Value, std::size_t DenominatorSize>
			constexpr auto is_operand_constant_v<OperandConstantFloatingPoint<Value, DenominatorSize>> = true;
			template<char... Cs>
			constexpr auto is_operand_constant_v<OperandConstantAuto<Cs...>> = true;
			template<typename T>
			concept operand_constant_t = is_operand_constant_v<T>;

			template<typename>
			constexpr auto is_operand_constant_character_v = false;
			template<char Value>
			constexpr auto is_operand_constant_character_v<OperandConstantCharacter<Value>> = true;
			template<typename T>
			concept operand_constant_character_t = is_operand_constant_character_v<T>;
			template<typename>
			constexpr auto is_operand_constant_integral_v = false;
			template<std::integral auto Value>
			constexpr auto is_operand_constant_integral_v<OperandConstantIntegral<Value>> = true;
			template<typename T>
			concept operand_constant_integral_t = is_operand_constant_integral_v<T>;
			template<typename T>
			constexpr auto is_operand_constant_floating_point_v = false;
			template<std::floating_point auto Value, std::size_t DenominatorSize>
			constexpr auto is_operand_constant_floating_point_v<OperandConstantFloatingPoint<Value, DenominatorSize>> = true;
			template<typename T>
			concept operand_constant_floating_point_t = is_operand_constant_floating_point_v<T>;
			template<typename>
			constexpr auto is_operand_constant_auto_v = false;
			template<char... Cs>
			constexpr auto is_operand_constant_auto_v<OperandConstantAuto<Cs...>> = true;
			template<typename T>
			concept operand_constant_auto_t = is_operand_constant_auto_v<T>;

			class OperandCompareIdentity
			{
			public:
				using value_type = OperandValue<bool>;

			private:
				value_type value_;
				bool       result_;

			public:
				constexpr OperandCompareIdentity(const value_type& value, const bool result) noexcept
					: value_{value},
					  result_{result} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> decltype(auto) { return value_.to_string(); }
			};

			template<typename Expression>
				requires operand::expression_t<Expression> or requires(Expression e) { static_cast<bool>(e.expression); }
			class OperandConstantExpression
			{
				constexpr static auto etv(Expression e) noexcept -> bool
				{
					if constexpr (requires { static_cast<bool>(e); }) { return static_cast<bool>(e); }
					else if constexpr (requires { static_cast<bool>(e.expression); }) { return static_cast<bool>(e.expression); }
					else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
				}

			public:
				bool value;

				// fixme: We shouldn't care what the result of the expression is, we just need to generate a compile error if we can't evaluate the expression during compilation.
				constexpr explicit(false) OperandConstantExpression(Expression expression) noexcept
					: value{etv(expression)} {}

				constexpr explicit operator bool() const noexcept { return value; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string_view
				{
					// fixme: Is there any way to preserve the expression?
					return value ? "constant<true>" : "constant<false>";
				}
			};

			template<typename>
			constexpr auto is_operand_constant_expression_v = false;
			template<typename Expression>
			constexpr auto is_operand_constant_expression_v<OperandConstantExpression<Expression>> = true;

			template<typename L, typename R>
				requires(not(is_operand_constant_auto_v<L> and is_operand_constant_auto_v<R>))
			class OperandCompareEqual
			{
			public:
				using left_type  = L;
				using right_type = R;

			private:
				left_type  left_;
				right_type right_;
				bool       result_;

				[[nodiscard]] constexpr auto do_check() const noexcept -> bool
				{
					using std::operator==;

					if constexpr (is_operand_type_v<left_type> and is_operand_type_v<right_type>)
					{
						return std::is_same_v<
							typename left_type::operand_type_no_alias,
							typename right_type::operand_type_no_alias>;
					}

					else if constexpr (requires { left_type::value == right_type::value; }) { return left_type::value == right_type::value; }
					else if constexpr (requires { left_type::value == right_; }) { return left_type::value == right_; }
					else if constexpr (requires { left_ == right_type::value; }) { return left_ == right_type::value; }
					else if constexpr (requires { left_ == right_; }) { return left_ == right_; }

					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value == static_cast<left_type>(right_type::value); }) { return left_type::value == static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) == right_type::value; }) { return static_cast<right_type>(left_type::value) == right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value == static_cast<left_type>(right_); }) { return left_type::value == static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) == right_; }) { return static_cast<right_type>(left_type::value) == right_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ == static_cast<left_type>(right_type::value); }) { return left_ == static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) == right_type::value; }) { return static_cast<right_type>(left_) == right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ == static_cast<left_type>(right_); }) { return left_ == static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) == right_; }) { return static_cast<right_type>(left_) == right_; }

					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value == left_type{right_type::value}; }) { return left_type::value == left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} == right_type::value; }) { return right_type{left_type::value} == right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value == left_type{right_}; }) { return left_type::value == left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} == right_; }) { return right_type{left_type::value} == right_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ == left_type{right_type::value}; }) { return left_ == left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} == right_type::value; }) { return right_type{left_} == right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ == left_type{right_}; }) { return left_ == left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} == right_; }) { return right_type{left_} == right_; }

					else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
				}

			public:
				template<typename Left, typename Right>
				constexpr explicit(false) OperandCompareEqual(Left&& left, Right&& right) noexcept
					: left_{std::forward<Left>(left)},
					  right_{std::forward<Right>(right)},
					  result_{do_check()} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
				{
					if constexpr (is_operand_constant_auto_v<left_type>) { return std::format("{} == {}", expression_to_string(left_type{}), expression_to_string(right_)); }
					else if constexpr (is_operand_constant_auto_v<right_type>) { return std::format("{} == {}", expression_to_string(left_), expression_to_string(right_type{})); }
					else if constexpr (std::is_pointer_v<left_type> or std::is_same_v<left_type, std::nullptr_t>)
					{
						static_assert(std::is_pointer_v<right_type> or std::is_same_v<right_type, std::nullptr_t>);

						// note: check the right side first, because it is more likely to be null
						if (right_ == nullptr) { return std::format("{} == nullptr", pointer_to_string(left_)); }
						if (left_ == nullptr) { return std::format("{} == nullptr", pointer_to_string(right_)); }

						return std::format("{} == {}", pointer_to_string(left_), pointer_to_string(right_));
					}
					else
					{
						const auto left_string = [this]
						{
							if constexpr (std::ranges::range<left_type> and not std::is_constructible_v<std::string_view, left_type>) { return range_to_string(left_); }
							else { return expression_to_string(left_); }
						}();
						const auto right_string = [this]
						{
							if constexpr (std::ranges::range<right_type> and not std::is_constructible_v<std::string_view, right_type>) { return range_to_string(right_); }
							else { return expression_to_string(right_); }
						}();

						return std::format("{} == {}", left_string, right_string);
					}
				}
			};

			template<typename L, typename R, typename Epsilon>
				requires(not(is_operand_constant_auto_v<L> and is_operand_constant_auto_v<R>))
			class OperandCompareApprox
			{
			public:
				using left_type    = L;
				using right_type   = R;
				using epsilon_type = Epsilon;

			private:
				left_type    left_;
				right_type   right_;
				epsilon_type epsilon_;
				bool         result_;

				[[nodiscard]] constexpr auto do_check() noexcept -> bool
				{
					using std::operator==;
					using std::operator-;
					using std::operator<;

					if constexpr (requires { wrap_abs(left_type::value - right_type::value) < epsilon_type::value; }) { return wrap_abs(left_type::value - right_type::value) < epsilon_type::value; }
					else if constexpr (requires { wrap_abs(left_type::value - right_) < epsilon_type::value; }) { return wrap_abs(left_type::value - right_) < epsilon_type::value; }
					else if constexpr (requires { wrap_abs(left_ - right_type::value) < epsilon_type::value; }) { return wrap_abs(left_ - right_type::value) < epsilon_type::value; }
					else if constexpr (requires { wrap_abs(left_type::value - right_) < epsilon_; }) { return wrap_abs(left_type::value - right_) < epsilon_; }
					else if constexpr (requires { wrap_abs(left_ - right_type::value) < epsilon_; }) { return wrap_abs(left_ - right_type::value) < epsilon_; }
					else if constexpr (requires { wrap_abs(left_ - right_) < epsilon_; }) { return wrap_abs(left_ - right_) < epsilon_; }

					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_type::value - static_cast<left_type>(right_type::value)) < epsilon_type::value; }) { return wrap_abs(left_type::value - static_cast<left_type>(right_type::value)) < epsilon_type::value; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_type::value) - right_type::value) < epsilon_type::value; }) { return wrap_abs(static_cast<right_type>(left_type::value) - right_type::value) < epsilon_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_type::value - static_cast<left_type>(right_)) < epsilon_type::value; }) { return wrap_abs(left_type::value - static_cast<left_type>(right_)) < epsilon_type::value; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_type::value) - right_) < epsilon_type::value; }) { return wrap_abs(static_cast<right_type>(left_type::value) - right_) < epsilon_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_ - static_cast<left_type>(right_type::value)) < epsilon_type::value; }) { return wrap_abs(left_ - static_cast<left_type>(right_type::value)) < epsilon_type::value; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_) - right_type::value) < epsilon_type::value; }) { return wrap_abs(static_cast<right_type>(left_) - right_type::value) < epsilon_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_type::value - static_cast<left_type>(right_)) < epsilon_; }) { return wrap_abs(left_type::value - static_cast<left_type>(right_)) < epsilon_; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_type::value) - right_) < epsilon_; }) { return wrap_abs(static_cast<right_type>(left_type::value) - right_) < epsilon_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_ - static_cast<left_type>(right_type::value)) < epsilon_; }) { return wrap_abs(left_ - static_cast<left_type>(right_type::value)) < epsilon_; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_) - right_type::value) < epsilon_; }) { return wrap_abs(static_cast<right_type>(left_) - right_type::value) < epsilon_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_ - static_cast<left_type>(right_)) < epsilon_; }) { return wrap_abs(left_ - static_cast<left_type>(right_)) < epsilon_; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_) - right_) < epsilon_; }) { return wrap_abs(static_cast<right_type>(left_) - right_) < epsilon_; }

					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_type::value - left_type{right_type::value}) < epsilon_type::value; }) { return wrap_abs(left_type::value - left_type{right_type::value}) < epsilon_type::value; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_type::value} - right_type::value) < epsilon_type::value; }) { return wrap_abs(right_type{left_type::value} - right_type::value) < epsilon_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_type::value - left_type{right_}) < epsilon_type::value; }) { return wrap_abs(left_type::value - left_type{right_}) < epsilon_type::value; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_type::value} - right_) < epsilon_type::value; }) { return wrap_abs(right_type{left_type::value} - right_) < epsilon_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_ - left_type{right_type::value}) < epsilon_type::value; }) { return wrap_abs(left_ - left_type{right_type::value}) < epsilon_type::value; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_} - right_type::value) < epsilon_type::value; }) { return wrap_abs(right_type{left_} - right_type::value) < epsilon_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_type::value - left_type{right_}) < epsilon_; }) { return wrap_abs(left_type::value - left_type{right_}) < epsilon_; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_type::value} - right_) < epsilon_; }) { return wrap_abs(right_type{left_type::value} - right_) < epsilon_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_ - left_type{right_type::value}) < epsilon_; }) { return wrap_abs(left_ - left_type{right_type::value}) < epsilon_; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_} - right_type::value) < epsilon_; }) { return wrap_abs(right_type{left_} - right_type::value) < epsilon_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_ - left_type{right_}) < epsilon_; }) { return wrap_abs(left_ - left_type{right_}) < epsilon_; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_} - right_) < epsilon_; }) { return wrap_abs(right_type{left_} - right_) < epsilon_; }

					else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
				}

			public:
				constexpr explicit(false) OperandCompareApprox(
						const left_type&    left,
						const right_type&   right,
						const epsilon_type& epsilon) noexcept
					: left_{left},
					  right_{right},
					  epsilon_{epsilon},
					  result_{do_check()} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
				{
					if constexpr (is_operand_constant_auto_v<left_type>)
					{
						if constexpr (is_operand_constant_auto_v<epsilon_type>) { return std::format("{} ≈≈ {} (+/- {})", expression_to_string(left_type{}), expression_to_string(right_), expression_to_string(epsilon_type{})); }
						else { return std::format("{} ≈≈ {} (+/- {})", expression_to_string(left_type{}), expression_to_string(right_), expression_to_string(epsilon_)); }
					}
					else if constexpr (is_operand_constant_auto_v<right_type>)
					{
						if constexpr (is_operand_constant_auto_v<epsilon_type>) { return std::format("{} ≈≈ {} (+/- {})", expression_to_string(left_), expression_to_string(right_type{}), expression_to_string(epsilon_type{})); }
						else { return std::format("{} ≈≈ {} (+/- {})", expression_to_string(left_), expression_to_string(right_type{}), expression_to_string(epsilon_)); }
					}
					else { return std::format("{} ≈≈ {} (+/- {})", expression_to_string(left_), expression_to_string(right_), expression_to_string(epsilon_)); }
				}
			};

			template<typename L, typename R>
				requires(not(is_operand_constant_auto_v<L> and is_operand_constant_auto_v<R>))
			class OperandCompareNotEqual
			{
			public:
				using left_type  = L;
				using right_type = R;

			private:
				left_type  left_;
				right_type right_;
				bool       result_;

				[[nodiscard]] constexpr auto do_check() const noexcept -> bool
				{
					using std::operator==;
					using std::operator!=;

					if constexpr (is_operand_type_v<left_type> and is_operand_type_v<right_type>)
					{
						return not std::is_same_v<
							typename left_type::operand_type_no_alias,
							typename right_type::operand_type_no_alias>;
					}

					else if constexpr (requires { left_type::value != right_type::value; }) { return left_type::value != right_type::value; }
					else if constexpr (requires { left_type::value != right_; }) { return left_type::value != right_; }
					else if constexpr (requires { left_ != right_type::value; }) { return left_ != right_type::value; }
					else if constexpr (requires { left_ != right_; }) { return left_ != right_; }

					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value != static_cast<left_type>(right_type::value); }) { return left_type::value != static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) != right_type::value; }) { return static_cast<right_type>(left_type::value) != right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value != static_cast<left_type>(right_); }) { return left_type::value != static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) != right_; }) { return static_cast<right_type>(left_type::value) != right_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ != static_cast<left_type>(right_type::value); }) { return left_ != static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) != right_type::value; }) { return static_cast<right_type>(left_) != right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ != static_cast<left_type>(right_); }) { return left_ != static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) != right_; }) { return static_cast<right_type>(left_) != right_; }

					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value != left_type{right_type::value}; }) { return left_type::value != left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} != right_type::value; }) { return right_type{left_type::value} != right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value != left_type{right_}; }) { return left_type::value != left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} != right_; }) { return right_type{left_type::value} != right_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ != left_type{right_type::value}; }) { return left_ != left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} != right_type::value; }) { return right_type{left_} != right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ != left_type{right_}; }) { return left_ != left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} != right_; }) { return right_type{left_} != right_; }

					else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
				}

			public:
				template<typename Left, typename Right>
				constexpr explicit(false) OperandCompareNotEqual(Left&& left, Right&& right) noexcept
					: left_{std::forward<Left>(left)},
					  right_{std::forward<Right>(right)},
					  result_{do_check()} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
				{
					if constexpr (is_operand_constant_auto_v<left_type>) { return std::format("{} != {}", expression_to_string(left_type{}), expression_to_string(right_)); }
					else if constexpr (is_operand_constant_auto_v<right_type>) { return std::format("{} != {}", expression_to_string(left_), expression_to_string(right_type{})); }
					else if constexpr (std::is_pointer_v<left_type> or std::is_same_v<left_type, std::nullptr_t>)
					{
						static_assert(std::is_pointer_v<right_type> or std::is_same_v<right_type, std::nullptr_t>);

						// note: check the right side first, because it is more likely to be null
						if (right_ == nullptr) { return std::format("{} != nullptr", pointer_to_string(left_)); }
						if (left_ == nullptr) { return std::format("{} != nullptr", pointer_to_string(right_)); }

						return std::format("{} != {}", pointer_to_string(left_), pointer_to_string(right_));
					}
					else
					{
						const auto left_string = [this]
						{
							if constexpr (std::ranges::range<left_type> and not std::is_constructible_v<std::string_view, left_type>) { return range_to_string(left_); }
							else { return expression_to_string(left_); }
						}();
						const auto right_string = [this]
						{
							if constexpr (std::ranges::range<right_type> and not std::is_constructible_v<std::string_view, right_type>) { return range_to_string(right_); }
							else { return expression_to_string(right_); }
						}();

						return std::format("{} != {}", left_string, right_string);
					}
				}
			};

			template<typename L, typename R, typename Epsilon>
				requires(not(is_operand_constant_auto_v<L> and is_operand_constant_auto_v<R>))
			class OperandCompareNotApprox
			{
			public:
				using left_type    = L;
				using right_type   = R;
				using epsilon_type = Epsilon;

			private:
				left_type    left_;
				right_type   right_;
				epsilon_type epsilon_;
				bool         result_;

				[[nodiscard]] constexpr auto do_check() noexcept -> bool
				{
					using std::operator==;
					using std::operator!=;
					using std::operator-;
					using std::operator>;

					if constexpr (requires { wrap_abs(left_type::value - right_type::value) > epsilon_type::value; }) { return wrap_abs(left_type::value - right_type::value) > epsilon_type::value; }
					else if constexpr (requires { wrap_abs(left_type::value - right_) > epsilon_type::value; }) { return wrap_abs(left_type::value - right_) > epsilon_type::value; }
					else if constexpr (requires { wrap_abs(left_ - right_type::value) > epsilon_type::value; }) { return wrap_abs(left_ - right_type::value) > epsilon_type::value; }
					else if constexpr (requires { wrap_abs(left_type::value - right_) > epsilon_; }) { return wrap_abs(left_type::value - right_) > epsilon_; }
					else if constexpr (requires { wrap_abs(left_ - right_type::value) > epsilon_; }) { return wrap_abs(left_ - right_type::value) > epsilon_; }
					else if constexpr (requires { wrap_abs(left_ - right_) > epsilon_; }) { return wrap_abs(left_ - right_) > epsilon_; }

					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_type::value - static_cast<left_type>(right_type::value)) > epsilon_type::value; }) { return wrap_abs(left_type::value - static_cast<left_type>(right_type::value)) > epsilon_type::value; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_type::value) - right_type::value) > epsilon_type::value; }) { return wrap_abs(static_cast<right_type>(left_type::value) - right_type::value) > epsilon_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_type::value - static_cast<left_type>(right_)) > epsilon_type::value; }) { return wrap_abs(left_type::value - static_cast<left_type>(right_)) > epsilon_type::value; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_type::value) - right_) > epsilon_type::value; }) { return wrap_abs(static_cast<right_type>(left_type::value) - right_) > epsilon_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_ - static_cast<left_type>(right_type::value)) > epsilon_type::value; }) { return wrap_abs(left_ - static_cast<left_type>(right_type::value)) > epsilon_type::value; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_) - right_type::value) > epsilon_type::value; }) { return wrap_abs(static_cast<right_type>(left_) - right_type::value) > epsilon_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_type::value - static_cast<left_type>(right_)) > epsilon_; }) { return wrap_abs(left_type::value - static_cast<left_type>(right_)) > epsilon_; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_type::value) - right_) > epsilon_; }) { return wrap_abs(static_cast<right_type>(left_type::value) - right_) > epsilon_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_ - static_cast<left_type>(right_type::value)) > epsilon_; }) { return wrap_abs(left_ - static_cast<left_type>(right_type::value)) > epsilon_; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_) - right_type::value) > epsilon_; }) { return wrap_abs(static_cast<right_type>(left_) - right_type::value) > epsilon_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { wrap_abs(left_ - static_cast<left_type>(right_)) > epsilon_; }) { return wrap_abs(left_ - static_cast<left_type>(right_)) > epsilon_; }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { wrap_abs(static_cast<right_type>(left_) - right_) > epsilon_; }) { return wrap_abs(static_cast<right_type>(left_) - right_) > epsilon_; }

					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_type::value - left_type{right_type::value}) > epsilon_type::value; }) { return wrap_abs(left_type::value - left_type{right_type::value}) > epsilon_type::value; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_type::value} - right_type::value) > epsilon_type::value; }) { return wrap_abs(right_type{left_type::value} - right_type::value) > epsilon_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_type::value - left_type{right_}) > epsilon_type::value; }) { return wrap_abs(left_type::value - left_type{right_}) > epsilon_type::value; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_type::value} - right_) > epsilon_type::value; }) { return wrap_abs(right_type{left_type::value} - right_) > epsilon_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_ - left_type{right_type::value}) > epsilon_type::value; }) { return wrap_abs(left_ - left_type{right_type::value}) > epsilon_type::value; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_} - right_type::value) > epsilon_type::value; }) { return wrap_abs(right_type{left_} - right_type::value) > epsilon_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_type::value - left_type{right_}) > epsilon_; }) { return wrap_abs(left_type::value - left_type{right_}) > epsilon_; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_type::value} - right_) > epsilon_; }) { return wrap_abs(right_type{left_type::value} - right_) > epsilon_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_ - left_type{right_type::value}) > epsilon_; }) { return wrap_abs(left_ - left_type{right_type::value}) > epsilon_; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_} - right_type::value) > epsilon_; }) { return wrap_abs(right_type{left_} - right_type::value) > epsilon_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { wrap_abs(left_ - left_type{right_}) > epsilon_; }) { return wrap_abs(left_ - left_type{right_}) > epsilon_; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { wrap_abs(right_type{left_} - right_) > epsilon_; }) { return wrap_abs(right_type{left_} - right_) > epsilon_; }

					else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
				}

			public:
				constexpr explicit(false) OperandCompareNotApprox(
						const left_type&    left,
						const right_type&   right,
						const epsilon_type& epsilon) noexcept
					: left_{left},
					  right_{right},
					  epsilon_{epsilon},
					  result_{do_check()} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
				{
					if constexpr (is_operand_constant_auto_v<left_type>)
					{
						if constexpr (is_operand_constant_auto_v<epsilon_type>) { return std::format("{} !≈ {} (+/- {})", expression_to_string(left_type{}), expression_to_string(right_), expression_to_string(epsilon_type{})); }
						else { return std::format("{} !≈ {} (+/- {})", expression_to_string(left_type{}), expression_to_string(right_), expression_to_string(epsilon_)); }
					}
					else if constexpr (is_operand_constant_auto_v<right_type>)
					{
						if constexpr (is_operand_constant_auto_v<epsilon_type>) { return std::format("{} !≈ {} (+/- {})", expression_to_string(left_), expression_to_string(right_type{}), expression_to_string(epsilon_type{})); }
						else { return std::format("{} !≈ {} (+/- {})", expression_to_string(left_), expression_to_string(right_type{}), expression_to_string(epsilon_)); }
					}
					else { return std::format("{} !≈ {} (+/- {})", expression_to_string(left_), expression_to_string(right_), expression_to_string(epsilon_)); }
				}
			};

			template<typename L, typename R>
				requires(not(is_operand_constant_auto_v<L> and is_operand_constant_auto_v<R>))
			class OperandCompareGreaterThan
			{
			public:
				using left_type  = L;
				using right_type = R;

			private:
				left_type  left_;
				right_type right_;
				bool       result_;

				[[nodiscard]] constexpr auto do_check() const noexcept -> bool
				{
					using std::operator>;

					if constexpr (requires { left_type::value > right_type::value; }) { return left_type::value > right_type::value; }
					else if constexpr (requires { left_type::value > right_; }) { return left_type::value > right_; }
					else if constexpr (requires { left_ > right_type::value; }) { return left_ > right_type::value; }
					else if constexpr (requires { left_ > right_; }) { return left_ > right_; }

					if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value > static_cast<left_type>(right_type::value); }) { return left_type::value > static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) > right_type::value; }) { return static_cast<right_type>(left_type::value) > right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value > static_cast<left_type>(right_); }) { return left_type::value > static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) > right_; }) { return static_cast<right_type>(left_type::value) > right_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ > static_cast<left_type>(right_type::value); }) { return left_ > static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) > right_type::value; }) { return static_cast<right_type>(left_) > right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ > static_cast<left_type>(right_); }) { return left_ > static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) > right_; }) { return static_cast<right_type>(left_) > right_; }

					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value > left_type{right_type::value}; }) { return left_type::value > left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} > right_type::value; }) { return right_type{left_type::value} > right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value > left_type{right_}; }) { return left_type::value > left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} > right_; }) { return right_type{left_type::value} > right_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ > left_type{right_type::value}; }) { return left_ > left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} > right_type::value; }) { return right_type{left_} > right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ > left_type{right_}; }) { return left_ > left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} > right_; }) { return right_type{left_} > right_; }

					else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
				}

			public:
				template<typename Left, typename Right>
				constexpr explicit(false) OperandCompareGreaterThan(Left&& left, Right&& right) noexcept
					: left_{std::forward<Left>(left)},
					  right_{std::forward<Right>(right)},
					  result_{do_check()} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
				{
					if constexpr (is_operand_constant_auto_v<left_type>) { return std::format("{} > {}", expression_to_string(left_type{}), expression_to_string(right_)); }
					else if constexpr (is_operand_constant_auto_v<right_type>) { return std::format("{} > {}", expression_to_string(left_), expression_to_string(right_type{})); }
					else
					{
						const auto left_string = [this]
						{
							if constexpr (std::is_same_v<left_type, std::nullptr_t> or std::is_pointer_v<left_type>) { return pointer_to_string(left_); }
							else if constexpr (std::ranges::range<left_type> and not std::is_constructible_v<std::string_view, left_type>) { return range_to_string(left_); }
							else { return expression_to_string(left_); }
						}();
						const auto right_string = [this]
						{
							if constexpr (std::is_same_v<right_type, std::nullptr_t> or std::is_pointer_v<right_type>) { return pointer_to_string(right_); }
							else if constexpr (std::ranges::range<right_type> and not std::is_constructible_v<std::string_view, right_type>) { return range_to_string(right_); }
							else { return expression_to_string(right_); }
						}();

						return std::format("{} > {}", left_string, right_string);
					}
				}
			};

			template<typename L, typename R>
				requires(not(is_operand_constant_auto_v<L> and is_operand_constant_auto_v<R>))
			class OperandCompareGreaterEqual
			{
			public:
				using left_type  = L;
				using right_type = R;

			private:
				left_type  left_;
				right_type right_;
				bool       result_;

				[[nodiscard]] constexpr auto do_check() const noexcept -> bool
				{
					using std::operator>=;

					if constexpr (requires { left_type::value >= right_type::value; }) { return left_type::value >= right_type::value; }
					else if constexpr (requires { left_type::value >= right_; }) { return left_type::value >= right_; }
					else if constexpr (requires { left_ >= right_type::value; }) { return left_ >= right_type::value; }
					else if constexpr (requires { left_ >= right_; }) { return left_ >= right_; }

					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value >= static_cast<left_type>(right_type::value); }) { return left_type::value >= static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) >= right_type::value; }) { return static_cast<right_type>(left_type::value) >= right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value >= static_cast<left_type>(right_); }) { return left_type::value >= static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) >= right_; }) { return static_cast<right_type>(left_type::value) >= right_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ >= static_cast<left_type>(right_type::value); }) { return left_ >= static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) >= right_type::value; }) { return static_cast<right_type>(left_) >= right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ >= static_cast<left_type>(right_); }) { return left_ >= static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) >= right_; }) { return static_cast<right_type>(left_) >= right_; }

					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value >= left_type{right_type::value}; }) { return left_type::value >= left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} >= right_type::value; }) { return right_type{left_type::value} >= right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value >= left_type{right_}; }) { return left_type::value >= left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} >= right_; }) { return right_type{left_type::value} >= right_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ >= left_type{right_type::value}; }) { return left_ >= left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} >= right_type::value; }) { return right_type{left_} >= right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ >= left_type{right_}; }) { return left_ >= left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} >= right_; }) { return right_type{left_} >= right_; }

					else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
				}

			public:
				template<typename Left, typename Right>
				constexpr explicit(false) OperandCompareGreaterEqual(Left&& left, Right&& right) noexcept
					: left_{std::forward<Left>(left)},
					  right_{std::forward<Right>(right)},
					  result_{do_check()} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
				{
					if constexpr (is_operand_constant_auto_v<left_type>) { return std::format("{} >= {}", expression_to_string(left_type{}), expression_to_string(right_)); }
					else if constexpr (is_operand_constant_auto_v<right_type>) { return std::format("{} >= {}", expression_to_string(left_), expression_to_string(right_type{})); }
					else
					{
						const auto left_string = [this]
						{
							if constexpr (std::is_same_v<left_type, std::nullptr_t> or std::is_pointer_v<left_type>) { return pointer_to_string(left_); }
							else if constexpr (std::ranges::range<left_type> and not std::is_constructible_v<std::string_view, left_type>) { return range_to_string(left_); }
							else { return expression_to_string(left_); }
						}();
						const auto right_string = [this]
						{
							if constexpr (std::is_same_v<right_type, std::nullptr_t> or std::is_pointer_v<right_type>) { return pointer_to_string(right_); }
							else if constexpr (std::ranges::range<right_type> and not std::is_constructible_v<std::string_view, right_type>) { return range_to_string(right_); }
							else { return expression_to_string(right_); }
						}();

						return std::format("{} >= {}", left_string, right_string);
					}
				}
			};

			template<typename L, typename R>
				requires(not(is_operand_constant_auto_v<L> and is_operand_constant_auto_v<R>))
			class OperandCompareLessThan
			{
			public:
				using left_type  = L;
				using right_type = R;

			private:
				left_type  left_;
				right_type right_;
				bool       result_;

				[[nodiscard]] constexpr auto do_check() const noexcept -> bool
				{
					using std::operator<;

					if constexpr (requires { left_type::value < right_type::value; }) { return left_type::value < right_type::value; }
					else if constexpr (requires { left_type::value < right_; }) { return left_type::value < right_; }
					else if constexpr (requires { left_ < right_type::value; }) { return left_ < right_type::value; }
					else if constexpr (requires { left_ < right_; }) { return left_ < right_; }

					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value < static_cast<left_type>(right_type::value); }) { return left_type::value < static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) < right_type::value; }) { return static_cast<right_type>(left_type::value) < right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value < static_cast<left_type>(right_); }) { return left_type::value < static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) < right_; }) { return static_cast<right_type>(left_type::value) < right_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ < static_cast<left_type>(right_type::value); }) { return left_ < static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) < right_type::value; }) { return static_cast<right_type>(left_) < right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ < static_cast<left_type>(right_); }) { return left_ < static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) < right_; }) { return static_cast<right_type>(left_) < right_; }

					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value < left_type{right_type::value}; }) { return left_type::value < left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} < right_type::value; }) { return right_type{left_type::value} < right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value < left_type{right_}; }) { return left_type::value < left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} < right_; }) { return right_type{left_type::value} < right_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ < left_type{right_type::value}; }) { return left_ < left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} < right_type::value; }) { return right_type{left_} < right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ < left_type{right_}; }) { return left_ < left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} < right_; }) { return right_type{left_} < right_; }

					else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
				}

			public:
				template<typename Left, typename Right>
				constexpr explicit(false) OperandCompareLessThan(Left&& left, Right&& right) noexcept
					: left_{std::forward<Left>(left)},
					  right_{std::forward<Right>(right)},
					  result_{do_check()} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
				{
					if constexpr (is_operand_constant_auto_v<left_type>) { return std::format("{} < {}", expression_to_string(left_type{}), expression_to_string(right_)); }
					else if constexpr (is_operand_constant_auto_v<right_type>) { return std::format("{} < {}", expression_to_string(left_), expression_to_string(right_type{})); }
					else
					{
						const auto left_string = [this]
						{
							if constexpr (std::is_same_v<left_type, std::nullptr_t> or std::is_pointer_v<left_type>) { return pointer_to_string(left_); }
							else if constexpr (std::ranges::range<left_type> and not std::is_constructible_v<std::string_view, left_type>) { return range_to_string(left_); }
							else { return expression_to_string(left_); }
						}();
						const auto right_string = [this]
						{
							if constexpr (std::is_same_v<right_type, std::nullptr_t> or std::is_pointer_v<right_type>) { return pointer_to_string(right_); }
							else if constexpr (std::ranges::range<right_type> and not std::is_constructible_v<std::string_view, right_type>) { return range_to_string(right_); }
							else { return expression_to_string(right_); }
						}();

						return std::format("{} < {}", left_string, right_string);
					}
				}
			};

			template<typename L, typename R>
				requires(not(is_operand_constant_auto_v<L> and is_operand_constant_auto_v<R>))
			class OperandCompareLessEqual
			{
			public:
				using left_type  = L;
				using right_type = R;

			private:
				left_type  left_;
				right_type right_;
				bool       result_;

				[[nodiscard]] constexpr auto do_check() const noexcept -> bool
				{
					using std::operator<=;

					if constexpr (requires { left_type::value <= right_type::value; }) { return left_type::value <= right_type::value; }
					else if constexpr (requires { left_type::value <= right_; }) { return left_type::value <= right_; }
					else if constexpr (requires { left_ <= right_type::value; }) { return left_ <= right_type::value; }
					else if constexpr (requires { left_ <= right_; }) { return left_ <= right_; }

					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value <= static_cast<left_type>(right_type::value); }) { return left_type::value <= static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) <= right_type::value; }) { return static_cast<right_type>(left_type::value) <= right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_type::value <= static_cast<left_type>(right_); }) { return left_type::value <= static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_type::value) <= right_; }) { return static_cast<right_type>(left_type::value) <= right_; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ <= static_cast<left_type>(right_type::value); }) { return left_ <= static_cast<left_type>(right_type::value); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) <= right_type::value; }) { return static_cast<right_type>(left_) <= right_type::value; }
					else if constexpr (std::is_convertible_v<right_type, left_type> and requires { left_ <= static_cast<left_type>(right_); }) { return left_ <= static_cast<left_type>(right_); }
					else if constexpr (std::is_convertible_v<left_type, right_type> and requires { static_cast<right_type>(left_) <= right_; }) { return static_cast<right_type>(left_) <= right_; }

					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value <= left_type{right_type::value}; }) { return left_type::value <= left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} <= right_type::value; }) { return right_type{left_type::value} <= right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_type::value <= left_type{right_}; }) { return left_type::value <= left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_type::value} <= right_; }) { return right_type{left_type::value} <= right_; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ <= left_type{right_type::value}; }) { return left_ <= left_type{right_type::value}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} <= right_type::value; }) { return right_type{left_} <= right_type::value; }
					else if constexpr (std::is_constructible_v<right_type, left_type> and requires { left_ <= left_type{right_}; }) { return left_ <= left_type{right_}; }
					else if constexpr (std::is_constructible_v<left_type, right_type> and requires { right_type{left_} <= right_; }) { return right_type{left_} <= right_; }

					else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
				}

			public:
				template<typename Left, typename Right>
				constexpr explicit(false) OperandCompareLessEqual(Left&& left, Right&& right) noexcept
					: left_{std::forward<Left>(left)},
					  right_{std::forward<Right>(right)},
					  result_{do_check()} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
				{
					if constexpr (is_operand_constant_auto_v<left_type>) { return std::format("{} <= {}", expression_to_string(left_type{}), expression_to_string(right_)); }
					else if constexpr (is_operand_constant_auto_v<right_type>) { return std::format("{} <= {}", expression_to_string(left_), expression_to_string(right_type{})); }
					else
					{
						const auto left_string = [this]
						{
							if constexpr (std::is_same_v<left_type, std::nullptr_t> or std::is_pointer_v<left_type>) { return pointer_to_string(left_); }
							else if constexpr (std::ranges::range<left_type> and not std::is_constructible_v<std::string_view, left_type>) { return range_to_string(left_); }
							else { return expression_to_string(left_); }
						}();
						const auto right_string = [this]
						{
							if constexpr (std::is_same_v<right_type, std::nullptr_t> or std::is_pointer_v<right_type>) { return pointer_to_string(right_); }
							else if constexpr (std::ranges::range<right_type> and not std::is_constructible_v<std::string_view, right_type>) { return range_to_string(right_); }
							else { return expression_to_string(right_); }
						}();

						return std::format("{} <= {}", left_string, right_string);
					}
				}
			};

			template<typename L, typename R>
			class OperandLogicalAnd
			{
			public:
				using left_type  = L;
				using right_type = R;

			private:
				left_type  left_;
				right_type right_;
				bool       result_;

			public:
				constexpr explicit(false) OperandLogicalAnd(const left_type& left, const right_type& right) noexcept
					: left_{left},
					  right_{right},
					  result_{static_cast<bool>(left_) and static_cast<bool>(right_)} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string { return std::format("{} and {}", expression_to_string(left_), expression_to_string(right_)); }
			};

			template<typename L, typename R>
			class OperandLogicalOr
			{
			public:
				using left_type  = L;
				using right_type = R;

			private:
				left_type  left_;
				right_type right_;
				bool       result_;

			public:
				constexpr explicit(false) OperandLogicalOr(const left_type& left, const right_type& right) noexcept
					: left_{left},
					  right_{right},
					  result_{static_cast<bool>(left_) or static_cast<bool>(right_)} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string { return std::format("{} or {}", expression_to_string(left_), expression_to_string(right_)); }
			};

			template<std::invocable InvocableType, typename Exception>
			class OperandThrow
			{
			public:
				using exception_type = Exception;
				using invocable_type = InvocableType;

			private:
				bool thrown_;
				bool caught_;

			public:
				constexpr explicit(false) OperandThrow(const invocable_type& invocable) noexcept
					: thrown_{false},
					  caught_{false}
				{
					if constexpr (not std::is_same_v<exception_type, void>)
					{
						try { std::invoke(invocable); }
						catch (const exception_type&)
						{
							thrown_ = true;
							caught_ = true;
						}
						catch (...)
						{
							thrown_ = true;
							caught_ = false;
						}
					}
					else
					{
						try { std::invoke(invocable); }
						catch (...)
						{
							thrown_ = true;
							caught_ = true;
						}
					}
				}

				[[nodiscard]] constexpr auto thrown() const noexcept -> bool { return thrown_; }
				[[nodiscard]] constexpr auto caught() const noexcept -> bool { return caught_; }

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return caught(); }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
				{
					return std::format(
							"throws<{}> -- [{}]",
							meta::name_of<exception_type>(),
							(not thrown())
								? "not thrown"
								://
								(not caught())
								? "thrown but not caught"
								://
								"caught");
				}
			};

			template<std::invocable InvocableType>
			class OperandThrow<InvocableType, void>
			{
			public:
				using invocable_type = InvocableType;

			private:
				bool thrown_;

			public:
				constexpr explicit(false) OperandThrow(const InvocableType& invocable) noexcept
					: thrown_{false}
				{
					try { std::invoke(invocable); }
					catch (...) { thrown_ = true; }
				}

				[[nodiscard]] constexpr auto thrown() const noexcept -> bool { return thrown_; }

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return thrown(); }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string { return std::format("throws - {:s}", thrown()); }
			};

			template<std::invocable InvocableType>
			class OperandNoThrow
			{
			public:
				using invocable_type = InvocableType;

			private:
				bool thrown_;

			public:
				constexpr explicit(false) OperandNoThrow(const InvocableType& invocable) noexcept
					: thrown_{false}
				{
					try { std::invoke(invocable); }
					catch (...) { thrown_ = true; }
				}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return not thrown_; }

				[[nodiscard]] constexpr auto to_string() const noexcept -> std::string { return std::format("nothrow - {:s}", not thrown_); }
			};

			#if __has_include(<unistd.h>) and __has_include(<sys/wait.h>)
			template<std::invocable InvocableType>
			class OperandAbort
			{
			public:
				using invocable_type = InvocableType;

			private:
				bool aborted_;

			public:
				constexpr explicit(false) OperandAbort(const InvocableType& invocable) noexcept
					: aborted_{
							  [&invocable]
							  {
								  if (const auto pid = fork();
									  not pid)
								  {
									  std::invoke(invocable);
									  std::exit(0);
								  }

								  int exit_status = 0;
								  wait(&exit_status);
								  return exit_status;
							  }()}
				{
				}

				[[nodiscard]] constexpr explicit	operator bool() const noexcept { return aborted_; }

				[[nodiscard]] constexpr static auto to_string() noexcept -> std::string_view { return "aborts"; }
			};
			#endif
		}

		class Reporter
		{
			struct hasher
			{
				using is_transparent = int;

				[[nodiscard]] auto operator()(const std::string& string) const noexcept -> std::size_t { return std::hash<std::string>{}(string); }

				[[nodiscard]] auto operator()(const std::string_view& string) const noexcept -> std::size_t { return std::hash<std::string_view>{}(string); }
			};

			template<typename T>
			class Stack
			{
				using container_type = std::vector<T>;

			public:
				using value_type             = typename container_type::value_type;
				using allocator_type         = typename container_type::allocator_type;
				using pointer                = typename container_type::pointer;
				using const_pointer          = typename container_type::const_pointer;
				using reference              = typename container_type::reference;
				using const_reference        = typename container_type::const_reference;
				using size_type              = typename container_type::size_type;
				using difference_type        = typename container_type::difference_type;
				using iterator               = typename container_type::iterator;
				using const_iterator         = typename container_type::const_iterator;
				using reverse_iterator       = typename container_type::reverse_iterator;
				using const_reverse_iterator = typename container_type::const_reverse_iterator;

			private:
				container_type data_;

			public:
				template<typename... Args>
				constexpr auto emplace(Args&&... args) noexcept -> decltype(auto)//
					requires requires { data_.emplace_back(std::forward<Args>(args)...); } { return data_.emplace_back(std::forward<Args>(args)...); }

				[[nodiscard]] constexpr auto top() const noexcept -> decltype(auto) { return data_.back(); }

				[[nodiscard]] constexpr auto pop() noexcept -> decltype(auto) { return data_.pop_back(); }

				[[nodiscard]] constexpr auto size() const noexcept -> decltype(auto) { return data_.size(); }

				[[nodiscard]] constexpr auto empty() const noexcept -> decltype(auto) { return data_.empty(); }

				[[nodiscard]] constexpr auto begin() const noexcept -> decltype(auto) { return data_.begin(); }

				[[nodiscard]] constexpr auto end() const noexcept -> decltype(auto) { return data_.end(); }
			};

		public:
			template<typename V>
			using map_type = std::unordered_map<std::string, V, hasher, std::equal_to<>>;

			using clock_type           = std::chrono::high_resolution_clock;
			using time_point_type      = clock_type::time_point;
			using time_difference_type = std::chrono::milliseconds;

			constexpr static std::string_view anonymous_suite_name{"anonymous_suite"};
			constexpr static std::string_view anonymous_test_name{"anonymous_test"};

			struct result_type
			{
				enum class Status
				{
					PENDING,

					PASSED,
					FAILED,
					SKIPPED,
					FATAL,
				};

				result_type* parent = nullptr;
				std::string  suite_name;
				std::string  test_name;

				Status                                 status                  = Status::PENDING;
				time_point_type                        time_start              = clock_type::now();
				time_point_type                        time_end                = clock_type::now();
				std::size_t                            total_tests_failed      = 0;
				std::size_t                            total_tests_passed      = 0;
				std::size_t                            total_tests_skipped     = 0;
				std::size_t                            total_assertions_passed = 0;
				std::size_t                            total_assertions_failed = 0;
				std::string                            report_string;
				map_type<std::unique_ptr<result_type>> nested_result;

				constexpr auto operator+=(const result_type& other) noexcept -> result_type&
				{
					total_tests_failed += other.total_tests_failed + 1;
					total_assertions_passed += other.total_assertions_passed;
					total_tests_passed += other.total_tests_passed;
					total_tests_skipped += other.total_tests_skipped;
					total_assertions_failed += other.total_assertions_failed;

					return *this;
				}
			};

		private:
			map_type<result_type>   results_;
			std::string_view        active_suite_;
			result_type*            active_scope_;
			Stack<std::string_view> active_test_;

			// fixme
			// mutable std::ostream    out_;
			// mutable std::streambuf* out_saved_;
			std::ostream& out_;// NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

			color_type color_;

			enum ReportType : std::uint16_t
			{
				// All assertions in the entire (test) scope are output (default).
				// note: Can be overridden by ReportType(FAILED_ONY/NONE) for individual assertions.
				SCOPE_ALWAYS = 0x00000000,

				// All assertions in the entire (test) scope are output only on failure.
				// note: Can be overridden by ReportType(NONE) for individual assertions.
				SCOPE_FAILED_ONLY = 0x00000001,
				// All assertions in the entire (test) scope are not output.
				SCOPE_NONE = 0x00000010,
				// The current assertion is only output on failure.
				FAILED_ONLY = 0x00000100,
				// The current assertions will not output.
				NONE = 0x00001000,
			};

			std::underlying_type_t<ReportType> report_type_;

			// Allow users to use `expect` outside `"xxx"_test`
			bool in_anonymous_test_;

			[[nodiscard]] auto get_test_full_name() const noexcept -> std::string
			{
				std::string result;

				std::format_to(std::back_inserter(result), "[{}] ", active_suite_);

				std::ranges::for_each(
						active_test_,
						[&result, first = true](const auto& sv) mutable noexcept -> void
						{
							result.append(first ? "" : ".");
							first = false;

							result.append_range(sv);
						});
				return result;
			}

			[[nodiscard]] auto get_duration_milliseconds() const noexcept -> clock_type::rep
			{
				active_scope_->time_end = clock_type::now();
				return std::chrono::duration_cast<time_difference_type>(
								active_scope_->time_end - active_scope_->time_start)
						.count();
			}

			auto scope_push(const std::string_view test_name) -> void
			{
				std::string test_name_s{test_name};

				const auto [it, inserted] = active_scope_->nested_result.try_emplace(
						test_name_s,
						std::make_unique<result_type>(
								// parent
								active_scope_,
								// suite_name
								std::string{active_suite_},
								// test_name
								test_name_s));

				active_test_.emplace(it->first);
				active_scope_ = it->second.get();

				if (not inserted)
				[[unlikely]]
				{
					std::print(
							std::cerr,
							"{}WARNING{}: test `{}` for test suite `{}` already present.\n",
							color_.fail,
							color_.none,
							test_name_s,
							active_suite_);
				}
			}

			auto scope_pop(const std::string_view test_name) -> void
			{
				if (active_scope_->total_tests_skipped)
				[[unlikely]]
				{
					active_scope_->status = result_type::Status::SKIPPED;
				}
				else
				{
					active_scope_->status =
							active_scope_->total_assertions_failed > 0
								? result_type::Status::FAILED
								: result_type::Status::PASSED;
				}

				if (active_test_.top() == test_name)
				[[likely]]
				{
					active_test_.pop();
					const auto* old_scope = active_scope_;
					if (active_scope_->parent) { active_scope_ = active_scope_->parent; }
					else
					{
						const auto scope_it = results_.find(anonymous_suite_name);
						GAL_PROMETHEUS_DEBUG_ASSUME(scope_it != results_.end());
						active_scope_ = std::addressof(scope_it->second);
					}

					*active_scope_ += *old_scope;
					return;
				}

				throw std::logic_error{
						std::format(
								"Executor returned from test w/o signaling: not popping because `{}` differs from `{}`",
								active_test_.top(),
								test_name)};
			}

			auto scope_clear() -> void
			{
				while (not active_test_.empty())
				{
					if (active_scope_->total_tests_skipped)
					[[unlikely]]
					{
						active_scope_->status = result_type::Status::SKIPPED;
					}
					else
					{
						active_scope_->status =
								active_scope_->total_assertions_failed > 0
									? result_type::Status::FAILED
									: result_type::Status::PASSED;
					}

					active_test_.pop();
					const auto* old_scope = active_scope_;
					if (active_scope_->parent) { active_scope_ = active_scope_->parent; }
					else
					{
						const auto scope_it = results_.find(anonymous_suite_name);
						GAL_PROMETHEUS_DEBUG_ASSUME(scope_it != results_.end());
						active_scope_ = std::addressof(scope_it->second);
					}

					*active_scope_ += *old_scope;
				}
			}

			auto bump_report_message(const bool force = false) const -> void
			{
				// Output all information at once only at the end of the outermost (at the top-level of suite) test.
				if (active_test_.size() == 1 or force)
				{
					const auto do_bump = functional::y_combinator{
							[this](auto& self, const result_type& result) -> void
							{
								out_ << result.report_string;

								// todo
								// if (
								// 	// 1. if a fatal error is encountered, the message is still output even if it is run silently
								// 	active_scope_->status == result_type::Status::FATAL
								// 	or
								// 	// 2. if just ignore the pass assertion, then output then message whenever it fails
								// 	(report_type_scope_ == ReportType::FAILED_ONLY and active_scope_->status == result_type::Status::FAILED)
								// )
								{
									std::ranges::for_each(
											result.nested_result,
											[this, &self](const auto& pair) -> void
											{
												const auto& [name, nested_result] = pair;

												self(*nested_result);
											});
								}
							}};

					do_bump(*active_scope_);

					std::string result_message;
					if (active_scope_->status == result_type::Status::FAILED)
					[[unlikely]]
					{
						active_scope_->total_tests_passed += 1;

						result_message = std::format(
								"{:{}}{}FAILED{} after {} milliseconds.\n",
								"",
								(active_test_.size() - 1) * 2,
								color_.fail,
								color_.none,
								get_duration_milliseconds());
					}
					else if (active_scope_->status == result_type::Status::SKIPPED)
					{
						active_scope_->total_tests_skipped += 1;

						std::format_to(
								std::back_inserter(active_scope_->report_string),
								"{:{}}{}SKIPPED{}\n",
								"",
								(active_test_.size() - 1) * 2,
								color_.skip,
								color_.none);
					}
					else
					{
						active_scope_->total_tests_passed += 1;

						result_message = std::format(
								"{:{}}{}PASSED{} after {} milliseconds.\n",
								"",
								(active_test_.size() - 1) * 2,
								color_.pass,
								color_.none,
								get_duration_milliseconds());
					}

					active_scope_->report_string += result_message;
					out_ << result_message;
				}
			}

		public:
			// Reporter(const Reporter& other)                        = delete;
			// Reporter(Reporter&& other) noexcept                    = delete;
			// auto operator=(const Reporter& other) -> Reporter&     = delete;
			// auto operator=(Reporter&& other) noexcept -> Reporter& = delete;

			explicit Reporter(const color_type& color = {})
				: active_suite_{anonymous_suite_name},
				  active_scope_{
						  std::addressof(results_.emplace(std::string{anonymous_suite_name}, result_type{}).first->second)},
				  // out_{std::cout.rdbuf()},
				  // out_saved_{std::cout.rdbuf()}
				  out_{std::cout},
				  color_{color},
				  report_type_{ReportType::SCOPE_ALWAYS},
				  in_anonymous_test_{false}
			{
				// todo: parse command line option
			}

			// ~Reporter() noexcept(noexcept(out_.rdbuf(out_saved_))) { out_.rdbuf(out_saved_); }

			// The name of the currently executing suite
			[[nodiscard]] constexpr auto suite_name() const noexcept -> std::string_view { return active_suite_; }

			// Whether the current suite is 'global' or not.
			[[nodiscard]] constexpr auto suite_is_global() const noexcept -> bool { return suite_name() == anonymous_suite_name; }

			// Can the currently executing test continue? (i.e. no fatal test case is executed)
			[[nodiscard]] constexpr auto test_corrupted() const noexcept -> bool { return active_scope_->status == result_type::Status::FATAL; }

			[[nodiscard]] auto result(const std::string_view name) const -> const result_type&
			{
				const auto it = results_.find(name);
				if (it == results_.end())
				[[unlikely]]
				{
					throw std::out_of_range{
							std::format(
									"Top level test {}{}{} not exists",
									color_.test,
									name,
									color_.none)};
				}
				return it->second;
			}

			[[nodiscard]] auto results() const & noexcept -> const map_type<result_type>& { return results_; }

			[[nodiscard]] auto results() && noexcept -> map_type<result_type>&& { return std::move(results_); }

			auto on(const events::EventSuiteBegin& suite_begin) -> void
			{
				if (in_anonymous_test_)
				[[unlikely]]
				{
					on(events::EventTestEnd{.name = anonymous_test_name});
					in_anonymous_test_ = false;
				}

				scope_clear();

				auto& [name, scope] = *results_.emplace(std::string{suite_begin.name}, result_type{}).first;

				active_suite_ = name;
				active_scope_ = std::addressof(scope);

				std::println(
						out_,
						"Executing suite {}{}{} vvv",
						color_.suite,
						active_suite_,
						color_.none);
			}

			auto on([[maybe_unused]] const events::EventSuiteEnd& suite_end) -> void
			{
				std::println(
						out_,
						"^^^ End of suite {}{}{} execution\n",
						color_.suite,
						active_suite_,
						color_.none);

				scope_clear();

				auto it = results_.find(anonymous_suite_name);
				GAL_PROMETHEUS_DEBUG_ASSUME(it != results_.end());
				auto& [name, scope] = *it;

				active_suite_ = name;
				active_scope_ = std::addressof(scope);
			}

			auto on(const events::EventTestBegin& test_begin) -> void
			{
				if (in_anonymous_test_)
				[[unlikely]]
				{
					on(events::EventTestEnd{.name = anonymous_test_name});
					in_anonymous_test_ = false;
				}

				scope_push(test_begin.name);

				std::format_to(
						std::back_inserter(active_scope_->report_string),
						"{:{}}Running{} test {}{}{}...\n",
						"",
						(active_test_.size() - 1) * 2,
						active_test_.size() == 1 ? "" : " nested",
						color_.test,
						get_test_full_name(),
						color_.none);
			}

			auto on(const events::EventTestSkip& test_skip) -> void
			{
				if (not active_scope_->nested_result.contains(test_skip.name))
				[[likely]]
				{
					on(events::EventTestBegin{.name = test_skip.name});

					active_scope_->status = result_type::Status::SKIPPED;

					on(events::EventTestEnd{.name = test_skip.name});
				}
			}

			auto on(const events::EventTestEnd& test_end) -> void
			{
				bump_report_message();
				scope_pop(test_end.name);
			}

			template<bool Scope>
			auto on(const events::EventSilenceBegin&) -> void
			{
				if constexpr (Scope) { report_type_ |= ReportType::SCOPE_NONE; }
				else { report_type_ |= ReportType::NONE; }
			}

			template<bool Scope>
			auto on(const events::EventSilenceEnd&) -> void
			{
				if constexpr (Scope) { report_type_ &= ~ReportType::SCOPE_NONE; }
				else { report_type_ &= ~ReportType::NONE; }
			}

			template<bool Scope>
			auto on(const events::EventIgnorePassBegin&) -> void
			{
				if constexpr (Scope) { report_type_ |= ReportType::SCOPE_FAILED_ONLY; }
				else { report_type_ |= ReportType::FAILED_ONLY; }
			}

			template<bool Scope>
			auto on(const events::EventIgnorePassEnd&) -> void
			{
				if constexpr (Scope) { report_type_ &= ~ReportType::SCOPE_FAILED_ONLY; }
				else { report_type_ &= ~ReportType::FAILED_ONLY; }
			}

			template<operand::expression_t Expression>
			auto on(const events::EventAssertionPass<Expression>& assertion_pass) -> void
			{
				if (report_type_ == ReportType::SCOPE_ALWAYS)
				{
					if (active_test_.empty())
					{
						// Add a dummy test in advance to allow users to use `expect` outside `"xxx"_test`.
						// Since we use `(active_test_.size() - 1) * 2` to indent, we need a dummy test here to take up space.
						on(events::EventTestBegin{.name = anonymous_test_name});
						in_anonymous_test_ = true;
					}

					std::format_to(
							std::back_inserter(active_scope_->report_string),
							"{:{}}[{}:{}] {}[{}]{} - {}PASSED{} \n",
							"",
							(active_test_.size() - 1) * 2,
							assertion_pass.location.file_name(),
							assertion_pass.location.line(),
							color_.expression,
							operand::expression_to_string(assertion_pass.expression),
							color_.none,
							color_.pass,
							color_.none);
				}

				active_scope_->total_assertions_passed += 1;
			}

			template<operand::expression_t Expression>
			auto on(const events::EventAssertionFail<Expression>& assertion_fail) -> void
			{
				if (not((report_type_ & ReportType::SCOPE_NONE) or (report_type_ & ReportType::NONE)))
				{
					std::format_to(
							std::back_inserter(active_scope_->report_string),
							"{:{}}[{}:{}] {}[{}]{} - {}FAILED{} \n",
							"",
							(active_test_.size() - 1) * 2,
							assertion_fail.location.file_name(),
							assertion_fail.location.line(),
							color_.expression,
							operand::expression_to_string(assertion_fail.expression),
							color_.none,
							color_.fail,
							color_.none);
				}

				active_scope_->total_assertions_failed += 1;

				if (config::fast_fail or active_scope_->total_assertions_failed > config::abort_after_n_failures)
				[[unlikely]]
				{
					bump_report_message(true);
					std::println(
							std::cerr,
							"{}fast fail for test {} after {} failures total.{}",
							color_.fail,
							get_test_full_name(),
							active_scope_->total_assertions_failed,
							color_.none);
					std::exit(-1);
				}
			}

			auto on(const events::EventAssertionFatal& assertion_fatal) const -> void
			{
				if (not((report_type_ & ReportType::SCOPE_NONE) or (report_type_ & ReportType::NONE)))
				{
					std::format_to(
							std::back_inserter(active_scope_->report_string),
							"{:{}}^^^ {}FATAL ERROR{}\n",
							"",
							(active_test_.size() - 1) * 2 + (
								// '['
								1 +
								// file_name
								std::string_view::traits_type::length(assertion_fatal.location.file_name())) +
							// ':'
							1 +
							// line
							[]<typename T>(T line)
							{
								T result = 0;
								while (line)
								{
									result += 1;
									line /= 10;
								}
								return result;
							}(assertion_fatal.location.line()) +
							// "] ["
							3,
							color_.fatal,
							color_.none);
				}

				active_scope_->total_assertions_failed += 1;
				active_scope_->status = result_type::Status::FATAL;
			}

			template<operand::expression_t Expression>
			auto on(const events::EventAssertionFatalSkip<Expression>& assertion_fatal_skip) -> void
			{
				// fixme: Output only when ReportType::ALWAYS or as long as it is not ReportType::NONE?
				// If the expression is only output when ReportType::ALWAYS, the expression modified by ignore_pass will not be output.
				if (not((report_type_ & ReportType::SCOPE_NONE) or (report_type_ & ReportType::NONE)))
				{
					std::format_to(
							std::back_inserter(active_scope_->report_string),
							"{:{}}[{}:{}] {}[{}]{} - {}SKIPPED{} \n",
							"",
							(active_test_.size() - 1) * 2,
							assertion_fatal_skip.location.file_name(),
							assertion_fatal_skip.location.line(),
							color_.expression,
							operand::expression_to_string(assertion_fatal_skip.expression),
							color_.none,
							color_.fatal,
							color_.none);
				}

				active_scope_->total_assertions_failed += 1;
			}

			template<typename MessageType>
			auto on(const events::EventLog<MessageType>& log) -> void
			{
				if (log.message == std::string_view{"\n"})
				[[unlikely]]
				{
					active_scope_->report_string.push_back('\n');
					return;
				}

				// pop '\n'
				active_scope_->report_string.pop_back();

				if (log.message == std::string_view{" "})
				[[unlikely]]
				{
					active_scope_->report_string.push_back(' ');
				}
				else
				{
					active_scope_->report_string.append(color_.expression);
					active_scope_->report_string.append_range(log.message);
					active_scope_->report_string.append(color_.none);
				}

				// push '\n'
				active_scope_->report_string.push_back('\n');
			}

			[[noreturn]] auto on(const events::EventException& exception) -> void
			{
				const auto test_name = get_test_full_name();

				on(events::EventTestEnd{.name = active_test_.top()});

				std::format_to(
						std::back_inserter(active_scope_->report_string),
						"{}Abort test because unexpected exception with message: {}.{}\n",
						color_.fail,
						exception.what(),
						color_.none);

				out_ << active_scope_->report_string;

				// fast fail
				std::println(
						std::cerr,
						"--- early abort for test {}{}{} after {} failures total.",
						color_.test,
						test_name,
						color_.none,
						active_scope_->total_assertions_failed);
				std::exit(-1);// NOLINT(concurrency-mt-unsafe)
			}

			auto on([[maybe_unused]] const events::EventSummary& summary) -> void
			{
				std::ranges::for_each(
						results_,
						[color = color_, &out = out_](
						const auto& name_result_pair) -> void
						{
							if (const auto& [name, result] = name_result_pair;
								result.total_assertions_failed)
							{
								std::println(
										out,
										"\n==========================================\n"
										"Suite {}{}{}\n"
										"tests {} | {} {}failed({:.6g}%){}\n"
										"assertions {} | {} {}passed({:.6g}%){} | {} {}failed({:.6g}%){}"
										"\n==========================================\n",
										color.suite,
										name,
										color.none,
										//
										result.total_tests_passed + result.total_tests_failed,
										result.total_tests_passed,
										color.fail,
										static_cast<double>(result.total_tests_passed) / static_cast<double>(result.total_tests_passed + result.total_tests_failed) * 100.0,
										color.none,
										//
										result.total_assertions_passed + result.total_assertions_failed,
										result.total_assertions_passed,
										color.pass,
										static_cast<double>(result.total_assertions_passed) /
										static_cast<
											double>(result.total_assertions_passed + result.total_assertions_failed) *
										100.0,
										color.none,
										//
										result.total_assertions_failed,
										color.fail,
										static_cast<double>(result.total_assertions_failed) / static_cast<double>(result.total_assertions_passed + result.total_assertions_failed) * 100.0,
										color.none);
							}
							else
							{
								std::println(
										out,
										"\n==========================================\n"
										"Suite {}{}{} -> all tests passed({} asserts in {} tests), {} tests skipped."
										"\n==========================================\n",
										color.suite,
										name,
										color.none,
										result.total_assertions_passed,
										result.total_tests_passed,
										result.total_tests_skipped);
							}
						});
			}
		};

		template<typename ReporterType>
		class Executor
		{
		public:
			using reporter_type = ReporterType;

			using suite_type = events::EventSuite;

			using suite_list_type      = std::vector<suite_type>;
			using suite_list_size_type = suite_list_type::size_type;

			using nested_test_path_type      = std::array<std::string_view, 16>;
			using nested_test_path_size_type = nested_test_path_type::size_type;

		private:
			reporter_type reporter_;

			suite_list_type suites_;

			// todo: Allows users to set which tests need to be executed.
			categories_type categories_should_run_;

			std::size_t fails_;
			bool        finished_;
			bool        dry_run_;

			constexpr auto run(const bool report_summary_required = false) noexcept -> bool try
			{
				// todo: Allow users to filter which suites need to be executed (via suite_name)
				std::ranges::for_each(
						suites_,
						[this](const suite_type& suite) -> void
						{
							reporter_.on(suite.operator events::EventSuiteBegin());

							std::invoke(suite);

							reporter_.on(suite.operator events::EventSuiteEnd());
						});

				suites_.clear();

				if (report_summary_required) { report_summary(); }

				finished_ = true;
				return fails_ != 0;
			}
			catch (...)
			{
				std::cerr << "Unhandled exception.";
				std::exit(-1);// NOLINT(concurrency-mt-unsafe)
			}

			constexpr auto report_summary() -> void { reporter_.on(events::EventSummary{}); }

		public:
			Executor(const Executor& other)                        = delete;
			Executor(Executor&& other) noexcept                    = delete;
			auto operator=(const Executor& other) -> Executor&     = delete;
			auto operator=(Executor&& other) noexcept -> Executor& = delete;

			constexpr explicit Executor() noexcept
				requires std::is_default_constructible_v<reporter_type>
				: fails_{0},
				  finished_{false},
				  dry_run_{false}
			{
				categories_should_run_.emplace_back(tag_silence::value);
				categories_should_run_.emplace_back(tag_ignore_pass::value);
			}

			constexpr ~Executor() noexcept
			{
				if (not finished_ and not run(not dry_run_))
				{
					// todo
				}
			}

			constexpr auto on(const events::EventSuite& suite) -> void { suites_.emplace_back(suite); }

			template<typename InvocableType, typename Arg>
			constexpr auto on(const events::EventTest<InvocableType, Arg>& test) -> void
			{
				if (const bool execute =
							std::ranges::empty(test.categories) or
							std::ranges::any_of(
									test.categories,
									[this](const auto& category) -> bool
									{
										if (category == tag_skip::value) { return false; }

										return std::ranges::any_of(
												categories_should_run_,
												[category](const auto& sv) -> bool { return string::make_wildcard_matcher(sv)(category); });
									});
					not execute)
				[[unlikely]]
				{
					reporter_.on(test.operator events::EventTestSkip());
					return;
				}

				reporter_.on(test.operator events::EventTestBegin());

				const bool silence     = std::ranges::contains(test.categories, tag_silence::value);
				const auto ignore_pass = std::ranges::contains(test.categories, tag_ignore_pass::value);

				// for scope
				// note: after test begin
				if (silence) { reporter_.template on<true>(events::EventSilenceBegin{}); }
				if (ignore_pass) { reporter_.template on<true>(events::EventIgnorePassBegin{}); }

				try { std::invoke(test); }
				// see on(const events::EventAssertionFatal& fatal)
				// catch (const events::EventAssertionFatal&)
				// {
				// }
				catch (const std::exception& exception)
				{
					fails_ += 1;
					reporter_.on(events::EventException{
							.message = exception.what()});
				}
				catch (...)
				{
					fails_ += 1;
					reporter_.on(events::EventException{
							.message = "unhandled exception, not derived from std::exception"});
				}

				reporter_.on(test.operator events::EventTestEnd());

				// for scope
				// note: after test end
				if (silence) { reporter_.template on<true>(events::EventSilenceEnd{}); }
				if (ignore_pass) { reporter_.template on<true>(events::EventIgnorePassEnd{}); }
			}

			template<typename MessageType>
			constexpr auto on(const events::EventLog<MessageType>& log) -> void { reporter_.on(log); }

			// for assertion
			constexpr auto on(const events::EventSilenceBegin& silence_begin) -> void { reporter_.template on<false>(silence_begin); }

			// for assertion
			constexpr auto on(const events::EventSilenceEnd& silence_end) -> void { reporter_.template on<false>(silence_end); }

			// for assertion
			constexpr auto on(const events::EventIgnorePassBegin& ignore_pass_begin) -> void { reporter_.template on<false>(ignore_pass_begin); }

			// for assertion
			constexpr auto on(const events::EventIgnorePassEnd& ignore_pass_end) -> void { reporter_.template on<false>(ignore_pass_end); }

			template<operand::expression_t Expression>
			constexpr auto on(const events::EventAssertion<Expression>& assertion) -> bool
			{
				if (dry_run_) { return true; }

				if (reporter_.test_corrupted())
				[[unlikely]]
				{
					reporter_.on(assertion.operator events::EventAssertionFatalSkip<Expression>());
					// Consider the test case execution successful and avoid undesired log output.
					return true;
				}

				if (static_cast<bool>(assertion.expression))
				[[likely]]
				{
					reporter_.on(assertion.operator events::EventAssertionPass<Expression>());
					return true;
				}

				fails_ += 1;
				reporter_.on(assertion.operator events::EventAssertionFail<Expression>());
				return false;
			}

			constexpr auto on(const events::EventAssertionFatal& fatal) -> void
			{
				reporter_.on(fatal);

				if (reporter_.suite_is_global()) { report_summary(); }

				// see reporter::test_corrupted
				// see on(const events::EventAssertion<Expression>& assertion)
				// throw fatal;
			}
		};
	}

	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	template<typename T>
	using as     = unit_test_detail::operand::OperandValue<T>;
	using as_c   = as<char>;
	using as_i   = as<int>;
	using as_u   = as<unsigned>;
	using as_l   = as<long>;
	using as_ul  = as<unsigned long>;
	using as_ll  = as<long long>;
	using as_ull = as<unsigned long long>;
	using as_i8  = as<std::int8_t>;
	using as_u8  = as<std::uint8_t>;
	using as_i16 = as<std::int16_t>;
	using as_u16 = as<std::uint16_t>;
	using as_i32 = as<std::int32_t>;
	using as_u32 = as<std::uint32_t>;
	using as_i64 = as<std::int64_t>;
	using as_u64 = as<std::uint64_t>;
	using as_f   = as<float>;
	using as_d   = as<double>;
	using as_ld  = as<long double>;
	using as_b   = as<bool>;
	using as_s   = as<std::string>;
	using as_sv  = as<std::string_view>;

	constexpr auto category = [](const std::string_view name) -> unit_test_detail::categories_type { return {name}; };
	constexpr auto cat      = category;

	constexpr unit_test_detail::tag_fatal fatal{};
	constexpr unit_test_detail::tag_skip  skip{};

	// see dispatcher::DispatcherSilence&dispatcher::DispatcherIgnorePass below.
	// constexpr unit_test_detail::tag_silence silence{};
	// constexpr unit_test_detail::tag_ignore_pass ignore_pass{};

	template<typename T, typename...>
	struct identity
	{
		using type = T;
	};

	struct override {};

	// todo: custom reporter
	template<typename = override, typename...>
	// fixme: An executor object is constructed/deconstructed multiple times.
	// [[maybe_unused]] inline auto executor = unit_test_detail::Executor<>{};
	[[nodiscard]] constexpr auto executor() -> unit_test_detail::Executor<>&
	{
		static unit_test_detail::Executor<> e{};
		return e;
	}

	GAL_PROMETHEUS_MODULE_EXPORT_END

	namespace unit_test_detail::dispatcher
	{
		template<typename... Ts, events::event_t EventType>
		constexpr auto register_event(EventType&& event) noexcept -> decltype(auto) { return executor<typename identity<override, Ts...>::type>().on(std::forward<EventType>(event)); }

		class DispatcherLogger
		{
			struct next
			{
				template<typename M>
				auto operator<<(M&& next_message) const -> const next&
				{
					register_event(events::EventLog{.message = " "});
					register_event(events::EventLog{.message = std::forward<M>(next_message)});
					return *this;
				}
			};

			constexpr static next n{};

		public:
			template<typename MessageType>
			constexpr auto operator<<(MessageType&& message) const noexcept -> const next&
			{
				register_event(events::EventLog{.message = "\n"});
				register_event(events::EventLog{.message = std::forward<MessageType>(message)});

				return n;
			}
		};

		template<typename Lhs, typename Dispatcher>
		struct dispatched_expression;

		template<typename>
		constexpr auto is_dispatched_expression_v = false;
		template<typename Lhs, typename Dispatcher>
		constexpr auto is_dispatched_expression_v<dispatched_expression<Lhs, Dispatcher>> = true;

		template<typename Dispatcher>
		class ExpressionDispatcher
		{
		public:
			template<typename Lhs>
			[[nodiscard]] constexpr auto operator%(const Lhs& lhs) const noexcept -> dispatched_expression<Lhs, Dispatcher> { return {lhs}; }
		};

		class DispatcherThat : public ExpressionDispatcher<DispatcherThat>
		{
		public:
			using ExpressionDispatcher::operator%;
		};

		template<typename>
		constexpr auto is_dispatched_expression_that_v = false;
		template<typename Expression>
			requires is_dispatched_expression_v<Expression>
		constexpr auto is_dispatched_expression_that_v<Expression> = std::is_same_v<typename Expression::dispatcher_type, DispatcherThat>;

		class DispatcherSilence : public ExpressionDispatcher<DispatcherSilence>
		{
		public:
			using ExpressionDispatcher::operator%;
		};

		template<typename>
		constexpr auto is_dispatched_expression_silence_v = false;
		template<typename Expression>
			requires is_dispatched_expression_v<Expression>
		constexpr auto is_dispatched_expression_silence_v<Expression> = std::is_same_v<typename Expression::dispatcher_type, DispatcherSilence>;

		class DispatcherIgnorePass : public ExpressionDispatcher<DispatcherIgnorePass>
		{
		public:
			using ExpressionDispatcher::operator%;
		};

		template<typename>
		constexpr auto is_dispatched_expression_ignore_pass_v = false;
		template<typename Expression>
			requires is_dispatched_expression_v<Expression>
		constexpr auto is_dispatched_expression_ignore_pass_v<Expression> = std::is_same_v<typename Expression::dispatcher_type, DispatcherIgnorePass>;

		template<operand::expression_t Expression>
		struct expect_result
		{
		private:
			template<typename T>
			struct fatal_location
			{
				std::source_location location;

				constexpr explicit(false) fatal_location(
						const T&,
						const std::source_location& l = std::source_location::current()) noexcept
					: location{l} {}
			};

		public:
			bool value;

			constexpr explicit expect_result(const bool v) noexcept
				: value{v} {}

			template<typename MessageType>
			constexpr auto operator<<(MessageType&& message) -> expect_result&//
				requires requires { events::EventLog{.message = std::forward<MessageType>(message)}; }
			{
				if (not value) { register_event<Expression>(events::EventLog{.message = std::forward<MessageType>(message)}); }

				return *this;
			}

			constexpr auto operator<<(const fatal_location<tag_fatal>& location) -> expect_result&
			{
				if (not value) { register_event<Expression>(events::EventAssertionFatal{.location = location.location}); }

				return *this;
			}
		};

		class DispatcherExpect
		{
		public:
			template<typename Expression>
				requires operand::expression_t<Expression> or (is_dispatched_expression_v<Expression>)
			constexpr auto operator()(Expression&& expression, const std::source_location location = std::source_location::current()) const noexcept -> auto
			{
				// workaround: dispatched expression vvv
				if constexpr (is_dispatched_expression_v<Expression>)
				{
					if constexpr (is_dispatched_expression_silence_v<Expression>) { register_event<typename Expression::expression_type>(events::EventSilenceBegin{}); }
					else if constexpr (is_dispatched_expression_ignore_pass_v<Expression>) { register_event<typename Expression::expression_type>(events::EventIgnorePassBegin{}); }

					const auto result = register_event<typename Expression::expression_type>(events::EventAssertion<typename Expression::expression_type>{.expression = std::forward<Expression>(expression).expression, .location = location});

					if constexpr (is_dispatched_expression_silence_v<Expression>) { register_event<typename Expression::expression_type>(events::EventSilenceEnd{}); }
					else if constexpr (is_dispatched_expression_ignore_pass_v<Expression>) { register_event<typename Expression::expression_type>(events::EventIgnorePassEnd{}); }

					return expect_result<typename Expression::expression_type>{result};
				}
				// ^^^ workaround: dispatched expression
				else { return expect_result<Expression>{register_event<Expression>(events::EventAssertion<Expression>{.expression = std::forward<Expression>(expression), .location = location})}; }
			}
		};

		template<typename D>
		class DispatcherTestBase
		{
		protected:
			categories_type categories_;

		public:
			template<std::invocable InvocableType>
			constexpr auto operator=(InvocableType invocable) const & noexcept -> InvocableType// NOLINT(misc-unconventional-assign-operator)
			{
				register_event(events::EventTest<InvocableType>{
						.name = static_cast<D&>(*this).name(),
						.categories = categories_,
						.invocable = invocable,
						.arg = events::none{}});

				return invocable;
			}

			template<std::invocable InvocableType>
			constexpr auto operator=(InvocableType invocable) && noexcept -> InvocableType// NOLINT(misc-unconventional-assign-operator)
			{
				register_event(events::EventTest<InvocableType>{
						.name = static_cast<D&>(*this).name(),
						.categories = std::move(categories_),
						.invocable = invocable,
						.arg = events::none{}});

				return invocable;
			}

		private:
			[[nodiscard]] constexpr auto do_move() && noexcept -> D&& { return std::move(*static_cast<D*>(this)); }

		public:
			#if __cpp_multidimensional_subscript < 202110L
			// todo
			#endif
			template<typename... Args>
			[[nodiscard]] constexpr auto operator[](Args&&... args) && noexcept -> D&&
			{
				const auto process = [this]<typename T>(T&& t) noexcept -> void
				{
					using type = std::decay_t<T>;

					// ==========================================
					// SKIP
					// ==========================================
					if constexpr (std::is_same_v<type, tag_skip>) { categories_.emplace_back(tag_skip::value); }
					// ==========================================
					// SILENCE
					// ==========================================
					else if constexpr (std::is_same_v<type, DispatcherSilence>) { categories_.emplace_back(tag_silence::value); }
					// ==========================================
					// IGNORE PASS
					// ==========================================
					else if constexpr (std::is_same_v<type, DispatcherIgnorePass>) { categories_.emplace_back(tag_ignore_pass::value); }
					// ==========================================
					// ANY
					// ==========================================
					else if constexpr (std::is_same_v<type, categories_type::value_type>) { categories_.push_back(std::forward<T>(t)); }
					// ==========================================
					// ANY RANGE
					// ==========================================
					else if constexpr (std::is_same_v<type, categories_type>) { categories_.append_range(std::forward<T>(t)); }
					else { GAL_PROMETHEUS_STATIC_UNREACHABLE("unknown type"); }
				};

				(process(std::forward<Args>(args)), ...);

				return std::move(*this).do_move();
			}
		};

		template<meta::basic_fixed_string StringLiteral>
		class DispatcherTestLiteral : public DispatcherTestBase<DispatcherTestLiteral<StringLiteral>>
		{
		public:
			using DispatcherTestBase<DispatcherTestLiteral>::operator=;

			// @see `_test`
			[[nodiscard]] constexpr static auto make() noexcept -> DispatcherTestLiteral { return {}; }

			[[nodiscard]] constexpr static auto name() noexcept -> auto { return StringLiteral.operator std::string_view(); }
		};

		class DispatcherTest : public DispatcherTestBase<DispatcherTest>
		{
			std::string_view name_;

		public:
			using DispatcherTestBase<DispatcherTest>::operator=;

			constexpr explicit DispatcherTest(const std::string_view name) noexcept
				: name_{name} {}

			[[nodiscard]] constexpr auto name() const noexcept -> auto { return name_; }
		};
	}

	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	constexpr unit_test_detail::dispatcher::DispatcherLogger     logger{};
	constexpr unit_test_detail::dispatcher::DispatcherThat       that{};
	constexpr unit_test_detail::dispatcher::DispatcherSilence    silence{};
	constexpr unit_test_detail::dispatcher::DispatcherIgnorePass ignore_pass{};
	constexpr unit_test_detail::dispatcher::DispatcherExpect     expect{};

	using test = unit_test_detail::dispatcher::DispatcherTest;

	// Overload the following operators to implement auto-dispatch.
	inline namespace operators
	{
		namespace detail
		{
			template<typename DispatchedExpression>
			// ReSharper disable once CppFunctionIsNotImplemented
			constexpr auto is_valid_dispatched_expression(DispatchedExpression&& expression) noexcept -> void requires requires { static_cast<bool>(expression.expression); };
		}

		// a == b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator==(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto)//
			requires(not unit_test_detail::dispatcher::is_dispatched_expression_v<Lhs>) and requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) == std::forward<Rhs>(rhs)); } { return that % std::forward<Lhs>(lhs) == std::forward<Rhs>(rhs); }

		// a != b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator!=(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto)//
			requires(not unit_test_detail::dispatcher::is_dispatched_expression_v<Lhs>) and requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) != std::forward<Rhs>(rhs)); } { return that % std::forward<Lhs>(lhs) != std::forward<Rhs>(rhs); }

		// a > b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator>(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto)//
			requires(not unit_test_detail::dispatcher::is_dispatched_expression_v<Lhs>) and requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) > std::forward<Rhs>(rhs)); } { return that % std::forward<Lhs>(lhs) > std::forward<Rhs>(rhs); }

		// a >= b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator>=(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto)//
			requires(not unit_test_detail::dispatcher::is_dispatched_expression_v<Lhs>) and requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) >= std::forward<Rhs>(rhs)); } { return that % std::forward<Lhs>(lhs) >= std::forward<Rhs>(rhs); }

		// a < b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator<(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto)//
			requires(not unit_test_detail::dispatcher::is_dispatched_expression_v<Lhs>) and requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) < std::forward<Rhs>(rhs)); } { return that % std::forward<Lhs>(lhs) < std::forward<Rhs>(rhs); }

		// a <= b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator<=(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto)//
			requires(not unit_test_detail::dispatcher::is_dispatched_expression_v<Lhs>) and requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) <= std::forward<Rhs>(rhs)); } { return that % std::forward<Lhs>(lhs) <= std::forward<Rhs>(rhs); }

		// a and b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator and(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto)//
			requires(not unit_test_detail::dispatcher::is_dispatched_expression_v<Lhs>) and requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) and std::forward<Rhs>(rhs)); } { return that % std::forward<Lhs>(lhs) and std::forward<Rhs>(rhs); }

		// a or b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator or(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto)//
			requires(not unit_test_detail::dispatcher::is_dispatched_expression_v<Lhs>) and requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) or std::forward<Rhs>(rhs)); } { return that % std::forward<Lhs>(lhs) or std::forward<Rhs>(rhs); }

		template<typename Function, std::ranges::range Container>
			requires std::is_invocable_v<Function, std::ranges::range_value_t<Container>>
		[[nodiscard]] constexpr auto operator|(const Function& function, const Container& container) noexcept -> auto
		{
			return
					[function, container](const std::string_view name) -> void
			{
				std::ranges::for_each(
						container,
						[&function, name](const auto& arg) -> void
						{
							unit_test_detail::dispatcher::register_event<Function>(
									unit_test_detail::events::EventTest<Function, std::ranges::range_value_t<Container>>{
											.name = name,
											.location = {},
											.categories = {},
											.invocable = function,
											.arg = arg});
						});
			};
		}
	}// namespace operators

	GAL_PROMETHEUS_MODULE_EXPORT_END

	namespace unit_test_detail::dispatcher
	{
		template<typename Lhs, typename Dispatcher>
		struct dispatched_expression
		{
			using expression_type = Lhs;
			using dispatcher_type = Dispatcher;

			expression_type expression;

			// // fixme: dispatched_expression is not allowed to be converted to bool, otherwise `a and b` or `a or b` would be ambiguous, instead of calling `operator and` or `operator or` as defined below.
			// fixme: dispatched_expression must be able to be converted to bool, otherwise many expressions will fail to compile due to overloaded operators in test::operators.
			// note: Once dispatched_expression is allowed to be converted to bool, operand::expression_t alone will no longer be sufficient for overloading, and it must be specifically required that the expression is not dispatched_expression (when the expression satisfies operand::expression_t).
			[[nodiscard]] constexpr explicit operator bool() const noexcept//
			{
				return static_cast<bool>(expression);
			}

			// ============================================
			// operator==
			// ============================================

			// OperandValue

			// as_b{...} == bool
			[[nodiscard]] constexpr auto operator==(const bool rhs) const && noexcept -> dispatched_expression<operand::OperandCompareIdentity, dispatcher_type>//
				requires operand::operand_value_boolean_t<expression_type> { return {.expression = {expression, rhs}}; }

			// bool == as_b{...}
			// [[nodiscard]] constexpr auto operator==(const operand::operand_value_boolean_t auto& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareIdentity, dispatcher_type>
			[[nodiscard]] constexpr auto operator==(const operand::OperandValue<bool>& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareIdentity, dispatcher_type>//
				requires std::is_same_v<expression_type, bool> { return {.expression = {rhs, expression}}; }

			// as_x{...} == floating_point
			template<std::floating_point Rhs>
				requires operand::operand_value_floating_point_t<expression_type>
			[[nodiscard]] constexpr auto operator==(const Rhs rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareApprox<typename expression_type::value_type, Rhs, Rhs>, dispatcher_type>{.expression = {expression.value(), rhs, expression.epsilon()}};
			}

			//  floating_point == as_x{...}
			template<operand::operand_value_floating_point_t Rhs>
				requires std::floating_point<expression_type>
			[[nodiscard]] constexpr auto operator==(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareApprox<expression_type, typename Rhs::value_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, rhs.value(), rhs.epsilon()}};
			}

			// as_x{...} == not(boolean/floating_point)
			template<typename Rhs>
				requires operand::operand_value_t<expression_type> and (not std::same_as<Rhs, bool> and not std::floating_point<Rhs>)
			[[nodiscard]] constexpr auto operator==(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareEqual<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression.value(), rhs}};
			}

			// not(boolean/floating_point) == as_x{...}
			template<operand::operand_value_t Rhs>
				requires(not std::same_as<expression_type, bool> and not std::floating_point<expression_type>)
			[[nodiscard]] constexpr auto operator==(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareEqual<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, rhs.value()}};
			}

			// OperandConstantXxx

			// "xxx"_b == bool => as_b{"xxx"} == bool
			// bool == "xxx"_b => bool == as_b{"xxx"}

			// "xxx"_c == character
			template<std::integral Rhs>
				requires operand::operand_constant_character_t<expression_type>
			[[nodiscard]] constexpr auto operator==(const Rhs rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareEqual<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression_type::value, rhs}};
			}

			// character == "xxx"_c
			template<operand::operand_constant_character_t Rhs>
				requires std::integral<expression_type>
			[[nodiscard]] constexpr auto operator==(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareEqual<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, Rhs::value}};
			}

			// "xxx"_x == integral
			template<std::integral Rhs>
				requires operand::operand_constant_integral_t<expression_type>
			[[nodiscard]] constexpr auto operator==(const Rhs rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareEqual<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression_type::value, rhs}};
			}

			// integral == "xxx"_x
			template<operand::operand_constant_integral_t Rhs>
				requires std::integral<expression_type>
			[[nodiscard]] constexpr auto operator==(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareEqual<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, Rhs::value}};
			}

			// "xxx"_x == floating_point
			template<std::floating_point Rhs>
				requires operand::operand_constant_floating_point_t<expression_type>
			[[nodiscard]] constexpr auto operator==(const Rhs rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareApprox<typename expression_type::value_type, Rhs, typename expression_type::value_type>, dispatcher_type>{.expression = {expression_type::value, rhs, expression_type::epsilon}};
			}

			// floating_point == "xxx"_x
			template<operand::operand_constant_floating_point_t Rhs>
				requires std::floating_point<expression_type>
			[[nodiscard]] constexpr auto operator==(const Rhs& rhs) const && noexcept -> auto//
				requires requires { static_cast<bool>(operand::OperandCompareApprox<expression_type, typename Rhs::value_type, typename Rhs::value_type>{expression, Rhs::value, Rhs::epsilon}); } { return dispatched_expression<operand::OperandCompareApprox<expression_type, typename Rhs::value_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, Rhs::value, Rhs::epsilon}}; }

			// "xxx"_auto == any
			template<typename Rhs>
				requires operand::operand_constant_auto_t<expression_type>
			[[nodiscard]] constexpr auto operator==(const Rhs& rhs) const && noexcept -> auto
			{
				using operators::operator==;

				// forward
				return typename expression_type::template rebind<Rhs>{} == rhs;
			}

			// any == "xxx"_auto
			template<operand::operand_constant_auto_t Rhs>
			[[nodiscard]] constexpr auto operator==(const Rhs& rhs) const && noexcept -> auto
			{
				using operators::operator==;

				// forward
				return expression == typename Rhs::template rebind<expression_type>{};
			}

			// ANY == ANY

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator==(Rhs&& rhs) const & noexcept -> dispatched_expression<operand::OperandCompareEqual<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {expression, std::forward<Rhs>(rhs)}};
			}

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator==(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareEqual<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}};
			}

			// any
			template<typename Rhs>
				requires(
					not(operand::operand_value_t<expression_type> or operand::operand_value_t<Rhs>) and      //
					not(operand::operand_constant_t<expression_type> or operand::operand_constant_t<Rhs>) and//
					not(std::ranges::range<expression_type> or std::ranges::range<Rhs>)                      //
				)
			[[nodiscard]] constexpr auto operator==(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareEqual<expression_type, Rhs>, dispatcher_type>//
				requires requires { static_cast<bool>(operand::OperandCompareEqual<expression_type, Rhs>{std::move(expression), std::forward<Rhs>(rhs)}); } { return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}}; }

			// ============================================
			// operator!=
			// ============================================

			// OperandValue

			// as_b{...} != bool
			[[nodiscard]] constexpr auto operator!=(const bool rhs) const && noexcept -> dispatched_expression<operand::OperandCompareIdentity, dispatcher_type>//
				requires operand::operand_value_boolean_t<expression_type> { return {.expression = {expression, not rhs}}; }

			// bool == as_b{...}
			// [[nodiscard]] constexpr auto operator!=(const operand::operand_value_boolean_t auto& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareIdentity, dispatcher_type>
			[[nodiscard]] constexpr auto operator!=(const operand::OperandValue<bool>& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareIdentity, dispatcher_type>//
				requires std::is_same_v<expression_type, bool> { return {.expression = {rhs, not expression}}; }

			// as_x{...} != floating_point
			template<std::floating_point Rhs>
				requires operand::operand_value_floating_point_t<expression_type>
			[[nodiscard]] constexpr auto operator!=(const Rhs rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareNotApprox<typename expression_type::value_type, Rhs, Rhs>, dispatcher_type>{.expression = {expression.value(), rhs, expression.epsilon()}};
			}

			//  floating_point != as_x{...}
			template<operand::operand_value_floating_point_t Rhs>
				requires std::floating_point<expression_type>
			[[nodiscard]] constexpr auto operator!=(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareNotApprox<expression_type, typename Rhs::value_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, rhs.value(), rhs.epsilon()}};
			}

			// as_x{...} != not(boolean/floating_point)
			template<typename Rhs>
				requires operand::operand_value_t<expression_type> and (not std::same_as<Rhs, bool> and not std::floating_point<Rhs>)
			[[nodiscard]] constexpr auto operator!=(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareNotEqual<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression.value(), rhs}};
			}

			// not(boolean/floating_point) != as_x{...}
			template<operand::operand_value_t Rhs>
				requires(not std::same_as<expression_type, bool> and not std::floating_point<expression_type>)
			[[nodiscard]] constexpr auto operator!=(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareNotEqual<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, rhs.value()}};
			}

			// OperandConstantXxx

			// "xxx"_b != bool => as_b{"xxx"} != bool
			// bool != "xxx"_b => bool != as_b{"xxx"}

			// "xxx"_x != integral
			template<std::integral Rhs>
				requires operand::operand_constant_integral_t<expression_type>
			[[nodiscard]] constexpr auto operator!=(const Rhs rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareNotEqual<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression_type::value, rhs}};
			}

			// integral != "xxx"_x
			template<operand::operand_constant_integral_t Rhs>
				requires std::integral<expression_type>
			[[nodiscard]] constexpr auto operator!=(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareNotEqual<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, Rhs::value}};
			}

			// "xxx"_x != floating_point
			template<std::floating_point Rhs>
				requires operand::operand_constant_floating_point_t<expression_type>
			[[nodiscard]] constexpr auto operator!=(const Rhs rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareNotApprox<expression_type, Rhs, typename expression_type::value_type>, dispatcher_type>{.expression = {expression_type::value, rhs, expression_type::epsilon}};
			}

			// floating_point != "xxx"_x
			template<operand::operand_constant_floating_point_t Rhs>
				requires std::floating_point<expression_type>
			[[nodiscard]] constexpr auto operator!=(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareNotApprox<expression_type, Rhs, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, Rhs::value, Rhs::epsilon}};
			}

			// "xxx"_auto != any
			template<typename Rhs>
				requires operand::operand_constant_auto_t<expression_type>
			[[nodiscard]] constexpr auto operator!=(const Rhs& rhs) const && noexcept -> auto
			{
				using operators::operator!=;

				// forward
				return typename expression_type::template rebind<Rhs>{} != rhs;
			}

			// any != "xxx"_auto
			template<operand::operand_constant_auto_t Rhs>
			[[nodiscard]] constexpr auto operator!=(const Rhs& rhs) const && noexcept -> auto
			{
				using operators::operator!=;

				// forward
				return expression != typename Rhs::template rebind<expression_type>{};
			}

			// ANY == ANY

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator!=(Rhs&& rhs) const & noexcept -> dispatched_expression<operand::OperandCompareNotEqual<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {expression, std::forward<Rhs>(rhs)}};
			}

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator!=(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareNotEqual<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}};
			}

			// any
			template<typename Rhs>
				requires(
					not(operand::operand_value_t<expression_type> or operand::operand_value_t<Rhs>) and      //
					not(operand::operand_constant_t<expression_type> or operand::operand_constant_t<Rhs>) and//
					not(std::ranges::range<expression_type> or std::ranges::range<Rhs>)                      //
				)
			[[nodiscard]] constexpr auto operator!=(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareNotEqual<expression_type, Rhs>, dispatcher_type>//
				requires requires { static_cast<bool>(operand::OperandCompareNotEqual<expression_type, Rhs>{std::move(expression), std::forward<Rhs>(rhs)}); } { return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}}; }

			// ============================================
			// operator>
			// ============================================

			// OperandValue

			// as_x{...} > ...
			template<typename Rhs>
				requires operand::operand_value_t<expression_type>
			[[nodiscard]] constexpr auto operator>(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareGreaterThan<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression.value(), rhs}};
			}

			// ... > as_x{...}
			template<operand::operand_value_t Rhs>
			[[nodiscard]] constexpr auto operator>(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareGreaterThan<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, rhs.value()}};
			}

			// OperandConstantXxx

			// "xxx"_x > ...
			template<typename Rhs>
				requires operand::operand_constant_t<expression_type>
			[[nodiscard]] constexpr auto operator>(const Rhs& rhs) const && noexcept -> auto
			{
				if constexpr (operand::operand_constant_auto_t<expression_type>)
				{
					using operators::operator>;

					// forward
					return typename expression_type::template rebind<Rhs>{} > rhs;
				}
				else { return dispatched_expression<operand::OperandCompareGreaterThan<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression_type::value, rhs}}; }
			}

			// ... > "xxx"_x
			template<operand::operand_constant_t Rhs>
			[[nodiscard]] constexpr auto operator>(const Rhs& rhs) const && noexcept -> auto
			{
				if constexpr (operand::operand_constant_auto_t<Rhs>)
				{
					using operators::operator>;

					// forward
					return expression > typename Rhs::template rebind<expression_type>{};
				}
				else { return dispatched_expression<operand::OperandCompareGreaterThan<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, Rhs::value}}; }
			}

			// ANY == ANY

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator>(Rhs&& rhs) const & noexcept -> dispatched_expression<operand::OperandCompareGreaterThan<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {expression, std::forward<Rhs>(rhs)}};
			}

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator>(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareGreaterThan<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}};
			}

			// any
			template<typename Rhs>
				requires(
					not(operand::operand_value_t<expression_type> or operand::operand_value_t<Rhs>) and      //
					not(operand::operand_constant_t<expression_type> or operand::operand_constant_t<Rhs>) and//
					not(std::ranges::range<expression_type> or std::ranges::range<Rhs>)                      //
				)
			[[nodiscard]] constexpr auto operator>(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareGreaterThan<expression_type, Rhs>, dispatcher_type>//
				requires requires { static_cast<bool>(operand::OperandCompareGreaterThan<expression_type, Rhs>{std::move(expression), std::forward<Rhs>(rhs)}); } { return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}}; }

			// ============================================
			// operator>=
			// ============================================

			// OperandValue

			// as_x{...} >= ...
			template<typename Rhs>
				requires operand::operand_value_t<expression_type>
			[[nodiscard]] constexpr auto operator>=(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareGreaterEqual<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression.value(), rhs}};
			}

			// ... >= as_x{...}
			template<operand::operand_value_t Rhs>
			[[nodiscard]] constexpr auto operator>=(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareGreaterEqual<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, rhs.value()}};
			}

			// OperandConstantXxx

			// "xxx"_x >= ...
			template<typename Rhs>
				requires operand::operand_constant_t<expression_type>
			[[nodiscard]] constexpr auto operator>=(const Rhs& rhs) const && noexcept -> auto
			{
				if constexpr (operand::operand_constant_auto_t<expression_type>)
				{
					using operators::operator>=;

					// forward
					return typename expression_type::template rebind<Rhs>{} >= rhs;
				}
				else { return dispatched_expression<operand::OperandCompareGreaterEqual<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression_type::value, rhs}}; }
			}

			// ... >= "xxx"_x
			template<operand::operand_constant_t Rhs>
			[[nodiscard]] constexpr auto operator>=(const Rhs& rhs) const && noexcept -> auto
			{
				if constexpr (operand::operand_constant_auto_t<Rhs>)
				{
					using operators::operator>=;

					// forward
					return expression >= typename Rhs::template rebind<expression_type>{};
				}
				else { return dispatched_expression<operand::OperandCompareGreaterEqual<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, Rhs::value}}; }
			}

			// ANY == ANY

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator>=(Rhs&& rhs) const & noexcept -> dispatched_expression<operand::OperandCompareGreaterEqual<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {expression, std::forward<Rhs>(rhs)}};
			}

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator>=(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareGreaterEqual<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}};
			}

			// any
			template<typename Rhs>
				requires(
					not(operand::operand_value_t<expression_type> or operand::operand_value_t<Rhs>) and      //
					not(operand::operand_constant_t<expression_type> or operand::operand_constant_t<Rhs>) and//
					not(std::ranges::range<expression_type> or std::ranges::range<Rhs>)                      //
				)
			[[nodiscard]] constexpr auto operator>=(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareGreaterEqual<expression_type, Rhs>, dispatcher_type>//
				requires requires { static_cast<bool>(operand::OperandCompareGreaterEqual<expression_type, Rhs>{std::move(expression), std::forward<Rhs>(rhs)}); } { return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}}; }

			// ============================================
			// operator<
			// ============================================

			// OperandValue

			// as_x{...} < ...
			template<typename Rhs>
				requires operand::operand_value_t<expression_type>
			[[nodiscard]] constexpr auto operator<(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareLessThan<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression.value(), rhs}};
			}

			// ... < as_x{...}
			template<operand::operand_value_t Rhs>
			[[nodiscard]] constexpr auto operator<(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareLessThan<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, rhs.value()}};
			}

			// OperandConstantXxx

			// "xxx"_x < ...
			template<typename Rhs>
				requires operand::operand_constant_t<expression_type>
			[[nodiscard]] constexpr auto operator<(const Rhs& rhs) const && noexcept -> auto
			{
				if constexpr (operand::operand_constant_auto_t<expression_type>)
				{
					using operators::operator<;

					// forward
					return typename expression_type::template rebind<Rhs>{} < rhs;
				}
				else { return dispatched_expression<operand::OperandCompareLessThan<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression_type::value, rhs}}; }
			}

			// ... < "xxx"_x
			template<operand::operand_constant_t Rhs>
			[[nodiscard]] constexpr auto operator<(const Rhs& rhs) const && noexcept -> auto
			{
				if constexpr (operand::operand_constant_auto_t<Rhs>)
				{
					using operators::operator<;

					// forward
					return expression < typename Rhs::template rebind<expression_type>{};
				}
				else { return dispatched_expression<operand::OperandCompareLessThan<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, Rhs::value}}; }
			}

			// ANY == ANY

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator<(Rhs&& rhs) const & noexcept -> dispatched_expression<operand::OperandCompareLessThan<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {expression, std::forward<Rhs>(rhs)}};
			}

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator<(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareLessThan<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}};
			}

			// any
			template<typename Rhs>
				requires(
					not(operand::operand_value_t<expression_type> or operand::operand_value_t<Rhs>) and      //
					not(operand::operand_constant_t<expression_type> or operand::operand_constant_t<Rhs>) and//
					not(std::ranges::range<expression_type> or std::ranges::range<Rhs>)                      //
				)
			[[nodiscard]] constexpr auto operator<(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareLessThan<expression_type, Rhs>, dispatcher_type>//
				requires requires { static_cast<bool>(operand::OperandCompareLessThan<expression_type, Rhs>{std::move(expression), std::forward<Rhs>(rhs)}); } { return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}}; }

			// ============================================
			// operator<=
			// ============================================

			// OperandValue

			// as_x{...} <= ...
			template<typename Rhs>
				requires operand::operand_value_t<expression_type>
			[[nodiscard]] constexpr auto operator<=(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareLessEqual<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression.value(), rhs}};
			}

			// ... <= as_x{...}
			template<operand::operand_value_t Rhs>
			[[nodiscard]] constexpr auto operator<=(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandCompareLessEqual<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, rhs.value()}};
			}

			// OperandConstantXxx

			// "xxx"_x <= ...
			template<typename Rhs>
				requires operand::operand_constant_t<expression_type>
			[[nodiscard]] constexpr auto operator<=(const Rhs& rhs) const && noexcept -> auto
			{
				if constexpr (operand::operand_constant_auto_t<expression_type>)
				{
					using operators::operator<=;

					// forward
					return typename expression_type::template rebind<Rhs>{} <= rhs;
				}
				else { return dispatched_expression<operand::OperandCompareLessEqual<typename expression_type::value_type, Rhs>, dispatcher_type>{.expression = {expression_type::value, rhs}}; }
			}

			// ... <= "xxx"_x
			template<operand::operand_constant_t Rhs>
			[[nodiscard]] constexpr auto operator<=(const Rhs& rhs) const && noexcept -> auto
			{
				if constexpr (operand::operand_constant_auto_t<Rhs>)
				{
					using operators::operator<=;

					// forward
					return expression <= typename Rhs::template rebind<expression_type>{};
				}
				else { return dispatched_expression<operand::OperandCompareLessEqual<expression_type, typename Rhs::value_type>, dispatcher_type>{.expression = {expression, Rhs::value}}; }
			}

			// ANY == ANY

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator<=(Rhs&& rhs) const & noexcept -> dispatched_expression<operand::OperandCompareLessEqual<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {expression, std::forward<Rhs>(rhs)}};
			}

			// range
			template<std::ranges::range Rhs>
				requires std::ranges::range<expression_type>
			[[nodiscard]] constexpr auto operator<=(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareLessEqual<std::decay_t<expression_type>, std::decay_t<Rhs>>, dispatcher_type>//
			{
				return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}};
			}

			// any
			template<typename Rhs>
				requires(
					not(operand::operand_value_t<expression_type> or operand::operand_value_t<Rhs>) and      //
					not(operand::operand_constant_t<expression_type> or operand::operand_constant_t<Rhs>) and//
					not(std::ranges::range<expression_type> or std::ranges::range<Rhs>)                      //
				)
			[[nodiscard]] constexpr auto operator<=(Rhs&& rhs) const && noexcept -> dispatched_expression<operand::OperandCompareLessEqual<expression_type, Rhs>, dispatcher_type>//
				requires requires { static_cast<bool>(operand::OperandCompareLessEqual<expression_type, Rhs>{std::move(expression), std::forward<Rhs>(rhs)}); } { return {.expression = {std::move(expression), std::forward<Rhs>(rhs)}}; }

			// ============================================
			// operator and
			// ============================================

			// operand::expression_t and dispatched_expression
			template<typename Rhs>
				requires(operand::expression_t<expression_type> and not is_dispatched_expression_v<expression_type>) and is_dispatched_expression_v<Rhs>
			[[nodiscard]] constexpr auto operator and(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandLogicalAnd<expression_type, typename Rhs::expression_type>, dispatcher_type>{.expression = {expression, rhs.expression}};
			}

			// dispatched_expression and operand::expression_t
			template<operand::expression_t Rhs>
				requires is_dispatched_expression_v<expression_type> and (not is_dispatched_expression_v<Rhs>)
			[[nodiscard]] constexpr auto operator and(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandLogicalAnd<typename expression_type::expression_type, Rhs>, dispatcher_type>{.expression = {expression.expression, rhs}};
			}

			// operand::expression_t and operand::expression_t
			template<operand::expression_t Rhs>
				requires(operand::expression_t<expression_type> and not is_dispatched_expression_v<expression_type>) and (not is_dispatched_expression_v<Rhs>)
			[[nodiscard]] constexpr auto operator and(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandLogicalAnd<expression_type, Rhs>, dispatcher_type>{.expression = {expression, rhs}};
			}

			// dispatched_expression and dispatched_expression
			template<typename Rhs>
				requires is_dispatched_expression_v<expression_type> and is_dispatched_expression_v<Rhs>
			[[nodiscard]] constexpr auto operator and(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandLogicalAnd<typename expression_type::expression_type, typename Rhs::expression_type>, dispatcher_type>{.expression = {expression.expression, rhs.expression}};
			}

			// ============================================
			// operator or
			// ============================================

			// operand::expression_t or dispatched_expression
			template<typename Rhs>
				requires(operand::expression_t<expression_type> and not is_dispatched_expression_v<expression_type>) and is_dispatched_expression_v<Rhs>
			[[nodiscard]] constexpr auto operator or(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandLogicalOr<expression_type, typename Rhs::expression_type>, dispatcher_type>{.expression = {expression, rhs.expression}};
			}

			// dispatched_expression or operand::expression_t
			template<operand::expression_t Rhs>
				requires is_dispatched_expression_v<expression_type> and (not is_dispatched_expression_v<Rhs>)
			[[nodiscard]] constexpr auto operator or(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandLogicalOr<typename expression_type::expression_type, Rhs>, dispatcher_type>{.expression = {expression.expression, rhs}};
			}

			// operand::expression_t or operand::expression_t
			template<operand::expression_t Rhs>
				requires(operand::expression_t<expression_type> and not is_dispatched_expression_v<expression_type>) and (not is_dispatched_expression_v<Rhs>)
			[[nodiscard]] constexpr auto operator or(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandLogicalOr<expression_type, Rhs>, dispatcher_type>{.expression = {expression, rhs}};
			}

			// dispatched_expression or dispatched_expression
			template<typename Rhs>
				requires is_dispatched_expression_v<expression_type> and is_dispatched_expression_v<Rhs>
			[[nodiscard]] constexpr auto operator or(const Rhs& rhs) const && noexcept -> auto//
			{
				return dispatched_expression<operand::OperandLogicalOr<typename expression_type::expression_type, typename Rhs::expression_type>, dispatcher_type>{.expression = {expression.expression, rhs.expression}};
			}
		};
	}

	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	inline namespace literals
	{
		template<meta::basic_fixed_string StringLiteral>
		constexpr auto operator""_test() noexcept -> unit_test_detail::dispatcher::DispatcherTestLiteral<StringLiteral> { return unit_test_detail::dispatcher::DispatcherTestLiteral<StringLiteral>::make(); }

		template<char... Cs>
		[[nodiscard]] constexpr auto operator""_auto() noexcept -> unit_test_detail::operand::OperandConstantAuto<Cs...> { return {}; }

		template<meta::basic_fixed_string StringLiteral>
		[[nodiscard]] constexpr auto operator""_c() noexcept -> unit_test_detail::operand::OperandConstantCharacter<*StringLiteral.begin()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<int>(); }
		[[nodiscard]] constexpr auto operator""_i() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<int>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned>(); }
		[[nodiscard]] constexpr auto operator""_u() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<unsigned>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<long>(); }
		[[nodiscard]] constexpr auto operator""_l() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<long>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned long>(); }
		[[nodiscard]] constexpr auto operator""_ul() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<unsigned long>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<long long>(); }
		[[nodiscard]] constexpr auto operator""_ll() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<long long>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned long long>(); }
		[[nodiscard]] constexpr auto operator""_ull() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<unsigned long long>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int8_t>(); }
		[[nodiscard]] constexpr auto operator""_i8() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<std::int8_t>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint8_t>(); }
		[[nodiscard]] constexpr auto operator""_u8() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<std::uint8_t>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int16_t>(); }
		[[nodiscard]] constexpr auto operator""_i16() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<std::int16_t>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint16_t>(); }
		[[nodiscard]] constexpr auto operator""_u16() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<std::uint16_t>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int32_t>(); }
		[[nodiscard]] constexpr auto operator""_i32() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<std::int32_t>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint32_t>(); }
		[[nodiscard]] constexpr auto operator""_u32() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<std::uint32_t>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int64_t>(); }
		[[nodiscard]] constexpr auto operator""_i64() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<std::int64_t>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint64_t>(); }
		[[nodiscard]] constexpr auto operator""_u64() noexcept -> unit_test_detail::operand::OperandConstantIntegral<functional::char_list<Cs...>.template to_integral<std::uint64_t>()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<float>(); }
		[[nodiscard]] constexpr auto operator""_f() noexcept -> unit_test_detail::operand::OperandConstantFloatingPoint<
			functional::char_list<Cs...>.template to_floating_point<float>(),
			functional::char_list<Cs...>.denominator_length()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<double>(); }
		[[nodiscard]] constexpr auto operator""_d() noexcept -> unit_test_detail::operand::OperandConstantFloatingPoint<
			functional::char_list<Cs...>.template to_floating_point<double>(),
			functional::char_list<Cs...>.denominator_length()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<long double>(); }
		[[nodiscard]] constexpr auto operator""_ld() noexcept -> unit_test_detail::operand::OperandConstantFloatingPoint<
			functional::char_list<Cs...>.template to_floating_point<long double>(),
			functional::char_list<Cs...>.denominator_length()> { return {}; }

		[[nodiscard]] constexpr auto operator""_b(const char* name, const std::size_t size) noexcept -> unit_test_detail::operand::OperandValue<bool>//
		{
			return unit_test_detail::operand::OperandValue<bool>{unit_test_detail::operand::OperandValue<bool>::value_type{name, size}};
		}

		[[nodiscard]] constexpr auto operator""_s(const char* name, std::size_t size) noexcept -> std::string_view { return {name, size}; }
	}

	// fixme: @see executor
	// If we don't use global singleton (instead, one object per compilation unit), an object will be destructed multiple times.
	// So it is not possible to set a default suite name (to avoid conflicts).
	template<meta::basic_fixed_string SuiteName>
	struct suite
	{
		template<std::invocable InvocableType>
		constexpr explicit(false) suite(InvocableType invocable) noexcept//
			requires requires { +invocable; }
		{
			unit_test_detail::dispatcher::register_event<decltype(+invocable)>(unit_test_detail::events::EventSuite{
					.name = SuiteName.operator std::string_view(),
					.function = +invocable});
		}
	};

	template<typename T>
	constexpr auto type = unit_test_detail::operand::OperandType<T>{};
	template<unit_test_detail::operand::OperandConstantExpression Constant>
	constexpr auto constant = Constant;

	template<typename ExceptionType = void, std::invocable InvocableType>
	[[nodiscard]] constexpr auto throws(const InvocableType& invocable) noexcept -> unit_test_detail::operand::OperandThrow<InvocableType, ExceptionType> { return {invocable}; }

	template<std::invocable InvocableType>
	[[nodiscard]] constexpr auto nothrow(const InvocableType& invocable) noexcept -> unit_test_detail::operand::OperandNoThrow<InvocableType> { return {invocable}; }

	#if __has_include(<unistd.h>) and __has_include(<sys/wait.h>)
	template<std::invocable InvocableType>
	[[nodiscard]] constexpr auto aborts(const InvocableType& invocable) noexcept -> unit_test_detail::operand::OperandAbort<InvocableType>
	{
		return {invocable};
	}
	#endif

	// template<typename Lhs, typename Rhs>
	// [[nodiscard]] constexpr auto eq(const Lhs& lhs, const Rhs& rhs) noexcept -> unit_test_detail::operand::OperandCompareEqual<Lhs, Rhs> { return {lhs, rhs}; }
	//
	// template<typename Lhs, typename Rhs, typename Epsilon>
	// [[nodiscard]] constexpr auto approx(const Lhs& lhs, const Rhs& rhs, const Epsilon& epsilon) noexcept -> unit_test_detail::operand::OperandCompareApprox<Lhs, Rhs, Epsilon> { return {lhs, rhs, epsilon}; }
	//
	// template<typename Lhs, typename Rhs>
	// [[nodiscard]] constexpr auto neq(const Lhs& lhs, const Rhs& rhs) noexcept -> unit_test_detail::operand::OperandCompareNotEqual<Lhs, Rhs> { return {lhs, rhs}; }
	//
	// template<typename Lhs, typename Rhs>
	// [[nodiscard]] constexpr auto gt(const Lhs& lhs, const Rhs& rhs) noexcept -> unit_test_detail::operand::OperandCompareGreaterThan<Lhs, Rhs> { return {lhs, rhs}; }
	//
	// template<typename Lhs, typename Rhs>
	// [[nodiscard]] constexpr auto ge(const Lhs& lhs, const Rhs& rhs) noexcept -> unit_test_detail::operand::OperandCompareGreaterEqual<Lhs, Rhs> { return {lhs, rhs}; }
	//
	// template<typename Lhs, typename Rhs>
	// [[nodiscard]] constexpr auto lt(const Lhs& lhs, const Rhs& rhs) noexcept -> unit_test_detail::operand::OperandCompareLessThan<Lhs, Rhs> { return {lhs, rhs}; }
	//
	// template<typename Lhs, typename Rhs>
	// [[nodiscard]] constexpr auto le(const Lhs& lhs, const Rhs& rhs) noexcept -> unit_test_detail::operand::OperandCompareLessEqual<Lhs, Rhs> { return {lhs, rhs}; }

	GAL_PROMETHEUS_MODULE_EXPORT_END
}