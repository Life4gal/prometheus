// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.error:exception;

import std;

#else
#pragma once

#include <string>
#include <source_location>
#include <stacktrace>

#include <prometheus/macro.hpp>
#endif

// todo
#if __cpp_lib_stacktrace < 202011L
#warning "fixme: std::stacktrace"
namespace std
{
	struct stacktrace
	{
		[[nodiscard]] static auto current() -> stacktrace
		{
			return {};
		}
	};
}
#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::error)
{
	class AbstractException
	{
	public:
		constexpr AbstractException() noexcept = default;
		constexpr AbstractException(const AbstractException&) noexcept = default;
		constexpr AbstractException(AbstractException&&) noexcept = default;
		constexpr auto operator=(const AbstractException&) noexcept -> AbstractException& = default;
		constexpr auto operator=(AbstractException&&) noexcept -> AbstractException& = default;
		constexpr virtual ~AbstractException() noexcept = default;

		[[nodiscard]] constexpr virtual auto what() const noexcept -> const std::string& = 0;

		[[nodiscard]] constexpr virtual auto where() const noexcept -> const std::source_location& = 0;

		[[nodiscard]] constexpr virtual auto when() const noexcept -> const std::stacktrace& = 0;
	};

	template<typename T>
	// ReSharper disable once CppClassCanBeFinal
	class Exception : public AbstractException
	{
	public:
		using data_type = T;

	private:
		std::string message_;
		std::source_location location_;
		std::stacktrace stacktrace_;
		data_type data_;

	public:
		template<typename StringType, typename DataType>
		constexpr Exception(
			StringType&& message,
			DataType&& data,
			const std::source_location location,
			std::stacktrace&& stacktrace
		) noexcept
			: AbstractException{},
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
	class Exception<void> : public AbstractException
	{
	public:
		using data_type = void;

	private:
		std::string message_;
		std::source_location location_;
		std::stacktrace stacktrace_;

	public:
		template<typename StringType>
		constexpr Exception(
			StringType&& message,
			const std::source_location location,
			std::stacktrace&& stacktrace
		) noexcept
			: AbstractException{},
			  message_{std::forward<StringType>(message)},
			  location_{location},
			  stacktrace_{std::move(stacktrace)} {}

		[[nodiscard]] constexpr auto what() const noexcept -> const std::string& override { return message_; }

		[[nodiscard]] constexpr auto where() const noexcept -> const std::source_location& override { return location_; }

		[[nodiscard]] constexpr auto when() const noexcept -> const std::stacktrace& override { return stacktrace_; }
	};

	template<typename ExceptionType, typename StringType, typename DataType>
		requires std::derived_from<ExceptionType, Exception<DataType>>
	[[noreturn]] constexpr auto panic(
		StringType&& message,
		DataType&& data,
		const std::source_location& location = std::source_location::current(),
		std::stacktrace stacktrace = std::stacktrace::current()) noexcept(false) -> ExceptionType //
	{
		throw ExceptionType{
				std::forward<StringType>(message),
				std::forward<DataType>(data),
				location,
				std::move(stacktrace)
		};
	}

	template<typename ExceptionType, typename StringType>
		requires std::derived_from<ExceptionType, Exception<void>>
	[[noreturn]] constexpr auto panic(
		StringType&& message,
		const std::source_location& location = std::source_location::current(),
		std::stacktrace stacktrace = std::stacktrace::current()
	) noexcept(false) -> ExceptionType //
	{
		throw ExceptionType{
				std::forward<StringType>(message),
				location,
				std::move(stacktrace)
		};
	}

	template<bool>
	struct panic_selector;

	template<>
	struct panic_selector<true>
	{
		template<typename E, typename StringType>
			requires (std::derived_from<E, Exception<typename E::data_type>> and std::is_same_v<typename E::data_type, void>)
		[[noreturn]] constexpr static auto invoke(
			StringType&& message,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> E
		{
			E::panic(
				std::forward<StringType>(message),
				location,
				std::move(stacktrace)
			);
			GAL_PROMETHEUS_ERROR_UNREACHABLE();
		}

		template<typename E, typename StringType>
			requires (std::derived_from<E, Exception<typename E::data_type>> and not std::is_same_v<typename E::data_type, void>)
		[[noreturn]] constexpr static auto invoke(
			StringType&& message,
			typename E::data_type&& data,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> E
		{
			E::panic(
				std::forward<StringType>(message),
				std::forward<typename E::data_type>(data),
				location,
				std::move(stacktrace)
			);
			GAL_PROMETHEUS_ERROR_UNREACHABLE();
		}
	};

	template<>
	struct panic_selector<false>
	{
		template<typename E, typename StringType>
			requires (std::derived_from<E, Exception<typename E::data_type>> and std::is_same_v<typename E::data_type, void>)
		[[noreturn]] constexpr static auto invoke(
			StringType&& message,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> E
		{
			error::panic<E>(
				std::forward<StringType>(message),
				location,
				std::move(stacktrace)
			);
		}

		template<typename E, typename StringType>
			requires (std::derived_from<E, Exception<typename E::data_type>> and not std::is_same_v<typename E::data_type, void>)
		[[noreturn]] constexpr static auto invoke(
			StringType&& message,
			typename E::data_type&& data,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> E
		{
			error::panic<E>(
				std::forward<StringType>(message),
				std::forward<typename E::data_type>(data),
				location,
				std::move(stacktrace)
			);
		}
	};

	template<typename E>
	using mob = panic_selector<
		requires { E::panic(std::declval<std::string>()); } or
		requires { E::panic(std::declval<std::string>(), std::declval<typename E::data_type>()); }
	>;
} // namespace gal::prometheus::error
