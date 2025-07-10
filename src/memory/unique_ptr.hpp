// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <memory>
#include <type_traits>

namespace gal::prometheus::memory
{
	namespace unique_ptr_detail
	{
		template<typename Deleter>
		struct default_constructible_deleter : std::conjunction<
					std::negation<std::is_pointer<Deleter>>,
					std::is_constructible<Deleter>
				> {};

		template<typename Deleter>
		constexpr auto default_constructible_deleter_v = default_constructible_deleter<Deleter>::value;

		template<typename Deleter>
		concept default_constructible_deleter_t = default_constructible_deleter_v<Deleter>;
	}

	template<typename T, typename Deleter = std::default_delete<T>>
	class UniquePointer;

	/**
	 * @brief A wrapper for std::unique_ptr, but propagating const.
	 */
	template<typename T, typename Deleter>
	class UniquePointer final
	{
		template<typename, typename>
		friend class UniquePointer;

		using phantom_type = std::unique_ptr<T, Deleter>;

	public:
		using pointer = typename phantom_type::pointer;
		using element_type = typename phantom_type::element_type;
		using deleter_type = typename phantom_type::deleter_type;

	private:
		template<typename U, typename D, typename Trait>
		constexpr static auto enable_conversion =
				std::conjunction_v<
					std::negation<std::is_array<U>>,
					std::is_convertible<typename UniquePointer<U, D>::pointer, pointer>,
					Trait
				>;

		phantom_type phantom_;

	public:
		// =====================================
		// UniquePointer p{};

		constexpr UniquePointer() noexcept //
			requires unique_ptr_detail::default_constructible_deleter_v<deleter_type>
			: phantom_{} {}

		// =====================================
		// UniquePointer p{nullptr};

		constexpr explicit(false) UniquePointer(std::nullptr_t) noexcept //
			requires unique_ptr_detail::default_constructible_deleter_v<deleter_type>
			: phantom_{} {}

		constexpr auto operator=(std::nullptr_t) noexcept -> UniquePointer&
		{
			phantom_ = nullptr;
			return *this;
		}

		// =====================================
		// UniquePointer p{new T{...}};

		constexpr explicit UniquePointer(pointer ptr) noexcept //
			requires unique_ptr_detail::default_constructible_deleter_v<deleter_type>
			: phantom_{ptr} {}

		constexpr UniquePointer(pointer ptr, const deleter_type& deleter) noexcept //
			requires std::conjunction_v<
				std::is_constructible<deleter_type, const deleter_type&>
			>
			: phantom_{ptr, deleter} {}

		constexpr UniquePointer(pointer ptr, deleter_type&& deleter) noexcept //
			requires std::conjunction_v<
				std::negation<std::is_reference<deleter_type>>,
				std::is_constructible<deleter_type, deleter_type>
			>
			: phantom_{ptr, std::move(deleter)} {}

		constexpr UniquePointer(pointer, std::remove_reference_t<deleter_type>&&) noexcept //
			requires std::conjunction_v<
				std::is_reference<deleter_type>,
				std::is_constructible<deleter_type, std::remove_reference<deleter_type>>
			> //
		= delete;

		// =====================================
		// UniquePointer p1{...};
		// UniquePointer p2{p1};

		constexpr UniquePointer(const UniquePointer&) noexcept = delete;

		constexpr auto operator=(const UniquePointer&) noexcept -> UniquePointer& = delete;

		// =====================================
		// UniquePointer p1{...};
		// UniquePointer p2{std::move(p1)};

		constexpr UniquePointer(UniquePointer&& other) noexcept //
			requires std::conjunction_v<
				std::is_move_constructible<deleter_type>
			>
			: phantom_{std::move(other.phantom_)} {}

		constexpr auto operator=(UniquePointer&& other) noexcept -> UniquePointer& //
			requires std::conjunction_v<
				std::is_move_assignable<deleter_type>
			>
		{
			phantom_ = std::move(other.phantom_);
			return *this;
		}

		template<typename U, typename D>
			requires enable_conversion<U, D, std::conditional_t<std::is_reference_v<deleter_type>, std::is_same<D, deleter_type>, std::is_convertible<D, deleter_type>>>
		constexpr explicit(false) UniquePointer(UniquePointer<U, D>&& other) noexcept
			: phantom_{std::move(other)} {}

		template<typename U, typename D>
			requires enable_conversion<U, D, std::is_assignable<deleter_type&, D>>
		constexpr auto operator=(UniquePointer<U, D>&& other) noexcept -> UniquePointer&
		{
			phantom_ = std::move(other).phantom_;
			return *this;
		}

		constexpr ~UniquePointer() noexcept = default;

		constexpr auto swap(UniquePointer& other) noexcept -> void
		{
			phantom_.swap(other.phantom_);
		}

		constexpr auto release() noexcept -> pointer
		{
			return phantom_.release();
		}

		constexpr auto reset(pointer ptr = nullptr) noexcept -> void
		{
			phantom_.reset(ptr);
		}

		[[nodiscard]] constexpr auto get_deleter() noexcept -> deleter_type&
		{
			return phantom_.get_deleter();
		}

		[[nodiscard]] constexpr auto get_deleter() const noexcept -> const deleter_type&
		{
			return phantom_.get_deleter();
		}

		[[nodiscard]] constexpr auto operator*() noexcept(noexcept(*std::declval<pointer>())) -> element_type&
		{
			return phantom_.operator*();
		}

		[[nodiscard]] constexpr auto operator*() const noexcept(noexcept(*std::declval<pointer>())) -> const element_type&
		{
			return phantom_.operator*();
		}

		[[nodiscard]] constexpr auto operator->() noexcept -> pointer
		{
			return phantom_.operator->();
		}

		[[nodiscard]] constexpr auto operator->() const noexcept -> std::add_pointer_t<std::add_const_t<element_type>>
		{
			return phantom_.operator->();
		}

		[[nodiscard]] constexpr auto get() noexcept -> pointer
		{
			return phantom_.get();
		}

		[[nodiscard]] constexpr auto get() const noexcept -> std::add_pointer_t<std::add_const_t<element_type>>
		{
			return phantom_.get();
		}

		[[nodiscard]] constexpr explicit operator bool() const noexcept
		{
			return static_cast<bool>(phantom_);
		}
	};

	/**
	 * @brief A wrapper for std::unique_ptr, but propagating const.
	 */
	template<typename T, typename Deleter>
	class UniquePointer<T[], Deleter> final
	{
		template<typename, typename>
		friend class UniquePointer;

		using phantom_type = std::unique_ptr<T[], Deleter>;

	public:
		using pointer = typename phantom_type::pointer;
		using element_type = typename phantom_type::element_type;
		using deleter_type = typename phantom_type::deleter_type;

	private:
		template<typename U, typename IsNullptr = std::is_same<U, std::nullptr_t>>
		constexpr static auto enable_ctor_reset =
				std::disjunction_v<
					IsNullptr,
					std::is_same<U, pointer>,
					std::conjunction<
						std::is_pointer<U>,
						std::is_same<pointer, element_type*>,
						std::is_convertible<std::remove_pointer_t<U>(*)[], element_type(*)[]>
					>
				>;

		template<typename U, typename D, typename Trait>
		constexpr static auto enable_conversion =
				std::conjunction_v<
					std::is_array<U>,
					std::is_same<pointer, element_type*>,
					std::is_same<typename UniquePointer<U, D>::pointer, typename UniquePointer<U, D>::element_type*>,
					std::is_convertible<typename UniquePointer<U, D>::element_type(*)[], element_type(*)[]>,
					Trait
				>;

		phantom_type phantom_;

	public:
		// =====================================
		// UniquePointer p{};

		constexpr UniquePointer() noexcept //
			requires unique_ptr_detail::default_constructible_deleter_v<deleter_type>
			: phantom_{} {}

		// =====================================
		// UniquePointer p{nullptr};

		constexpr explicit(false) UniquePointer(std::nullptr_t) noexcept //
			requires unique_ptr_detail::default_constructible_deleter_v<deleter_type>
			: phantom_{} {}

		constexpr auto operator=(std::nullptr_t) noexcept -> UniquePointer&
		{
			phantom_ = nullptr;
			return *this;
		}

		// =====================================
		// UniquePointer p{new T{...}};

		template<typename U>
			requires enable_ctor_reset<U>
		constexpr explicit UniquePointer(U ptr) noexcept //
			requires unique_ptr_detail::default_constructible_deleter_v<deleter_type>
			: phantom_{ptr} {}

		template<typename U>
			requires enable_ctor_reset<U>
		constexpr UniquePointer(U ptr, const deleter_type& deleter) noexcept //
			requires std::conjunction_v<
				std::is_constructible<deleter_type, const deleter_type&>
			>
			: phantom_{ptr, deleter} {}

		template<typename U>
			requires enable_ctor_reset<U>
		constexpr UniquePointer(U ptr, deleter_type&& deleter) noexcept //
			requires std::conjunction_v<
				std::negation<std::is_reference<deleter_type>>,
				std::is_constructible<deleter_type, deleter_type>
			>
			: phantom_{ptr, std::move(deleter)} {}

		template<typename U>
		constexpr UniquePointer(U pointer, std::remove_reference_t<deleter_type>&&) noexcept //
			requires std::conjunction_v<
				std::is_reference<deleter_type>,
				std::is_constructible<deleter_type, std::remove_reference<deleter_type>>
			> //
		= delete;

		// =====================================
		// UniquePointer p1{...};
		// UniquePointer p2{p1};

		constexpr UniquePointer(const UniquePointer&) noexcept = delete;

		constexpr auto operator=(const UniquePointer&) noexcept -> UniquePointer& = delete;

		// =====================================
		// UniquePointer p1{...};
		// UniquePointer p2{std::move(p1)};

		constexpr UniquePointer(UniquePointer&& other) noexcept //
			requires std::conjunction_v<
				std::is_move_constructible<deleter_type>
			>
			: phantom_{std::move(other.phantom_)} {}

		constexpr auto operator=(UniquePointer&& other) noexcept -> UniquePointer& //
			requires std::conjunction_v<
				std::is_move_assignable<deleter_type>
			>
		{
			phantom_ = std::move(other.phantom_);
			return *this;
		}

		template<typename U, typename D>
			requires enable_conversion<U, D, std::conditional_t<std::is_reference_v<deleter_type>, std::is_same<D, deleter_type>, std::is_convertible<D, deleter_type>>>
		constexpr explicit(false) UniquePointer(UniquePointer<U, D>&& other) noexcept
			: phantom_{std::move(other)} {}

		template<typename U, typename D>
			requires enable_conversion<U, D, std::is_assignable<deleter_type&, D>>
		constexpr auto operator=(UniquePointer<U, D>&& other) noexcept -> UniquePointer&
		{
			phantom_ = std::move(other).phantom_;
			return *this;
		}

		constexpr ~UniquePointer() noexcept = default;

		constexpr auto swap(UniquePointer& other) noexcept -> void
		{
			phantom_.swap(other.phantom_);
		}

		constexpr auto release() noexcept -> pointer
		{
			return phantom_.release();
		}

		template<typename U>
			requires enable_ctor_reset<U, std::false_type>
		constexpr auto reset(U ptr) noexcept -> void
		{
			phantom_.reset(ptr);
		}

		[[nodiscard]] constexpr auto get_deleter() noexcept -> deleter_type&
		{
			return phantom_.get_deleter();
		}

		[[nodiscard]] constexpr auto get_deleter() const noexcept -> const deleter_type&
		{
			return phantom_.get_deleter();
		}

		[[nodiscard]] constexpr auto operator[](const std::size_t index) noexcept -> element_type&
		{
			return phantom_[index];
		}

		[[nodiscard]] constexpr auto operator[](const std::size_t index) const noexcept -> const element_type&
		{
			return phantom_[index];
		}

		[[nodiscard]] constexpr auto get() noexcept -> pointer
		{
			return phantom_.get();
		}

		[[nodiscard]] constexpr auto get() const noexcept -> std::add_pointer_t<std::add_const_t<element_type>>
		{
			return phantom_.get();
		}

		[[nodiscard]] constexpr explicit operator bool() const noexcept
		{
			return static_cast<bool>(phantom_);
		}
	};

	template<typename T, typename... Args>
		requires std::conjunction_v<
			std::negation<std::is_array<T>>,
			std::is_constructible<T, Args...>
		>
	[[nodiscard]] constexpr auto make_unique(Args&&... args) noexcept -> UniquePointer<T>
	{
		// return UniquePointer<T>{new T{std::forward<Args>(args)...}};
		return UniquePointer<T>{new T(std::forward<Args>(args)...)};
	}

	template<typename T>
		requires std::conjunction_v<
			std::is_array<T>,
			std::bool_constant<std::extent_v<T> == 0>
		>
	[[nodiscard]] constexpr auto make_unique(const std::size_t size) noexcept -> UniquePointer<T>
	{
		using element_type = std::remove_extent_t<T>;
		// initialized
		return UniquePointer<T>{new element_type[size]{}};
	}

	template<typename T, typename... Args>
		requires std::conjunction_v<
			std::bool_constant<std::extent_v<T> != 0>
		>
	[[nodiscard]] constexpr auto make_unique(Args&&...) noexcept -> UniquePointer<T> = delete;

	template<typename T>
		requires std::conjunction_v<
			std::negation<std::is_array<T>>,
			std::is_default_constructible<T>
		>
	[[nodiscard]] constexpr auto make_unique_for_overwrite() noexcept -> UniquePointer<T>
	{
		// unspecified value (for trivial type)
		return UniquePointer<T>{new T};
	}

	template<typename T>
		requires std::conjunction_v<
			std::is_unbounded_array<T>
		>
	[[nodiscard]] constexpr auto make_unique_for_overwrite(const std::size_t size) noexcept -> UniquePointer<T>
	{
		using element_type = std::remove_extent_t<T>;
		// unspecified value (for trivial type)
		return UniquePointer<T>{new element_type[size]};
	}

	template<typename T, typename... Args>
		requires std::conjunction_v<
			std::is_bounded_array<T>
		>
	[[nodiscard]] constexpr auto make_unique_for_overwrite(Args&&...) noexcept = delete;

	template<typename T, typename Deleter>
		requires std::conjunction_v<
			std::is_swappable<Deleter>
		>
	constexpr auto swap(UniquePointer<T, Deleter>& lhs, UniquePointer<T, Deleter>& rhs) noexcept -> void
	{
		lhs.swap(rhs);
	}

	template<typename T1, typename Deleter1, typename T2, typename Deleter2>
	[[nodiscard]] constexpr auto operator==(const UniquePointer<T1, Deleter1>& lhs, const UniquePointer<T2, Deleter2>& rhs) noexcept -> bool
	{
		return lhs.get() == rhs.get();
	}

	template<typename T1, typename Deleter1, typename T2, typename Deleter2>
	[[nodiscard]] constexpr auto operator<(const UniquePointer<T1, Deleter1>& lhs, const UniquePointer<T2, Deleter2>& rhs) noexcept -> bool
	{
		using p1 = typename UniquePointer<T1, Deleter1>::pointer;
		using p2 = typename UniquePointer<T2, Deleter2>::pointer;
		using common = std::common_type_t<p1, p2>;
		return std::less<common>{}(lhs.get(), rhs.get());
	}

	template<typename T1, typename Deleter1, typename T2, typename Deleter2>
	[[nodiscard]] constexpr auto operator<=(const UniquePointer<T1, Deleter1>& lhs, const UniquePointer<T2, Deleter2>& rhs) noexcept -> bool
	{
		return not(rhs < lhs);
	}

	template<typename T1, typename Deleter1, typename T2, typename Deleter2>
	[[nodiscard]] constexpr auto operator>(const UniquePointer<T1, Deleter1>& lhs, const UniquePointer<T2, Deleter2>& rhs) noexcept -> bool
	{
		return rhs < lhs;
	}

	template<typename T1, typename Deleter1, typename T2, typename Deleter2>
	[[nodiscard]] constexpr auto operator>=(const UniquePointer<T1, Deleter1>& lhs, const UniquePointer<T2, Deleter2>& rhs) noexcept -> bool
	{
		return not(lhs < rhs);
	}

	template<typename T1, typename Deleter1, typename T2, typename Deleter2>
		requires std::three_way_comparable_with<
			typename UniquePointer<T1, Deleter1>::pointer,
			typename UniquePointer<T2, Deleter2>::pointer
		>
	[[nodiscard]] constexpr auto operator<=>(
		const UniquePointer<T1, Deleter1>& lhs,
		const UniquePointer<T2, Deleter2>& rhs
	) noexcept -> std::compare_three_way_result_t<typename UniquePointer<T1, Deleter1>::pointer, typename UniquePointer<T2, Deleter2>::pointer>
	{
		return lhs.get() <=> rhs.get();
	}

	template<typename T, typename Deleter>
	[[nodiscard]] constexpr auto operator==(const UniquePointer<T, Deleter>& lhs, std::nullptr_t) noexcept -> bool
	{
		return not lhs;
	}

	template<typename T, typename Deleter>
	[[nodiscard]] constexpr auto operator<(const UniquePointer<T, Deleter>& lhs, std::nullptr_t) noexcept -> bool
	{
		using p = typename UniquePointer<T, Deleter>::pointer;
		return std::less<p>{}(lhs.get(), nullptr);
	}

	template<typename T, typename Deleter>
	[[nodiscard]] constexpr auto operator<(std::nullptr_t, const UniquePointer<T, Deleter>& rhs) noexcept -> bool
	{
		using p = typename UniquePointer<T, Deleter>::pointer;
		return std::less<p>{}(nullptr, rhs.get());
	}

	template<typename T, typename Deleter>
	[[nodiscard]] constexpr auto operator<=(const UniquePointer<T, Deleter>& lhs, std::nullptr_t) noexcept -> bool
	{
		return not(nullptr < lhs);
	}

	template<typename T, typename Deleter>
	[[nodiscard]] constexpr auto operator<=(std::nullptr_t, const UniquePointer<T, Deleter>& rhs) noexcept -> bool
	{
		return not(rhs < nullptr);
	}

	template<typename T, typename Deleter>
	[[nodiscard]] constexpr auto operator>(const UniquePointer<T, Deleter>& lhs, std::nullptr_t) noexcept -> bool
	{
		return nullptr < lhs;
	}

	template<typename T, typename Deleter>
	constexpr auto operator>(std::nullptr_t, const UniquePointer<T, Deleter>& rhs) noexcept -> bool
	{
		return rhs < nullptr;
	}

	template<typename T, typename Deleter>
	[[nodiscard]] constexpr auto operator>=(const UniquePointer<T, Deleter>& lhs, std::nullptr_t) noexcept -> bool
	{
		return not(lhs < nullptr);
	}

	template<typename T, typename Deleter>
	[[nodiscard]] constexpr auto operator>=(std::nullptr_t, const UniquePointer<T, Deleter>& rhs) noexcept -> bool
	{
		return not(nullptr < rhs);
	}

	template<typename T, typename Deleter>
		requires std::three_way_comparable<typename UniquePointer<T, Deleter>::pointer>
	[[nodiscard]] constexpr auto operator<=>(
		const UniquePointer<T, Deleter>& lhs,
		std::nullptr_t //
	) noexcept -> std::compare_three_way_result_t<typename UniquePointer<T, Deleter>::pointer>
	{
		return lhs.get() <=> static_cast<typename UniquePointer<T, Deleter>::pointer>(nullptr);
	}

	template<typename Element, typename Traits, typename T, typename Deleter>
	constexpr auto operator<<(std::basic_ostream<Element, Traits>& out, const UniquePointer<T, Deleter>& pointer) noexcept -> std::basic_ostream<Element, Traits> //
		requires requires
		{
			out << pointer.get();
		}
	{
		out << pointer.get();
		return out;
	}
}

namespace std {}
