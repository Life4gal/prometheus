// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// unit_test module, inspired by boost-ext/ut(https://github.com/boost-ext/ut)(http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <unit_test/def.hpp>
#include <unit_test/events.hpp>
#include <unit_test/operands.hpp>
#include <unit_test/executor.hpp>
#include <unit_test/dispatcher.hpp>

namespace gal::prometheus::unit_test
{
	// =========================================
	// OPERANDS
	// =========================================

	template<typename T>
	[[nodiscard]] constexpr auto value(T&& v) noexcept -> auto
	{
		return operands::OperandValue{std::forward<T>(v)};
	}

	template<typename T>
	[[nodiscard]] constexpr auto ref(T& v) noexcept -> auto
	{
		return operands::OperandValueRef{v};
	}

	template<typename T>
	[[nodiscard]] constexpr auto ref(const T& v) noexcept -> auto
	{
		return operands::OperandValueRef{v};
	}

	template<typename ExceptionType = void, std::invocable InvocableType>
	[[nodiscard]] constexpr auto throws(const InvocableType& invocable) noexcept -> operands::OperandThrow<ExceptionType>
	{
		return {invocable};
	}

	template<std::invocable InvocableType>
	[[nodiscard]] constexpr auto nothrow(const InvocableType& invocable) noexcept -> operands::OperandNoThrow
	{
		return {invocable};
	}

	// =========================================
	// DISPATCHER
	// =========================================

	constexpr dispatcher::DispatcherThat that{};

	constexpr dispatcher::DispatcherExpect expect{};
	// The assertion must succeed, otherwise the assertion(s) (and nested test(s)) followed (this test) will be skipped
	constexpr dispatcher::ExpectResult::fatal fatal{};

	// =========================================
	// CONFIG
	// =========================================

	inline auto set_config(config_type&& config) noexcept -> void
	{
		return executor::Executor::instance().set_config(std::move(config));
	}

	// =========================================
	// SUITE & TEST
	// =========================================

	template<meta::basic_fixed_string SuiteName>
	using suite = dispatcher::DispatcherSuite<SuiteName>;

	using test = dispatcher::DispatcherTest;

	// =========================================
	// OPERATORS
	// =========================================

	inline namespace operators
	{
		namespace detail
		{
			template<typename Lhs, typename Rhs>
			concept dispatchable_t =
					not(
						dispatcher::is_dispatched_expression_v<Lhs> or
						dispatcher::is_dispatched_expression_v<Rhs>
					);

			template<typename DispatchedExpression>
			// ReSharper disable once CppFunctionIsNotImplemented
			constexpr auto is_valid_dispatched_expression(DispatchedExpression&& expression) noexcept -> void //
				requires requires
				{
					static_cast<bool>(expression.expression);
				};
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
		[[nodiscard]] constexpr auto operator""_test() noexcept -> dispatcher::DispatcherTestLiteral<StringLiteral> //
		{
			return dispatcher::DispatcherTestLiteral<StringLiteral>{};
		}

		template<char... Cs>
		[[nodiscard]] constexpr auto operator""_auto() noexcept -> operands::OperandLiteralAuto<Cs...> //
		{
			return {};
		}

		template<meta::basic_fixed_string StringLiteral>
		[[nodiscard]] constexpr auto operator""_c() noexcept -> operands::OperandLiteralCharacter<StringLiteral.value[0]> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<int>(); }
		[[nodiscard]] constexpr auto operator""_i() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<int>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned>(); }
		[[nodiscard]] constexpr auto operator""_u() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<unsigned>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<long>(); }
		[[nodiscard]] constexpr auto operator""_l() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned long>(); }
		[[nodiscard]] constexpr auto operator""_ul() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<unsigned long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<long long>(); }
		[[nodiscard]] constexpr auto operator""_ll() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<long long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<unsigned long long>(); }
		[[nodiscard]] constexpr auto operator""_ull() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<unsigned long long>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int8_t>(); }
		[[nodiscard]] constexpr auto operator""_i8() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::int8_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint8_t>(); }
		[[nodiscard]] constexpr auto operator""_u8() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::uint8_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int16_t>(); }
		[[nodiscard]] constexpr auto operator""_i16() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::int16_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint16_t>(); }
		[[nodiscard]] constexpr auto operator""_u16() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::uint16_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int32_t>(); }
		[[nodiscard]] constexpr auto operator""_i32() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::int32_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint32_t>(); }
		[[nodiscard]] constexpr auto operator""_u32() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::uint32_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::int64_t>(); }
		[[nodiscard]] constexpr auto operator""_i64() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::int64_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_integral<std::uint64_t>(); }
		[[nodiscard]] constexpr auto operator""_u64() noexcept
			-> operands::OperandLiteralIntegral<functional::char_list<Cs...>.template to_integral<std::uint64_t>()> //
		{
			return {};
		}

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<float>(); }
		[[nodiscard]] constexpr auto operator""_f() noexcept
			-> operands::OperandLiteralFloatingPoint<
				functional::char_list<Cs...>.template to_floating_point<float>(),
				functional::char_list<Cs...>.denominator_length()
			> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<double>(); }
		[[nodiscard]] constexpr auto operator""_d() noexcept
			-> operands::OperandLiteralFloatingPoint<
				functional::char_list<Cs...>.template to_floating_point<double>(),
				functional::char_list<Cs...>.denominator_length()
			> { return {}; }

		template<char... Cs>
			requires requires { functional::char_list<Cs...>.template to_floating_point<long double>(); }
		[[nodiscard]] constexpr auto operator""_ld() noexcept
			-> operands::OperandLiteralFloatingPoint<
				functional::char_list<Cs...>.template to_floating_point<long double>(),
				functional::char_list<Cs...>.denominator_length()
			> { return {}; }

		// We can't construct OperandIdentityBoolean directly here, because we don't know the result of the comparison yet.
		[[nodiscard]] constexpr auto operator""_b(const char* name, const std::size_t size) noexcept -> operands::OperandIdentityBoolean::value_type //
		{
			return {.string = {name, size}};
		}

		[[nodiscard]] constexpr auto operator""_s(const char* name, const std::size_t size) noexcept -> operands::OperandIdentityString //
		{
			const operands::OperandIdentityString::value_type value{.string = {name, size}};
			return operands::OperandIdentityString{value};
		}
	} // namespace literals
}
