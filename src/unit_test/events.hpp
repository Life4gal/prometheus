// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <source_location>

#include <prometheus/macro.hpp>

#include <unit_test/def.hpp>

namespace gal::prometheus::unit_test::events
{
	class Event {};

	template<typename E>
	constexpr auto is_event_v = std::is_base_of_v<Event, E>;
	template<typename E>
	concept event_t = is_event_v<E>;

	#if defined(GAL_PROMETHEUS_COMPILER_GNU) or defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
	GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH

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
		suite_name_view_type name;
	};

	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSuiteEnd final : public Event
	{
	public:
		suite_name_view_type name;
	};

	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventSuite final : public Event
	{
	public:
		using suite_type = void (*)();

		suite_name_view_type name;
		suite_type suite;

		// throwable: end suite
		constexpr auto operator()() noexcept(false) -> void
		{
			std::invoke(suite);
		}

		// throwable: end suite
		constexpr auto operator()() const noexcept(false) -> void
		{
			std::invoke(suite);
		}

	private:
		[[nodiscard]] constexpr explicit operator EventSuiteBegin() const noexcept
		{
			return {.name = name};
		}

		[[nodiscard]] constexpr explicit operator EventSuiteEnd() const noexcept
		{
			return {.name = name};
		}

	public:
		[[nodiscard]] constexpr auto begin() const noexcept -> EventSuiteBegin
		{
			return this->operator EventSuiteBegin();
		}

		[[nodiscard]] constexpr auto end() const noexcept -> EventSuiteEnd
		{
			return this->operator EventSuiteEnd();
		}
	};

	// =========================================
	// TEST
	// =========================================

	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTestBegin final : public Event
	{
	public:
		test_name_view_type name;
	};

	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTestSkip final : public Event
	{
	public:
		test_name_view_type name;
	};

	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTestEnd final : public Event
	{
	public:
		test_name_view_type name;
	};

	struct none {};

	template<typename InvocableType, typename Arg = none>
		requires std::is_invocable_v<InvocableType> or std::is_invocable_v<InvocableType, Arg>
	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE EventTest final : public Event
	{
	public:
		using invocable_type = InvocableType;
		using arg_type = Arg;

		test_name_view_type name;
		test_categories_type categories;

		mutable invocable_type invocable;
		mutable arg_type arg;

		// throwable: end test
		constexpr auto operator()() const noexcept(false) -> void
		{
			return []<typename I, typename A>(I&& i, A&& a) noexcept(false) -> void
			{
				if constexpr (requires { std::invoke(std::forward<I>(i)); })
				{
					std::invoke(std::forward<I>(i));
				}
				else if constexpr (requires { std::invoke(std::forward<I>(i), std::forward<A>(a)); })
				{
					std::invoke(std::forward<I>(i), std::forward<A>(a));
				}
				else if constexpr (requires { std::invoke(i.template operator()<A>()); })
				{
					std::invoke(i.template operator()<A>());
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			}(invocable, arg);
		}

	private:
		[[nodiscard]] constexpr explicit operator EventTestBegin() const noexcept
		{
			return {.name = name};
		}

		[[nodiscard]] constexpr explicit operator EventTestEnd() const noexcept
		{
			return {.name = name};
		}

		[[nodiscard]] constexpr explicit operator EventTestSkip() const noexcept
		{
			return {.name = name};
		}

	public:
		[[nodiscard]] constexpr auto begin() const noexcept -> EventTestBegin
		{
			return this->operator EventTestBegin();
		}

		[[nodiscard]] constexpr auto end() const noexcept -> EventTestEnd
		{
			return this->operator EventTestEnd();
		}

		[[nodiscard]] constexpr auto skip() const noexcept -> EventTestSkip
		{
			return this->operator EventTestSkip();
		}
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

	#if defined(GAL_PROMETHEUS_COMPILER_GNU) or defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG)
	GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH
	#endif
}
