// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>

namespace gal::prometheus::memory
{
	namespace reference_wrapper_detail
	{
		template<typename T>
		// ReSharper disable once CppFunctionIsNotImplemented
		auto ctor(std::type_identity_t<T&>) noexcept -> void;

		template<typename T>
		auto ctor(std::type_identity_t<T&&>) noexcept -> void = delete;

		template<typename T, typename U>
		struct ref_wrap_has_ctor_from : std::bool_constant<
					requires
					{
						reference_wrapper_detail::ctor<T>(std::declval<U>());
					}
				> {};
	}

	/**
	 * @brief A wrapper for std::reference_wrapper, but propagating const.
	 */
	template<typename T>
	class RefWrapper
	{
	public:
		static_assert(std::is_object_v<T> || std::is_function_v<T>, "reference_wrapper<T> requires T to be an object type or a function type.");

		using type = T;

	private:
		type* pointer_;

	public:
		template<typename U>
			requires std::conjunction_v<
				std::negation<std::is_same<std::remove_cvref_t<U>, RefWrapper>>,
				reference_wrapper_detail::ref_wrap_has_ctor_from<type, U>
			>
		constexpr explicit(false) RefWrapper(U&& value) noexcept(noexcept(reference_wrapper_detail::ctor<type>(std::declval<U>())))
			: pointer_{nullptr}
		{
			auto& ref = std::forward<U>(value);
			pointer_ = std::addressof(ref);
		}

		// ReSharper disable once CppNonExplicitConversionOperator
		constexpr explicit(false) operator type&() noexcept
		{
			return *pointer_;
		}

		// ReSharper disable once CppNonExplicitConversionOperator
		constexpr explicit (false) operator const type&() noexcept //
			requires (not std::is_const_v<type>) // avoid redefinition (RefWrapper<const T>)
		{
			return *pointer_;
		}

		[[nodiscard]] constexpr auto get() noexcept -> type&
		{
			return *pointer_;
		}

		[[nodiscard]] constexpr auto get() const noexcept -> const type&
		{
			return *pointer_;
		}

		template<typename... Args>
		constexpr auto operator()(Args&&... args) //
			noexcept(noexcept(std::invoke(*pointer_, std::forward<Args>(args)...))) //
			-> decltype(std::invoke(*pointer_, std::forward<Args>(args)...)) //
		{
			return std::invoke(*pointer_, std::forward<Args>(args)...);
		}
	};

	template<typename T>
	// ReSharper disable once CppInconsistentNaming
	RefWrapper(T&) -> RefWrapper<T>;

	template<typename T>
	[[nodiscard]] constexpr auto ref(T& value) noexcept -> RefWrapper<T>
	{
		return RefWrapper<T>{value};
	}

	template<typename T>
	[[nodiscard]] constexpr auto ref(const T&&) noexcept -> RefWrapper<T> = delete;

	template<typename T>
	[[nodiscard]] constexpr auto ref(RefWrapper<T> value) noexcept -> RefWrapper<T>
	{
		return value;
	}

	template<typename Ref>
	[[nodiscard]] constexpr auto ref(Ref value) noexcept -> RefWrapper<typename Ref::type> //
		requires std::is_same_v<Ref, std::reference_wrapper<typename Ref::type>>
	{
		return RefWrapper<typename Ref::type>{value.get()};
	}

	template<typename T>
	[[nodiscard]] constexpr auto cref(const T& value) noexcept -> RefWrapper<const T>
	{
		return RefWrapper<const T>{value};
	}

	template<typename T>
	[[nodiscard]] constexpr auto cref(const T&&) noexcept -> RefWrapper<T> = delete;

	template<typename T>
	[[nodiscard]] constexpr auto cref(RefWrapper<T> value) noexcept -> RefWrapper<const T>
	{
		return value;
	}

	template<typename Ref>
	[[nodiscard]] constexpr auto cref(Ref value) noexcept -> RefWrapper<const typename Ref::type> //
		requires std::is_same_v<Ref, std::reference_wrapper<typename Ref::type>>
	{
		return RefWrapper<const typename Ref::type>{value.get()};
	}

	namespace reference_wrapper_detail
	{
		template<typename T>
		concept ref_wrapper_t = std::is_same_v<T, RefWrapper<typename T::type>>;

		// for std::basic_common_reference
		template<typename RefWrap, typename Type, typename RefWrapQ, typename TypeQ>
		concept ref_wrap_common_reference_exists_with =
				ref_wrapper_t<RefWrap> and
				requires
				{
					typename std::common_reference_t<typename RefWrap::type&, TypeQ>;
				} and
				std::convertible_to<RefWrapQ, std::common_reference_t<typename RefWrap::type&, TypeQ>>;
	}
}

namespace std
{
	// fixme: std::invoke

	template<typename T>
	struct unwrap_reference<gal::prometheus::memory::RefWrapper<T>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T&;
	};

	template<typename RefWrap, typename Type, template<typename> typename RefWrapQ, template<typename> typename TypeQ>
		requires (
			gal::prometheus::memory::reference_wrapper_detail::ref_wrap_common_reference_exists_with<RefWrap, Type, RefWrapQ<RefWrap>, TypeQ<Type>> and
			not gal::prometheus::memory::reference_wrapper_detail::ref_wrap_common_reference_exists_with<Type, RefWrap, TypeQ<Type>, RefWrapQ<RefWrap>>
		)
	struct basic_common_reference<RefWrap, Type, RefWrapQ, TypeQ> // NOLINT(cert-dcl58-cpp)
	{
		using type = std::common_reference_t<typename RefWrap::type&, TypeQ<Type>>;
	};

	template<typename Type, typename RefWrap, template<typename> typename TypeQ, template<typename> typename RefWrapQ>
		requires (
			gal::prometheus::memory::reference_wrapper_detail::ref_wrap_common_reference_exists_with<RefWrap, Type, RefWrapQ<RefWrap>, TypeQ<Type>> and
			not gal::prometheus::memory::reference_wrapper_detail::ref_wrap_common_reference_exists_with<Type, RefWrap, TypeQ<Type>, RefWrapQ<RefWrap>>
		)
	struct basic_common_reference<Type, RefWrap, TypeQ, RefWrapQ> // NOLINT(cert-dcl58-cpp)
	{
		using type = std::common_reference_t<typename RefWrap::type&, TypeQ<Type>>;
	};
}
