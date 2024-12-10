#include <command_line_parser/command_line_parser.hpp>
#include <unit_test/unit_test.hpp>

using namespace gal::prometheus;

int main(const int, char*[])
{
	using parser_type = clp::CommandLineOptionParser<>;
	using option_type = parser_type::option_type;

	clp::CommandLineOptionParser parser{};

	parser
			.add_option("tab-width", option_type::implicit_value("4"))
			.add_option("max-failures", option_type::implicit_value("100"))
			.add_option("dry,dry-run", option_type::default_value("true"))
			.add_option("exec-suite-name")
			.add_option("exec-test-name")
			.add_option("call-debugger-if-fail", option_type::default_value("false"))
			.add_option("call-debugger-if-required", option_type::default_value("true"))
			.add_option("call-debugger-if-fatal", option_type::default_value("true"))
			.add_option("call-debugger-if-exception", option_type::default_value("true"))
			//
			;
	parser
			.add_alias("x-fail", "call-debugger-if-fail")
			.add_alias("x-required", "call-debugger-if-required")
			.add_alias("x-fatal", "call-debugger-if-fatal")
			.add_alias("x-exception", "call-debugger-if-exception")
			//
			;

	parser.parse();

	const auto tab_width = parser["tab-width"].as<std::size_t>();
	const auto max_failures = parser["max-failures"].as<std::size_t>();
	const auto dry_run = parser["dry-run"].as<bool>();
	auto exec_suite_name = parser["exec-suite-name"].as<std::vector<std::string>>();
	auto exec_test_name = parser["exec-test-name"].as<std::vector<std::string>>();
	const auto call_debugger_if_fail = parser["call-debugger-if-fail"].as<bool>();
	const auto call_debugger_if_required = parser["call-debugger-if-required"].as<bool>();
	const auto call_debugger_if_fatal = parser["call-debugger-if-fatal"].as<bool>();
	const auto call_debugger_if_exception = parser["call-debugger-if-exception"].as<bool>();

	auto& config = unit_test::config();

	if (tab_width.has_value())
	{
		config.tab_width = *tab_width;
	}
	if (max_failures.has_value())
	{
		config.abort_after_n_failures = *max_failures;
	}
	if (dry_run.has_value())
	{
		config.dry_run = *dry_run;
	}
	if (exec_suite_name.has_value())
	{
		config.filter_execute_suite_name =
				[suite = std::move(*exec_suite_name)](const auto& name) noexcept -> bool
				{
					return std::ranges::contains(suite, name);
				};
	}
	if (exec_test_name.has_value())
	{
		config.filter_execute_test_name =
				[test = std::move(*exec_test_name)](const auto& name) noexcept -> bool
				{
					return std::ranges::contains(test, name);
				};
	}
	if (call_debugger_if_fail.has_value())
	{
		if (*call_debugger_if_fail)
		{
			config.debug_break_point |= unit_test::DebugBreakPoint::FAIL;
		}
		else
		{
			config.debug_break_point &= ~unit_test::DebugBreakPoint::FAIL;
		}
	}
	if (call_debugger_if_required.has_value())
	{
		if (*call_debugger_if_required)
		{
			config.debug_break_point |= unit_test::DebugBreakPoint::REQUIRED;
		}
		else
		{
			config.debug_break_point &= ~unit_test::DebugBreakPoint::REQUIRED;
		}
	}
	if (call_debugger_if_fatal.has_value())
	{
		if (*call_debugger_if_fatal)
		{
			config.debug_break_point |= unit_test::DebugBreakPoint::FATAL;
		}
		else
		{
			config.debug_break_point &= ~unit_test::DebugBreakPoint::FATAL;
		}
	}
	if (call_debugger_if_exception.has_value())
	{
		if (*call_debugger_if_exception)
		{
			config.debug_break_point |= unit_test::DebugBreakPoint::EXCEPTION;
		}
		else
		{
			config.debug_break_point &= ~unit_test::DebugBreakPoint::EXCEPTION;
		}
	}

	return 0;
}
