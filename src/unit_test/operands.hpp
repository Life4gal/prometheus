// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <concepts>
#include <functional>

#include <prometheus/macro.hpp>

#include <meta/to_string.hpp>
#include <functional/value_list.hpp>
#include <math/cmath.hpp>

#include <unit_test/def.hpp>

namespace gal::prometheus::unit_test::operands
{
	class Operand
	{
	public:
		// magic
		using prefer_no_type_name = int;
	};

	template<typename O>
	constexpr auto is_operand_v = std::is_base_of_v<Operand, O>;
	template<typename O>
	concept operand_t = is_operand_v<O>;

	// =========================================
	// VALUE / REFERENCE
	// =========================================

	template<typename T>
	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandValue final : public Operand
	{
	public:
		using value_type = T;

	private:
		value_type value_;

	public:
		constexpr explicit(false) OperandValue(const value_type value) //
			noexcept(std::is_nothrow_copy_constructible_v<value_type>) //
			requires(std::is_trivially_copy_constructible_v<value_type>)
			: value_{value} {}

		constexpr explicit(false) OperandValue(const value_type& value) //
			noexcept(std::is_nothrow_copy_constructible_v<value_type>) //
			requires(
				not std::is_trivially_copy_constructible_v<value_type> and
				std::is_copy_constructible_v<value_type>)
			: value_{value} {}

		constexpr explicit(false) OperandValue(value_type&& value) noexcept(std::is_nothrow_move_constructible_v<value_type>) //
			requires(
				not std::is_trivially_copy_constructible_v<value_type> and
				std::is_move_constructible_v<value_type>)
			: value_{std::move(value)} {}

		template<typename U>
			requires(std::is_trivially_constructible_v<value_type, U>)
		constexpr explicit(false) OperandValue(const value_type value) //
			noexcept(std::is_nothrow_constructible_v<value_type, U>) //
			: value_{value} {}

		template<typename U>
			requires(
				not std::is_trivially_constructible_v<value_type, const U&> and
				std::is_constructible_v<value_type, const U&>)
		constexpr explicit(false) OperandValue(const U& value) //
			noexcept(std::is_nothrow_constructible_v<value_type, const U&>) //
			: value_{value} {}

		template<typename U>
			requires(
				not std::is_trivially_constructible_v<value_type, U&&> and
				std::is_constructible_v<value_type, U&&>)
		constexpr explicit(false) OperandValue(U&& value) //
			noexcept(std::is_nothrow_constructible_v<value_type, U&&>)
			: value_{std::forward<U>(value)} {}

		template<typename... Args>
			requires((sizeof...(Args) != 1) and std::is_constructible_v<value_type, Args...>)
		constexpr explicit OperandValue(Args&&... args) //
			noexcept(std::is_nothrow_constructible_v<value_type, Args...>)
			: value_{std::forward<Args>(args)...} {}

		[[nodiscard]] constexpr auto value() noexcept -> value_type& { return value_; }

		[[nodiscard]] constexpr auto value() const noexcept -> const value_type& { return value_; }

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			return meta::to_string(value_, out);
		}
	};

	// ReSharper disable CppInconsistentNaming
	template<typename T>
	OperandValue(T) -> OperandValue<T>;
	// ReSharper restore CppInconsistentNaming

	template<typename T>
	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandValueRef final : public Operand
	{
	public:
		using value_type = T;

	private:
		std::reference_wrapper<value_type> ref_;

	public:
		constexpr explicit(false) OperandValueRef(value_type& ref) noexcept
			: ref_{ref} {}

		[[nodiscard]] constexpr auto value() noexcept -> value_type& { return ref_.get(); }

		[[nodiscard]] constexpr auto value() const noexcept -> const value_type& { return ref_.get(); }

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			return meta::to_string(ref_.get(), out);
		}
	};

	// ReSharper disable CppInconsistentNaming
	template<typename T>
	OperandValueRef(T&) -> OperandValueRef<T>;
	// propagate const
	template<typename T>
	OperandValueRef(const T&) -> OperandValueRef<const T>;
	// ReSharper restore CppInconsistentNaming

	template<typename>
	struct is_operand_value : std::false_type {};

	template<typename T>
	struct is_operand_value<OperandValue<T>> : std::true_type {};

	template<typename T>
	struct is_operand_value<OperandValueRef<T>> : std::true_type {};

	template<typename T>
	constexpr auto is_operand_value_v = is_operand_value<T>::value;
	template<typename O>
	concept operand_value_t = is_operand_value_v<O>;

	// =========================================
	// LITERAL
	// =========================================

	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteral : public Operand {};

	template<char Value>
	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralCharacter final : public OperandLiteral
	{
	public:
		using value_type = char;

		constexpr static auto value = Value;

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			return meta::to_string(value, out);
		}
	};

	template<std::integral auto Value>
	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralIntegral final : public OperandLiteral
	{
	public:
		using value_type = std::remove_cvref_t<decltype(Value)>;

		constexpr static value_type value = Value;

		[[nodiscard]] constexpr auto operator-() const noexcept -> OperandLiteralIntegral<-static_cast<std::make_signed_t<value_type>>(value)>
		{
			return {};
		}

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			return meta::to_string(value, out);
		}
	};

	template<std::floating_point auto Value, std::size_t DenominatorSize>
	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralFloatingPoint final : public OperandLiteral
	{
	public:
		using value_type = std::remove_cvref_t<decltype(Value)>;

		constexpr static value_type value = Value;
		constexpr static std::size_t denominator_size = DenominatorSize;
		constexpr static value_type epsilon = [](std::size_t n) noexcept -> value_type
		{
			auto epsilon = static_cast<value_type>(1);
			while (n--) { epsilon /= static_cast<value_type>(10); }
			return epsilon;
		}(DenominatorSize);

		[[nodiscard]] constexpr auto operator-() const noexcept -> OperandLiteralFloatingPoint<-value, DenominatorSize> { return {}; }

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			return std::format_to(std::back_inserter(out), "{:.{}g}", DenominatorSize, value);
		}
	};

	template<char... Cs>
	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandLiteralAuto final : public OperandLiteral
	{
	public:
		constexpr static auto char_list = functional::char_list<Cs...>;

		template<typename T>
		struct rep;

		template<char Value>
		struct rep<OperandLiteralCharacter<Value>>
		{
			using type = OperandLiteralCharacter<char_list.template nth_value<0>()>;
		};

		template<std::integral auto Value>
		struct rep<OperandLiteralIntegral<Value>>
		{
			using type = OperandLiteralIntegral<char_list.template to_integral<OperandLiteralIntegral<Value>::value_type>()>;
		};

		template<std::floating_point auto Value, std::size_t DenominatorSize>
		struct rep<OperandLiteralFloatingPoint<Value, DenominatorSize>>
		{
			using type = OperandLiteralFloatingPoint<
				char_list.template to_floating_point<OperandLiteralFloatingPoint<Value, DenominatorSize>::value_type>,
				char_list.denominator_length()
			>;
		};

		template<std::integral T>
		struct rep<T>
		{
			using type = std::conditional_t<
				std::is_same_v<T, char>,
				OperandLiteralCharacter<char_list.template nth_value<0>()>,
				OperandLiteralIntegral<char_list.template to_integral<T>()>
			>;
		};

		template<std::floating_point T>
		struct rep<T>
		{
			using type = OperandLiteralFloatingPoint<char_list.template to_floating_point<T>(), char_list.denominator_length()>;
		};

		template<typename T>
		using rebind = typename rep<std::remove_cvref_t<T>>::type;
	};

	// ========================
	// literal

	template<typename O>
	constexpr auto is_operand_literal_v = std::is_base_of_v<OperandLiteral, O>;
	template<typename O>
	concept operand_literal_t = is_operand_literal_v<O>;

	// ========================
	// literal character

	template<typename>
	struct is_operand_literal_character : std::false_type {};

	template<char Value>
	struct is_operand_literal_character<OperandLiteralCharacter<Value>> : std::true_type {};

	template<typename T>
	constexpr auto is_operand_literal_character_v = is_operand_literal_character<T>::value;
	template<typename O>
	concept operand_literal_character_t = is_operand_literal_character_v<O>;

	// ========================
	// literal integral

	template<typename>
	struct is_operand_literal_integral : std::false_type {};

	template<std::integral auto Value>
	struct is_operand_literal_integral<OperandLiteralIntegral<Value>> : std::true_type {};

	template<typename T>
	constexpr auto is_operand_literal_integral_v = is_operand_literal_integral<T>::value;
	template<typename O>
	concept operand_literal_integral_t = is_operand_literal_integral_v<O>;

	// ========================
	// literal floating point

	template<typename>
	struct is_operand_literal_floating_point : std::false_type {};

	template<std::floating_point auto Value, std::size_t DenominatorSize>
	struct is_operand_literal_floating_point<OperandLiteralFloatingPoint<Value, DenominatorSize>> : std::true_type {};

	template<typename T>
	constexpr auto is_operand_literal_floating_point_v = is_operand_literal_floating_point<T>::value;
	template<typename O>
	concept operand_literal_floating_point_t = is_operand_literal_floating_point_v<O>;

	// ========================
	// literal auto-deducing

	template<typename>
	struct is_operand_literal_auto : std::false_type {};

	template<char... Cs>
	struct is_operand_literal_auto<OperandLiteralAuto<Cs...>> : std::true_type {};

	template<typename T>
	constexpr auto is_operand_literal_auto_v = is_operand_literal_auto<T>::value;
	template<typename O>
	concept operand_literal_auto_t = is_operand_literal_auto_v<O>;

	// =========================================
	// IDENTITY (message)
	// =========================================

	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandIdentityBoolean final : public Operand
	{
	public:
		// explicit unique type
		struct value_type
		{
			std::string_view string;
		};

	private:
		value_type value_;
		bool result_;

	public:
		constexpr OperandIdentityBoolean(const value_type value, const bool result) noexcept
			: value_{value},
			  result_{result} {}

		[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			std::format_to(std::back_inserter(out), "{}", value_.string);
		}
	};

	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandIdentityString final : public Operand
	{
	public:
		// explicit unique type
		struct value_type
		{
			std::string_view string;
		};

	private:
		value_type value_;

	public:
		constexpr explicit OperandIdentityString(const value_type value) noexcept
			: value_{value} {}

		[[nodiscard]] constexpr auto value() const noexcept -> std::string_view
		{
			return value_.string;
		}

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			std::format_to(std::back_inserter(out), "\"{}\"", value_.string);
		}
	};

	// ========================
	// identity boolean

	template<typename>
	struct is_operand_identity_boolean : std::false_type {};

	template<>
	struct is_operand_identity_boolean<OperandIdentityBoolean> : std::true_type {};

	template<typename T>
	constexpr auto is_operand_identity_boolean_v = is_operand_identity_boolean<T>::value;

	template<typename T>
	concept operand_identity_boolean_t = is_operand_identity_boolean_v<T>;

	// ========================
	// identity string

	template<typename>
	struct is_operand_identity_string : std::false_type {};

	template<>
	struct is_operand_identity_string<OperandIdentityString> : std::true_type {};

	template<typename T>
	constexpr auto is_operand_identity_string_v = is_operand_identity_string<T>::value;

	template<typename T>
	concept operand_identity_string_t = is_operand_identity_string_v<T>;

	// =========================================
	// EXPRESSION
	// =========================================

	enum class ExpressionCategory : std::uint8_t
	{
		EQUAL,
		APPROX,
		NOT_EQUAL,
		NOT_APPROX,
		GREATER_THAN,
		GREATER_EQUAL,
		LESS_THAN,
		LESS_EQUAL,
		LOGICAL_AND,
		LOGICAL_OR,
	};

	struct no_epsilon {};

	template<ExpressionCategory Category, typename Left, typename Right, typename Epsilon = no_epsilon>
		requires(not(is_operand_literal_auto_v<Left> and is_operand_literal_auto_v<Right>))
	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandExpression final : public Operand
	{
	public:
		constexpr static auto category = Category;

		// If one side of the expression is OperandLiterAuto and the other side is not OperandLiteralXxx then a compile error should be raised.
		static_assert((not is_operand_literal_auto_v<Left>) or (is_operand_literal_auto_v<Left> == is_operand_literal_v<Right>));
		static_assert((not is_operand_literal_auto_v<Right>) or (is_operand_literal_auto_v<Right> == is_operand_literal_v<Left>));

		// using left_type = std::conditional_t<is_operand_literal_auto_v<Left>, typename Left::template rebind<Right>, Left>;
		// using right_type = std::conditional_t<is_operand_literal_auto_v<Right>, typename Right::template rebind<Left>, Right>;
		using left_type = Left;
		using right_type = Right;
		using epsilon_type = Epsilon;

	private:
		left_type left_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
		right_type right_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
		epsilon_type epsilon_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
		bool result_;

		[[nodiscard]] constexpr auto do_check() const noexcept -> bool
		{
			const auto do_compare = [](const auto& left, const auto& right, const auto& epsilon) noexcept -> bool
			{
				if constexpr (category == ExpressionCategory::EQUAL)
				{
					using std::operator==;
					using std::operator!=;
					if constexpr (requires { left == right; })
					{
						return left == right;
					}
					else if constexpr (requires { right == left; })
					{
						return right == left;
					}
					else if constexpr (requires { left != right; })
					{
						return not(left != right);
					}
					else if constexpr (requires { right != left; })
					{
						return not(right != left);
					}
					else if constexpr (requires { { left.compare(right) } -> std::same_as<bool>; })
					{
						return left.compare(right);
					}
					else if constexpr (requires { left.compare(right); })
					{
						return left.compare(right) == 0;
					}
					else if constexpr (requires { { right.compare(left) } -> std::same_as<bool>; })
					{
						return right.compare(left);
					}
					else if constexpr (requires { right.compare(left); })
					{
						return right.compare(left) == 0;
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("Not comparable!");
					}
				}
				else if constexpr (category == ExpressionCategory::APPROX)
				{
					using std::operator-;
					using std::operator<;
					return math::abs(left - right) < epsilon; // NOLINT(clang-diagnostic-implicit-int-float-conversion)
				}
				else if constexpr (category == ExpressionCategory::NOT_EQUAL)
				{
					using std::operator!=;
					using std::operator==;
					if constexpr (requires { left != right; })
					{
						return left != right;
					}
					else if constexpr (requires { right != left; })
					{
						return right != left;
					}
					else if constexpr (requires { left == right; })
					{
						return not(left == right);
					}
					else if constexpr (requires { right == left; })
					{
						return not(right == left);
					}
					else if constexpr (requires { { left.compare(right) } -> std::same_as<bool>; })
					{
						return not left.compare(right);
					}
					else if constexpr (requires { left.compare(right); })
					{
						return left.compare(right) != 0;
					}
					else if constexpr (requires { { right.compare(left) } -> std::same_as<bool>; })
					{
						return not right.compare(left);
					}
					else if constexpr (requires { right.compare(left); })
					{
						return right.compare(left) != 0;
					}
					else
					{
						GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE("Not comparable!");
					}
				}
				else if constexpr (category == ExpressionCategory::NOT_APPROX)
				{
					using std::operator-;
					using std::operator<;
					return epsilon < math::abs(left - right);
				}
				else if constexpr (category == ExpressionCategory::GREATER_THAN)
				{
					using std::operator>;
					return left > right;
				}
				else if constexpr (category == ExpressionCategory::GREATER_EQUAL)
				{
					using std::operator>=;
					return left >= right;
				}
				else if constexpr (category == ExpressionCategory::LESS_THAN)
				{
					using std::operator<;
					return left < right;
				}
				else if constexpr (category == ExpressionCategory::LESS_EQUAL)
				{
					using std::operator<=;
					return left <= right;
				}
				else if constexpr (category == ExpressionCategory::LOGICAL_AND) { return static_cast<bool>(left) and static_cast<bool>(right); }
				else if constexpr (category == ExpressionCategory::LOGICAL_OR) { return static_cast<bool>(left) or static_cast<bool>(right); }
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			};

			const auto get_value = []<typename T>([[maybe_unused]] const T& target) noexcept -> decltype(auto) //
			{
				[[maybe_unused]] constexpr auto not_member_function_pointer = []<typename P>(P) constexpr noexcept
				{
					return not std::is_member_function_pointer_v<std::decay_t<P>>;
				};

				if constexpr (requires { target.value(); })
				{
					// member function
					return target.value();
				}
				else if constexpr (requires { not_member_function_pointer(T::value); })
				{
					// static variable
					return T::value;
				}
				else
				{
					return target;
				}
			};

			return do_compare(get_value(left_), get_value(right_), get_value(epsilon_));
		}

	public:
		template<typename L, typename R, typename E>
		constexpr OperandExpression(
			L&& left,
			R&& right,
			E&& epsilon
		) noexcept
			: left_{std::forward<L>(left)},
			  right_{std::forward<R>(right)},
			  epsilon_{std::forward<E>(epsilon)},
			  result_{do_check()} {}

		template<typename L, typename R>
		constexpr OperandExpression(
			L&& left,
			R&& right
		) noexcept
			: left_{std::forward<L>(left)},
			  right_{std::forward<R>(right)},
			  epsilon_{},
			  result_{do_check()} {}

		[[nodiscard]] constexpr explicit operator bool() const noexcept { return result_; }

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			constexpr auto left_prefer_no_type_name = requires { typename left_type::prefer_no_type_name; };
			constexpr auto right_prefer_no_type_name = requires { typename right_type::prefer_no_type_name; };
			constexpr auto epsilon_prefer_no_type_name = requires { typename right_type::prefer_no_type_name; };

			if constexpr (category == ExpressionCategory::EQUAL)
			{
				std::format_to(
					std::back_inserter(out),
					"{} == {}",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_)
				);
			}
			else if constexpr (category == ExpressionCategory::APPROX)
			{
				std::format_to(
					std::back_inserter(out),
					"{} ≈≈ {} (+/- {})",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_),
					meta::to_string<StringType, not epsilon_prefer_no_type_name>(epsilon_)
				);
			}
			else if constexpr (category == ExpressionCategory::NOT_EQUAL)
			{
				std::format_to(
					std::back_inserter(out),
					"{} != {}",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_)
				);
			}
			else if constexpr (category == ExpressionCategory::NOT_APPROX)
			{
				std::format_to(
					std::back_inserter(out),
					"{} !≈ {} (+/- {})",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_),
					meta::to_string<StringType, not epsilon_prefer_no_type_name>(epsilon_)
				);
			}
			else if constexpr (category == ExpressionCategory::GREATER_THAN)
			{
				std::format_to(
					std::back_inserter(out),
					"{} > {}",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_)
				);
			}
			else if constexpr (category == ExpressionCategory::GREATER_EQUAL)
			{
				std::format_to(
					std::back_inserter(out),
					"{} >= {}",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_)
				);
			}
			else if constexpr (category == ExpressionCategory::LESS_THAN)
			{
				std::format_to(
					std::back_inserter(out),
					"{} < {}",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_)
				);
			}
			else if constexpr (category == ExpressionCategory::LESS_EQUAL)
			{
				std::format_to(
					std::back_inserter(out),
					"{} <= {}",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_)
				);
			}
			else if constexpr (category == ExpressionCategory::LOGICAL_AND)
			{
				std::format_to(
					std::back_inserter(out),
					"{} and {}",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_)
				);
			}
			else if constexpr (category == ExpressionCategory::LOGICAL_OR)
			{
				std::format_to(
					std::back_inserter(out),
					"{} or {}",
					meta::to_string<StringType, not left_prefer_no_type_name>(left_),
					meta::to_string<StringType, not right_prefer_no_type_name>(right_)
				);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}
	};

	template<typename>
	struct is_operand_expression : std::false_type {};

	template<ExpressionCategory Category, typename Left, typename Right, typename Epsilon>
	struct is_operand_expression<OperandExpression<Category, Left, Right, Epsilon>> : std::true_type {};

	template<typename T>
	constexpr auto is_operand_expression_v = is_operand_expression<T>::value;
	template<typename O>
	concept operand_expression_t = is_operand_expression_v<O>;

	// =========================================
	// EXCEPTION
	// =========================================

	template<typename Exception>
	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandThrow final : public Operand
	{
	public:
		using exception_type = Exception;

	private:
		bool thrown_;
		bool caught_;

	public:
		template<std::invocable Invocable>
		constexpr explicit OperandThrow(Invocable&& invocable) noexcept
			: thrown_{false},
			  caught_{false}
		{
			if constexpr (std::is_same_v<exception_type, void>)
			{
				try { std::invoke(std::forward<Invocable>(invocable)); }
				catch (...)
				{
					thrown_ = true;
					caught_ = true;
				}
			}
			else
			{
				try { std::invoke(std::forward<Invocable>(invocable)); }
				catch ([[maybe_unused]] const exception_type& exception)
				{
					thrown_ = true;
					caught_ = true;
				}
				catch (...)
				{
					thrown_ = true;
					caught_ = false;
				}
			}
		}

		[[nodiscard]] constexpr auto thrown() const noexcept -> bool { return thrown_; }
		[[nodiscard]] constexpr auto caught() const noexcept -> bool { return caught_; }

		[[nodiscard]] constexpr explicit operator bool() const noexcept { return caught(); }

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			std::format_to(
				std::back_inserter(out),
				"throws<{}> -- [{}]",
				meta::name_of<exception_type>(),
				(not thrown())
					? "not thrown"
					: //
					(not caught())
					? "thrown but not caught"
					: //
					"caught"
			);
		}
	};

	class GAL_PROMETHEUS_COMPILER_EMPTY_BASE OperandNoThrow final : public Operand
	{
	public:
		using exception_type = void;

	private:
		bool thrown_;

	public:
		template<std::invocable Invocable>
		constexpr explicit OperandNoThrow(Invocable&& invocable) noexcept
			: thrown_{false}
		{
			try { std::invoke(std::forward<Invocable>(invocable)); }
			catch (...) { thrown_ = true; }
		}

		[[nodiscard]] constexpr explicit operator bool() const noexcept { return not thrown_; }

		template<std::ranges::output_range<char> StringType>
			requires std::ranges::contiguous_range<StringType>
		constexpr auto to_string(StringType& out) const noexcept -> void
		{
			std::format_to(std::back_inserter(out), "nothrow - {:s}", not thrown_);
		}
	};
}
