// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <tuple>
#include <functional>

#include <prometheus/macro.hpp>

#include <functional/aligned_union.hpp>

namespace gal::prometheus::functional
{
	template<typename Signature>
	struct function_reference_wrapper;

	namespace function_ref_detail
	{
		template<typename>
		struct compatible_function_pointer
		{
			using type = void;

			template<typename...>
			constexpr static auto value = false;
		};

		template<typename FunctionPointer>
			requires std::is_function_v<std::remove_pointer_t<std::remove_cvref_t<std::decay_t<FunctionPointer>>>>
		struct compatible_function_pointer<FunctionPointer>
		{
			using type = std::decay_t<FunctionPointer>;

			template<typename Return, typename... Args>
			constexpr static auto value =
					std::is_invocable_v<type, Args...> and
					(std::is_void_v<Return> or std::is_convertible_v<std::invoke_result_t<type, Args...>, Return>);
		};

		template<typename Return, typename... Args>
		struct reference_wrapper;

		template<typename>
		struct is_reference_wrapper : std::false_type {};

		template<typename Return, typename... Args>
		struct is_reference_wrapper<reference_wrapper<Return, Args...>> : std::true_type {};

		template<typename Return, typename... Args, template<typename, typename...> typename Wrapper>
			requires std::is_base_of_v<reference_wrapper<Return, Args...>, Wrapper<Return, Args...>>
		struct is_reference_wrapper<Wrapper<Return, Args...>> : std::true_type {};

		template<typename T>
		constexpr auto is_reference_wrapper_v = is_reference_wrapper<T>::value;

		template<typename T>
		concept reference_wrapper_t = is_reference_wrapper_v<T>;

		template<typename Return, typename... Args>
		struct reference_wrapper
		{
			using result_type = Return;
			using signature = result_type(Args...);

			using arguments = std::tuple<Args...>;
			constexpr static auto argument_size = std::tuple_size_v<arguments>;
			template<std::size_t Index>
				requires (Index < argument_size)
			using argument_type = std::tuple_element_t<Index, arguments>;

		private:
			using function_pointer = void (*)();
			using functor_pointer = void*;

			using data_type = AlignedUnion<function_pointer, functor_pointer>;
			using invoker = result_type (*)(data_type, Args...);

			struct constructor_tag {};

			data_type data_;
			invoker invoker_;

			template<typename FunctionType>
			constexpr static auto do_invoke_function(data_type data,
			                                         Args... args) //
				noexcept(noexcept(
					static_cast<result_type>(std::invoke(reinterpret_cast<FunctionType>(data.load<function_pointer>()), static_cast<Args>(args)...))
				))
				-> result_type
			{
				auto pointer = data.load<function_pointer>();
				auto function = reinterpret_cast<FunctionType>(pointer); // NOLINT(clang-diagnostic-cast-function-type-strict)

				return static_cast<result_type>(std::invoke(function, static_cast<Args>(args)...));
			}

			template<typename Functor>
			constexpr static auto do_invoke_functor(data_type data,
			                                        Args... args) //
				noexcept(noexcept(
						static_cast<result_type>(std::invoke(*static_cast<Functor*>(data.load<functor_pointer>()), static_cast<Args>(args)...)
						))
				)
				-> result_type
			{
				auto pointer = data.load<functor_pointer>();
				auto& functor = *static_cast<Functor*>(pointer);

				return static_cast<result_type>(std::invoke(functor, static_cast<Args>(args)...));
			}

			template<typename FunctionPointer>
			constexpr reference_wrapper(constructor_tag, FunctionPointer function) noexcept
				: data_{data_type::constructor_tag<function_pointer>{}, reinterpret_cast<function_pointer>(function)},
				  // NOLINT(clang-diagnostic-cast-function-type-strict)
				  invoker_{&do_invoke_function<typename compatible_function_pointer<FunctionPointer>::type>}
			{
				// throw exception?
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(function != nullptr, "function pointer must not be null");
			}

		public:
			template<typename FunctionPointer>
				requires compatible_function_pointer<FunctionPointer>::template value<Return, Args...> //
			constexpr explicit(false) reference_wrapper(FunctionPointer function)
				noexcept(std::is_nothrow_constructible_v<reference_wrapper, constructor_tag, FunctionPointer>)
				: reference_wrapper{constructor_tag{}, function} {}

			/**
			 * @brief Creates a reference to the function created by the stateless lambda.
			 */
			template<typename StatelessLambda>
				requires
				(not compatible_function_pointer<StatelessLambda>::template value<Return, Args...>) and
				std::is_invocable_v<StatelessLambda, Args...> and
				(std::is_void_v<Return> or std::is_convertible_v<std::invoke_result_t<StatelessLambda, Args...>, Return>) and
				requires(StatelessLambda& functor)
				{
					(+functor)(std::declval<Args>()...);
				}
			// note: const reference to avoid ambiguous
			constexpr explicit(false) reference_wrapper(const StatelessLambda& functor)
				: reference_wrapper{constructor_tag{}, +functor} {}

			/**
			 * @brief Creates a reference to the specified functor.
			 * It will store a pointer to the function object, so it must live as long as the reference.
			 */
			template<typename Functor>
				requires
				(not compatible_function_pointer<Functor>::template value<Return, Args...>) and
				(not is_reference_wrapper_v<Functor>) and
				std::is_invocable_v<Functor, Args...> and
				(std::is_void_v<Return> or std::is_convertible_v<std::invoke_result_t<Functor, Args...>, Return>)
			constexpr explicit(false) reference_wrapper(Functor& functor) noexcept
				: data_{data_type::constructor_tag<functor_pointer>{}, const_cast<functor_pointer>(static_cast<const void*>(&functor))},
				  invoker_{&do_invoke_functor<Functor>} {}

			constexpr auto operator()(Args... args) const noexcept(noexcept(std::invoke(invoker_, data_, static_cast<Args>(args)...)))
			{
				return std::invoke(invoker_, data_, static_cast<Args>(args)...);
			}
		};
	}

	/**
	 * @brief A reference to a function.
	 * This is a lightweight reference to a function.
	 * It can refer to any function that is compatible with given signature.
	 *
	 * A function is compatible if it is callable with regular function call syntax from the given argument types,
	 * and its return type is either implicitly convertible to the specified return type or the specified return type is `void`.
	 *
	 * In general, it will store a pointer to the functor, requiring a lvalue.
	 * But if it is created with a function pointer or something convertible to a function pointer,
	 * it will store the function pointer itself.
	 */
	template<typename Return, typename... Args>
	struct function_reference_wrapper<Return(Args...)> : function_ref_detail::reference_wrapper<Return, Args...>
	{
		using function_ref_detail::reference_wrapper<Return, Args...>::reference_wrapper;
	};

	template<typename Return, typename... Args, typename T>
	[[nodiscard]] constexpr auto ref(T&& object)
		noexcept(std::is_nothrow_constructible_v<function_reference_wrapper<Return(Args...)>, T>)
		-> function_reference_wrapper<Return(Args...)>
	{
		return function_reference_wrapper<Return(Args...)>{std::forward<T>(object)};
	}
}
