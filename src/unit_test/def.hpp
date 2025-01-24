// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <memory>
#include <functional>
#include <print>
#include <algorithm>

namespace gal::prometheus::unit_test
{
	// =====================================
	// TIME
	// =====================================

	using clock_type = std::chrono::high_resolution_clock;
	using time_point_type = clock_type::time_point;
	using time_difference_type = std::chrono::milliseconds;

	struct time_range_type
	{
		time_point_type start;
		time_point_type end;

		time_range_type() noexcept
			: start{clock_type::now()} {}

		[[nodiscard]] auto count() noexcept -> time_point_type::rep
		{
			end = clock_type::now();
			return std::chrono::duration_cast<time_difference_type>(end - start).count();
		}
	};

	// =====================================
	// FILTER
	// =====================================

	using suite_name_type = std::string;
	using suite_name_view_type = std::string_view;
	using test_name_type = std::string;
	using test_name_view_type = std::string_view;
	using test_category_type = std::string;
	using test_category_view_type = std::string_view;
	using test_categories_type = std::vector<test_category_type>;
	using test_categories_view_type = std::reference_wrapper<const test_categories_type>;

	// nested suite is not allowed
	struct suite_node_type
	{
		suite_name_view_type name;
	};

	// ROOT_TEST->NESTED_TEST->NESTED_TEST
	struct test_node_type
	{
		// parent == nullptr ==> ROOT
		std::unique_ptr<test_node_type> parent;

		test_name_view_type name;
		test_categories_view_type categories;
	};

	using suite_filter_type = std::function<bool(suite_node_type)>;
	using test_filter_type = std::function<bool(test_node_type)>;

	// =====================================
	// RESULT
	// =====================================

	using report_string_type = std::string;

	struct test_result_type;
	using test_results_type = std::vector<test_result_type>;

	struct suite_result_type
	{
		suite_name_type name;
		test_results_type results;

		report_string_type report_string;
	};

	using suite_results_type = std::vector<suite_result_type>;

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

		test_name_type name;

		test_result_type* parent;
		test_results_type children;

		std::size_t total_assertions_passed;
		std::size_t total_assertions_failed;

		time_range_type time;
		Status status;
	};

	// =====================================
	// CONFIG
	// =====================================

	struct config_type
	{
		struct color_type
		{
			std::string_view none = "\033[0m";

			std::string_view failure = "\033[31m\033[7m";
			std::string_view pass = "\033[32m\033[7m";
			std::string_view skip = "\033[33m\033[7m";
			std::string_view fatal = "\033[35m\033[7m";

			std::string_view suite = "\033[34m\033[7m";
			std::string_view test = "\033[36m\033[7m";
			std::string_view expression = "\033[38;5;207m\033[7m";
			std::string_view message = "\033[38;5;27m\033[7m";
		};

		enum class ReportLevel : std::uint16_t
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

		enum class BreakPointLevel : std::uint8_t
		{
			// for functional::enumeration
			PROMETHEUS_MAGIC_ENUM_FLAG = 0b0000'0000,

			FATAL = 0b0000'0001,
			FAILURE = 0b0000'0010,

			NONE = 0b0001'0000,
		};

		// =====================================
		// OUTPUT
		// =====================================

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

		std::function<void(suite_results_type&&)> out = [](const suite_results_type& results) noexcept -> void
		{
			std::ranges::for_each(
				results,
				[](const suite_result_type& result) noexcept -> void
				{
					std::println(stdout, "{}", result.report_string);
				}
			);
		};

		ReportLevel report_level = ReportLevel::DEFAULT;

		// =====================================
		// RUN
		// =====================================

		bool dry_run = false;

		BreakPointLevel break_point_level = BreakPointLevel::NONE;

		std::size_t abort_after_n_failures = std::numeric_limits<std::size_t>::max();

		// =====================================
		// FILTER
		// =====================================

		std::function<bool(const suite_node_type&)> filter_suite = [](const suite_node_type& node) noexcept -> bool
		{
			std::ignore = node;
			return true;
		};

		std::function<bool(const test_node_type&)> filter_test = [](const test_node_type& node) noexcept -> bool
		{
			return not std::ranges::contains(node.categories.get(), "skip");
		};
	};

	// =====================================
	// EXPRESSION
	// =====================================

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
}
