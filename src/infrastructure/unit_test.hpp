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
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	struct color_type
	{
		std::string_view none = "\033[0m";

		std::string_view fail = "\033[31m\033[7m";
		std::string_view pass = "\033[32m\033[7m";
		std::string_view skip = "\033[33m\033[7m";
		std::string_view fatal = "\033[35m\033[7m";

		std::string_view suite = "\033[34m\033[7m";
		std::string_view test = "\033[36m\033[7m";
		std::string_view expression = "\033[38;5;207m\033[7m";
		std::string_view message = "\033[38;5;27m\033[7m";
	};

	using clock_type = std::chrono::high_resolution_clock;
	using time_point_type = clock_type::time_point;
	using time_difference_type = std::chrono::milliseconds;

	struct test_result_type;
	using test_results_type = std::vector<test_result_type>;

	struct test_result_type
	{
		enum class Status
		{
			PENDING,

			PASSED,
			FAILED,
			SKIPPED,
			FATAL,
		};

		std::string name;

		test_result_type* parent;
		test_results_type children;

		Status status;
		time_point_type time_start;
		time_point_type time_end;
		std::size_t total_assertions_passed;
		std::size_t total_assertions_failed;
	};

	constexpr static std::string_view anonymous_suite_name{"anonymous_suite"};

	struct suite_result_type
	{
		std::string name;

		std::string report_string;

		test_results_type test_results;
	};

	/**
	 * result: std::vector<suite> {																			 
	 * anonymous_suite: suite																						 
	 * user_suite_0: suite																								
	 * user_suite_1: suite																								
	 * user_suite_2: suite																								
	 * user_suite_3: suite																								
	 * user_suite_n: suite																								 
	 * }																														
	 *																															
	 * *_suite_*: suite {																									 
	 * name: std::string																								
	 * user_test_0: test																									 
	 * user_test_1: test																									 
	 * user_test_2: test																									 
	 * user_test_3: test																									 
	 * user_test_n: test																									 
	 * }																														
	 *																															
	 * *_test_*: test {																									
	 * name: std::string																								
	 * parent: test*																										 
	 * children(nested test): std::vector<test>																
	 *																															
	 * status: Status																										 
	 * time_start: time_point_type																				
	 * time_end: time_point_type																				
	 * total_assertions_passed: std::size_t																		
	 * total_assertions_failed: std::size_t																		
	 * }
	 */
	// assume suite_results.front() == anonymous_suite
	using suite_results_type = std::vector<suite_result_type>;

	enum class OutputLevel
	{
		NONE = 0,
		// Only the results of each suite execution are output.
		RESULT_ONLY = 1,
		// RESULT_ONLY + the expression of each test.
		INCLUDE_EXPRESSION = 2,
		// INCLUDE_EXPRESSION + the source location of each expression.
		INCLUDE_EXPRESSION_LOCATION = 3,
	};

	struct config
	{
		using name_type = std::string_view;
		using category_type = std::string_view;
		using categories_type = std::vector<category_type>;

		color_type color;

		// terminate the program after n failed assertions (per suite).
		// if abort_after_n_failures == 0:
		//	terminate the program immediately if the assertion fails
		std::size_t abort_after_n_failures = std::numeric_limits<std::size_t>::max();

		OutputLevel output_level = OutputLevel::INCLUDE_EXPRESSION_LOCATION;
		bool dry_run = false;

		// how to terminate the program
		std::function<void()> terminator =
				[]() -> void
		{
			std::exit(-1); // NOLINT(concurrency-mt-unsafe)
		};

		std::function<void(std::string_view)> message_reporter =
				[](const std::string_view report_message) -> void { std::cout << report_message; };

		// Used to filter the suite/test cases that need to be executed.
		std::function<bool(name_type)> filter_execute_suite_name =
				[]([[maybe_unused]] const name_type suite_name) noexcept -> bool { return true; };
		std::function<bool(name_type)> filter_execute_test_name =
				[]([[maybe_unused]] const name_type test_name) noexcept -> bool { return true; };
		std::function<bool(const categories_type&)> filter_execute_test_categories =
				[](const categories_type& categories) noexcept -> bool
		{
			if (std::ranges::contains(categories, "skip")) { return false; }

			return true;
		};

		[[noreturn]] auto terminate() const noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(terminator);

			terminator();
			std::exit(-1); // NOLINT(concurrency-mt-unsafe)
		}

		auto report_message(const std::string_view message) const noexcept -> void
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(message_reporter);

			message_reporter(message);
		}

		[[nodiscard]] auto is_suite_execute_required(const name_type suite_name) const noexcept -> bool //
		{
			return filter_execute_suite_name(suite_name);
		}

		[[nodiscard]] auto is_test_execute_required(const name_type test_name, const categories_type& categories) const noexcept -> bool //
		{
			return filter_execute_test_name(test_name) and filter_execute_test_categories(categories);
		}
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END

	template<typename Expression>
	struct is_expression
			: std::bool_constant<
				// implicit
				std::is_convertible_v<Expression, bool> or
				// explicit
				std::is_constructible_v<bool, Expression>
			> {};

	template<typename Expression>
	constexpr auto is_expression_v = is_expression<Expression>::value;
	template<typename Expression>
	concept expression_t = is_expression_v<Expression>;

	namespace events
	{
		using name_type = std::string_view;

		struct none {};

		class Event {};

		template<typename E>
		constexpr auto is_event_v = std::is_base_of_v<Event, E>;
		template<typename E>
		concept event_t = is_event_v<E>;

		// =========================================
		// SUITE
		// =========================================

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSuiteBegin : public Event
		{
		public:
			name_type name;
		};

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSuiteEnd : public Event
		{
		public:
			name_type name;
		};

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSuite : public Event
		{
		public:
			using suite_type = void (*)();

			name_type name;
			suite_type suite;

			constexpr auto operator()() -> void { std::invoke(suite); }
			constexpr auto operator()() const -> void { std::invoke(suite); }

		private:
			[[nodiscard]] constexpr explicit operator EventSuiteBegin() const noexcept { return {.name = name}; }

			[[nodiscard]] constexpr explicit operator EventSuiteEnd() const noexcept { return {.name = name}; }

		public:
			[[nodiscard]] constexpr auto begin() const noexcept -> EventSuiteBegin { return this->operator EventSuiteBegin(); }

			[[nodiscard]] constexpr auto end() const noexcept -> EventSuiteEnd { return this->operator EventSuiteEnd(); }
		};

		// =========================================
		// TEST
		// =========================================

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTestBegin : public Event
		{
		public:
			name_type name;
		};

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTestSkip : public Event
		{
		public:
			name_type name;
		};

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTestEnd : public Event
		{
		public:
			name_type name;
		};

		template<typename InvocableType, typename Arg = none>
			requires std::is_invocable_v<InvocableType> or std::is_invocable_v<InvocableType, Arg>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTest : public Event
		{
		public:
			using invocable_type = InvocableType;
			using arg_type = Arg;

			name_type name;
			config::categories_type categories;

			mutable invocable_type invocable;
			mutable arg_type arg;

			constexpr auto operator()() const -> void
			{
				return []<typename I, typename A>(I&& i, A&& a) -> void
				{
					if constexpr (requires { std::invoke(std::forward<I>(i)); }) { std::invoke(std::forward<I>(i)); }
					else if constexpr (requires { std::invoke(std::forward<I>(i), std::forward<A>(a)); })
					{
						std::invoke(std::forward<I>(i), std::forward<A>(a));
					}
					else if constexpr (requires { std::invoke(i.template operator()<A>()); }) { std::invoke(i.template operator()<A>()); }
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				}(invocable, arg);
			}

		private:
			[[nodiscard]] constexpr explicit operator EventTestBegin() const noexcept { return {.name = name}; }

			[[nodiscard]] constexpr explicit operator EventTestEnd() const noexcept { return {.name = name}; }

			[[nodiscard]] constexpr explicit operator EventTestSkip() const noexcept { return {.name = name}; }

		public:
			[[nodiscard]] constexpr auto begin() const noexcept -> EventTestBegin { return this->operator EventTestBegin(); }

			[[nodiscard]] constexpr auto end() const noexcept -> EventTestEnd { return this->operator EventTestEnd(); }

			[[nodiscard]] constexpr auto skip() const noexcept -> EventTestSkip { return this->operator EventTestSkip(); }
		};

		// =========================================
		// ASSERTION
		// =========================================

		template<expression_t Expression>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertionPass : public Event
		{
		public:
			using expression_type = Expression;

			expression_type expression;
			std::source_location location;
		};

		template<expression_t Expression>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertionFail : public Event
		{
		public:
			using expression_type = Expression;

			expression_type expression;
			std::source_location location;
		};

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertionFatal : public Event
		{
		public:
			std::source_location location;
		};

		template<expression_t Expression>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertionFatalSkip : public Event
		{
		public:
			using expression_type = Expression;

			expression_type expression;
			std::source_location location;
		};

		template<expression_t Expression>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertion : public Event
		{
		public:
			using expression_type = Expression;

			expression_type expression;
			std::source_location location;

		private:
			[[nodiscard]] constexpr explicit operator EventAssertionPass<expression_type>() const noexcept
			{
				return {.expression = expression, .location = location};
			}

			[[nodiscard]] constexpr explicit operator EventAssertionFail<expression_type>() const noexcept
			{
				return {.expression = expression, .location = location};
			}

			[[nodiscard]] constexpr explicit operator EventAssertionFatal() const noexcept { return {.location = location}; }

			[[nodiscard]] constexpr explicit operator EventAssertionFatalSkip<expression_type>() const noexcept
			{
				return {.expression = expression, .location = location};
			}

		public:
			[[nodiscard]] constexpr auto pass() const noexcept -> EventAssertionPass<expression_type>
			{
				// fixme: Compiler Error: C2273
				// return this->operator EventAssertionPass<expression_type>();
				return operator EventAssertionPass<expression_type>();
			}

			[[nodiscard]] constexpr auto fail() const noexcept -> EventAssertionFail<expression_type>
			{
				// fixme: Compiler Error: C2273
				// return this->operator EventAssertionFail<expression_type>();
				return operator EventAssertionFail<expression_type>();
			}

			[[nodiscard]] constexpr auto fatal() const noexcept -> EventAssertionFatal
			{
				// fixme: Compiler Error: C2273
				// return this->operator EventAssertionFatal();
				return operator EventAssertionFatal();
			}

			[[nodiscard]] constexpr auto fatal_skip() const noexcept -> EventAssertionFatalSkip<expression_type>
			{
				// fixme: Compiler Error: C2273
				// return this->operator EventAssertionFatalSkip<expression_type>();
				return operator EventAssertionFatalSkip<expression_type>();
			}
		};

		// =========================================
		// EXCEPTION
		// =========================================

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventException : public Event
		{
		public:
			std::string_view message;

			[[nodiscard]] constexpr auto what() const noexcept -> std::string_view { return message; }
		};

		// =========================================
		// LOG
		// =========================================

		template<typename MessageType>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventLog : public Event
		{
		public:
			using message_type = MessageType;

			message_type message;
		};

		EventLog(const char*) -> EventLog<std::basic_string_view<char>>;
		template<std::size_t N>
		EventLog(const char (&)[N]) -> EventLog<std::basic_string_view<char>>;
		EventLog(const wchar_t*) -> EventLog<std::basic_string_view<wchar_t>>;
		template<std::size_t N>
		EventLog(const wchar_t (&)[N]) -> EventLog<std::basic_string_view<wchar_t>>;
		EventLog(const char8_t*) -> EventLog<std::basic_string_view<char8_t>>;
		template<std::size_t N>
		EventLog(const char8_t (&)[N]) -> EventLog<std::basic_string_view<char8_t>>;
		EventLog(const char16_t*) -> EventLog<std::basic_string_view<char16_t>>;
		template<std::size_t N>
		EventLog(const char16_t (&)[N]) -> EventLog<std::basic_string_view<char16_t>>;
		EventLog(const char32_t*) -> EventLog<std::basic_string_view<char32_t>>;
		template<std::size_t N>
		EventLog(const char32_t (&)[N]) -> EventLog<std::basic_string_view<char32_t>>;

		// =========================================
		// SUMMARY
		// =========================================

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSummary : public Event {};
	} // namespace events

	namespace operands
	{
		template<typename T>
		[[nodiscard]] constexpr auto wrap_abs(const T value) noexcept -> T //
			requires requires { std::abs(value); }
		{
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED { return value > 0 ? value : -value; }

			return std::abs(value);
		}

		class Operand
		{
		public:
			// magic
			using prefer_no_type_name = int;
		};

		template<typename O>
		constexpr auto is_operand_v = std::is_base_of_v<Operand, O>;
		template<typename O>
		concept operand_t = is_operand_v<O>;

		template<typename T>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandValue : public Operand
		{
		public:
			using value_type = T;

		private:
			value_type value_;

		public:
			constexpr explicit(false) OperandValue(const value_type value) //
				noexcept(std::is_nothrow_copy_constructible_v<value_type>) //
				requires(std::is_trivially_copy_constructible_v<value_type>)
				: value_{value} {}

			constexpr explicit(false) OperandValue(const value_type& value) //
				noexcept(std::is_nothrow_copy_constructible_v<value_type>) //
				requires(
					not std::is_trivially_copy_constructible_v<value_type> and
					std::is_copy_constructible_v<value_type>)
				: value_{value} {}

			constexpr explicit(false) OperandValue(value_type&& value) noexcept(std::is_nothrow_move_constructible_v<value_type>) //
				requires(
					not std::is_trivially_copy_constructible_v<value_type> and
					std::is_move_constructible_v<value_type>)
				: value_{std::move(value)} {}

			template<typename U>
				requires(std::is_trivially_constructible_v<value_type, U>)
			constexpr explicit(false) OperandValue(const value_type value) //
				noexcept(std::is_nothrow_constructible_v<value_type, U>) //
				: value_{value} {}

			template<typename U>
				requires(
					not std::is_trivially_constructible_v<value_type, const U&> and
					std::is_constructible_v<value_type, const U&>)
			constexpr explicit(false) OperandValue(const U& value) //
				noexcept(std::is_nothrow_constructible_v<value_type, const U&>) //
				: value_{value} {}

			template<typename U>
				requires(
					not std::is_trivially_constructible_v<value_type, U&&> and
					std::is_constructible_v<value_type, U&&>)
			constexpr explicit(false) OperandValue(U&& value) //
				noexcept(std::is_nothrow_constructible_v<value_type, U&&>)
				: value_{std::forward<U>(value)} {}

			template<typename... Args>
				requires((sizeof...(Args) != 1) and std::is_constructible_v<value_type, Args...>)
			constexpr explicit OperandValue(Args&&... args) //
				noexcept(std::is_nothrow_constructible_v<value_type, Args...>)
				: value_{std::forward<Args>(args)...} {}

			[[nodiscard]] constexpr auto value() noexcept -> value_type& { return value_; }

			[[nodiscard]] constexpr auto value() const noexcept -> const value_type& { return value_; }

			[[nodiscard]] constexpr auto to_string() const noexcept -> std::string { return meta::to_string(value_); }
		};

		template<typename T>
		OperandValue(T) -> OperandValue<T>;

		template<typename>
		struct is_operand_value : std::false_type {};

		template<typename T>
		struct is_operand_value<OperandValue<T>> : std::true_type {};

		template<typename T>
		constexpr auto is_operand_value_v = is_operand_value<T>::value;
		template<typename O>
		concept operand_value_t = is_operand_value_v<O>;

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteral : public Operand {};

		template<typename O>
		constexpr auto is_operand_literal_v = std::is_base_of_v<OperandLiteral, O>;
		template<typename O>
		concept operand_literal_t = is_operand_literal_v<O>;

		template<char Value>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralCharacter : public OperandLiteral
		{
		public:
			using value_type = char;

			constexpr static auto value = Value;

			[[nodiscard]] constexpr static auto to_string() noexcept -> std::string { return meta::to_string(value); }
		};

		template<typename>
		struct is_operand_literal_character : std::false_type {};

		template<char Value>
		struct is_operand_literal_character<OperandLiteralCharacter<Value>> : std::true_type {};

		template<typename T>
		constexpr auto is_operand_literal_character_v = is_operand_literal_character<T>::value;
		template<typename O>
		concept operand_literal_character_t = is_operand_literal_character_v<O>;

		template<std::integral auto Value>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralIntegral : public OperandLiteral
		{
		public:
			using value_type = std::remove_cvref_t<decltype(Value)>;

			constexpr static value_type value = Value;

			[[nodiscard]] constexpr auto operator-() const noexcept -> OperandLiteralIntegral<-static_cast<std::make_signed_t<value_type>>(value)>
			{
				return {};
			}

			[[nodiscard]] constexpr static auto to_string() noexcept -> std::string { return meta::to_string(value); }
		};

		template<typename>
		struct is_operand_literal_integral : std::false_type {};

		template<std::integral auto Value>
		struct is_operand_literal_integral<OperandLiteralIntegral<Value>> : std::true_type {};

		template<typename T>
		constexpr auto is_operand_literal_integral_v = is_operand_literal_integral<T>::value;
		template<typename O>
		concept operand_literal_integral_t = is_operand_literal_integral_v<O>;

		template<std::floating_point auto Value, std::size_t DenominatorSize>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralFloatingPoint : public OperandLiteral
		{
		public:
			using value_type = std::remove_cvref_t<decltype(Value)>;

			constexpr static value_type value = Value;
			constexpr static std::size_t denominator_size = DenominatorSize;
			constexpr static value_type epsilon = [](std::size_t n) noexcept -> value_type
			{
				auto epsilon = static_cast<value_type>(1);
				while (n--) { epsilon /= static_cast<value_type>(10); }
				return epsilon;
			}(DenominatorSize);

			[[nodiscard]] constexpr auto operator-() const noexcept -> OperandLiteralFloatingPoint<-value, DenominatorSize> { return {}; }

			[[nodiscard]] constexpr static auto to_string() noexcept -> std::string { return std::format("{:.{}g}", DenominatorSize, value); }
		};

		template<typename>
		struct is_operand_literal_floating_point : std::false_type {};

		template<std::floating_point auto Value, std::size_t DenominatorSize>
		struct is_operand_literal_floating_point<OperandLiteralFloatingPoint<Value, DenominatorSize>> : std::true_type {};

		template<typename T>
		constexpr auto is_operand_literal_floating_point_v = is_operand_literal_floating_point<T>::value;
		template<typename O>
		concept operand_literal_floating_point_t = is_operand_literal_floating_point_v<O>;

		template<char... Cs>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralAuto : public OperandLiteral
		{
		public:
			constexpr static auto char_list = functional::char_list<Cs...>;

			template<typename T>
			struct rep;

			template<char Value>
			struct rep<OperandLiteralCharacter<Value>>
			{
				using type = OperandLiteralCharacter<char_list.template nth_value<0>()>;
			};

			template<std::integral auto Value>
			struct rep<OperandLiteralIntegral<Value>>
			{
				using type = OperandLiteralIntegral<char_list.template to_integral<OperandLiteralIntegral<Value>::value_type>()>;
			};

			template<std::floating_point auto Value, std::size_t DenominatorSize>
			struct rep<OperandLiteralFloatingPoint<Value, DenominatorSize>>
			{
				using type = OperandLiteralFloatingPoint<char_list.template to_floating_point<OperandLiteralFloatingPoint<
					                                         Value, DenominatorSize>::value_type>, char_list.denominator_length()>;
			};

			template<std::integral T>
			struct rep<T>
			{
				using type = std::conditional_t<std::is_same_v<T, char>, OperandLiteralCharacter<char_list.template nth_value<0>()>,
				                                OperandLiteralIntegral<char_list.template to_integral<T>()>>;
			};

			template<std::floating_point T>
			struct rep<T>
			{
				using type = OperandLiteralFloatingPoint<char_list.template to_floating_point<T>(), char_list.denominator_length()>;
			};

			template<typename T>
			using rebind = typename rep<std::remove_cvref_t<T>>::type;
		};

		template<typename>
		struct is_operand_literal_auto : std::false_type {};

		template<char... Cs>
		struct is_operand_literal_auto<OperandLiteralAuto<Cs...>> : std::true_type {};

		template<typename T>
		constexpr auto is_operand_literal_auto_v = is_operand_literal_auto<T>::value;
		template<typename O>
		concept operand_literal_auto_t = is_operand_literal_auto_v<O>;

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandIdentity : public Operand
		{
		public:
			struct boolean
			{
				std::string_view message;
			};

			using value_type = bool;
			using message_type = boolean;

		private:
			value_type value_;
			message_type message_;

		public:
			constexpr OperandIdentity(const value_type value, const message_type message) noexcept
				: value_{value},
				  message_{message} {}

			[[nodiscard]] constexpr explicit operator bool() const noexcept { return value_; }

			[[nodiscard]] constexpr auto to_string() const noexcept -> std::string_view { return message_.message; }
		};

		enum class ExpressionCategory
		{
			EQUAL,
			APPROX,
			NOT_EQUAL,
			NOT_APPROX,
			GREATER_THAN,
			GREATER_EQUAL,
			LESS_THAN,
			LESS_EQUAL,
			LOGICAL_AND,
			LOGICAL_OR,
		};

		struct no_epsilon {};

		template<ExpressionCategory Category, typename Left, typename Right, typename Epsilon = no_epsilon>
			requires(not(is_operand_literal_auto_v<Left> and is_operand_literal_auto_v<Right>))
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandExpression : public Operand
		{
		public:
			constexpr static auto category = Category;

			// If one side of the expression is OperandLiterAuto and the other side is not OperandLiteralXxx then a compile error should be raised.
			static_assert((not is_operand_literal_auto_v<Left>) or (is_operand_literal_auto_v<Left> == is_operand_literal_v<Right>));
			static_assert((not is_operand_literal_auto_v<Right>) or (is_operand_literal_auto_v<Right> == is_operand_literal_v<Left>));

			// using left_type = std::conditional_t<is_operand_literal_auto_v<Left>, typename Left::template rebind<Right>, Left>;
			// using right_type = std::conditional_t<is_operand_literal_auto_v<Right>, typename Right::template rebind<Left>, Right>;
			using left_type = Left;
			using right_type = Right;
			using epsilon_type = Epsilon;

		private:
			left_type left_;
			right_type right_;
			epsilon_type epsilon_;
			bool result_;

			[[nodiscard]] constexpr auto do_check() const noexcept -> bool
			{
				const auto do_compare = [&e = epsilon_](const auto& left, const auto& right) noexcept -> bool
				{
					if constexpr (category == ExpressionCategory::EQUAL)
					{
						using std::operator==;
						return left == right;
					}
					else if constexpr (category == ExpressionCategory::APPROX)
					{
						using std::operator-;
						using std::operator<;
						return operands::wrap_abs(left - right) < e; // NOLINT(clang-diagnostic-implicit-int-float-conversion)
					}
					else if constexpr (category == ExpressionCategory::NOT_EQUAL)
					{
						using std::operator!=;
						return left != right;
					}
					else if constexpr (category == ExpressionCategory::NOT_APPROX)
					{
						using std::operator-;
						using std::operator<;
						return e < operands::wrap_abs(left - right);
					}
					else if constexpr (category == ExpressionCategory::GREATER_THAN)
					{
						using std::operator>;
						return left > right;
					}
					else if constexpr (category == ExpressionCategory::GREATER_EQUAL)
					{
						using std::operator>=;
						return left >= right;
					}
					else if constexpr (category == ExpressionCategory::LESS_THAN)
					{
						using std::operator<;
						return left < right;
					}
					else if constexpr (category == ExpressionCategory::LESS_EQUAL)
					{
						using std::operator<=;
						return left <= right;
					}
					else if constexpr (category == ExpressionCategory::LOGICAL_AND) { return static_cast<bool>(left) and static_cast<bool>(right); }
					else if constexpr (category == ExpressionCategory::LOGICAL_OR) { return static_cast<bool>(left) or static_cast<bool>(right); }
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				};

				if constexpr (requires { do_compare(left_type::value, right_type::value); })
				{
					return do_compare(left_type::value, right_type::value);
				}
				else if constexpr (requires { do_compare(left_type::value, right_); }) { return do_compare(left_type::value, right_); }
				else if constexpr (requires { do_compare(left_, right_type::value); }) { return do_compare(left_, right_type::value); }
				else if constexpr (requires { do_compare(left_, right_); }) { return do_compare(left_, right_); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("can not compare."); }
			}

		public:
			template<typename L, typename R, typename E>
			constexpr OperandExpression(
					L&& left,
					R&& right,
					E&& epsilon) noexcept
				: left_{std::forward<L>(left)},
				  right_{std::forward<R>(right)},
				  epsilon_{std::forward<E>(epsilon)},
				  result_{do_check()} {}

			template<typename L, typename R>
			constexpr OperandExpression(
					L&& left,
					R&& right) noexcept
				: left_{std::forward<L>(left)},
				  right_{std::forward<R>(right)},
				  epsilon_{},
				  result_{do_check()} {}

			[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

			[[nodiscard]] constexpr auto to_string() const noexcept -> std::string
			{
				if constexpr (category == ExpressionCategory::EQUAL)
				{
					return std::format("{} == {}", meta::to_string(left_), meta::to_string(right_));
				}
				else if constexpr (category == ExpressionCategory::APPROX)
				{
					return std::format("{} ≈≈ {} (+/- {})", meta::to_string(left_), meta::to_string(right_), meta::to_string(epsilon_));
				}
				else if constexpr (category == ExpressionCategory::NOT_EQUAL)
				{
					return std::format("{} != {}", meta::to_string(left_), meta::to_string(right_));
				}
				else if constexpr (category == ExpressionCategory::NOT_APPROX)
				{
					return std::format("{} !≈ {} (+/- {})", meta::to_string(left_), meta::to_string(right_), meta::to_string(epsilon_));
				}
				else if constexpr (category == ExpressionCategory::GREATER_THAN)
				{
					return std::format("{} > {}", meta::to_string(left_), meta::to_string(right_));
				}
				else if constexpr (category == ExpressionCategory::GREATER_EQUAL)
				{
					return std::format("{} >= {}", meta::to_string(left_), meta::to_string(right_));
				}
				else if constexpr (category == ExpressionCategory::LESS_THAN)
				{
					return std::format("{} < {}", meta::to_string(left_), meta::to_string(right_));
				}
				else if constexpr (category == ExpressionCategory::LESS_EQUAL)
				{
					return std::format("{} <= {}", meta::to_string(left_), meta::to_string(right_));
				}
				else if constexpr (category == ExpressionCategory::LOGICAL_AND)
				{
					return std::format("{} and {}", meta::to_string(left_), meta::to_string(right_));
				}
				else if constexpr (category == ExpressionCategory::LOGICAL_OR)
				{
					return std::format("{} or {}", meta::to_string(left_), meta::to_string(right_));
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}
		};

		template<typename>
		constexpr auto is_operand_expression_v = false;
		template<ExpressionCategory Category, typename Left, typename Right, typename Epsilon>
		constexpr auto is_operand_expression_v<OperandExpression<Category, Left, Right, Epsilon>> = true;

		template<typename Exception>
		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandThrow : public Operand
		{
		public:
			using exception_type = Exception;

		private:
			bool thrown_;
			bool caught_;

		public:
			template<std::invocable Invocable>
			constexpr explicit OperandThrow(Invocable&& invocable) noexcept
				: thrown_{false},
				  caught_{false}
			{
				if constexpr (std::is_same_v<exception_type, void>)
				{
					try { std::invoke(std::forward<Invocable>(invocable)); }
					catch (...)
					{
						thrown_ = true;
						caught_ = true;
					}
				}
				else
				{
					try { std::invoke(std::forward<Invocable>(invocable)); }
					catch ([[maybe_unused]] const exception_type& exception)
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
							: //
							(not caught())
							? "thrown but not caught"
							: //
							"caught");
			}
		};

		class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandNoThrow : public Operand
		{
		public:
			using exception_type = void;

		private:
			bool thrown_;

		public:
			template<std::invocable Invocable>
			constexpr explicit OperandNoThrow(Invocable&& invocable) noexcept
				: thrown_{false}
			{
				try { std::invoke(std::forward<Invocable>(invocable)); }
				catch (...) { thrown_ = true; }
			}

			[[nodiscard]] constexpr explicit operator bool() const noexcept { return not thrown_; }

			[[nodiscard]] /*constexpr*/ auto to_string() const noexcept -> std::string { return std::format("nothrow - {:s}", not thrown_); }
		};
	} // namespace operands

	namespace executor
	{
		class Executor
		{
			struct internal_tag {};

		public:
			// not nullable
			using suite_results_iterator_type = suite_results_type::iterator;
			// nullable
			using test_results_iterator_type = test_results_type::pointer;

		private:
			std::shared_ptr<config> config_;

			suite_results_type suite_results_;

			suite_results_iterator_type current_suite_result_;
			test_results_iterator_type current_test_result_;

			std::size_t total_fails_exclude_current_test_;

			enum class IdentType
			{
				TEST,
				ASSERTION,
			};

			template<IdentType Type>
			[[nodiscard]] auto nested_level_of_current_test() const noexcept -> std::size_t
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(not current_suite_result_->test_results.empty());

				if constexpr (Type == IdentType::ASSERTION) { GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_); }
				else
				{
					// top level
					if (current_test_result_ == nullptr) { return 1; }
				}

				std::size_t result = 0;
				for (const auto* p = current_test_result_; p != nullptr; p = p->parent) { result += 1; }

				return result + (Type == IdentType::ASSERTION);
			}

			template<IdentType Type>
			[[nodiscard]] auto ident_size_of_current_test() const noexcept -> std::size_t { return nested_level_of_current_test<Type>() * 4; }

			// [suite_name] test1.test2.test3
			[[nodiscard]] auto fullname_of_current_test() const noexcept -> std::string
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(not current_suite_result_->test_results.empty());

				auto result = std::format("[{}] ", current_suite_result_->name);

				const auto* p = std::addressof(current_suite_result_->test_results.back());
				while (p != nullptr)
				{
					result.append_range(p->name);
					result.push_back('.');

					p = p->children.empty() ? nullptr : std::addressof(p->children.back());
				}

				result.pop_back();
				return result;
			}

			[[nodiscard]] auto ms_duration_of_current_test() const noexcept -> time_point_type::rep
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);
				return std::chrono::duration_cast<time_difference_type>(current_test_result_->time_end - current_test_result_->time_start).count();
			}

			auto check_fails_may_terminate() noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);

				if (total_fails_exclude_current_test_ + current_test_result_->total_assertions_failed > config_->abort_after_n_failures)
				[[unlikely]]
				{
					on(events::EventTestEnd{.name = current_test_result_->name});
					on(events::EventSuiteEnd{.name = current_suite_result_->name});
					on(events::EventSummary{});

					std::println(
							std::cerr,
							"{}fast fail for test {} after {} failures total.{}",
							config_->color.fail,
							fullname_of_current_test(),
							current_test_result_->total_assertions_failed,
							config_->color.none);
					config_->terminate();
				}
			}

			// =========================================
			// SUITE
			// =========================================

			auto on(const events::EventSuiteBegin& suite_begin) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);

				auto& [name, report_string, test_results] = suite_results_.emplace_back(std::string{suite_begin.name});
				current_suite_result_ = std::ranges::prev(suite_results_.end(), 1);

				std::format_to(
						std::back_inserter(report_string),
						"Executing suite {}{}{} vvv\n",
						config_->color.suite,
						name,
						config_->color.none);
			}

			auto on(const events::EventSuiteEnd& suite_end) -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());

				// todo: If the user catches the exception, how do we make sure the executor is still available? How to handle the current suite?
				if (current_suite_result_->name != suite_end.name)
				[[unlikely]]
				{
					throw std::logic_error{
							std::format(
									"can not pop suite because `{}` differs from `{}`",
									current_suite_result_->name,
									suite_end.name)};
				}

				auto& [name, report_string, test_results] = *current_suite_result_;

				std::format_to(
						std::back_inserter(report_string),
						"^^^ End of suite {}{}{} execution\n",
						config_->color.suite,
						name,
						config_->color.none);

				// reset to anonymous suite
				current_suite_result_ = suite_results_.begin();
			}

			// =========================================
			// TEST
			// =========================================

			auto on(const events::EventTestBegin& test_begin) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());

				// we chose to construct a temporary object here to avoid possible errors, and trust that the optimizer will forgive us ;)
				auto t = test_result_type{
						.name = std::string{test_begin.name},
						.parent = current_test_result_,
						.children = {},
						.status = test_result_type::Status::PENDING,
						.time_start = clock_type::now(),
						.time_end = {},
						.total_assertions_passed = 0,
						.total_assertions_failed = 0};

				if (current_test_result_)
				{
					// nested
					auto& this_test = current_test_result_->children.emplace_back(std::move(t));
					current_test_result_ = std::addressof(this_test);

					if (std::to_underlying(config_->output_level) > std::to_underlying(OutputLevel::NONE))
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}}Running nested test {}{}{}...\n",
								" ",
								ident_size_of_current_test<IdentType::TEST>(),
								config_->color.test,
								fullname_of_current_test(),
								config_->color.none);
					}
				}
				else
				{
					// top level
					auto& this_test = current_suite_result_->test_results.emplace_back(std::move(t));
					current_test_result_ = std::addressof(this_test);

					if (std::to_underlying(config_->output_level) > std::to_underlying(OutputLevel::NONE))
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}}Running test {}{}{}...\n",
								" ",
								ident_size_of_current_test<IdentType::TEST>(),
								config_->color.test,
								fullname_of_current_test(),
								config_->color.none);
					}
				}
			}

			auto on(const events::EventTestSkip& test_skip) -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);

				on(events::EventTestBegin{.name = test_skip.name});
				current_test_result_->status = test_result_type::Status::SKIPPED;
				on(events::EventTestEnd{.name = test_skip.name});
			}

			auto on(const events::EventTestEnd& test_end) -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);

				// todo: If the user catches the exception, how do we make sure the executor is still available? How to handle the current test and suite?
				if (current_test_result_->name != test_end.name)
				[[unlikely]]
				{
					throw std::logic_error{
							std::format(
									"can not pop test because `{}` differs from `{}`",
									current_test_result_->name,
									test_end.name)
					};
				}

				current_test_result_->time_end = clock_type::now();
				// if (current_test_result_->status == test_result_type::Status::FATAL)
				if (current_test_result_->status == test_result_type::Status::PENDING)
				[[likely]]
				{
					// the current test is considered SKIPPED only if it does not have any assertions and has no children.
					if (current_test_result_->total_assertions_failed == 0 and current_test_result_->total_assertions_passed == 0)
					{
						if (current_test_result_->children.empty()) { current_test_result_->status = test_result_type::Status::SKIPPED; }
						else
						{
							current_test_result_->status =
									std::ranges::all_of(
											current_test_result_->children,
											[](const auto& child_test) noexcept { return child_test.total_assertions_failed == 0; }
											)
										? test_result_type::Status::PASSED
										: test_result_type::Status::FAILED;
						}
					}
					else
					{
						current_test_result_->status = current_test_result_->total_assertions_failed == 0
							                               ? test_result_type::Status::PASSED
							                               : test_result_type::Status::FAILED;
					}
				}

				total_fails_exclude_current_test_ += current_test_result_->total_assertions_failed;

				if (std::to_underlying(config_->output_level) > std::to_underlying(OutputLevel::NONE))
				{
					if (const auto status = current_test_result_->status;
						status == test_result_type::Status::PASSED or status == test_result_type::Status::FAILED)
					[[likely]]
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}}{}{}{} after {} milliseconds.\n",
								"",
								ident_size_of_current_test<IdentType::TEST>(),
								status == test_result_type::Status::PASSED ? config_->color.pass : config_->color.fail,
								status == test_result_type::Status::PASSED ? "PASSED" : "FAILED",
								config_->color.none,
								ms_duration_of_current_test()
								);
					}
					else if (status == test_result_type::Status::SKIPPED)
					[[unlikely]]
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}}{}SKIPPED{}\n",
								"",
								ident_size_of_current_test<IdentType::TEST>(),
								config_->color.skip,
								config_->color.none
								);
					}
					else if (status == test_result_type::Status::FATAL)
					[[unlikely]]
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}}{}INTERRUPTED{}\n",
								"",
								ident_size_of_current_test<IdentType::TEST>(),
								config_->color.skip,
								config_->color.none
								);
					}
					else { std::unreachable(); }
				}

				current_test_result_ = current_test_result_->parent;
			}

			// =========================================
			// ASSERTION
			// =========================================

			template<expression_t Expression>
			auto on(const events::EventAssertionPass<Expression>& assertion_pass) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);

				if (const auto level = std::to_underlying(config_->output_level);
					level >= std::to_underlying(OutputLevel::INCLUDE_EXPRESSION))
				{
					// @see: Operand::prefer_no_type_name
					constexpr auto prefer_no_type_name = requires { typename Expression::prefer_no_type_name; };

					if (level >= std::to_underlying(OutputLevel::INCLUDE_EXPRESSION_LOCATION))
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}}[{}:{}] {}[{}]{} - {}PASSED{} \n",
								" ",
								ident_size_of_current_test<IdentType::ASSERTION>(),
								assertion_pass.location.file_name(),
								assertion_pass.location.line(),
								config_->color.expression,
								meta::to_string<std::string, not prefer_no_type_name>(assertion_pass.expression),
								config_->color.none,
								config_->color.pass,
								config_->color.none);
					}
					else
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}} {}[{}]{} - {}PASSED{} \n",
								" ",
								ident_size_of_current_test<IdentType::ASSERTION>(),
								config_->color.expression,
								meta::to_string<std::string, not prefer_no_type_name>(assertion_pass.expression),
								config_->color.none,
								config_->color.pass,
								config_->color.none);
					}
				}

				current_test_result_->total_assertions_passed += 1;
			}

			template<expression_t Expression>
			auto on(const events::EventAssertionFail<Expression>& assertion_fail) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);

				if (const auto level = std::to_underlying(config_->output_level);
					level >= std::to_underlying(OutputLevel::INCLUDE_EXPRESSION))
				{
					// @see: Operand::prefer_no_type_name
					constexpr auto prefer_no_type_name = requires { typename Expression::prefer_no_type_name; };

					if (level >= std::to_underlying(OutputLevel::INCLUDE_EXPRESSION_LOCATION))
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}}[{}:{}] {}[{}]{} - {}FAILED{} \n",
								" ",
								ident_size_of_current_test<IdentType::ASSERTION>(),
								assertion_fail.location.file_name(),
								assertion_fail.location.line(),
								config_->color.expression,
								meta::to_string<std::string, not prefer_no_type_name>(assertion_fail.expression),
								config_->color.none,
								config_->color.fail,
								config_->color.none);
					}
					else
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}} {}[{}]{} - {}FAILED{} \n",
								" ",
								ident_size_of_current_test<IdentType::ASSERTION>(),
								config_->color.expression,
								meta::to_string<std::string, not prefer_no_type_name>(assertion_fail.expression),
								config_->color.none,
								config_->color.pass,
								config_->color.none);
					}
				}

				current_test_result_->total_assertions_failed += 1;

				check_fails_may_terminate();
			}

			auto on(const events::EventAssertionFatal& assertion_fatal, internal_tag) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);

				if (const auto level = std::to_underlying(config_->output_level);
					level >= std::to_underlying(OutputLevel::INCLUDE_EXPRESSION))
				{
					std::format_to(
							std::back_inserter(current_suite_result_->report_string),
							"{:{}}^^^ {}FATAL ERROR{}\n",
							" ",
							ident_size_of_current_test<IdentType::ASSERTION>() +
							(
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
							config_->color.fatal,
							config_->color.none);
				}

				current_test_result_->total_assertions_failed += 1;
				current_test_result_->status = test_result_type::Status::FATAL;

				check_fails_may_terminate();
			}

			template<expression_t Expression>
			auto on(const events::EventAssertionFatalSkip<Expression>& assertion_fatal_skip) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);

				if (const auto level = std::to_underlying(config_->output_level);
					level >= std::to_underlying(OutputLevel::INCLUDE_EXPRESSION))
				{
					// @see: Operand::prefer_no_type_name
					constexpr auto prefer_no_type_name = requires { typename Expression::prefer_no_type_name; };

					if (level >= std::to_underlying(OutputLevel::INCLUDE_EXPRESSION_LOCATION))
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}}[{}:{}] {}[{}]{} - {}SKIPPED{} \n",
								" ",
								ident_size_of_current_test<IdentType::ASSERTION>(),
								assertion_fatal_skip.location.file_name(),
								assertion_fatal_skip.location.line(),
								config_->color.expression,
								meta::to_string<std::string, not prefer_no_type_name>(assertion_fatal_skip.expression),
								config_->color.none,
								config_->color.fatal,
								config_->color.none);
					}
					else
					{
						std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:{}} {}[{}]{} - {}SKIPPED{} \n",
								" ",
								ident_size_of_current_test<IdentType::ASSERTION>(),
								config_->color.expression,
								meta::to_string<std::string, not prefer_no_type_name>(assertion_fatal_skip.expression),
								config_->color.none,
								config_->color.fatal,
								config_->color.none);
					}
				}

				current_test_result_->total_assertions_failed += 1;

				check_fails_may_terminate();
			}

			// =========================================
			// EXCEPTION
			// =========================================

			auto on(const events::EventException& exception) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);

				auto& [suite_name, report_string, _] = *current_suite_result_;
				const auto& test_name = current_test_result_->name;

				on(events::EventTestEnd{.name = test_name});
				on(events::EventSuiteEnd{.name = suite_name});

				std::format_to(
						std::back_inserter(report_string),
						"{}Abort test because unexpected exception with message: {}.{}\n",
						config_->color.fail,
						exception.what(),
						config_->color.none);

				std::ranges::for_each(
						suite_results_,
						[this](const auto& suite_result) noexcept { config_->report_message(suite_result.report_string); });

				config_->report_message(
						std::format(
								"--- early abort for test {}{}{} after {} failures total.",
								config_->color.test,
								test_name,
								config_->color.none,
								total_fails_exclude_current_test_));

				config_->terminate();
			}

			// =========================================
			// LOG
			// =========================================

			template<typename MessageType>
			auto on(const events::EventLog<MessageType>& log, internal_tag) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());

				if (log.message != "\n")
				[[likely]]
				{
					// pop '\n'
					current_suite_result_->report_string.pop_back();
				}

				current_suite_result_->report_string.append(config_->color.message);
				current_suite_result_->report_string.append_range(log.message);
				current_suite_result_->report_string.append(config_->color.none);

				// push '\n'
				current_suite_result_->report_string.push_back('\n');
			}

			// =========================================
			// SUMMARY
			// =========================================

			auto on(const events::EventSummary&) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());

				if (const auto level = std::to_underlying(config_->output_level);
					level >= std::to_underlying(OutputLevel::NONE))
				{
					struct total_result
					{
						std::size_t test_passed;
						std::size_t test_failed;
						std::size_t test_skipped;

						std::size_t assertion_passed;
						std::size_t assertion_failed;

						constexpr auto operator+(const total_result& other) const noexcept -> total_result
						{
							total_result new_one{*this};

							new_one.test_passed += other.test_passed;
							new_one.test_failed += other.test_failed;
							new_one.test_skipped += other.test_skipped;

							new_one.assertion_passed += other.assertion_passed;
							new_one.assertion_failed += other.assertion_failed;

							return new_one;
						}
					};

					constexpr auto calc_result_of_test = functional::y_combinator{
							[](auto self, const test_result_type& test_result) noexcept -> total_result
							{
								return std::ranges::fold_left(
										test_result.children,
										total_result{
												.test_passed = (test_result.status == test_result_type::Status::PASSED) ? 1ull : 0,
												.test_failed = (
													               test_result.status == test_result_type::Status::FAILED or
													               test_result.status == test_result_type::Status::FATAL
												               )
													               ? 1ull
													               : 0,
												.test_skipped = (test_result.status == test_result_type::Status::SKIPPED) ? 1ull : 0,
												.assertion_passed = test_result.total_assertions_passed,
												.assertion_failed = test_result.total_assertions_failed},
										[self](const total_result& total, const test_result_type& nested_test_result) noexcept -> total_result
										{
											return total + self(nested_test_result);
										});
							}};

					constexpr auto calc_result_of_suite = [calc_result_of_test](const suite_result_type& suite_result) noexcept -> total_result
					{
						return std::ranges::fold_left(
								suite_result.test_results,
								total_result{
										.test_passed = 0,
										.test_failed = 0,
										.test_skipped = 0,
										.assertion_passed = 0,
										.assertion_failed = 0},
								[calc_result_of_test](const total_result& total, const test_result_type& test_result) noexcept -> total_result
								{
									return total + calc_result_of_test(test_result);
								});
					};

					std::ranges::for_each(
							suite_results_,
							[&color = config_->color, c = config_, calc_result_of_suite](suite_result_type& suite_result) noexcept -> void
							{
								// ReSharper disable once CppUseStructuredBinding
								if (const auto result = calc_result_of_suite(suite_result);
									result.assertion_failed == 0)
								[[likely]]
								{
									std::format_to(
											std::back_inserter(suite_result.report_string),
											"\n==========================================\n"
											"Suite {}{}{} -> all tests passed({} assertions in {} tests), {} tests skipped."
											"\n==========================================\n",
											color.suite,
											suite_result.name,
											color.none,
											result.assertion_passed,
											result.test_passed,
											result.test_skipped);
								}
								else
								[[unlikely]]
								{
									std::format_to(
											std::back_inserter(suite_result.report_string),
											"\n==========================================\n"
											"Suite {}{}{}\n"
											"tests {} | {} {}passed({:.6g}%){} | {} {}failed({:.6g}%){} | {} {}skipped({:.6g}%){}\n"
											"assertions {} | {} {}passed({:.6g}%){} | {} {}failed({:.6g}%){}"
											"\n==========================================\n",
											color.suite,
											suite_result.name,
											color.none,
											// test
											result.test_passed + result.test_failed + result.test_skipped,
											// passed
											result.test_passed,
											color.pass,
											static_cast<double>(result.test_passed) /
											static_cast<double>(result.test_passed + result.test_failed + result.test_skipped)
											* 100.0,
											color.none,
											// failed
											result.test_failed,
											color.fail,
											static_cast<double>(result.test_failed) /
											static_cast<double>(result.test_passed + result.test_failed + result.test_skipped)
											* 100.0,
											color.none,
											// skipped
											result.test_skipped,
											color.skip,
											static_cast<double>(result.test_skipped) /
											static_cast<double>(result.test_passed + result.test_failed + result.test_skipped)
											* 100.0,
											color.none,
											// assertion
											result.assertion_passed + result.assertion_failed,
											// passed
											result.assertion_passed,
											color.pass,
											static_cast<double>(result.assertion_passed) /
											static_cast<double>(result.assertion_passed + result.assertion_failed)
											* 100.0,
											color.none,
											// failed
											result.assertion_failed,
											color.fail,
											static_cast<double>(result.assertion_failed) /
											static_cast<double>(result.assertion_passed + result.assertion_failed)
											* 100.0,
											color.none);
								}

								c->report_message(suite_result.report_string);
							});
				}
			}

		public:
			Executor(const Executor&) noexcept = delete;
			Executor(Executor&&) noexcept = delete;
			auto operator=(const Executor&) noexcept -> Executor& = delete;
			auto operator=(Executor&&) noexcept -> Executor& = delete;

			explicit Executor()
				: config_{std::make_shared<struct config>()},
				  current_suite_result_{suite_results_.end()},
				  current_test_result_{nullptr},
				  total_fails_exclude_current_test_{0}
			{
				// we chose to construct a temporary object here to avoid possible errors, and trust that the optimizer will forgive us ;)
				auto t = suite_result_type{
						.name = std::string{anonymous_suite_name},
						.report_string = {},
						.test_results = {}};

				suite_results_.emplace_back(std::move(t));
				current_suite_result_ = suite_results_.begin();
			}

			~Executor() noexcept
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);

				if (not config_->dry_run) { on(events::EventSummary{}); }
			}

			[[nodiscard]] auto config() const noexcept -> config&
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);

				return *config_;
			}

			// =========================================
			// SUITE
			// =========================================

			auto on(const events::EventSuite& suite) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);

				if (config_->is_suite_execute_required(suite.name))
				{
					on(suite.begin());

					// throwable
					std::invoke(suite);

					on(suite.end());
				}
			}

			// =========================================
			// TEST
			// =========================================

			template<typename InvocableType, typename Arg>
			auto on(const events::EventTest<InvocableType, Arg>& test) noexcept -> void
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);

				if (config_->is_test_execute_required(test.name, test.categories))
				{
					this->on(test.begin());

					try //
					{
						std::invoke(test);
					}
					catch (const std::exception& exception) { on(events::EventException{.message = exception.what()}); }
					catch (...) { on(events::EventException{.message = "unhandled exception, not derived from std::exception"}); }

					this->on(test.end());
				}
				else { this->on(test.skip()); }
			}

			// =========================================
			// ASSERTION
			// =========================================

			template<expression_t Expression>
			auto on(const events::EventAssertion<Expression>& assertion) noexcept -> bool
			{
				GAL_PROMETHEUS_DEBUG_NOT_NULL(config_);
				GAL_PROMETHEUS_DEBUG_NOT_NULL(current_test_result_);

				if (config_->dry_run)
				[[unlikely]]
				{
					return true;
				}

				if (current_test_result_->status == test_result_type::Status::FATAL)
				[[unlikely]]
				{
					this->on(assertion.fatal_skip());
					// Consider the test case execution successful and avoid undesired log output.
					return true;
				}

				if (static_cast<bool>(assertion.expression))
				[[likely]]
				{
					this->on(assertion.pass());
					return true;
				}

				this->on(assertion.fail());
				return false;
			}

			auto on(const events::EventAssertionFatal& assertion_fatal) noexcept -> void
			{
				on(assertion_fatal, internal_tag{});
				//
			}

			// =========================================
			// LOG
			// =========================================

			template<typename MessageType>
			auto on(const events::EventLog<MessageType>& log) noexcept -> void
			{
				this->on(log, internal_tag{});
				//
			}
		};

		[[nodiscard]] inline auto executor() noexcept -> Executor&
		{
			static Executor executor{};
			return executor;
		}
	} // namespace executor

	namespace dispatcher
	{
		namespace detail
		{
			template<typename Lhs, typename Dispatcher>
			struct dispatched_expression
			{
				using expression_type = Lhs;
				using dispatcher_type = Dispatcher;

				expression_type expression;

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
			struct is_type_or_dispatched_type : std::bool_constant<std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<RequiredType>>> {};

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
				typename R>
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

			// OperandIdentity

			// bool == operands::OperandIdentity::message_type{...}
			template<
				type_or_dispatched_type_t<bool> L,
				type_or_dispatched_type_t<operands::OperandIdentity::message_type> R>
			// can we trust users not to (inadvertently) mess up the ADL? vvv
				requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
			[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
			{
				using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

				return dispatched_expression<
					operands::OperandIdentity,
					dispatcher_type
				>{
						.expression = {
								type_or_dispatched_type::get(lhs),
								type_or_dispatched_type::get(rhs)
						}
				};
			}

			// operands::OperandIdentity::message_type{...} == bool
			template<
				type_or_dispatched_type_t<operands::OperandIdentity::message_type> L,
				type_or_dispatched_type_t<bool> R>
			// can we trust users not to (inadvertently) mess up the ADL? vvv
				requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
			[[nodiscard]] constexpr auto operator==(const L& lhs, const R& rhs) noexcept -> auto //
			{
				// forward
				return rhs == lhs;
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_against_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				typename R>
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

			// OperandIdentity

			// bool != operands::OperandIdentity::message_type{...}
			template<
				type_or_dispatched_type_t<bool> L,
				type_or_dispatched_type_t<operands::OperandIdentity::message_type> R>
			// can we trust users not to (inadvertently) mess up the ADL? vvv
				requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
			[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
			{
				using dispatcher_type = lazy_dispatcher_type_t<is_dispatched_expression_v<L>, L, R>;

				return dispatched_expression<operands::OperandIdentity, dispatcher_type>{
						.expression = {not type_or_dispatched_type::get(lhs), type_or_dispatched_type::get(rhs)}
				};
			}

			// operands::OperandIdentity::message_type{...} != bool
			template<
				type_or_dispatched_type_t<operands::OperandIdentity::message_type> L,
				type_or_dispatched_type_t<bool> R>
			// can we trust users not to (inadvertently) mess up the ADL? vvv
				requires(is_dispatched_expression_v<L> or is_dispatched_expression_v<R>)
			[[nodiscard]] constexpr auto operator!=(const L& lhs, const R& rhs) noexcept -> auto //
			{
				// forward
				return rhs != lhs;
			}

			// ============================================
			// operator>
			// ============================================

			// OperandValue

			// floating_point > value{...}
			template<
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> L,
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
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
				constrain_against_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				typename R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
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
				constrain_against_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				typename R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
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
				constrain_against_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				typename R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_value> R>
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
				constrain_against_type_or_dispatched_type_t<std::is_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_character> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_integral> R>
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
				constrain_satisfied_type_or_dispatched_type_t<operands::is_operand_literal_floating_point> R>
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
				constrain_satisfied_type_or_dispatched_type_t<std::is_floating_point> R>
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
				typename R>
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
				constrain_satisfied_type_or_dispatched_type_t<is_expression> R>
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
				constrain_satisfied_type_or_dispatched_type_t<is_expression> R>
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
		} // namespace detail

		template<events::event_t EventType>
		auto register_event(EventType&& event) noexcept -> decltype(auto) //
		{
			return executor::executor().on(std::forward<EventType>(event));
		}

		struct expect_result
		{
			struct fatal {};

		private:
			template<typename T>
			struct fatal_location
			{
				std::source_location location;

				constexpr explicit(false) fatal_location(const T&, const std::source_location& l = std::source_location::current()) noexcept
					: location{l} {}
			};

		public:
			bool value;

			constexpr explicit expect_result(const bool v) noexcept
				: value{v} {}

			template<typename MessageType>
			constexpr auto operator<<(MessageType&& message) noexcept -> expect_result& //
				requires requires { events::EventLog{.message = std::forward<MessageType>(message)}; }
			{
				if (not value) //
				{
					register_event(events::EventLog{.message = std::forward<MessageType>(message)});
				}

				return *this;
			}

			constexpr auto operator<<(const fatal_location<fatal>& location) noexcept -> expect_result&
			{
				if (not value) //
				{
					register_event(events::EventAssertionFatal{.location = location.location});
				}

				return *this;
			}
		};

		template<typename Dispatcher>
		class ExpressionDispatcher
		{
		public:
			template<typename Lhs>
			[[nodiscard]] constexpr auto operator%(Lhs&& lhs) const noexcept -> detail::dispatched_expression<Lhs, Dispatcher> //
			{
				return {.expression = std::forward<Lhs>(lhs)};
			}
		};

		class DispatcherThat final : public ExpressionDispatcher<DispatcherThat>
		{
		public:
			using ExpressionDispatcher::operator%;
		};

		// fixme: more dispatched expression vvv
		// ...

		class DispatcherExpect
		{
		public:
			template<typename Expression>
				requires(is_expression_v<Expression> or detail::is_dispatched_expression_v<Expression>)
			constexpr auto operator()(
					Expression&& expression,
					const std::source_location& location = std::source_location::current()
					) const noexcept -> expect_result
			{
				if constexpr (detail::is_dispatched_expression_v<Expression>)
				{
					// workaround if needed
					// using dispatcher_type = typename Expression::dispatcher_type;

					const auto result = register_event(
							events::EventAssertion<typename Expression::expression_type>{
									.expression = std::forward<Expression>(expression).expression,
									.location = location});

					return expect_result{result};
				}
				else //
				{
					return expect_result{
							register_event(
									events::EventAssertion<Expression>{.expression = std::forward<Expression>(expression), .location = location})};
				}
			}
		};

		template<typename D>
		class DispatcherTestBase
		{
		public:
			using categories_type = config::categories_type;

		protected:
			categories_type categories_;

		public:
			template<std::invocable InvocableType>
			constexpr auto operator=(InvocableType&& invocable) & noexcept -> DispatcherTestBase&
			{
				register_event(events::EventTest<InvocableType>{
						.name = static_cast<D&>(*this).name(),
						.categories = categories_,
						.invocable = std::forward<InvocableType>(invocable),
						.arg = {}});

				return *this;
			}

			template<std::invocable InvocableType>
			constexpr auto operator=(InvocableType&& invocable) && noexcept -> DispatcherTestBase&
			{
				register_event(events::EventTest<InvocableType>{
						.name = static_cast<D&>(*this).name(),
						.categories = std::move(categories_),
						.invocable = std::forward<InvocableType>(invocable),
						.arg = {}});

				return *this;
			}

		private:
			[[nodiscard]] constexpr auto do_move() && noexcept -> D&& { return std::move(*static_cast<D*>(this)); }

		public:
			#if __cpp_multidimensional_subscript < 202110L
			// fixme
			#endif
			template<typename... Args>
			[[nodiscard]] constexpr auto operator[](Args&&... args) && noexcept -> D&&
			{
				const auto process = [this]<typename T>(T&& t) noexcept -> void
				{
					using type = std::decay_t<T>;

					if constexpr (requires { type::value; }) { categories_.emplace_back(type::value); }
					else if constexpr (std::is_same_v<type, categories_type::value_type>) { categories_.emplace_back(std::forward<T>(t)); }
					else if constexpr (std::is_same_v<type, categories_type>) { categories_.append_range(std::forward<T>(t)); }
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("unknown type"); }
				};

				(process(std::forward<Args>(args)), ...);

				return std::move(*this).do_move();
			}
		};

		template<meta::basic_fixed_string StringLiteral>
		class DispatcherTestLiteral : public DispatcherTestBase<DispatcherTestLiteral<StringLiteral>>
		{
		public:
			using name_type = events::name_type;

			using DispatcherTestBase<DispatcherTestLiteral>::operator=;

			[[nodiscard]] constexpr static auto name() noexcept -> auto { return StringLiteral.operator name_type(); }
		};

		class DispatcherTest : public DispatcherTestBase<DispatcherTest>
		{
		public:
			using name_type = events::name_type;

			using DispatcherTestBase::operator=;

		private:
			name_type name_;

		public:
			constexpr explicit DispatcherTest(const name_type name) noexcept
				: name_{name} {}

			[[nodiscard]] constexpr auto name() const noexcept -> auto { return name_; }
		};
	} // namespace dispatcher

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	// =========================================
	// OPERANDS
	// =========================================

	template<typename T>
	using value = operands::OperandValue<T>;

	template<typename ExceptionType = void, std::invocable InvocableType>
	[[nodiscard]] constexpr auto throws(const InvocableType& invocable) noexcept -> operands::OperandThrow<ExceptionType> { return {invocable}; }

	template<std::invocable InvocableType>
	[[nodiscard]] constexpr auto nothrow(const InvocableType& invocable) noexcept -> operands::OperandNoThrow { return {invocable}; }

	// =========================================
	// DISPATCHER
	// =========================================

	constexpr dispatcher::expect_result::fatal fatal{};

	constexpr dispatcher::DispatcherThat that{};
	constexpr dispatcher::DispatcherExpect expect{};

	// =========================================
	// CONFIG
	// =========================================

	[[nodiscard]] inline auto config() noexcept -> auto& { return executor::executor().config(); }

	// =========================================
	// TEST & SUITE
	// =========================================

	using test = dispatcher::DispatcherTest;

	template<meta::basic_fixed_string SuiteName>
	struct suite
	{
		template<std::invocable InvocableType>
		constexpr explicit(false) suite(InvocableType invocable) noexcept //
			requires requires { +invocable; }
		{
			dispatcher::register_event(events::EventSuite{
					.name = SuiteName.operator std::string_view(),
					.suite = +invocable});
		}
	};

	// =========================================
	// OPERATORS
	// =========================================

	namespace operators
	{
		namespace detail
		{
			template<typename DispatchedExpression>
			// ReSharper disable once CppFunctionIsNotImplemented
			constexpr auto is_valid_dispatched_expression(DispatchedExpression&& expression) noexcept -> void requires requires
			{
				static_cast<bool>(expression.expression);
			};

			template<typename Lhs, typename Rhs>
			concept dispatchable_t = not(dispatcher::detail::is_dispatched_expression_v<Lhs> or dispatcher::detail::is_dispatched_expression_v<Rhs>);
		} // namespace detail

		// a == b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator==(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and requires
			{
				detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) == std::forward<Rhs>(rhs));
			} //
		{
			return that % std::forward<Lhs>(lhs) == std::forward<Rhs>(rhs);
		}

		// a != b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator!=(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and requires
			{
				detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) != std::forward<Rhs>(rhs));
			} //
		{
			return that % std::forward<Lhs>(lhs) != std::forward<Rhs>(rhs);
		}

		// a > b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator>(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and requires
			{
				detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) > std::forward<Rhs>(rhs));
			} //
		{
			return that % std::forward<Lhs>(lhs) > std::forward<Rhs>(rhs);
		}

		// a >= b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator>=(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and requires
			{
				detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) >= std::forward<Rhs>(rhs));
			} //
		{
			return that % std::forward<Lhs>(lhs) >= std::forward<Rhs>(rhs);
		}

		// a < b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator<(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and requires
			{
				detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) < std::forward<Rhs>(rhs));
			} //
		{
			return that % std::forward<Lhs>(lhs) < std::forward<Rhs>(rhs);
		}

		// a <= b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator<=(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and requires
			{
				detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) <= std::forward<Rhs>(rhs));
			} //
		{
			return that % std::forward<Lhs>(lhs) <= std::forward<Rhs>(rhs);
		}

		// todo: It doesn't look like we can take over [operator and] and [operator or] :(

		// a and b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator and(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and requires
			{
				detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) and std::forward<Rhs>(rhs));
			} //
		{
			return that % std::forward<Lhs>(lhs) and std::forward<Rhs>(rhs);
		}

		// a or b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator or(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and requires
			{
				detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) or std::forward<Rhs>(rhs));
			} //
		{
			return that % std::forward<Lhs>(lhs) or std::forward<Rhs>(rhs);
		}
	} // namespace operators

	// =========================================
	// LITERALS
	// =========================================

	namespace literals
	{
		template<meta::basic_fixed_string StringLiteral>
		constexpr auto operator""_test() noexcept -> dispatcher::DispatcherTestLiteral<StringLiteral> //
		{
			return dispatcher::DispatcherTestLiteral<StringLiteral>{};
		}

		template<char... Cs>
		[[nodiscard]] constexpr auto operator""_auto() noexcept -> operands::OperandLiteralAuto<Cs...> //
		{
			return {};
		}

		template<meta::basic_fixed_string StringLiteral>
		[[nodiscard]] constexpr auto operator""_c() noexcept -> operands::OperandLiteralCharacter<StringLiteral.value[0]> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<int>(); }
		[[nodiscard]] constexpr auto operator
		""_i() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<int>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned>(); }
		[[nodiscard]] constexpr auto operator
		""_u() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<unsigned>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<long>(); }
		[[nodiscard]] constexpr auto operator
		""_l() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned long>(); }
		[[nodiscard]] constexpr auto operator""_ul() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			unsigned long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<long long>(); }
		[[nodiscard]] constexpr auto operator
		""_ll() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<long long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned long long>(); }
		[[nodiscard]] constexpr auto operator""_ull() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			unsigned long long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int8_t>(); }
		[[nodiscard]] constexpr auto operator""_i8() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			std::int8_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint8_t>(); }
		[[nodiscard]] constexpr auto operator""_u8() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			std::uint8_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int16_t>(); }
		[[nodiscard]] constexpr auto operator""_i16() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			std::int16_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint16_t>(); }
		[[nodiscard]] constexpr auto operator""_u16() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			std::uint16_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int32_t>(); }
		[[nodiscard]] constexpr auto operator""_i32() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			std::int32_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint32_t>(); }
		[[nodiscard]] constexpr auto operator""_u32() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			std::uint32_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int64_t>(); }
		[[nodiscard]] constexpr auto operator""_i64() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			std::int64_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint64_t>(); }
		[[nodiscard]] constexpr auto operator""_u64() noexcept -> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<
			std::uint64_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<float>(); }
		[[nodiscard]] constexpr auto operator""_f() noexcept -> operands::OperandLiteralFloatingPoint<
			functional::char_list<Cs...>.template to_floating_point<float>(),
			functional::char_list<Cs...>.denominator_length()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<double>(); }
		[[nodiscard]] constexpr auto operator""_d() noexcept -> operands::OperandLiteralFloatingPoint<
			functional::char_list<Cs...>.template to_floating_point<double>(),
			functional::char_list<Cs...>.denominator_length()> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<long double>(); }
		[[nodiscard]] constexpr auto operator""_ld() noexcept -> operands::OperandLiteralFloatingPoint<
			functional::char_list<Cs...>.template to_floating_point<long double>(),
			functional::char_list<Cs...>.denominator_length()> { return {}; }

		[[nodiscard]] constexpr auto operator""_b(const char* name, const std::size_t size) noexcept -> operands::OperandIdentity::message_type //
		{
			return {operands::OperandIdentity::boolean{.message = {name, size}}};
		}

		[[nodiscard]] constexpr auto operator""_s(const char* name, std::size_t size) noexcept -> std::string_view { return {name, size}; }
	} // namespace literals

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
} // namespace gal::prometheus::unit_test
