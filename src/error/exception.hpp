// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.error:exception;

import std;

#else
#include <source_location>
#include <stacktrace>

#include <prometheus/macro.hpp>
#endif

namespace gal::prometheus::error
{
	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	class Exception
	{
	public:
		constexpr         Exception() noexcept                               = default;
		constexpr         Exception(const Exception&) noexcept               = default;
		constexpr         Exception(Exception&&) noexcept                    = default;
		constexpr auto    operator=(const Exception&) noexcept -> Exception& = default;
		constexpr auto    operator=(Exception&&) noexcept -> Exception&      = default;
		constexpr virtual ~Exception() noexcept                              = default;

		[[nodiscard]] constexpr virtual auto what() const noexcept -> const std::string& = 0;

		[[nodiscard]] constexpr virtual auto where() const noexcept -> const std::source_location& = 0;

		[[nodiscard]] constexpr virtual auto when() const noexcept -> const std::stacktrace& = 0;
	};

	template<typename T>
	// ReSharper disable once CppClassCanBeFinal
	class ExceptionWithUserData : public Exception
	{
	public:
		using data_type = T;

	private:
		std::string          message_;
		std::source_location location_;
		std::stacktrace      stacktrace_;
		data_type            data_;

	public:
		template<typename StringType, typename DataType>
		constexpr ExceptionWithUserData(StringType&& message, DataType&& data, const std::source_location location, std::stacktrace&& stacktrace) noexcept
			: Exception{},
			  message_{std::forward<StringType>(message)},
			  location_{location},
			  stacktrace_{std::move(stacktrace)},
			  data_{std::forward<DataType>(data)} {}

		[[nodiscard]] constexpr auto what() const noexcept -> const std::string& override { return message_; }

		[[nodiscard]] constexpr auto where() const noexcept -> const std::source_location& override { return location_; }

		[[nodiscard]] constexpr auto when() const noexcept -> const std::stacktrace& override { return stacktrace_; }

		[[nodiscard]] constexpr auto data() const noexcept -> data_type& { return data_; }
	};

	template<>
	// ReSharper disable once CppClassCanBeFinal
	class ExceptionWithUserData<void> : public Exception
	{
	public:
	private:
		std::string          message_;
		std::source_location location_;
		std::stacktrace      stacktrace_;

	public:
		template<typename StringType>
		constexpr ExceptionWithUserData(StringType&& message, const std::source_location location, std::stacktrace&& stacktrace) noexcept
			: Exception{},
			  message_{std::forward<StringType>(message)},
			  location_{location},
			  stacktrace_{std::move(stacktrace)} {}

		[[nodiscard]] constexpr auto what() const noexcept -> const std::string& override { return message_; }

		[[nodiscard]] constexpr auto where() const noexcept -> const std::source_location& override { return location_; }

		[[nodiscard]] constexpr auto when() const noexcept -> const std::stacktrace& override { return stacktrace_; }
	};

	template<typename ExceptionType, typename StringType, typename DataType>
		requires std::derived_from<ExceptionType, ExceptionWithUserData<DataType>>
	[[noreturn]] constexpr auto panic(
			StringType&&         message,
			DataType&&           data,
			std::source_location location   = std::source_location::current(),
			std::stacktrace      stacktrace = std::stacktrace::current()) noexcept(false) -> ExceptionType//
	{
		throw ExceptionType{std::forward<StringType>(message), std::forward<DataType>(data), location, std::move(stacktrace)};// NOLINT(hicpp-exception-baseclass)
	}

	template<typename ExceptionType, typename StringType>
		requires std::derived_from<ExceptionType, ExceptionWithUserData<void>>
	[[noreturn]] constexpr auto panic(
			StringType&&         message,
			std::source_location location   = std::source_location::current(),
			std::stacktrace      stacktrace = std::stacktrace::current()) noexcept(false) -> ExceptionType//
	{
		throw ExceptionType{std::forward<StringType>(message), location, std::move(stacktrace)};// NOLINT(hicpp-exception-baseclass)
	}

	GAL_PROMETHEUS_MODULE_EXPORT_END
}
