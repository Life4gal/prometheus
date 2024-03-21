#pragma once

#include <type_traits>

namespace gal::prometheus::functional
{
	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	template<typename FunctionType>
	struct y_combinator
	{
		using function_type = FunctionType;

		function_type function;

		template<typename... Args>
		constexpr auto operator()(Args&&... args) const noexcept(std::is_nothrow_invocable_v<function_type, decltype(*this), Args...>) -> decltype(auto)//
		{
			// we pass ourselves to function, the lambda should take the first argument as `auto&& self` or similar.
			return std::invoke(function, *this, std::forward<Args>(args)...);
		}
	};

	template<typename... Ts>
	struct overloaded : Ts...
	{
		constexpr explicit overloaded(Ts&&... ts) noexcept((std::is_nothrow_constructible_v<Ts, decltype(ts)> and ...))
			: Ts{std::forward<Ts>(ts)}... {}
	};

	GAL_PROMETHEUS_MODULE_EXPORT_END
}
