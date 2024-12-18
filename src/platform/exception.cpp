#include <print>
#include <format>

#include <prometheus/macro.hpp>

#include <platform/exception.hpp>

namespace gal::prometheus::platform
{
	auto IException::print() const noexcept -> void
	{
		const auto& message = what();
		const auto& location = where();
		const auto& stacktrace = when();

		std::println(
			stderr,
			"Error occurs while invoke function:\n{}\nat {}:{}\nReason:\n{}\nStack trace:\n{}",
			location.function_name(),
			location.file_name(),
			location.line(),
			message,
			stacktrace
		);
	}
}
