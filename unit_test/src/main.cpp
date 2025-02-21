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
			.add_option("call-debugger-if-fatal", option_type::default_value("true"))
			//
			.add_alias("x-fail", "call-debugger-if-fail")
			.add_alias("x-fatal", "call-debugger-if-fatal")
			//
			;

	unit_test::config_type config{};

	try
	{
		parser.parse();
	}
	catch (const platform::IException& exception)
	{
		exception.print();
		config.dry_run = true;
		return 1;
	}

	const auto tab_width = parser["tab-width"].as<std::size_t>();
	const auto max_failures = parser["max-failures"].as<std::size_t>();
	const auto dry_run = parser["dry-run"].as<bool>();
	const auto call_debugger_if_fail = parser["call-debugger-if-fail"].as<bool>();
	const auto call_debugger_if_fatal = parser["call-debugger-if-fatal"].as<bool>();

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
	if (call_debugger_if_fail.has_value())
	{
		if (*call_debugger_if_fail)
		{
			config.break_point_level |= unit_test::config_type::BreakPointLevel::FAILURE;
		}
		else
		{
			config.break_point_level &= ~unit_test::config_type::BreakPointLevel::FAILURE;
		}
	}
	if (call_debugger_if_fatal.has_value())
	{
		if (*call_debugger_if_fatal)
		{
			config.break_point_level |= unit_test::config_type::BreakPointLevel::FATAL;
		}
		else
		{
			config.break_point_level &= ~unit_test::config_type::BreakPointLevel::FATAL;
		}
	}

	if (const auto& option = parser["exec-suite-name"];
		option.set())
	{
		if (const auto wildcard = option.as<std::string_view>();
			wildcard.has_value() and *wildcard == "*")
		{
			config.filter_suite = [](const unit_test::suite_node_type& node) noexcept -> bool
			{
				std::ignore = node;
				return true;
			};
		}
		else if (auto suite_names = option.as<std::vector<std::string>>();
			suite_names.has_value())
		{
			config.filter_suite =
					[suite = std::move(*suite_names)](const unit_test::suite_node_type& node) noexcept -> bool
					{
						return std::ranges::contains(suite, node.name);
					};
		}
	}

	if (const auto& option = parser["exec-test-name"];
		option.set())
	{
		if (const auto wildcard = option.as<std::string_view>();
			wildcard.has_value() and *wildcard == "*")
		{
			config.filter_test = [](const unit_test::test_node_type& node) noexcept -> bool
			{
				std::ignore = node;
				return true;
			};
		}
		else if (auto test_names = option.as<std::vector<std::string>>();
			test_names.has_value())
		{
			config.filter_test =
					[test = std::move(*test_names)](const unit_test::test_node_type& node) noexcept -> bool
					{
						if (node.parent != nullptr)
						{
							// child
							return true;
						}

						return std::ranges::contains(test, node.name) and not std::ranges::contains(node.categories.get(), "skip");
					};
		}
	}

	set_config(std::move(config));

	return 0;
}
