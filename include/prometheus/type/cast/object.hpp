// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <span>
#include <bit>

#include <prometheus/debug/exception.hpp>
#include <prometheus/type/traits/object.hpp>

namespace gal::prometheus::type::cast
{
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

	template<typename Out, traits::byte_like In>
		requires std::is_trivially_default_constructible_v<traits::keep_cv_t<Out, In>> and std::is_trivially_destructible_v<traits::keep_cv_t<Out, In>>
	[[nodiscard]] constexpr auto implicit_cast(const std::span<In> bytes) noexcept(false) -> std::add_lvalue_reference<traits::keep_cv_t<Out, In>>
	{
		using value_type = traits::keep_cv_t<Out, In>;

		if (sizeof(value_type) > bytes.size()) { throw std::bad_cast{}; }

		GAL_PROMETHEUS_DEBUG_NOT_NULL(bytes.data(), "Cannot implicit_cast null data!");

		if constexpr (alignof(value_type) != 1) { if (std::bit_cast<std::uintptr_t>(bytes.data()) % alignof(value_type) != 0) { throw std::bad_cast{}; } }

		return *reinterpret_cast<value_type*>(bytes.data());
	}

	template<typename Out, traits::byte_like In>
		requires std::is_trivially_default_constructible_v<traits::keep_cv_t<Out, In>> and std::is_trivially_destructible_v<traits::keep_cv_t<Out, In>>
	[[nodiscard]] constexpr auto implicit_cast(const std::span<In> bytes, std::size_t& offset) noexcept(false) -> std::add_lvalue_reference<traits::keep_cv_t<Out, In>>
	{
		using value_type = traits::keep_cv_t<Out, In>;

		if (sizeof(value_type) + offset > bytes.size()) { throw std::bad_cast{}; }

		GAL_PROMETHEUS_DEBUG_NOT_NULL(bytes.data(), "Cannot implicit_cast null data!");

		const auto data = bytes.data() + offset;
		if constexpr (alignof(value_type) != 1) { if (std::bit_cast<std::uintptr_t>(data) % alignof(value_type) != 0) { throw std::bad_cast{}; } }

		offset += sizeof(value_type);
		return *reinterpret_cast<value_type*>(data);
	}

	template<typename Out, traits::byte_like In>
		requires std::is_trivially_default_constructible_v<traits::keep_cv_t<Out, In>> and std::is_trivially_destructible_v<traits::keep_cv_t<Out, In>>
	[[nodiscard]] constexpr auto implicit_cast(const std::span<In> bytes, const std::size_t n) noexcept(false) -> std::span<traits::keep_cv_t<Out, In>>
	{
		using value_type = traits::keep_cv_t<Out, In>;

		if (sizeof(value_type) * n > bytes.size()) { throw std::bad_cast{}; }
		GAL_PROMETHEUS_DEBUG_NOT_NULL(bytes.data(), "Cannot implicit_cast null data!");

		if constexpr (alignof(value_type) != 1) { if (std::bit_cast<std::uintptr_t>(bytes.data()) % alignof(value_type) != 0) { throw std::bad_cast{}; } }

		return std::span<traits::keep_cv_t<Out, In>>{reinterpret_cast<value_type*>(bytes.data()), n};
	}

	template<typename Out, traits::byte_like In>
		requires std::is_trivially_default_constructible_v<traits::keep_cv_t<Out, In>> and std::is_trivially_destructible_v<traits::keep_cv_t<Out, In>>
	[[nodiscard]] constexpr auto implicit_cast(const std::span<In> bytes, const std::size_t n, std::size_t& offset) noexcept(false) -> std::span<traits::keep_cv_t<Out, In>>
	{
		using value_type = traits::keep_cv_t<Out, In>;

		if (sizeof(value_type) * n + offset > bytes.size()) { throw std::bad_cast{}; }

		GAL_PROMETHEUS_DEBUG_NOT_NULL(bytes.data(), "Cannot implicit_cast null data!");

		const auto data = bytes.data() + offset;
		if constexpr (alignof(value_type) != 1) { if (std::bit_cast<std::uintptr_t>(data) % alignof(value_type) != 0) { throw std::bad_cast{}; } }

		offset += sizeof(value_type) * n;
		return std::span<traits::keep_cv_t<Out, In>>{reinterpret_cast<value_type*>(bytes.data()), n};
	}
}
