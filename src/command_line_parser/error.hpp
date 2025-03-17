// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <source_location>
#include <stacktrace>
#include <format>

#include <platform/exception.hpp>

namespace gal::prometheus::clp
{
	class CommandLineOptionNameFormatError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionNameFormatError>(
				std::format("Cannot parse `{}` as option name", option),
				location,
				std::move(stacktrace)
			);
		}
	};

	class CommandLineOptionAlreadyExistsError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionAlreadyExistsError>(
				std::format("Option `{}` already exists!", option),
				location,
				std::move(stacktrace)
			);
		}
	};

	class CommandLineOptionUnrecognizedError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionUnrecognizedError>(
				std::format("Unrecognized option:\n {}", option),
				location,
				std::move(stacktrace)
			);
		}
	};

	class CommandLineOptionRequiredNotPresentError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionRequiredNotPresentError>(
				std::format("Required option `{}` not present", option),
				location,
				std::move(stacktrace)
			);
		}
	};

	class CommandLineOptionRequiredNotSetError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionRequiredNotSetError>(
				std::format("Required option `{}` not set and no default value present", option),
				location,
				std::move(stacktrace)
			);
		}
	};
}
