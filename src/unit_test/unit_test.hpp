// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// unit_test module, inspired by boost-ext/ut(https://github.com/boost-ext/ut)(http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <chrono>
#include <utility>
#include <vector>
#include <functional>
#include <iostream>
#include <ranges>
#include <algorithm>
#include <source_location>
#include <print>

#include <prometheus/macro.hpp>

#include <meta/to_string.hpp>
#include <math/cmath.hpp>
#include <functional/enumeration.hpp>
#include <functional/value_list.hpp>
#include <functional/functor.hpp>
#include <platform/os.hpp>

namespace gal::prometheus::unit_test
{
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
		enum class Status : std::uint8_t
		{
			// The current test has not been executed yet (internal use only)
			PENDING,

			// All assertions passed
			PASSED,
			// At least one assertion failed
			FAILED,
			// No assertions found in test or 
			SKIPPED_NO_ASSERTION,
			// The test was filtered
			SKIPPED_FILTERED,
			// The test execution was interrupted (at least one fatal assertion failed)
			INTERRUPTED,
			// The number of errors has reached the specified threshold config_type::abort_after_n_failures,
			// all suite/test executions are terminated and the last test is marked TERMINATED
			TERMINATED,
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

	constexpr std::string_view anonymous_suite_name{"anonymous_suite"};

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

	enum class OutputLevel : std::uint16_t
	{
		// for functional::enumeration
		PROMETHEUS_MAGIC_ENUM_FLAG = 0b0000'0000'0000'0000,

		// SUITE & TEST NAME
		SUITE_NAME = 0b0000'0000'0000'0001,
		TEST_NAME = 0b0000'0000'0000'0010 | SUITE_NAME,

		// ASSERTION
		ASSERTION_FATAL = 0b0000'0000'0001'0000 | TEST_NAME,
		ASSERTION_FAILURE = 0b0000'0000'0010'0000 | ASSERTION_FATAL,
		ASSERTION_SKIP = 0b0000'0000'0100'0000 | ASSERTION_FAILURE,
		ASSERTION_PASS = 0b0000'0000'1000'0000 | ASSERTION_SKIP,

		ASSERTION_ERROR_ONLY = ASSERTION_FAILURE,
		ASSERTION_NOT_PASS = ASSERTION_SKIP,
		ASSERTION_ALL = ASSERTION_PASS,

		DEFAULT = ASSERTION_NOT_PASS,
		NO_ASSERTION = TEST_NAME,
		ALL = ASSERTION_ALL,
		NONE = 0b1000'0000'0000'0000,
	};

	enum class DebugBreakPoint : std::uint8_t
	{
		// for functional::enumeration
		PROMETHEUS_MAGIC_ENUM_FLAG = 0b0000'0000,

		FATAL = 0b0000'0001,
		FAIL = 0b0000'0010,

		NONE = 0b0001'0000,
	};

	struct config_type
	{
		using name_type = std::string_view;
		using category_type = std::string_view;
		using categories_type = std::vector<category_type>;

		color_type color = {};

		// Dynamic width, the space before each output is based on the nesting depth and tab width
		// std::format("{:{}}-xxx", "", tab_width * depth)
		std::size_t tab_width = 4;
		// The prefix of each output is not recommended to be longer than tab_width * depth
		// prefix = "Prefix:"
		// [tab_width * depth == 10] ==> [Prefix:   OUTPUT]
		// [tab_width * depth == 20] ==> [Prefix:             OUTPUT]
		// [tab_width * depth == 5]   ==> [Prefix:OUTPUT]
		std::string_view prefix = "-";

		std::size_t abort_after_n_failures = std::numeric_limits<std::size_t>::max();

		OutputLevel output_level = OutputLevel::DEFAULT;

		bool dry_run = false;

		DebugBreakPoint debug_break_point = DebugBreakPoint::NONE;

		std::function<void(std::string_view)> message_reporter = [](const std::string_view report_message) -> void
		{
			std::cout << report_message;
		};

		// Used to filter the suite/test cases that need to be executed.
		std::function<bool(name_type)> filter_execute_suite_name = []([[maybe_unused]] const name_type suite_name) noexcept -> bool
		{
			return true;
		};
		std::function<bool(name_type)> filter_execute_test_name = []([[maybe_unused]] const name_type test_name) noexcept -> bool
		{
			return true;
		};
		std::function<bool(const categories_type&)> filter_execute_test_categories = [](const categories_type& categories) noexcept -> bool
		{
			if (std::ranges::contains(categories, "skip")) { return false; }

			return true;
		};

		[[nodiscard]] auto debug_break_point_required(const DebugBreakPoint point) const noexcept -> bool
		{
			return static_cast<bool>(debug_break_point & point);
		}

		auto report_message(const std::string_view message) const noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(message_reporter);

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

	namespace unit_test_detail
	{
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

			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH

			#if defined(GAL_PROMETHEUS_COMPILER_GNU) or defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
			// struct bar {};
			// struct foo : bar
			// {
			//	int a;
			// };
			// foo f{.a = 42}; // <-- warning: missing initializer for member `foo::<anonymous>` [-Wmissing-field-initializers]
			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING(-Wmissing-field-initializers)
			#endif

			// =========================================
			// SUITE
			// =========================================

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSuiteBegin final : public Event
			{
			public:
				name_type name;
			};

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSuiteEnd final : public Event
			{
			public:
				name_type name;
			};

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSuite final : public Event
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

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTestBegin final : public Event
			{
			public:
				name_type name;
			};

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTestSkip final : public Event
			{
			public:
				name_type name;
			};

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTestEnd final : public Event
			{
			public:
				name_type name;
			};

			template<typename InvocableType, typename Arg = none>
				requires std::is_invocable_v<InvocableType> or std::is_invocable_v<InvocableType, Arg>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTest final : public Event
			{
			public:
				using invocable_type = InvocableType;
				using arg_type = Arg;

				name_type name;
				config_type::categories_type categories;

				mutable invocable_type invocable;
				mutable arg_type arg;

				constexpr auto operator()() const noexcept(false) -> void
				{
					return []<typename I, typename A>(I&& i, A&& a) noexcept(false) -> void
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
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertionPass final : public Event
			{
			public:
				using expression_type = Expression;

				expression_type expression;
				std::source_location location;
			};

			template<expression_t Expression>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertionFail final : public Event
			{
			public:
				using expression_type = Expression;

				expression_type expression;
				std::source_location location;
			};

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertionFatal final : public Event
			{
			public:
				std::source_location location;
			};

			// no longer needed
			// @see Executor::InterruptTestInvoke
			// @see Executor::on(const events::EventAssertionFatal&, internal_tag)
			// template<expression_t Expression>
			// class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertionSkip final : public Event
			// {
			// public:
			// 	using expression_type = Expression;
			//
			// 	expression_type expression;
			// 	std::source_location location;
			// };

			template<expression_t Expression>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventAssertion final : public Event
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

				[[nodiscard]] constexpr explicit operator EventAssertionFatal() const noexcept
				{
					return {.location = location};
				}

				// [[nodiscard]] constexpr explicit operator EventAssertionSkip<expression_type>() const noexcept
				// {
				// 	return {.expression = expression, .location = location};
				// }

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

				// [[nodiscard]] constexpr auto skip() const noexcept -> EventAssertionSkip<expression_type>
				// {
				// 	// fixme: Compiler Error: C2273
				// 	// return this->operator EventAssertionSkip<expression_type>();
				// 	return operator EventAssertionSkip<expression_type>();
				// }
			};

			// =========================================
			// UNEXPECTED
			// =========================================

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventUnexpected final : public Event
			{
			public:
				std::string_view message;

				[[nodiscard]] constexpr auto what() const noexcept -> std::string_view
				{
					return message;
				}
			};

			// =========================================
			// LOG
			// =========================================

			template<typename MessageType>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventLog final : public Event
			{
			public:
				using message_type = MessageType;

				message_type message;
			};

			// ReSharper disable CppInconsistentNaming
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
			// ReSharper restore CppInconsistentNaming

			// =========================================
			// SUMMARY
			// =========================================

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSummary final : public Event {};

			GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH
		} // namespace events

		namespace operands
		{
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

			// =========================================
			// VALUE / REFERENCE
			// =========================================

			template<typename T>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandValue final : public Operand
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

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					return meta::to_string(value_, out);
				}
			};

			// ReSharper disable CppInconsistentNaming
			template<typename T>
			OperandValue(T) -> OperandValue<T>;
			// ReSharper restore CppInconsistentNaming

			template<typename T>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandValueRef final : public Operand
			{
			public:
				using value_type = T;

			private:
				std::reference_wrapper<value_type> ref_;

			public:
				constexpr explicit(false) OperandValueRef(value_type& ref) noexcept
					: ref_{ref} {}

				[[nodiscard]] constexpr auto value() noexcept -> value_type& { return ref_.get(); }

				[[nodiscard]] constexpr auto value() const noexcept -> const value_type& { return ref_.get(); }

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					return meta::to_string(ref_.get(), out);
				}
			};

			// ReSharper disable CppInconsistentNaming
			template<typename T>
			OperandValueRef(T&) -> OperandValueRef<T>;
			// propagate const
			template<typename T>
			OperandValueRef(const T&) -> OperandValueRef<const T>;
			// ReSharper restore CppInconsistentNaming

			template<typename>
			struct is_operand_value : std::false_type {};

			template<typename T>
			struct is_operand_value<OperandValue<T>> : std::true_type {};

			template<typename T>
			struct is_operand_value<OperandValueRef<T>> : std::true_type {};

			template<typename T>
			constexpr auto is_operand_value_v = is_operand_value<T>::value;
			template<typename O>
			concept operand_value_t = is_operand_value_v<O>;

			// =========================================
			// LITERAL
			// =========================================

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteral : public Operand {};

			template<char Value>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralCharacter final : public OperandLiteral
			{
			public:
				using value_type = char;

				constexpr static auto value = Value;

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					return meta::to_string(value, out);
				}
			};

			template<std::integral auto Value>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralIntegral final : public OperandLiteral
			{
			public:
				using value_type = std::remove_cvref_t<decltype(Value)>;

				constexpr static value_type value = Value;

				[[nodiscard]] constexpr auto operator-() const noexcept -> OperandLiteralIntegral<-static_cast<std::make_signed_t<value_type>>(value)>
				{
					return {};
				}

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					return meta::to_string(value, out);
				}
			};

			template<std::floating_point auto Value, std::size_t DenominatorSize>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralFloatingPoint final : public OperandLiteral
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

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					return std::format_to(std::back_inserter(out), "{:.{}g}", DenominatorSize, value);
				}
			};

			template<char... Cs>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralAuto final : public OperandLiteral
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
					using type = OperandLiteralFloatingPoint<
						char_list.template to_floating_point<OperandLiteralFloatingPoint<Value, DenominatorSize>::value_type>,
						char_list.denominator_length()
					>;
				};

				template<std::integral T>
				struct rep<T>
				{
					using type = std::conditional_t<
						std::is_same_v<T, char>,
						OperandLiteralCharacter<char_list.template nth_value<0>()>,
						OperandLiteralIntegral<char_list.template to_integral<T>()>
					>;
				};

				template<std::floating_point T>
				struct rep<T>
				{
					using type = OperandLiteralFloatingPoint<char_list.template to_floating_point<T>(), char_list.denominator_length()>;
				};

				template<typename T>
				using rebind = typename rep<std::remove_cvref_t<T>>::type;
			};

			// ========================
			// literal

			template<typename O>
			constexpr auto is_operand_literal_v = std::is_base_of_v<OperandLiteral, O>;
			template<typename O>
			concept operand_literal_t = is_operand_literal_v<O>;

			// ========================
			// literal character

			template<typename>
			struct is_operand_literal_character : std::false_type {};

			template<char Value>
			struct is_operand_literal_character<OperandLiteralCharacter<Value>> : std::true_type {};

			template<typename T>
			constexpr auto is_operand_literal_character_v = is_operand_literal_character<T>::value;
			template<typename O>
			concept operand_literal_character_t = is_operand_literal_character_v<O>;

			// ========================
			// literal integral

			template<typename>
			struct is_operand_literal_integral : std::false_type {};

			template<std::integral auto Value>
			struct is_operand_literal_integral<OperandLiteralIntegral<Value>> : std::true_type {};

			template<typename T>
			constexpr auto is_operand_literal_integral_v = is_operand_literal_integral<T>::value;
			template<typename O>
			concept operand_literal_integral_t = is_operand_literal_integral_v<O>;

			// ========================
			// literal floating point

			template<typename>
			struct is_operand_literal_floating_point : std::false_type {};

			template<std::floating_point auto Value, std::size_t DenominatorSize>
			struct is_operand_literal_floating_point<OperandLiteralFloatingPoint<Value, DenominatorSize>> : std::true_type {};

			template<typename T>
			constexpr auto is_operand_literal_floating_point_v = is_operand_literal_floating_point<T>::value;
			template<typename O>
			concept operand_literal_floating_point_t = is_operand_literal_floating_point_v<O>;

			// ========================
			// literal auto-deducing

			template<typename>
			struct is_operand_literal_auto : std::false_type {};

			template<char... Cs>
			struct is_operand_literal_auto<OperandLiteralAuto<Cs...>> : std::true_type {};

			template<typename T>
			constexpr auto is_operand_literal_auto_v = is_operand_literal_auto<T>::value;
			template<typename O>
			concept operand_literal_auto_t = is_operand_literal_auto_v<O>;

			// =========================================
			// IDENTITY (message)
			// =========================================

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandIdentityBoolean final : public Operand
			{
			public:
				// explicit unique type
				struct value_type
				{
					std::string_view string;
				};

			private:
				value_type value_;
				bool result_;

			public:
				constexpr OperandIdentityBoolean(const value_type value, const bool result) noexcept
					: value_{value},
					  result_{result} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					std::format_to(std::back_inserter(out), "{}", value_.string);
				}
			};

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandIdentityString final : public Operand
			{
			public:
				// explicit unique type
				struct value_type
				{
					std::string_view string;
				};

			private:
				value_type value_;

			public:
				constexpr explicit OperandIdentityString(const value_type value) noexcept
					: value_{value} {}

				[[nodiscard]] constexpr auto value() const noexcept -> std::string_view
				{
					return value_.string;
				}

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					std::format_to(std::back_inserter(out), "\"{}\"", value_.string);
				}
			};

			// ========================
			// identity boolean

			template<typename>
			struct is_operand_identity_boolean : std::false_type {};

			template<>
			struct is_operand_identity_boolean<OperandIdentityBoolean> : std::true_type {};

			template<typename T>
			constexpr auto is_operand_identity_boolean_v = is_operand_identity_boolean<T>::value;

			template<typename T>
			concept operand_identity_boolean_t = is_operand_identity_boolean_v<T>;

			// ========================
			// identity string

			template<typename>
			struct is_operand_identity_string : std::false_type {};

			template<>
			struct is_operand_identity_string<OperandIdentityString> : std::true_type {};

			template<typename T>
			constexpr auto is_operand_identity_string_v = is_operand_identity_string<T>::value;

			template<typename T>
			concept operand_identity_string_t = is_operand_identity_string_v<T>;

			// =========================================
			// EXPRESSION
			// =========================================

			enum class ExpressionCategory : std::uint8_t
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
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandExpression final : public Operand
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
				left_type left_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
				right_type right_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
				epsilon_type epsilon_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
				bool result_;

				[[nodiscard]] constexpr auto do_check() const noexcept -> bool
				{
					const auto do_compare = [](const auto& left, const auto& right, const auto& epsilon) noexcept -> bool
					{
						if constexpr (category == ExpressionCategory::EQUAL)
						{
							using std::operator==;
							using std::operator!=;
							if constexpr (requires { left == right; })
							{
								return left == right;
							}
							else if constexpr (requires { right == left; })
							{
								return right == left;
							}
							else if constexpr (requires { left != right; })
							{
								return not(left != right);
							}
							else if constexpr (requires { right != left; })
							{
								return not(right != left);
							}
							else if constexpr (requires { { left.compare(right) } -> std::same_as<bool>; })
							{
								return left.compare(right);
							}
							else if constexpr (requires { left.compare(right); })
							{
								return left.compare(right) == 0;
							}
							else if constexpr (requires { { right.compare(left) } -> std::same_as<bool>; })
							{
								return right.compare(left);
							}
							else if constexpr (requires { right.compare(left); })
							{
								return right.compare(left) == 0;
							}
							else
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("Not comparable!");
							}
						}
						else if constexpr (category == ExpressionCategory::APPROX)
						{
							using std::operator-;
							using std::operator<;
							return math::abs(left - right) < epsilon; // NOLINT(clang-diagnostic-implicit-int-float-conversion)
						}
						else if constexpr (category == ExpressionCategory::NOT_EQUAL)
						{
							using std::operator!=;
							using std::operator==;
							if constexpr (requires { left != right; })
							{
								return left != right;
							}
							else if constexpr (requires { right != left; })
							{
								return right != left;
							}
							else if constexpr (requires { left == right; })
							{
								return not(left == right);
							}
							else if constexpr (requires { right == left; })
							{
								return not(right == left);
							}
							else if constexpr (requires { { left.compare(right) } -> std::same_as<bool>; })
							{
								return not left.compare(right);
							}
							else if constexpr (requires { left.compare(right); })
							{
								return left.compare(right) != 0;
							}
							else if constexpr (requires { { right.compare(left) } -> std::same_as<bool>; })
							{
								return not right.compare(left);
							}
							else if constexpr (requires { right.compare(left); })
							{
								return right.compare(left) != 0;
							}
							else
							{
								GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("Not comparable!");
							}
						}
						else if constexpr (category == ExpressionCategory::NOT_APPROX)
						{
							using std::operator-;
							using std::operator<;
							return epsilon < math::abs(left - right);
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

					const auto get_value = []<typename T>([[maybe_unused]] const T& target) noexcept -> decltype(auto) //
					{
						[[maybe_unused]] constexpr auto not_member_function_pointer = []<typename P>(P) constexpr noexcept
						{
							return not std::is_member_function_pointer_v<std::decay_t<P>>;
						};

						if constexpr (requires { target.value(); })
						{
							// member function
							return target.value();
						}
						else if constexpr (requires { not_member_function_pointer(T::value); })
						{
							// static variable
							return T::value;
						}
						else
						{
							return target;
						}
					};

					return do_compare(get_value(left_), get_value(right_), get_value(epsilon_));
				}

			public:
				template<typename L, typename R, typename E>
				constexpr OperandExpression(
					L&& left,
					R&& right,
					E&& epsilon
				) noexcept
					: left_{std::forward<L>(left)},
					  right_{std::forward<R>(right)},
					  epsilon_{std::forward<E>(epsilon)},
					  result_{do_check()} {}

				template<typename L, typename R>
				constexpr OperandExpression(
					L&& left,
					R&& right
				) noexcept
					: left_{std::forward<L>(left)},
					  right_{std::forward<R>(right)},
					  epsilon_{},
					  result_{do_check()} {}

				[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					constexpr auto left_prefer_no_type_name = requires { typename left_type::prefer_no_type_name; };
					constexpr auto right_prefer_no_type_name = requires { typename right_type::prefer_no_type_name; };
					constexpr auto epsilon_prefer_no_type_name = requires { typename right_type::prefer_no_type_name; };

					if constexpr (category == ExpressionCategory::EQUAL)
					{
						std::format_to(
							std::back_inserter(out),
							"{} == {}",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_)
						);
					}
					else if constexpr (category == ExpressionCategory::APPROX)
					{
						std::format_to(
							std::back_inserter(out),
							"{} ≈≈ {} (+/- {})",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_),
							meta::to_string<StringType, not epsilon_prefer_no_type_name>(epsilon_)
						);
					}
					else if constexpr (category == ExpressionCategory::NOT_EQUAL)
					{
						std::format_to(
							std::back_inserter(out),
							"{} != {}",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_)
						);
					}
					else if constexpr (category == ExpressionCategory::NOT_APPROX)
					{
						std::format_to(
							std::back_inserter(out),
							"{} !≈ {} (+/- {})",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_),
							meta::to_string<StringType, not epsilon_prefer_no_type_name>(epsilon_)
						);
					}
					else if constexpr (category == ExpressionCategory::GREATER_THAN)
					{
						std::format_to(
							std::back_inserter(out),
							"{} > {}",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_)
						);
					}
					else if constexpr (category == ExpressionCategory::GREATER_EQUAL)
					{
						std::format_to(
							std::back_inserter(out),
							"{} >= {}",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_)
						);
					}
					else if constexpr (category == ExpressionCategory::LESS_THAN)
					{
						std::format_to(
							std::back_inserter(out),
							"{} < {}",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_)
						);
					}
					else if constexpr (category == ExpressionCategory::LESS_EQUAL)
					{
						std::format_to(
							std::back_inserter(out),
							"{} <= {}",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_)
						);
					}
					else if constexpr (category == ExpressionCategory::LOGICAL_AND)
					{
						std::format_to(
							std::back_inserter(out),
							"{} and {}",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_)
						);
					}
					else if constexpr (category == ExpressionCategory::LOGICAL_OR)
					{
						std::format_to(
							std::back_inserter(out),
							"{} or {}",
							meta::to_string<StringType, not left_prefer_no_type_name>(left_),
							meta::to_string<StringType, not right_prefer_no_type_name>(right_)
						);
					}
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				}
			};

			template<typename>
			struct is_operand_expression : std::false_type {};

			template<ExpressionCategory Category, typename Left, typename Right, typename Epsilon>
			struct is_operand_expression<OperandExpression<Category, Left, Right, Epsilon>> : std::true_type {};

			template<typename T>
			constexpr auto is_operand_expression_v = is_operand_expression<T>::value;
			template<typename O>
			concept operand_expression_t = is_operand_expression_v<O>;

			// =========================================
			// EXCEPTION
			// =========================================

			template<typename Exception>
			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandThrow final : public Operand
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

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					std::format_to(
						std::back_inserter(out),
						"throws<{}> -- [{}]",
						meta::name_of<exception_type>(),
						(not thrown())
							? "not thrown"
							: //
							(not caught())
							? "thrown but not caught"
							: //
							"caught"
					);
				}
			};

			class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandNoThrow final : public Operand
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

				template<std::ranges::output_range<char> StringType>
					requires std::ranges::contiguous_range<StringType>
				constexpr auto to_string(StringType& out) const noexcept -> void
				{
					std::format_to(std::back_inserter(out), "nothrow - {:s}", not thrown_);
				}
			};
		} // namespace operands

		namespace executor
		{
			class Executor
			{
			public:
				using suites_type = std::vector<events::EventSuite>;

				// not nullable
				using suite_results_iterator_type = suite_results_type::iterator;
				// nullable
				using test_results_iterator_type = test_results_type::pointer;

			private:
				struct internal_tag {};

				// Unfortunately, we cannot precisely control whether each assertion in the test should be evaluated,
				// so we choose to throw an exception to interrupt the entire suite/test to avoid subsequent code execution.

				class InterruptTestInvoke final {};

				class InterruptSuiteInvoke final {};

				config_type config_;

				suites_type suites_;

				suite_results_type suite_results_;
				suite_results_iterator_type current_suite_result_;
				test_results_iterator_type current_test_result_;

				std::size_t total_fails_exclude_current_test_;

				bool is_executor_fatal_error_;
				std::uint8_t pad_1_;
				std::uint16_t pad_2_;

				[[nodiscard]] auto is_executor_fatal_error() const noexcept -> bool
				{
					return is_executor_fatal_error_;
				}

				auto make_executor_fatal_error() noexcept -> void
				{
					is_executor_fatal_error_ = true;
				}

				template<OutputLevel RequiredLevel>
				[[nodiscard]] auto is_level_match() const noexcept -> bool
				{
					return (RequiredLevel & config_.output_level) == RequiredLevel;
				}

				enum class IdentType : std::uint8_t
				{
					TEST,
					ASSERTION,
				};

				template<IdentType Type>
				[[nodiscard]] auto nested_level_of_current_test() const noexcept -> std::size_t
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not current_suite_result_->test_results.empty());

					if constexpr (Type == IdentType::ASSERTION)
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);
					}
					else
					{
						// top level
						if (current_test_result_ == nullptr)
						{
							return 1;
						}
					}

					std::size_t result = 0;
					for (const auto* p = current_test_result_; p != nullptr; p = p->parent)
					{
						result += 1;
					}

					return result + (Type == IdentType::ASSERTION);
				}

				template<IdentType Type>
				[[nodiscard]] auto ident_size_of_current_test() const noexcept -> std::size_t
				{
					return nested_level_of_current_test<Type>() * config_.tab_width;
				}

				// [suite_name] test1.test2.test3
				[[nodiscard]] auto fullname_of_current_test() const noexcept -> std::string
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not current_suite_result_->test_results.empty());

					auto result = std::format("[{}] ", current_suite_result_->name);

					const auto* p = std::addressof(current_suite_result_->test_results.back());
					while (p != nullptr)
					{
						result.append(p->name);
						result.push_back('.');

						p = p->children.empty() ? nullptr : std::addressof(p->children.back());
					}

					result.pop_back();
					return result;
				}

				[[nodiscard]] auto ms_duration_of_current_test() const noexcept -> time_point_type::rep
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);
					return std::chrono::duration_cast<time_difference_type>(current_test_result_->time_end - current_test_result_->time_start).count();
				}

				auto check_total_failures() noexcept(false) -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

					if (total_fails_exclude_current_test_ + current_test_result_->total_assertions_failed >= config_.abort_after_n_failures)
					[[unlikely]]
					{
						current_test_result_->status = test_result_type::Status::TERMINATED;

						const auto old_suite_result = current_suite_result_;
						const auto old_test_result = current_test_result_;
						const auto old_test_name = fullname_of_current_test();

						std::format_to(
							std::back_inserter(old_suite_result->report_string),
							"{:<{}}{}The number of errors has reached the specified threshold {} "
							"(this test raises {} error(s)), "
							"terminate all suite/test!{}\n",
							config_.prefix,
							ident_size_of_current_test<IdentType::ASSERTION>(),
							config_.color.fail,
							config_.abort_after_n_failures,
							old_test_result->total_assertions_failed,
							config_.color.none
						);

						make_executor_fatal_error();

						// terminate current suite/test :)
						throw InterruptSuiteInvoke{}; // NOLINT(hicpp-exception-baseclass)
					}
				}

				// =========================================
				// SUITE
				// =========================================

				auto on(const events::EventSuite& suite, internal_tag) noexcept -> void
				{
					// A (fatal) error occurred in the executor, and all suites/tests followed will be skipped
					if (is_executor_fatal_error())
					{
						return;
					}

					// Nested suite is not allowed
					// suite<"root"> root = []
					// {
					//	suite<"nested"> nested = [] { }; // <-- ERROR!!!
					// };
					if (current_suite_result_ != suite_results_.begin())
					{
						const auto message = std::format(
							"Unable to define nested suite {} within suite {}, skipped...",
							suite.name,
							current_suite_result_->name
						);
						on(events::EventUnexpected{.message = message});
						return;
					}

					if (config_.is_suite_execute_required(suite.name))
					{
						on(suite.begin());

						try
						{
							std::invoke(suite);
						}
						// Even if an InterruptSuiteInvoke exception is thrown when executing test, it will not be propagated here.
						// In other words, it is guaranteed that `on(events::EventTest)` will not throw any exception.
						// catch (const InterruptSuiteInvoke&) {}
						//
						// The user's suite threw an exception, but it was not handled.
						// We capture the exception here to avoid early termination of the program.
						// suite<"throw"> _ = []
						// {
						//	throw std::runtime_error{"throw!"};
						//
						// "unreachable"_test = [] { };
						// };
						catch (const std::exception& exception)
						{
							on(events::EventUnexpected{.message = exception.what()});
						}
						catch (...)
						{
							on(events::EventUnexpected{.message = "unknown exception type, not derived from std::exception"});
						}

						on(suite.end());
					}
				}

				auto on(const events::EventSuiteBegin& suite_begin) noexcept -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ == suite_results_.begin());

					auto& [name, report_string, test_results] = suite_results_.emplace_back(std::string{suite_begin.name});
					current_suite_result_ = std::ranges::prev(suite_results_.end(), 1);

					if (is_level_match<OutputLevel::SUITE_NAME>())
					{
						std::format_to(
							std::back_inserter(report_string),
							"Executing suite {}{}{} vvv\n",
							config_.color.suite,
							name,
							config_.color.none
						);
					}
				}

				auto on(const events::EventSuiteEnd& suite_end) -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_->name == suite_end.name);
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ == nullptr);

					auto& [name, report_string, test_results] = *current_suite_result_;

					if (is_level_match<OutputLevel::SUITE_NAME>())
					{
						std::format_to(
							std::back_inserter(report_string),
							"^^^ End of suite {}{}{} execution\n",
							config_.color.suite,
							name,
							config_.color.none
						);
					}

					// reset to anonymous suite
					current_suite_result_ = suite_results_.begin();
				}

				// =========================================
				// TEST
				// =========================================

				template<typename InvocableType, typename Arg>
				auto on(const events::EventTest<InvocableType, Arg>& test, internal_tag) noexcept -> void
				{
					// fixme:
					// We should not have to determine whether the current executor is still available here,
					// but we did not propagate the exception InterruptSuiteInvoke,
					// so the suite will continue to execute.
					// Maybe later we will propagate the exception InterruptSuiteInvoke, but not now :)
					if (is_executor_fatal_error())
					{
						return;
					}

					if (config_.is_test_execute_required(test.name, test.categories))
					{
						this->on(test.begin());

						try
						{
							std::invoke(test);
						}
						catch (const InterruptTestInvoke&) // NOLINT(bugprone-empty-catch)
						{
							// This exception is thrown by and only by the function `on(events::EventAssertionFatal)` and is intercepted here and no longer propagated
							// do nothing here :)
						}
						catch (const InterruptSuiteInvoke&) // NOLINT(bugprone-empty-catch)
						{
							// This exception is thrown by and only by the function `check_total_failures()` and is intercepted here and no longer propagated
							// do nothing here :)
						}
						// The user's suite threw an exception, but it was not handled.
						// We capture the exception here to avoid early termination of the program.
						catch (const std::exception& exception)
						{
							on(events::EventUnexpected{.message = exception.what()});
						}
						catch (...)
						{
							on(events::EventUnexpected{.message = "unhandled exception, not derived from std::exception"});
						}

						this->on(test.end());
					}
					else
					{
						this->on(test.skip());
					}
				}

				auto on(const events::EventTestBegin& test_begin) noexcept -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());

					// we chose to construct a temporary object here to avoid possible errors, and trust that the optimizer will forgive us ;)
					auto t = test_result_type
					{
							.name = std::string{test_begin.name},
							.parent = current_test_result_,
							.children = {},
							.status = test_result_type::Status::PENDING,
							.time_start = clock_type::now(),
							.time_end = {},
							.total_assertions_passed = 0,
							.total_assertions_failed = 0
					};

					if (current_test_result_)
					{
						// nested
						auto& this_test = current_test_result_->children.emplace_back(std::move(t));
						current_test_result_ = std::addressof(this_test);

						if (is_level_match<OutputLevel::TEST_NAME>())
						{
							std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:<{}}Running nested test {}{}{}...\n",
								config_.prefix,
								ident_size_of_current_test<IdentType::TEST>(),
								config_.color.test,
								fullname_of_current_test(),
								config_.color.none
							);
						}
					}
					else
					{
						// top level
						auto& this_test = current_suite_result_->test_results.emplace_back(std::move(t));
						current_test_result_ = std::addressof(this_test);

						if (is_level_match<OutputLevel::TEST_NAME>())
						{
							std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:<{}}Running test {}{}{}...\n",
								config_.prefix,
								ident_size_of_current_test<IdentType::TEST>(),
								config_.color.test,
								fullname_of_current_test(),
								config_.color.none
							);
						}
					}
				}

				auto on(const events::EventTestSkip& test_skip) noexcept -> void
				{
					on(events::EventTestBegin{.name = test_skip.name});
					current_test_result_->status = test_result_type::Status::SKIPPED_FILTERED;
					on(events::EventTestEnd{.name = test_skip.name});
				}

				auto on(const events::EventTestEnd& test_end) noexcept -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_->name == test_end.name);

					current_test_result_->time_end = clock_type::now();
					if (current_test_result_->status == test_result_type::Status::PENDING)
					[[likely]]
					{
						// the current test is considered SKIPPED only if it does not have any assertions and has no children.
						if (current_test_result_->total_assertions_failed == 0 and current_test_result_->total_assertions_passed == 0)
						{
							if (current_test_result_->children.empty())
							{
								current_test_result_->status = test_result_type::Status::SKIPPED_NO_ASSERTION;
							}
							else
							{
								current_test_result_->status =
										std::ranges::all_of(
											current_test_result_->children,
											[](const auto& child_test) noexcept
											{
												return child_test.total_assertions_failed == 0;
											}
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

					if (is_level_match<OutputLevel::TEST_NAME>())
					{
						if (const auto status = current_test_result_->status;
							status == test_result_type::Status::PASSED or status == test_result_type::Status::FAILED)
						[[likely]]
						{
							std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:<{}}{}{}{} after {} milliseconds.\n",
								config_.prefix,
								ident_size_of_current_test<IdentType::TEST>(),
								status == test_result_type::Status::PASSED ? config_.color.pass : config_.color.fail,
								status == test_result_type::Status::PASSED ? "PASSED" : "FAILED",
								config_.color.none,
								ms_duration_of_current_test()
							);
						}
						else if (status == test_result_type::Status::SKIPPED_NO_ASSERTION or status == test_result_type::Status::SKIPPED_FILTERED)
						[[unlikely]]
						{
							std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:<{}}{}SKIPPED{} --- [{}] \n",
								config_.prefix,
								ident_size_of_current_test<IdentType::TEST>(),
								config_.color.skip,
								config_.color.none,
								status == test_result_type::Status::SKIPPED_NO_ASSERTION
									? "No Assertion(s) Found"
									: "FILTERED"
							);
						}
						else if (status == test_result_type::Status::INTERRUPTED or status == test_result_type::Status::TERMINATED)
						[[unlikely]]
						{
							std::format_to(
								std::back_inserter(current_suite_result_->report_string),
								"{:<{}}{}{}{}\n",
								config_.prefix,
								ident_size_of_current_test<IdentType::TEST>(),
								config_.color.fatal,
								status == test_result_type::Status::INTERRUPTED
									? "INTERRUPTED"
									: "TERMINATED",
								config_.color.none
							);
						}
						else { std::unreachable(); }
					}

					// reset to parent test
					current_test_result_ = current_test_result_->parent;
				}

				// =========================================
				// ASSERTION
				// =========================================

				template<expression_t Expression>
				auto on(const events::EventAssertion<Expression>& assertion, internal_tag) noexcept(false) -> bool
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

					if (config_.dry_run)
					[[unlikely]]
					{
						return true;
					}

					// if (current_test_result_->status == test_result_type::Status::FATAL)
					// [[unlikely]]
					// {
					// 	this->on(assertion.skip());
					// 	// Consider the test case execution successful and avoid undesired log output
					// 	return true;
					// }

					if (static_cast<bool>(assertion.expression))
					[[likely]]
					{
						this->on(assertion.pass());
						return true;
					}

					this->on(assertion.fail());
					return false;
				}

				template<expression_t Expression>
				auto on(const events::EventAssertionPass<Expression>& assertion_pass) noexcept -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

					if (is_level_match<OutputLevel::ASSERTION_PASS>())
					{
						// @see: Operand::prefer_no_type_name
						constexpr auto prefer_no_type_name = requires { typename Expression::prefer_no_type_name; };

						std::format_to(
							std::back_inserter(current_suite_result_->report_string),
							"{:<{}}[{}:{}] {}[{}]{} - {}PASSED{} \n",
							config_.prefix,
							ident_size_of_current_test<IdentType::ASSERTION>(),
							assertion_pass.location.file_name(),
							assertion_pass.location.line(),
							config_.color.expression,
							meta::to_string<std::string, not prefer_no_type_name>(assertion_pass.expression),
							config_.color.none,
							config_.color.pass,
							config_.color.none
						);
					}

					current_test_result_->total_assertions_passed += 1;
				}

				template<expression_t Expression>
				auto on(const events::EventAssertionFail<Expression>& assertion_fail) noexcept(false) -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

					GAL_PROMETHEUS_ERROR_BREAKPOINT_IF(config_.debug_break_point_required(DebugBreakPoint::FAIL), "EventAssertionFail");

					if (is_level_match<OutputLevel::ASSERTION_FAILURE>())
					{
						// @see: Operand::prefer_no_type_name
						constexpr auto prefer_no_type_name = requires { typename Expression::prefer_no_type_name; };

						std::format_to(
							std::back_inserter(current_suite_result_->report_string),
							"{:<{}}[{}:{}] {}[{}]{} - {}FAILED{} \n",
							config_.prefix,
							ident_size_of_current_test<IdentType::ASSERTION>(),
							assertion_fail.location.file_name(),
							assertion_fail.location.line(),
							config_.color.expression,
							meta::to_string<std::string, not prefer_no_type_name>(assertion_fail.expression),
							config_.color.none,
							config_.color.fail,
							config_.color.none
						);
					}

					current_test_result_->total_assertions_failed += 1;

					check_total_failures();
				}

				auto on(const events::EventAssertionFatal& assertion_fatal, internal_tag) noexcept(false) -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

					GAL_PROMETHEUS_ERROR_BREAKPOINT_IF(config_.debug_break_point_required(DebugBreakPoint::FATAL), "EventAssertionFatal");

					if (is_level_match<OutputLevel::ASSERTION_FATAL>())
					{
						std::format_to(
							std::back_inserter(current_suite_result_->report_string),
							"{:<{}}^^^ {}FATAL ERROR! END TEST!{}\n",
							config_.prefix,
							ident_size_of_current_test<IdentType::ASSERTION>() +
							(
								// '['
								1 +
								// file_name
								std::string_view::traits_type::length(assertion_fatal.location.file_name())) +
							// ':'
							1 +
							// line
							[]<typename Line>(Line line)
							{
								Line result = 0;
								while (line)
								{
									result += 1;
									line /= 10;
								}
								return result;
							}(assertion_fatal.location.line()) +
							// "] ["
							3,
							config_.color.fatal,
							config_.color.none
						);
					}

					check_total_failures();

					// terminate current test :)
					throw InterruptTestInvoke{}; // NOLINT(hicpp-exception-baseclass)
				}

				// template<expression_t Expression>
				// auto on(const events::EventAssertionSkip<Expression>& assertion_skip) noexcept -> void
				// {
				// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
				// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);
				//
				// 	if (is_level_match<OutputLevel::ASSERTION_SKIP>())
				// 	{
				// 		// @see: Operand::prefer_no_type_name
				// 		constexpr auto prefer_no_type_name = requires { typename Expression::prefer_no_type_name; };
				//
				// 		std::format_to(
				// 			std::back_inserter(current_suite_result_->report_string),
				// 			"{:<{}}[{}:{}] {}[{}]{} - {}SKIPPED{}\n",
				// 			config_.prefix,
				// 			ident_size_of_current_test<IdentType::ASSERTION>(),
				// 			assertion_skip.location.file_name(),
				// 			assertion_skip.location.line(),
				// 			config_.color.expression,
				// 			meta::to_string<std::string, not prefer_no_type_name>(assertion_skip.expression),
				// 			config_.color.none,
				// 			config_.color.fatal,
				// 			config_.color.none
				// 		);
				// 	}
				//
				// 	current_test_result_->total_assertions_failed += 1;
				//
				// 	check_total_failures();
				// }

				// =========================================
				// UNEXPECTED
				// =========================================

				auto on(const events::EventUnexpected& unexpected) noexcept -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

					auto& [suite_name, report_string, _] = *current_suite_result_;

					std::format_to(
						std::back_inserter(report_string),
						"Unhandled exception threw from {}: {}{}{}\n",
						fullname_of_current_test(),
						config_.color.fail,
						unexpected.what(),
						config_.color.none
					);
				}

				// =========================================
				// LOG
				// =========================================

				template<typename MessageType>
				auto on(const events::EventLog<MessageType>& log, internal_tag) noexcept -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());

					if (log.message != "\n")
					[[likely]]
					{
						// pop '\n'
						current_suite_result_->report_string.pop_back();
					}

					current_suite_result_->report_string.append(config_.color.message);
					#if __cpp_lib_containers_ranges >= 202202L
					current_suite_result_->report_string.append_range(log.message);
					#else
					current_suite_result_->report_string.insert(current_suite_result_->report_string.end(), std::ranges::begin(log.message), std::ranges::end(log.message));
					#endif
					current_suite_result_->report_string.append(config_.color.none);

					// push '\n'
					current_suite_result_->report_string.push_back('\n');
				}

				// =========================================
				// SUMMARY
				// =========================================

				auto on(const events::EventSummary&) noexcept -> void
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_suite_result_ != suite_results_.end());

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

					// todo
					constexpr auto calc_result_of_test = functional::y_combinator
					{
							[](auto&& self, const test_result_type& test_result) noexcept -> total_result
							{
								const auto passed = static_cast<std::size_t>(
									+(
										test_result.status == test_result_type::Status::PASSED
									)
								);
								const auto failed = static_cast<std::size_t>(
									+(
										test_result.status == test_result_type::Status::FAILED or
										test_result.status == test_result_type::Status::INTERRUPTED or
										test_result.status == test_result_type::Status::TERMINATED
									)
								);
								const auto skipped = static_cast<std::size_t>(
									+(
										test_result.status == test_result_type::Status::SKIPPED_NO_ASSERTION or
										test_result.status == test_result_type::Status::SKIPPED_FILTERED
									)
								);

								return std::ranges::fold_left(
									test_result.children,
									total_result
									{
											.test_passed = passed,
											.test_failed = failed,
											.test_skipped = skipped,
											.assertion_passed = test_result.total_assertions_passed,
											.assertion_failed = test_result.total_assertions_failed
									},
									[self](const total_result& total, const test_result_type& nested_test_result) noexcept -> total_result
									{
										return total + self(nested_test_result);
									}
								);
							}
					};

					constexpr auto calc_result_of_suite = [calc_result_of_test](const suite_result_type& suite_result) noexcept -> total_result
					{
						return std::ranges::fold_left(
							suite_result.test_results,
							total_result
							{
									.test_passed = 0,
									.test_failed = 0,
									.test_skipped = 0,
									.assertion_passed = 0,
									.assertion_failed = 0
							},
							[calc_result_of_test](const total_result& total, const test_result_type& test_result) noexcept -> total_result
							{
								// todo: 
								//   \src\infrastructure\unit_test.ixx(1624,26): error : function 'operator()<const gal::prometheus::unit_test::test_result_type &>' with deduced return type cannot be used before it is defined
								//    1624 |                                                                                 return total + self(nested_test_result);
								//         |                                                                                                ^
								//   \src\infrastructure\unit_test.ixx(1622,10): note: while substituting into a lambda expression here
								//    1622 |                                                                      [self](const total_result& total, const test_result_type& nested_test_result) noexcept -> total_result
								//         |                                                                         ^
								//   C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.41.33923\include\type_traits(1701,16): note: in instantiation of function template specialization 'gal::prometheus::unit_test::executor::Executor::on(const events::EventSummary &)::(anonymous class)::operator()<gal::prometheus::functional::y_combinator<(lambda at \src\infrastructure\unit_test.ixx:1607:8)>>' requested here
								//    1701 |         return static_cast<_Callable&&>(_Obj)(static_cast<_Ty1&&>(_Arg1), static_cast<_Types2&&>(_Args2)...);
								//         |                       ^
								//   \src\functional\functor.ixx(99,16): note: in instantiation of function template specialization 'std::invoke<const (lambda at \src\infrastructure\unit_test.ixx:1607:8) &, const gal::prometheus::functional::y_combinator<(lambda at \src\infrastructure\unit_test.ixx:1607:8)> &, const gal::prometheus::unit_test::test_result_type &>' requested here
								//      99 |                         return std::invoke(function, *this, std::forward<Args>(args)...);
								//         |                                           ^
								//   \src\infrastructure\unit_test.ixx(1643,43): note: in instantiation of function template specialization 'gal::prometheus::functional::y_combinator<(lambda at \src\infrastructure\unit_test.ixx:1607:8)>::operator()<const gal::prometheus::unit_test::test_result_type &>' requested here
								//    1643 |                                                                 return total + calc_result_of_test(test_result);
								//         |                                                                                                                        ^
								// clang 17.0.1: OK
								// clang 17.0.3: ERROR
								// clang trunk: ERROR
								return total + calc_result_of_test(test_result);
							}
						);
					};

					std::ranges::for_each(
						suite_results_,
						[&color = config_.color, c = config_, calc_result_of_suite](suite_result_type& suite_result) noexcept -> void
						{
							// ReSharper disable once CppUseStructuredBinding
							if (const auto result = calc_result_of_suite(suite_result);
								result.assertion_failed == 0)
							[[likely]]
							{
								if (result.assertion_passed == 0)
								{
									// skip empty suite report
								}
								else
								{
									std::format_to(
										std::back_inserter(suite_result.report_string),
										"\n==========================================\n"
										"Suite {}{}{} -> {}all tests passed{}({} assertions in {} tests), {} tests skipped."
										"\n==========================================\n",
										color.suite,
										suite_result.name,
										color.none,
										color.pass,
										color.none,
										result.assertion_passed,
										result.test_passed,
										result.test_skipped
									);
								}
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
									color.none
								);
							}

							c.report_message(suite_result.report_string);
						}
					);
				}

			public:
				Executor(const Executor&) noexcept = delete;
				Executor(Executor&&) noexcept = delete;
				auto operator=(const Executor&) noexcept -> Executor& = delete;
				auto operator=(Executor&&) noexcept -> Executor& = delete;

				explicit Executor() noexcept
					:
					current_suite_result_{suite_results_.end()},
					current_test_result_{nullptr},
					total_fails_exclude_current_test_{0},
					is_executor_fatal_error_{false},
					pad_1_{0},
					pad_2_{0}
				{
					// we chose to construct a temporary object here to avoid possible errors, and trust that the optimizer will forgive us ;)
					auto t = suite_result_type
					{
							.name = std::string{anonymous_suite_name},
							.report_string = {},
							.test_results = {}
					};

					suite_results_.emplace_back(std::move(t));
					current_suite_result_ = suite_results_.begin();
				}

				~Executor() noexcept
				{
					if (not config_.dry_run)
					{
						// fixme: multi-thread invoke
						auto it = suites_.begin();
						do
						{
							this->on(*it, internal_tag{});
							++it;
						} while (not is_executor_fatal_error() and it != suites_.end());

						on(events::EventSummary{});
					}
				}

				[[nodiscard]] auto config() noexcept -> config_type&
				{
					return config_;
				}

				// =========================================
				// SUITE
				// =========================================

				auto on(const events::EventSuite& suite) noexcept -> void
				{
					// @see ~Executor
					suites_.emplace_back(suite);
				}

				// =========================================
				// TEST
				// =========================================

				template<typename InvocableType, typename Arg>
				auto on(const events::EventTest<InvocableType, Arg>& test) noexcept -> void
				{
					this->on(test, internal_tag{});
				}

				// =========================================
				// ASSERTION
				// =========================================

				template<expression_t Expression>
				auto on(const events::EventAssertion<Expression>& assertion) noexcept(false) -> bool
				{
					return this->on(assertion, internal_tag{});
				}

				auto on(const events::EventAssertionFatal& assertion_fatal) noexcept(false) -> void
				{
					on(assertion_fatal, internal_tag{});
				}

				// =========================================
				// LOG
				// =========================================

				template<typename MessageType>
				auto on(const events::EventLog<MessageType>& log) noexcept -> void
				{
					this->on(log, internal_tag{});
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
			} // namespace detail

			template<events::event_t EventType>
			auto register_event(EventType&& event)
				noexcept(
					noexcept(
						executor::executor()
						.on(std::forward<EventType>(event))
					)
				) -> decltype(auto)
			{
				return
						executor::executor()
						.on(std::forward<EventType>(event));
			}

			struct expect_result
			{
				struct fatal {};

			private:
				template<typename T>
				struct with_location
				{
					std::source_location location;

					constexpr explicit(false) with_location(const T&, const std::source_location& l = std::source_location::current()) noexcept
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
					if (not value)
					{
						dispatcher::register_event(events::EventLog{.message = std::forward<MessageType>(message)});
					}

					return *this;
				}

				constexpr auto operator<<(const with_location<fatal>& location) noexcept(false) -> expect_result&
				{
					if (not value)
					{
						register_event(events::EventAssertionFatal{.location = location.location});
					}

					return *this;
				}
			};

			template<typename Dispatcher>
			class ExpressionDispatcher // NOLINT(bugprone-crtp-constructor-accessibility)
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

			class DispatcherExpect final
			{
			public:
				template<typename Expression>
					requires(is_expression_v<Expression> or detail::is_dispatched_expression_v<Expression>)
				constexpr auto operator()(
					Expression&& expression,
					const std::source_location& location = std::source_location::current()
				) const noexcept(false) -> expect_result
				{
					if constexpr (detail::is_dispatched_expression_v<Expression>)
					{
						// workaround if needed
						// using dispatcher_type = typename Expression::dispatcher_type;

						const auto result = dispatcher::register_event(
							events::EventAssertion<typename Expression::expression_type>{
									.expression = std::forward<Expression>(expression).expression,
									.location = location
							}
						);

						return expect_result{result};
					}
					else
					{
						return expect_result
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

			template<typename D>
			class DispatcherTestBase // NOLINT(bugprone-crtp-constructor-accessibility)
			{
			public:
				using categories_type = config_type::categories_type;

			protected:
				categories_type categories_;

			public:
				template<std::invocable InvocableType>
				constexpr auto operator=(InvocableType&& invocable) & noexcept -> DispatcherTestBase&
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
				constexpr auto operator=(InvocableType&& invocable) && noexcept -> DispatcherTestBase&
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
						else if constexpr (std::is_same_v<type, categories_type>)
						{
							#if __cpp_lib_containers_ranges >= 202202L
							categories_.append_range(std::forward<T>(t));
							#else
							categories_.insert(categories_.end(), std::ranges::begin(std::forward<T>(t)), std::ranges::end(std::forward<T>(t)));
							#endif
						}
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("unknown type"); }
					};

					(process(std::forward<Args>(args)), ...);

					return std::move(*this).do_move();
				}
			};

			template<meta::basic_fixed_string StringLiteral>
			class DispatcherTestLiteral final : public DispatcherTestBase<DispatcherTestLiteral<StringLiteral>>
			{
			public:
				using name_type = events::name_type;

				using DispatcherTestBase<DispatcherTestLiteral>::operator=;

				[[nodiscard]] constexpr static auto name() noexcept -> auto { return StringLiteral.template as<name_type>(); }
			};

			class DispatcherTest final : public DispatcherTestBase<DispatcherTest>
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
	}

	// =========================================
	// OPERANDS
	// =========================================

	#if defined(__clang__) and __clang_major__ < 19
	// #warning "error: alias template 'value' requires template arguments; argument deduction only allowed for class templates"
	#endif
	template<typename T>
	// using value = unit_test_detail::operands::OperandValue<T>;
	[[nodiscard]] constexpr auto value(T&& v) noexcept -> auto
	{
		return unit_test_detail::operands::OperandValue{std::forward<T>(v)};
	}

	// template<typename T>
	// using ref = unit_test_detail::operands::OperandValueRef<T>;
	template<typename T>
	[[nodiscard]] constexpr auto ref(T& v) noexcept -> auto
	{
		return unit_test_detail::operands::OperandValueRef{v};
	}

	template<typename T>
	[[nodiscard]] constexpr auto ref(const T& v) noexcept -> auto
	{
		return unit_test_detail::operands::OperandValueRef{v};
	}

	template<typename ExceptionType = void, std::invocable InvocableType>
	[[nodiscard]] constexpr auto throws(const InvocableType& invocable) noexcept -> unit_test_detail::operands::OperandThrow<ExceptionType>
	{
		return {invocable};
	}

	template<std::invocable InvocableType>
	[[nodiscard]] constexpr auto nothrow(const InvocableType& invocable) noexcept -> unit_test_detail::operands::OperandNoThrow
	{
		return {invocable};
	}

	// =========================================
	// DISPATCHER
	// =========================================

	// The assertion must succeed, otherwise the assertion(s) (and nested test(s)) followed (this test) will be skipped
	constexpr unit_test_detail::dispatcher::expect_result::fatal fatal{};

	constexpr unit_test_detail::dispatcher::DispatcherThat that{};

	constexpr unit_test_detail::dispatcher::DispatcherExpect expect{};

	// =========================================
	// CONFIG
	// =========================================

	[[nodiscard]] inline auto config() noexcept -> auto&
	{
		return unit_test_detail::executor::executor().config();
	}

	// =========================================
	// TEST & SUITE
	// =========================================

	using test = unit_test_detail::dispatcher::DispatcherTest;

	template<meta::basic_fixed_string SuiteName>
	struct suite
	{
		template<std::invocable InvocableType>
		constexpr explicit(false) suite(InvocableType invocable) noexcept //
			requires requires { +invocable; }
		{
			unit_test_detail::dispatcher::register_event(
				unit_test_detail::events::EventSuite{.name = SuiteName.template as<unit_test_detail::events::name_type>(), .suite = +invocable}
			);
		}
	};

	// =========================================
	// OPERATORS
	// =========================================

	inline namespace operators
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
			concept dispatchable_t =
					not(
						unit_test_detail::dispatcher::detail::is_dispatched_expression_v<Lhs> or
						unit_test_detail::dispatcher::detail::is_dispatched_expression_v<Rhs>
					);
		} // namespace detail

		// a == b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator==(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and
			         requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) == std::forward<Rhs>(rhs)); } //
		{
			return that % std::forward<Lhs>(lhs) == std::forward<Rhs>(rhs);
		}

		// a != b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator!=(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and
			         requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) != std::forward<Rhs>(rhs)); } //
		{
			return that % std::forward<Lhs>(lhs) != std::forward<Rhs>(rhs);
		}

		// a > b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator>(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and
			         requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) > std::forward<Rhs>(rhs)); } //
		{
			return that % std::forward<Lhs>(lhs) > std::forward<Rhs>(rhs);
		}

		// a >= b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator>=(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and
			         requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) >= std::forward<Rhs>(rhs)); } //
		{
			return that % std::forward<Lhs>(lhs) >= std::forward<Rhs>(rhs);
		}

		// a < b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator<(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and
			         requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) < std::forward<Rhs>(rhs)); } //
		{
			return that % std::forward<Lhs>(lhs) < std::forward<Rhs>(rhs);
		}

		// a <= b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator<=(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and
			         requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) <= std::forward<Rhs>(rhs)); } //
		{
			return that % std::forward<Lhs>(lhs) <= std::forward<Rhs>(rhs);
		}

		// todo: It doesn't look like we can take over [operator and] and [operator or] :(

		// a and b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator and(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and
			         requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) and std::forward<Rhs>(rhs)); } //
		{
			return that % std::forward<Lhs>(lhs) and std::forward<Rhs>(rhs);
		}

		// a or b
		template<typename Lhs, typename Rhs>
		[[nodiscard]] constexpr auto operator or(Lhs&& lhs, Rhs&& rhs) noexcept -> decltype(auto) //
			requires detail::dispatchable_t<Lhs, Rhs> and
			         requires { detail::is_valid_dispatched_expression(that % std::forward<Lhs>(lhs) or std::forward<Rhs>(rhs)); } //
		{
			return that % std::forward<Lhs>(lhs) or std::forward<Rhs>(rhs);
		}
	} // namespace operators

	// =========================================
	// LITERALS
	// =========================================

	inline namespace literals
	{
		template<meta::basic_fixed_string StringLiteral>
		[[nodiscard]] constexpr auto operator""_test() noexcept -> unit_test_detail::dispatcher::DispatcherTestLiteral<StringLiteral> //
		{
			return unit_test_detail::dispatcher::DispatcherTestLiteral<StringLiteral>{};
		}

		template<char... Cs>
		[[nodiscard]] constexpr auto operator""_auto() noexcept -> unit_test_detail::operands::OperandLiteralAuto<Cs...> //
		{
			return {};
		}

		template<meta::basic_fixed_string StringLiteral>
		[[nodiscard]] constexpr auto operator""_c() noexcept -> unit_test_detail::operands::OperandLiteralCharacter<StringLiteral.value[0]> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<int>(); }
		[[nodiscard]] constexpr auto operator""_i() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<int>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned>(); }
		[[nodiscard]] constexpr auto operator""_u() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<unsigned>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<long>(); }
		[[nodiscard]] constexpr auto operator""_l() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned long>(); }
		[[nodiscard]] constexpr auto operator""_ul() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<unsigned long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<long long>(); }
		[[nodiscard]] constexpr auto operator""_ll() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<long long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned long long>(); }
		[[nodiscard]] constexpr auto operator""_ull() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<unsigned long long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int8_t>(); }
		[[nodiscard]] constexpr auto operator""_i8() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::int8_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint8_t>(); }
		[[nodiscard]] constexpr auto operator""_u8() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::uint8_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int16_t>(); }
		[[nodiscard]] constexpr auto operator""_i16() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::int16_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint16_t>(); }
		[[nodiscard]] constexpr auto operator""_u16() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::uint16_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int32_t>(); }
		[[nodiscard]] constexpr auto operator""_i32() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::int32_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint32_t>(); }
		[[nodiscard]] constexpr auto operator""_u32() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::uint32_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int64_t>(); }
		[[nodiscard]] constexpr auto operator""_i64() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::int64_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint64_t>(); }
		[[nodiscard]] constexpr auto operator""_u64() noexcept
			-> unit_test_detail::operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::uint64_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<float>(); }
		[[nodiscard]] constexpr auto operator""_f() noexcept
			-> unit_test_detail::operands::OperandLiteralFloatingPoint<
				functional::char_list<Cs...>.template to_floating_point<float>(),
				functional::char_list<Cs...>.denominator_length()
			> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<double>(); }
		[[nodiscard]] constexpr auto operator""_d() noexcept
			-> unit_test_detail::operands::OperandLiteralFloatingPoint<
				functional::char_list<Cs...>.template to_floating_point<double>(),
				functional::char_list<Cs...>.denominator_length()
			> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<long double>(); }
		[[nodiscard]] constexpr auto operator""_ld() noexcept
			-> unit_test_detail::operands::OperandLiteralFloatingPoint<
				functional::char_list<Cs...>.template to_floating_point<long double>(),
				functional::char_list<Cs...>.denominator_length()
			> { return {}; }

		// We can't construct OperandIdentityBoolean directly here, because we don't know the result of the comparison yet.
		[[nodiscard]] constexpr auto operator""_b(const char* name, const std::size_t size) noexcept -> unit_test_detail::operands::OperandIdentityBoolean::value_type //
		{
			return {.string = {name, size}};
		}

		[[nodiscard]] constexpr auto operator""_s(const char* name, const std::size_t size) noexcept -> unit_test_detail::operands::OperandIdentityString //
		{
			const unit_test_detail::operands::OperandIdentityString::value_type value{.string = {name, size}};
			return unit_test_detail::operands::OperandIdentityString{value};
		}
	} // namespace literals
}
