// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.utility:cast;

import std;
import gal.prometheus.error;

import :concepts;

namespace gal::prometheus::utility
{
	template<typename Left, typename Right>
	struct three_way_comparison;

	template<typename Left, typename Right>
		requires std::is_same_v<Left, Right> and
		         (std::unsigned_integral<Left> or
		          std::signed_integral<Left> or
		          std::floating_point<Left>)
	struct three_way_comparison<Left, Right>
	{
		[[nodiscard]] constexpr auto operator()(const Left left, const Right right) const noexcept -> std::strong_ordering { return left <=> right; }
	};

	template<typename Left, typename Right>
		requires(std::signed_integral<Left> and std::unsigned_integral<Right>) or
		        (std::unsigned_integral<Left> and std::signed_integral<Right>)
	struct three_way_comparison<Left, Right>
	{
		[[nodiscard]] constexpr auto operator()(const Left left, const Right right) const noexcept -> std::strong_ordering
		{
			if constexpr (std::signed_integral<Left>)
			{
				if (left < 0) { return std::strong_ordering::less; }
				return static_cast<Right>(left) <=> right;
			}
			else
			{
				if (right < 0) { return std::strong_ordering::greater; }
				return left <=> static_cast<Left>(right);
			}
		}
	};

	template<typename L, typename R>
		requires(std::floating_point<L> and std::integral<R>) or
		        (std::integral<L> and std::floating_point<R>)
	struct three_way_comparison<L, R>
	{
		[[nodiscard]] constexpr auto operator()(const L l, const R r) const noexcept -> std::partial_ordering
		{
			if constexpr (std::floating_point<L>)
			{
				if constexpr (sizeof(R) < sizeof(float)) { return l <=> static_cast<float>(r); }
				else if constexpr (sizeof(R) < sizeof(double)) { return l <=> static_cast<double>(r); }
				else if constexpr (sizeof(R) < sizeof(long double)) { return l <=> static_cast<long double>(r); }
				else { GAL_PROMETHEUS_STATIC_UNREACHABLE("Invalid floating point type!"); }
			}
			else
			{
				if constexpr (sizeof(L) < sizeof(float)) { return static_cast<float>(l) <=> r; }
				else if constexpr (sizeof(L) < sizeof(double)) { return static_cast<double>(l) <=> r; }
				else if constexpr (sizeof(L) < sizeof(long double)) { return static_cast<long double>(l) <=> r; }
				else { GAL_PROMETHEUS_STATIC_UNREACHABLE("Invalid floating point type!"); }
			}
		}
	};

	template<typename Left, typename Right>
		requires std::is_arithmetic_v<Left> and std::is_arithmetic_v<Right>
	[[nodiscard]] constexpr auto three_way_compare(const Left left, const Right right) noexcept -> auto { return three_way_comparison<Left, Right>{}(left, right); }

	export
	{
		/**
		 * @brief Cast a number to a type that will be able to represent all values without loss of precision.
		 * @tparam Out The numeric type to cast to.
		 * @tparam In The numeric type to cast from.
		 * @param input The input value.
		 * @return The output value.
		 */
		template<concepts::arithmetic Out, concepts::arithmetic In>
			requires concepts::type_in_range<Out, In>
		[[nodiscard]] constexpr auto wide_cast(const In input) noexcept -> Out { return static_cast<Out>(input); }

		/**
		 * @brief Cast a numeric value to an integer saturating on overflow.
		 * @tparam Out The signed-integer or unsigned-integer type to cast to.
		 * @tparam In The numeric type to cast from.
		 * @param input The input value.
		 * @return The converted value, which is saturated if @b input is overflowing or under-flowing.
		 */
		template<std::integral Out, concepts::arithmetic In>
		[[nodiscard]] constexpr auto saturate_cast(const In input) noexcept -> Out
		{
			if constexpr (std::is_floating_point_v<In>) { if (std::isnan(input)) { return Out{0}; } }

			if (three_way_compare(input, std::numeric_limits<Out>::lowest()) != std::strong_ordering::greater) { return std::numeric_limits<Out>::lowest(); }

			if (three_way_compare(input, std::numeric_limits<Out>::max()) != std::strong_ordering::less) { return std::numeric_limits<Out>::max(); }

			return static_cast<Out>(input);
		}

		/**
		 * @brief Cast numeric values without loss of precision.
		 * @tparam Out The numeric type to cast to.
		 * @tparam In The numeric type to cast from
		 * @param input The input value.
		 * @return The value cast to a different type without loss of precision.
		 *
		 * @note It is undefined behavior to cast a value which will cause a loss of precision.
		 */
		template<concepts::arithmetic Out, concepts::arithmetic In>
		[[nodiscard]] constexpr auto narrow_cast(const In input) noexcept -> Out
		{
			constexpr auto narrow_validate = [](const Out o, const In i) noexcept -> bool
			{
				// in-value and out-value compares the same, after converting out-value back to in-type.
				auto result = (i == static_cast<In>(o));

				// If the types have different signs we need to do an extra test to make sure the actual sign
				// of the values are the same as well.
				if constexpr (std::numeric_limits<Out>::is_signed != std::numeric_limits<In>::is_signed) { result &= (i < In{0}) == (o < Out{0}); }

				return result;
			};

			if constexpr (concepts::type_in_range<Out, In>) { return static_cast<Out>(input); }
			else
			{
				const auto out = static_cast<Out>(input);
				GAL_PROMETHEUS_DEBUG_ASSUME(narrow_validate(out, input), "Invalid narrow cast!");
				return out;
			}
		}

		template<concepts::arithmetic Out, concepts::arithmetic In>
		[[nodiscard]] constexpr auto round_cast(const In input) noexcept -> Out
		{
			if constexpr (std::is_floating_point_v<In>) { return narrow_cast<Out>(std::round(input)); }
			else { return narrow_cast<Out>(input); }
		}

		template<concepts::arithmetic Out, concepts::arithmetic In>
		[[nodiscard]] constexpr auto floor_cast(const In input) noexcept -> Out
		{
			if constexpr (std::is_floating_point_v<In>) { return narrow_cast<Out>(std::floor(input)); }
			else { return narrow_cast<Out>(input); }
		}

		template<concepts::arithmetic Out, concepts::arithmetic In>
		[[nodiscard]] constexpr auto ceil_cast(const In input) noexcept -> Out
		{
			if constexpr (std::is_floating_point_v<In>) { return narrow_cast<Out>(std::ceil(input)); }
			else { return narrow_cast<Out>(input); }
		}

		/**
		 * @brief Cast a character.
		 * @tparam Out The value type of the result.
		 * @tparam In The value type of the character.
		 * @param input The value of the character.
		 * @return The cast value.
		 *
		 * @note Both the input and output types are interpreted as unsigned values, even if
		 * 	they are signed values. For example `char` may be either signed or unsigned,
		 * 	but you have to treat those as unsigned values.
		 * @note @b input value after casting, must fit in the output type.
		 */
		template<std::integral Out, std::integral In>
		[[nodiscard]] constexpr auto char_cast(const In input) noexcept -> Out
		{
			using unsigned_in_type = std::make_unsigned_t<In>;
			using unsigned_out_type = std::make_unsigned_t<Out>;

			// We cast to unsigned of the same type, so that we don't accidentally sign extent 'char'.
			auto unsigned_in  = static_cast<unsigned_in_type>(input);
			auto unsigned_out = narrow_cast<unsigned_out_type>(unsigned_in);
			return static_cast<Out>(unsigned_out);
		}

		template<std::integral Out>
		[[nodiscard]] constexpr auto char_cast(const std::byte input) noexcept -> Out
		{
			static_assert(sizeof(std::byte) == sizeof(std::uint8_t));
			return char_cast<Out>(static_cast<std::uint8_t>(input));
		}

		/**
		 * @brief Return the low half of the input value.
		 */
		template<concepts::unsigned_integral Out, concepts::unsigned_integral In>
			requires(sizeof(Out) * 2 == sizeof(In))// Return value of low_bit_cast must be half the size of the input
		[[nodiscard]] constexpr auto low_bit_cast(const In input) noexcept -> Out { return static_cast<Out>(input); }

		/**
		 * @brief Return the high half of the input value.
		 */
		template<concepts::unsigned_integral Out, concepts::unsigned_integral In>
			requires(sizeof(Out) * 2 == sizeof(In))// Return value of high_bit_cast must be half the size of the input
		[[nodiscard]] constexpr auto high_bit_cast(const In input) noexcept -> Out { return static_cast<Out>(input >> sizeof(Out) * std::numeric_limits<unsigned char>::digits); }

		/**
		 * @brief Return the upper half of the input value.
		 */
		template<concepts::unsigned_integral Out, concepts::unsigned_integral In>
			requires(sizeof(Out) * 2 == sizeof(In))// Return value of merge_bit_cast must be half the size of the input
		[[nodiscard]] constexpr auto merge_bit_cast(const In high, const In low) noexcept -> Out
		{
			auto result = static_cast<Out>(high);
			result <<= sizeof(In) * std::numeric_limits<unsigned char>::digits;
			result |= static_cast<Out>(low);
			return result;
		}

		/**
		 * @brief Return the low half of the input value.
		 */
		template<concepts::signed_integral Out, concepts::signed_integral In>
			requires(sizeof(Out) * 2 == sizeof(In))// Return value of low_bit_cast must be half the size of the input
		[[nodiscard]] constexpr auto low_bit_cast(const In input) noexcept -> Out
		{
			using unsigned_in_type = std::make_unsigned_t<In>;
			using unsigned_out_type = std::make_unsigned_t<Out>;

			return static_cast<Out>(low_bit_cast<unsigned_out_type>(static_cast<unsigned_in_type>(input)));
		}

		/**
		 * @brief Return the high half of the input value.
		 */
		template<concepts::signed_integral Out, concepts::signed_integral In>
			requires(sizeof(Out) * 2 == sizeof(In))// Return value of high_bit_cast must be half the size of the input
		[[nodiscard]] constexpr auto high_bit_cast(const In input) noexcept -> Out
		{
			using unsigned_in_type = std::make_unsigned_t<In>;
			using unsigned_out_type = std::make_unsigned_t<Out>;

			return static_cast<Out>(high_bit_cast<unsigned_out_type>(static_cast<unsigned_in_type>(input)));
		}

		/**
		 * @brief Return the upper half of the input value.
		 */
		template<concepts::signed_integral Out, concepts::signed_integral In>
			requires(sizeof(Out) * 2 == sizeof(In))// Return value of merge_bit_cast must be half the size of the input
		[[nodiscard]] constexpr auto merge_bit_cast(const In high, const In low) noexcept -> Out
		{
			using unsigned_in_type = std::make_unsigned_t<In>;
			using unsigned_out_type = std::make_unsigned_t<Out>;

			return static_cast<Out>(merge_bit_cast<unsigned_out_type>(static_cast<unsigned_in_type>(high), static_cast<unsigned_in_type>(low)));
		}

		/**
		 * @brief Return the low half of the input value.
		 */
		template<concepts::unsigned_integral Out, concepts::signed_integral In>
			requires(sizeof(Out) * 2 == sizeof(In))// Return value of low_bit_cast must be half the size of the input
		[[nodiscard]] constexpr auto low_bit_cast(const In input) noexcept -> Out
		{
			using unsigned_in_type = std::make_unsigned_t<In>;

			return static_cast<Out>(low_bit_cast<Out>(static_cast<unsigned_in_type>(input)));
		}

		/**
		 * @brief Return the high half of the input value.
		 */
		template<concepts::unsigned_integral Out, concepts::signed_integral In>
			requires(sizeof(Out) * 2 == sizeof(In))// Return value of high_bit_cast must be half the size of the input
		[[nodiscard]] constexpr auto high_bit_cast(const In input) noexcept -> Out
		{
			using unsigned_in_type = std::make_unsigned_t<In>;

			return static_cast<Out>(high_bit_cast<Out>(static_cast<unsigned_in_type>(input)));
		}

		/**
		 * @brief Return the upper half of the input value.
		 */
		template<concepts::signed_integral Out, concepts::unsigned_integral In>
			requires(sizeof(Out) * 2 == sizeof(In))// Return value of merge_bit_cast must be half the size of the input
		[[nodiscard]] constexpr auto merge_bit_cast(const In high, const In low) noexcept -> Out
		{
			using unsigned_out_type = std::make_unsigned_t<Out>;

			return narrow_cast<Out>(merge_bit_cast<unsigned_out_type>(high, low));
		}

		/**
		 * @brief Cast an integral to an unsigned integral of the same size.
		 */
		template<std::integral In>
		[[nodiscard]] constexpr auto to_unsigned(const In input) noexcept -> std::make_unsigned_t<In> { return static_cast<std::make_unsigned_t<In>>(input); }

		/**
		 * @briefc Cast an integral to an signed integral of the same size.
		 */
		template<std::integral In>
		[[nodiscard]] constexpr auto to_signed(const In input) noexcept -> std::make_signed_t<In> { return static_cast<std::make_signed_t<In>>(input); }

		/**
		 * @brief Cast between integral types truncating or zero-extending the result.
		 */
		template<std::integral Out, std::integral In>
		[[nodiscard]] constexpr auto truncate(const In input) noexcept -> Out { return static_cast<Out>(to_unsigned(input)); }

		/**
		 * @brief Cast a pointer to a class to its base class or itself.
		 * @tparam TargetPointer The base class or itself pointer type.
		 * @tparam Type The derived class type.
		 * @param input The pointer to cast.
		 * @return A pointer of the base class or itself.
		 */
		template<typename TargetPointer, std::derived_from<std::remove_pointer_t<TargetPointer>> Type>
			requires std::is_pointer_v<TargetPointer> and
			         (std::is_const_v<std::remove_pointer_t<TargetPointer>> == std::is_const_v<Type> or
			          std::is_const_v<std::remove_pointer_t<TargetPointer>>)
		[[nodiscard]] constexpr auto up_cast(Type* input) noexcept -> TargetPointer
		{
			if constexpr (std::is_same_v<std::remove_const_t<Type>, std::remove_cv_t<std::remove_pointer_t<TargetPointer>>>) { return input; }
			else { return static_cast<TargetPointer>(input); }
		}


		/**
		 * @brief Cast a pointer to a class to its derived class or itself.
		 * @tparam TargetPointer The derived class or itself pointer type.
		 * @tparam Type The base class type.
		 * @param input The pointer to cast.
		 * @return A pointer of the derived class or itself.
		 *
		 * @note It is undefined behavior if the argument is not of type @b TargetPointer.
		 */
		template<typename TargetPointer, typename Type>
			requires std::is_pointer_v<TargetPointer> and
			         (std::is_const_v<std::remove_pointer_t<TargetPointer>> == std::is_const_v<Type> or
			          std::is_const_v<std::remove_pointer_t<TargetPointer>>) and
			         std::derived_from<std::remove_pointer_t<TargetPointer>, Type>
		[[nodiscard]] constexpr auto down_cast(Type* input) noexcept -> TargetPointer
		{
			if constexpr (std::is_same_v<std::remove_const_t<Type>, std::remove_cv_t<std::remove_pointer_t<TargetPointer>>>) { return input; }
			else
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(input == nullptr or dynamic_cast<TargetPointer>(input) != nullptr);
				return static_cast<TargetPointer>(input);
			}
		}

		template<typename TargetPointer>
			requires std::is_pointer_v<TargetPointer>
		[[nodiscard]] constexpr auto up_cast(std::nullptr_t) noexcept -> TargetPointer { return nullptr; }

		template<typename TargetPointer>
			requires std::is_pointer_v<TargetPointer>
		[[nodiscard]] constexpr auto down_cast(std::nullptr_t) noexcept -> TargetPointer { return nullptr; }

		/**
		 * @brief Cast a reference to a class to its base class or itself.
		 * @tparam TargetReference The base class or itself reference type.
		 * @tparam Type The derived class type.
		 * @param input The reference to cast.
		 * @return A reference of the base class or itself.
		 */
		template<typename TargetReference, std::derived_from<std::remove_reference_t<TargetReference>> Type>
			requires std::is_reference_v<TargetReference> and
			         (std::is_const_v<std::remove_reference_t<TargetReference>> == std::is_const_v<TargetReference> or
			          std::is_const_v<std::remove_reference_t<TargetReference>>)
		[[nodiscard]] constexpr auto up_cast(Type& input) noexcept -> TargetReference
		{
			if constexpr (std::is_same_v<std::remove_const_t<Type>, std::remove_cvref_t<TargetReference>>) { return input; }
			else { return static_cast<TargetReference>(input); }
		}

		/**
		 * @brief Cast a reference to a class to its derived class or itself.
		 * @tparam TargetReference The derived class or itself reference type.
		 * @tparam Type The base class type.
		 * @param input The reference to cast.
		 * @return A reference of the derived class or itself.
		 */
		template<typename TargetReference, typename Type>
			requires std::is_reference_v<TargetReference> and
			         (std::is_const_v<std::remove_reference_t<TargetReference>> == std::is_const_v<TargetReference> or
			          std::is_const_v<std::remove_reference_t<TargetReference>>) and
			         std::derived_from<std::remove_reference_t<TargetReference>, Type>
		[[nodiscard]] constexpr auto down_cast(Type& input) noexcept -> TargetReference
		{
			if constexpr (std::is_same_v<std::remove_const_t<Type>, std::remove_cvref_t<TargetReference>>) { return input; }
			else
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(dynamic_cast<std::add_pointer_t<std::remove_reference_t<TargetReference>>>(std::addressof(input)) != nullptr);
				return static_cast<TargetReference>(input);
			}
		}

		template<typename OutPointer>
			requires std::is_pointer_v<OutPointer>
		[[nodiscard]] constexpr auto to_pointer(const std::intptr_t address) noexcept -> OutPointer { return reinterpret_cast<OutPointer>(address); }

		template<typename OutPointer>
			requires std::is_pointer_v<OutPointer>
		[[nodiscard]] constexpr auto to_pointer(const std::uintptr_t address) noexcept -> OutPointer { return reinterpret_cast<OutPointer>(address); }

		template<typename OutInteger = std::intptr_t>
			requires(std::is_same_v<OutInteger, std::intptr_t> or std::is_same_v<OutInteger, std::uintptr_t>)
		[[nodiscard]] constexpr auto to_address(const void* const pointer) noexcept -> OutInteger { return reinterpret_cast<OutInteger>(pointer); }
	}

	namespace cast_detail
	{
		template<typename, typename>
		struct keep_cv;

		template<typename To, typename From>
			requires(not std::is_const_v<From> and not std::is_volatile_v<From>)
		struct keep_cv<To, From>
		{
			using type = std::remove_cv_t<To>;
		};

		template<typename To, typename From>
			requires(not std::is_const_v<From> and std::is_volatile_v<From>)
		struct keep_cv<To, From>
		{
			using type = std::add_volatile_t<std::remove_cv_t<To>>;
		};

		template<typename To, typename From>
			requires(std::is_const_v<From> and not std::is_volatile_v<From>)
		struct keep_cv<To, From>
		{
			using type = std::add_const_t<std::remove_cv_t<To>>;
		};

		template<typename To, typename From>
			requires(std::is_const_v<From> and std::is_volatile_v<From>)
		struct keep_cv<To, From>
		{
			using type = std::add_const_t<std::add_volatile_t<std::remove_cv_t<To>>>;
		};

		template<typename To, typename From>
		using keep_cv_t = typename keep_cv<To, From>::type;
	}

	export
	{
		template<typename Out, concepts::byte_like In>
			requires std::is_trivially_default_constructible_v<cast_detail::keep_cv_t<Out, In>> and std::is_trivially_destructible_v<cast_detail::keep_cv_t<Out, In>>//
		[[nodiscard]] constexpr auto implicit_cast(const std::span<In> bytes) noexcept(false) -> std::add_lvalue_reference<cast_detail::keep_cv_t<Out, In>>
		{
			using value_type = cast_detail::keep_cv_t<Out, In>;

			if (sizeof(value_type) > bytes.size()) { throw error::BadCastError{"Data incomplete!"}; }

			GAL_PROMETHEUS_DEBUG_NOT_NULL(bytes.data(), "Cannot implicit_cast null data!");

			if constexpr (alignof(value_type) != 1) { if (std::bit_cast<std::uintptr_t>(bytes.data()) % alignof(value_type) != 0) { throw error::BadCastError{"Alignment mismatch!"}; } }

			return *reinterpret_cast<value_type*>(bytes.data());
		}

		template<typename Out, concepts::byte_like In>
			requires std::is_trivially_default_constructible_v<cast_detail::keep_cv_t<Out, In>> and std::is_trivially_destructible_v<cast_detail::keep_cv_t<Out, In>>//
		[[nodiscard]] constexpr auto implicit_cast(const std::span<In> bytes, std::size_t& offset) noexcept(false) -> std::add_lvalue_reference<cast_detail::keep_cv_t<Out, In>>
		{
			using value_type = cast_detail::keep_cv_t<Out, In>;

			if (sizeof(value_type) + offset > bytes.size()) { throw error::BadCastError{"Data incomplete!"}; }

			GAL_PROMETHEUS_DEBUG_NOT_NULL(bytes.data(), "Cannot implicit_cast null data!");

			const auto data = bytes.data() + offset;
			if constexpr (alignof(value_type) != 1) { if (std::bit_cast<std::uintptr_t>(data) % alignof(value_type) != 0) { throw error::BadCastError{"Alignment mismatch!"}; } }

			offset += sizeof(value_type);
			return *reinterpret_cast<value_type*>(data);
		}

		template<typename Out, concepts::byte_like In>
			requires std::is_trivially_default_constructible_v<cast_detail::keep_cv_t<Out, In>> and std::is_trivially_destructible_v<cast_detail::keep_cv_t<Out, In>>//
		[[nodiscard]] constexpr auto implicit_cast(const std::span<In> bytes, const std::size_t n) noexcept(false) -> std::span<cast_detail::keep_cv_t<Out, In>>
		{
			using value_type = cast_detail::keep_cv_t<Out, In>;

			if (sizeof(value_type) * n > bytes.size()) { throw error::BadCastError{"Data incomplete!"}; }

			GAL_PROMETHEUS_DEBUG_NOT_NULL(bytes.data(), "Cannot implicit_cast null data!");

			if constexpr (alignof(value_type) != 1) { if (std::bit_cast<std::uintptr_t>(bytes.data()) % alignof(value_type) != 0) { throw error::BadCastError{"Alignment mismatch!"}; } }

			return std::span<cast_detail::keep_cv_t<Out, In>>{reinterpret_cast<value_type*>(bytes.data()), n};
		}

		template<typename Out, concepts::byte_like In>
			requires std::is_trivially_default_constructible_v<cast_detail::keep_cv_t<Out, In>> and std::is_trivially_destructible_v<cast_detail::keep_cv_t<Out, In>>//
		[[nodiscard]] constexpr auto implicit_cast(const std::span<In> bytes, const std::size_t n, std::size_t& offset) noexcept(false) -> std::span<cast_detail::keep_cv_t<Out, In>>
		{
			using value_type = cast_detail::keep_cv_t<Out, In>;

			if (sizeof(value_type) * n + offset > bytes.size()) { throw error::BadCastError{"Data incomplete!"}; }

			GAL_PROMETHEUS_DEBUG_NOT_NULL(bytes.data(), "Cannot implicit_cast null data!");

			const auto data = bytes.data() + offset;
			if constexpr (alignof(value_type) != 1) { if (std::bit_cast<std::uintptr_t>(data) % alignof(value_type) != 0) { throw error::BadCastError{"Alignment mismatch!"}; } }

			offset += sizeof(value_type) * n;
			return std::span<cast_detail::keep_cv_t<Out, In>>{reinterpret_cast<value_type*>(bytes.data()), n};
		}
	}
}// namespace gal::prometheus::infrastructure
