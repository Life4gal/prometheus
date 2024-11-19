// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <string>
#include <source_location>
#include <stacktrace>
#include <utility>

#include <prometheus/macro.hpp>

namespace gal::prometheus::platform
{
	class IException
	{
	public:
		constexpr IException() noexcept = default;
		constexpr IException(const IException&) noexcept = default;
		constexpr IException(IException&&) noexcept = default;
		constexpr auto operator=(const IException&) noexcept -> IException& = default;
		constexpr auto operator=(IException&&) noexcept -> IException& = default;
		constexpr virtual ~IException() noexcept = default;

		[[nodiscard]] constexpr virtual auto what() const noexcept -> const std::string& = 0;

		[[nodiscard]] constexpr virtual auto where() const noexcept -> const std::source_location& = 0;

		[[nodiscard]] constexpr virtual auto when() const noexcept -> const std::stacktrace& = 0;

		auto print() const noexcept -> void;
	};

	template<typename T>
	// ReSharper disable once CppClassCanBeFinal
	class Exception : public IException
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
			std::stacktrace stacktrace
		) noexcept
			: IException{},
			  message_{std::forward<StringType>(message)},
			  location_{location},
			  stacktrace_{std::move(stacktrace)},
			  data_{std::forward<DataType>(data)} {}

		[[nodiscard]] constexpr auto what() const noexcept -> const std::string& override
		{
			return message_;
		}

		[[nodiscard]] constexpr auto where() const noexcept -> const std::source_location& override
		{
			return location_;
		}

		[[nodiscard]] constexpr auto when() const noexcept -> const std::stacktrace& override
		{
			return stacktrace_;
		}

		[[nodiscard]] constexpr auto data() const noexcept -> data_type&
		{
			return data_;
		}
	};

	template<>
	// ReSharper disable once CppClassCanBeFinal
	class Exception<void> : public IException
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
			std::stacktrace stacktrace
		) noexcept
			: IException{},
			  message_{std::forward<StringType>(message)},
			  location_{location},
			  stacktrace_{std::move(stacktrace)} {}

		[[nodiscard]] constexpr auto what() const noexcept -> const std::string& override
		{
			return message_;
		}

		[[nodiscard]] constexpr auto where() const noexcept -> const std::source_location& override
		{
			return location_;
		}

		[[nodiscard]] constexpr auto when() const noexcept -> const std::stacktrace& override
		{
			return stacktrace_;
		}
	};

	template<typename ExceptionType, typename StringType, typename DataType>
		requires std::derived_from<ExceptionType, Exception<DataType>>
	[[noreturn]] constexpr auto panic(
		StringType&& message,
		DataType&& data,
		const std::source_location& location = std::source_location::current(),
		std::stacktrace stacktrace = std::stacktrace::current()
	) noexcept(false) -> ExceptionType //
	{
		throw ExceptionType // NOLINT(hicpp-exception-baseclass)
		{
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
		throw ExceptionType // NOLINT(hicpp-exception-baseclass)
		{
				std::forward<StringType>(message),
				location,
				std::move(stacktrace)
		};
	}

	namespace exception_detail
	{
		template<bool>
		struct panic_selector;

		template<>
		struct panic_selector<true>
		{
			template<typename ExceptionType, typename StringType>
				requires (
					std::derived_from<ExceptionType, Exception<typename ExceptionType::data_type>> and
					std::is_same_v<typename ExceptionType::data_type, void>
				)
			[[noreturn]] constexpr static auto invoke(
				StringType&& message,
				const std::source_location& location = std::source_location::current(),
				std::stacktrace stacktrace = std::stacktrace::current()
			) noexcept(false) -> ExceptionType
			{
				ExceptionType::panic(
					std::forward<StringType>(message),
					location,
					std::move(stacktrace)
				);
				GAL_PROMETHEUS_ERROR_UNREACHABLE();
			}

			template<typename ExceptionType, typename StringType>
				requires (
					std::derived_from<ExceptionType, Exception<typename ExceptionType::data_type>> and
					not std::is_same_v<typename ExceptionType::data_type, void>
				)
			[[noreturn]] constexpr static auto invoke(
				StringType&& message,
				typename ExceptionType::data_type data,
				const std::source_location& location = std::source_location::current(),
				std::stacktrace stacktrace = std::stacktrace::current()
			) noexcept(false) -> ExceptionType
			{
				ExceptionType::panic(
					std::forward<StringType>(message),
					std::move(data),
					location,
					std::move(stacktrace)
				);
				GAL_PROMETHEUS_ERROR_UNREACHABLE();
			}
		};

		template<>
		struct panic_selector<false>
		{
			template<typename ExceptionType, typename StringType>
				requires (
					std::derived_from<ExceptionType, Exception<typename ExceptionType::data_type>> and
					std::is_same_v<typename ExceptionType::data_type, void>
				)
			[[noreturn]] constexpr static auto invoke(
				StringType&& message,
				const std::source_location& location = std::source_location::current(),
				std::stacktrace stacktrace = std::stacktrace::current()
			) noexcept(false) -> ExceptionType
			{
				platform::panic<ExceptionType>(
					std::forward<StringType>(message),
					location,
					std::move(stacktrace)
				);
			}

			template<typename ExceptionType, typename StringType>
				requires (
					std::derived_from<ExceptionType, Exception<typename ExceptionType::data_type>> and
					not std::is_same_v<typename ExceptionType::data_type, void>
				)
			[[noreturn]] constexpr static auto invoke(
				StringType&& message,
				typename ExceptionType::data_type data,
				const std::source_location& location = std::source_location::current(),
				std::stacktrace stacktrace = std::stacktrace::current()
			) noexcept(false) -> ExceptionType
			{
				platform::panic<ExceptionType>(
					std::forward<StringType>(message),
					std::move(data),
					location,
					std::move(stacktrace)
				);
			}
		};
	}

	template<typename ExceptionType>
	using mob = exception_detail::panic_selector<
		requires { ExceptionType::panic(std::declval<std::string>()); } or
		requires { ExceptionType::panic(std::declval<std::string>(), std::declval<typename ExceptionType::data_type>()); }
	>;
}
