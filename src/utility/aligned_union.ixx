// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.utility:aligned_union;

import std;
import :concepts;
import :functional;

export namespace gal::prometheus::utility
{
	template<typename... Ts>
		requires(not std::is_reference_v<Ts> && ...)
	class AlignedUnion final
	{
	public:
		constexpr static std::size_t max_size      = functor::max(sizeof(Ts)...);
		constexpr static std::size_t max_alignment = functor::max(alignof(Ts)...);

		template<typename T>
			requires concepts::any_of<T, Ts...>
		struct constructor_tag { };

	private:
		alignas(max_alignment) unsigned char data_[max_size]{};

	public:
		constexpr AlignedUnion() noexcept = default;

		template<typename T, typename... Args>
			requires std::is_constructible_v<T, Args...>
		constexpr explicit AlignedUnion(constructor_tag<T>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) { this->template store<T>(std::forward<Args>(args)...); }

		constexpr AlignedUnion(const AlignedUnion&) noexcept((std::is_nothrow_copy_constructible_v<Ts> && ...))//
			requires(std::is_copy_constructible_v<Ts> && ...)
		= default;
		constexpr auto operator=(const AlignedUnion&) noexcept((std::is_nothrow_copy_assignable_v<Ts> && ...)) -> AlignedUnion&//
			requires(std::is_copy_assignable_v<Ts> && ...)
		= default;
		// constexpr      AlignedUnion(const AlignedUnion&)               = delete;
		// constexpr auto operator=(const AlignedUnion&) -> AlignedUnion& = delete;

		constexpr AlignedUnion(AlignedUnion&&) noexcept((std::is_nothrow_move_constructible_v<Ts> && ...))//
			requires(std::is_move_constructible_v<Ts> && ...)
		= default;
		constexpr auto operator=(AlignedUnion&&) noexcept((std::is_nothrow_move_assignable_v<Ts> && ...)) -> AlignedUnion&//
			requires(std::is_move_assignable_v<Ts> && ...)
		= default;
		// constexpr      AlignedUnion(AlignedUnion&&)               = delete;
		// constexpr auto operator=(AlignedUnion&&) -> AlignedUnion& = delete;

		constexpr ~AlignedUnion() noexcept = default;

		template<typename T, typename... Args>
			requires concepts::any_of<T, Ts...> and std::is_constructible_v<T, Args...>
		constexpr auto store(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) { std::construct_at(reinterpret_cast<T*>(&data_), std::forward<Args>(args)...); }

		// Note: If the type saved by AlignedUnion has a non-trivial destructor but no destroy is called, leaving AlignedUnion to destruct or calling store (if it already has a value) will be undefined behavior! (maybe memory or resource leak)
		template<typename T>
			requires concepts::any_of<T, Ts...>
		constexpr auto destroy() noexcept(std::is_nothrow_destructible_v<T>) -> void { std::destroy_at(reinterpret_cast<T*>(&data_)); }

		template<typename New, typename Old, typename... Args>
			requires concepts::any_of<New, Ts...> and concepts::any_of<Old, Ts...> and std::is_constructible_v<New, Args...>
		constexpr auto exchange(Args&&... args) noexcept(std::is_nothrow_constructible_v<New, Args...> and std::is_nothrow_move_constructible_v<Old>) -> Old
		{
			auto&& old = std::move(this->template load<Old>());
			this->template store<New>(std::forward<Args>(args)...);
			return old;
		}

		template<typename New, typename Old, typename... Args>
			requires concepts::any_of<New, Ts...> and concepts::any_of<Old, Ts...> and std::is_constructible_v<New, Args...>
		constexpr auto replace(Args&&... args) noexcept(std::is_nothrow_constructible_v<New, Args...> and std::is_nothrow_destructible_v<Old>) -> void
		{
			this->template destroy<Old>();
			this->template store<New>(std::forward<Args>(args)...);
		}

		template<typename T>
			requires concepts::any_of<T, Ts...>
		[[nodiscard]] constexpr auto load() noexcept -> T& { return *std::launder(reinterpret_cast<T*>(data_)); }

		template<typename T>
			requires concepts::any_of<T, Ts...>
		[[nodiscard]] constexpr auto load() const noexcept -> const T& { return *std::launder(reinterpret_cast<const T*>(data_)); }

		// Compare pointers, returns true iff both AlignedUnion store the same pointer.
		// fixme: remove it?
		[[nodiscard]] constexpr auto operator==(const AlignedUnion& other) const noexcept -> bool
		{
			GAL_PROMETHEUS_DISABLE_WARNING_PUSH
			GAL_PROMETHEUS_DISABLE_WARNING_CLANG(-Wundefined-reinterpret-cast)

			return *reinterpret_cast<const void* const*>(&data_) == *reinterpret_cast<const void* const*>(&other.data_);

			GAL_PROMETHEUS_DISABLE_WARNING_POP
		}

		template<typename T>
			requires concepts::any_of<T, Ts...> and std::is_copy_constructible_v<T>
		[[nodiscard]] constexpr explicit operator T() const noexcept(std::is_nothrow_copy_constructible_v<T>) { return load<T>(); }

		template<typename T>
			requires concepts::any_of<T, Ts...>
		[[nodiscard]] constexpr explicit operator T&() noexcept { return load<T>(); }

		template<typename T>
			requires concepts::any_of<T, Ts...>
		[[nodiscard]] constexpr explicit operator const T&() const noexcept { return load<T>(); }

		template<typename T>
			requires concepts::any_of<T, Ts...>
		[[nodiscard]] constexpr auto equal(const AlignedUnion& other) const noexcept(noexcept(std::declval<const T&>() == std::declval<const T&>())) -> bool { return load<T>() == other.template load<T>(); }

		template<typename T>
			requires concepts::any_of<T, Ts...>
		[[nodiscard]] constexpr auto equal(const T& other) const noexcept(noexcept(std::declval<const T&>() == std::declval<const T&>())) -> bool { return load<T>() == other; }
	};
}
