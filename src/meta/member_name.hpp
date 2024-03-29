#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.meta:string;

import std;
import :name;

#else
#include <type_traits>

#include <meta/name.hpp>
#endif

namespace gal::prometheus::meta
{
	namespace member_name
	{
		template<std::size_t>
		struct placeholder
		{
			constexpr explicit(false) placeholder(auto) noexcept {}
		};

		struct any
		{
			template<typename T>
			// ReSharper disable once CppFunctionIsNotImplemented
			// ReSharper disable once CppNonExplicitConversionOperator
			constexpr explicit(false) operator T() const noexcept;
		};

		template<typename T>
		struct wrapper
		{
			T& ref;// NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
		};

		template<typename T>
		wrapper(T&) -> wrapper<T>;

		template<typename T>
		extern const T extern_var{};

		template<typename T, typename... Args>
		[[nodiscard]] constexpr auto member_size_impl() noexcept -> std::size_t//
		{
			if constexpr (requires { T{Args{}...}; } and not requires { T{Args{}..., any{}}; })//
			{
				return sizeof...(Args);
			}
			else { return member_size_impl<T, Args..., any>(); }
		}

		template<typename T>
		[[nodiscard]] constexpr auto member_size() noexcept -> std::size_t//
		{
			return member_size_impl<std::remove_cvref_t<T>>();
		}

		template<typename T>
		[[nodiscard]] constexpr auto member_size(const T&) noexcept -> std::size_t//
		{
			return member_size<T>();
		}

		template<typename Function, typename T>
		[[nodiscard]] constexpr auto visit(Function function, const T& value) noexcept -> decltype(auto)
		{
			#if __cpp_structured_bindings >= 202401L
			const auto& [... vs] = value;
			return std::invoke(function, vs...);
			#else
			if constexpr (constexpr auto size = member_size<T>();
				size == 1)
			{
				const auto& [m0] = value;
				return std::invoke(function, m0);
			}
			else if constexpr (size == 2)
			{
				const auto& [m0, m1] = value;
				return std::invoke(function, m0, m1);
			}
			else if constexpr (size == 3)
			{
				const auto& [m0, m1, m2] = value;
				return std::invoke(function, m0, m1, m2);
			}
			else if constexpr (size == 4)
			{
				const auto& [m0, m1, m2, m3] = value;
				return std::invoke(function, m0, m1, m2, m3);
			}
			else if constexpr (size == 5)
			{
				const auto& [m0, m1, m2, m3, m4] = value;
				return std::invoke(function, m0, m1, m2, m3, m4);
			}
			else if constexpr (size == 6)
			{
				const auto& [m0, m1, m2, m3, m4, m5] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5);
			}
			else if constexpr (size == 7)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6);
			}
			else if constexpr (size == 8)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7);
			}
			else if constexpr (size == 9)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8);
			}
			else if constexpr (size == 10)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9);
			}
			else if constexpr (size == 11)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10);
			}
			else if constexpr (size == 12)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11);
			}
			else if constexpr (size == 13)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12);
			}
			else if constexpr (size == 14)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13);
			}
			else if constexpr (size == 15)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14);
			}
			else if constexpr (size == 16)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15);
			}
			else if constexpr (size == 17)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16);
			}
			else if constexpr (size == 18)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17);
			}
			else if constexpr (size == 19)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18);
			}
			else if constexpr (size == 20)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19);
			}
			else if constexpr (size == 21)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20);
			}
			else if constexpr (size == 22)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21);
			}
			else if constexpr (size == 23)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22);
			}
			else if constexpr (size == 24)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23);
			}
			else if constexpr (size == 25)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24);
			}
			else if constexpr (size == 26)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25);
			}
			else if constexpr (size == 27)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26);
			}
			else if constexpr (size == 28)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27);
			}
			else if constexpr (size == 29)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28);
			}
			else if constexpr (size == 30)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29);
			}
			else if constexpr (size == 31)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30);
			}
			else if constexpr (size == 32)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31);
			}
			else if constexpr (size == 33)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32);
			}
			else if constexpr (size == 34)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33);
			}
			else if constexpr (size == 35)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34);
			}
			else if constexpr (size == 36)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35);
			}
			else if constexpr (size == 37)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36);
			}
			else if constexpr (size == 38)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37);
			}
			else if constexpr (size == 39)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38);
			}
			else if constexpr (size == 40)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39);
			}
			else if constexpr (size == 41)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40);
			}
			else if constexpr (size == 42)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41);
			}
			else if constexpr (size == 43)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42);
			}
			else if constexpr (size == 44)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43);
			}
			else if constexpr (size == 45)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44);
			}
			else if constexpr (size == 46)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45);
			}
			else if constexpr (size == 47)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46);
			}
			else if constexpr (size == 48)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47);
			}
			else if constexpr (size == 49)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48);
			}
			else if constexpr (size == 50)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49);
			}
			else if constexpr (size == 51)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50);
			}
			else if constexpr (size == 52)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51);
			}
			else if constexpr (size == 53)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52);
			}
			else if constexpr (size == 54)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53);
			}
			else if constexpr (size == 55)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54);
			}
			else if constexpr (size == 56)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55);
			}
			else if constexpr (size == 57)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56);
			}
			else if constexpr (size == 58)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57);
			}
			else if constexpr (size == 59)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58);
			}
			else if constexpr (size == 60)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59);
			}
			else if constexpr (size == 61)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60);
			}
			else if constexpr (size == 62)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61);
			}
			else if constexpr (size == 63)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62);
			}
			else if constexpr (size == 64)
			{
				const auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62, m63] = value;
				return std::invoke(function, m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62, m63);
			}
			else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
			#endif
		}

		template<std::size_t N>
		constexpr auto nth_element_impl = []<std::size_t... Index>(std::index_sequence<Index...>) noexcept -> decltype(auto)//
		{
			return []<typename NthType>(placeholder<Index>&&..., NthType&& nth, auto&&...) noexcept -> decltype(auto)//
			{
				return std::forward<NthType>(nth);
			};
		}(std::make_index_sequence<N>{});

		template<std::size_t N, typename... Args>
			requires (N < sizeof...(Args))
		[[nodiscard]] constexpr auto nth_element(Args&&... args) noexcept -> decltype(auto)//
		{
			return nth_element_impl<N>(std::forward<Args>(args)...);
		}
	}

	template<typename T, std::size_t N>
		requires std::is_aggregate_v<T>
	[[nodiscard]] constexpr auto name_of_member() noexcept -> std::string_view
	{
		constexpr auto full_function_name = name::get_full_function_name<
			member_name::visit(
					[](const auto&... args) noexcept -> auto { return member_name::wrapper{member_name::nth_element<N>(args...)}; },
					member_name::extern_var<std::decay_t<T>>)//
		>();

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		// MSVC
		// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<struct `namespace`::member_name::wrapper<`member_type` const >{const `member_type`&:`namespace`::member_name::extern_var<struct `my_struct`>->`member_name`}>(void) noexcept
		constexpr auto full_function_name_size = full_function_name.size();

		constexpr std::string_view splitter{">->"};
		constexpr auto             splitter_size = splitter.size();

		// class std::basic_string_view<char,struct std::char_traits<char> > `__calling_convention` `namespace`::get_full_function_name<struct `namespace`::member_name::wrapper<`member_type` const >{const `member_type`&:`namespace`::member_name::extern_var<struct `my_struct`>->
		static_assert(full_function_name.find(splitter) != std::string_view::npos);
		constexpr auto full_function_name_prefix_size = full_function_name.find(splitter) + splitter_size;

		// }>(void) noexcept
		constexpr std::string_view suffix{"}>(void) noexcept"};
		constexpr auto             full_function_name_suffix_size = suffix.size() + 1;
		#elif defined(GAL_PROMETHEUS_COMPILER_CLANG) or defined(GAL_PROMETHEUS_COMPILER_CLANG_CL)
		// CLANG/CLANG-CL
		// std::string_view `namespace`::get_full_function_name() [Vs = <decltype(_Invoker1<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &, const `member_type` &>::_Call(static_cast<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &>(_Obj), static_cast<const `member_type` &>(_Arg1))){extern_var.`member_name`}>]
		constexpr auto full_function_name_size = full_function_name.size();

		constexpr std::string_view splitter{"extern_var."};
		constexpr auto             splitter_size = splitter.size();

		// std::string_view `namespace`::get_full_function_name() [Vs = <decltype(_Invoker1<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &, const `member_type` &>::_Call(static_cast<(lambda at `ABS_FILE_PATH`\member_name.hpp:413:6) &>(_Obj), static_cast<const `member_type` &>(_Arg1))){extern_var.
		// static_assert(full_function_name.find(splitter) != std::string_view::npos);
		constexpr auto full_function_name_prefix_size = full_function_name.find(splitter) + splitter_size;

		// }>]
		constexpr std::string_view suffix{"}>]"};
		constexpr auto             full_function_name_suffix_size = suffix.size() + 1;
		#else
		// GCC
		// constexpr std::string_view `namespace`::get_full_function_name() [with auto ...<anonymous> = {`namespace`::member_name::wrapper<const bool>{`namespace`::member_name::extern_var<`my_struct`>.`my_struct`::`member_name`}}; std::string_view = std::basic_string_view<char>]
		constexpr auto full_function_name_size = full_function_name.size();

		// fixme: find a suitable splitter.
		// extern_var<`my_struct`>.`my_struct`::`member_name`
		constexpr std::string_view type_name      = name_of<T>();
		constexpr auto             type_name_size = type_name.size() + 2; // 2 == `::`
		constexpr std::string_view splitter{">."};
		constexpr auto             splitter_size = splitter.size();

		// constexpr std::string_view `namespace`::get_full_function_name() [with auto ...<anonymous> = {`namespace`::member_name::wrapper<const bool>{`namespace`::member_name::extern_var<`my_struct`>.`my_struct`::
		static_assert(full_function_name.find(splitter) != std::string_view::npos);
		constexpr auto full_function_name_prefix_size = full_function_name.find(splitter) + splitter_size + type_name_size;

		// }}; std::string_view = std::basic_string_view<char>]
		constexpr std::string_view suffix{"}}; std::string_view = std::basic_string_view<char>]"};
		constexpr auto             full_function_name_suffix_size = suffix.size() + 1;
		#endif

		auto name = full_function_name;
		name.remove_prefix(full_function_name_prefix_size);
		name.remove_suffix(full_function_name_suffix_size);
		return name;
	}
}
