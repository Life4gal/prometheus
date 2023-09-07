// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

// fixme: std::unreachable() / [[assume]]

#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
#define GAL_PROMETHEUS_ASSUME(condition) __assume(condition)
#define GAL_PROMETHEUS_UNREACHABLE() GAL_PROMETHEUS_ASSUME(0)
#define GAL_PROMETHEUS_DEBUG_TRAP() __debugbreak()
#define GAL_PROMETHEUS_IMPORTED_SYMBOL __declspec(dllimport)
#define GAL_PROMETHEUS_EXPORTED_SYMBOL __declspec(dllexport)
#define GAL_PROMETHEUS_LOCAL_SYMBOL

#define GAL_PROMETHEUS_DISABLE_WARNING_PUSH __pragma(warning(push))
#define GAL_PROMETHEUS_DISABLE_WARNING_POP __pragma(warning(pop))
#define GAL_PROMETHEUS_DISABLE_WARNING(warningNumber) __pragma(warning(disable \
																			   : warningNumber))
#elif defined(GAL_PROMETHEUS_COMPILER_GNU)
	#define GAL_PROMETHEUS_ASSUME(condition) \
		do \
		{ \
			if (not(condition)) \
			{ \
				GAL_PROMETHEUS_UNREACHABLE(); \
			} \
		} while (false)
	#define GAL_PROMETHEUS_UNREACHABLE() __builtin_unreachable()
	#define GAL_PROMETHEUS_DEBUG_TRAP() __builtin_trap()
	#define GAL_PROMETHEUS_IMPORTED_SYMBOL __attribute__((visibility("default")))
	#define GAL_PROMETHEUS_EXPORTED_SYMBOL __attribute__((visibility("default")))
	#define GAL_PROMETHEUS_LOCAL_SYMBOL __attribute__((visibility("hidden")))

	#define GAL_PROMETHEUS_DISABLE_WARNING_PUSH _Pragma("GCC diagnostic push")
	#define GAL_PROMETHEUS_DISABLE_WARNING_POP _Pragma("GCC diagnostic pop")

	#define GAL_PROMETHEUS_PRIVATE_DO_PRAGMA(X) _Pragma(#X)
	#define GAL_PROMETHEUS_DISABLE_WARNING(warningName) GAL_PROMETHEUS_PRIVATE_DO_PRAGMA(GCC diagnostic ignored #warningName)
#elif defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) || defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) || defined(GAL_PROMETHEUS_COMPILER_CLANG)
	#define GAL_PROMETHEUS_ASSUME(condition) __builtin_assume(not not(condition))
	#define GAL_PROMETHEUS_UNREACHABLE() __builtin_unreachable()
	#define GAL_PROMETHEUS_DEBUG_TRAP() __builtin_trap()
	#define GAL_PROMETHEUS_IMPORTED_SYMBOL __attribute__((visibility("default")))
	#define GAL_PROMETHEUS_EXPORTED_SYMBOL __attribute__((visibility("default")))
	#define GAL_PROMETHEUS_LOCAL_SYMBOL __attribute__((visibility("hidden")))

	#define GAL_PROMETHEUS_DISABLE_WARNING_PUSH _Pragma("clang diagnostic push")
	#define GAL_PROMETHEUS_DISABLE_WARNING_POP _Pragma("clang diagnostic pop")

	#define GAL_PROMETHEUS_PRIVATE_DO_PRAGMA(X) _Pragma(#X)
	#define GAL_PROMETHEUS_DISABLE_WARNING(warningName) GAL_PROMETHEUS_PRIVATE_DO_PRAGMA(clang diagnostic ignored #warningName)
#else
	#define GAL_PROMETHEUS_UNREACHABLE()
	#define GAL_PROMETHEUS_DEBUG_TRAP()
	#define GAL_PROMETHEUS_IMPORTED_SYMBOL
	#define GAL_PROMETHEUS_EXPORTED_SYMBOL
	#define GAL_PROMETHEUS_LOCAL_SYMBOL

	#define GAL_PROMETHEUS_DISABLE_WARNING_PUSH
	#define GAL_PROMETHEUS_DISABLE_WARNING_POP
	#define GAL_PROMETHEUS_DISABLE_WARNING(warningName)
#endif

#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
#define GAL_PROMETHEUS_DISABLE_WARNING_MSVC(...) GAL_PROMETHEUS_DISABLE_WARNING(__VA_ARGS__)
#else
	#define GAL_PROMETHEUS_DISABLE_WARNING_MSVC(...)
#endif

#if defined(GAL_PROMETHEUS_COMPILER_GNU)
	#define GAL_PROMETHEUS_DISABLE_WARNING_GNU(...) GAL_PROMETHEUS_DISABLE_WARNING(__VA_ARGS__)
#else
#define GAL_PROMETHEUS_DISABLE_WARNING_GNU(...)
#endif

#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) || defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) || defined(GAL_PROMETHEUS_COMPILER_CLANG)
	#define GAL_PROMETHEUS_DISABLE_WARNING_CLANG(...) GAL_PROMETHEUS_DISABLE_WARNING(__VA_ARGS__)
#else
#define GAL_PROMETHEUS_DISABLE_WARNING_CLANG(...)
#endif

#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
#define GAL_PROMETHEUS_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
	#define GAL_PROMETHEUS_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if defined(GAL_PROMETHEUS_COMPILER_APPLE_CLANG) || defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) || defined(GAL_PROMETHEUS_COMPILER_CLANG)
	#define GAL_PROMETHEUS_NO_DESTROY [[clang::no_destroy]]
#else
#define GAL_PROMETHEUS_NO_DESTROY
#endif

#define GAL_PROMETHEUS_STATIC_UNREACHABLE(...)                                                             \
	[]<bool AlwaysFalse>() { static_assert(AlwaysFalse, "[UNREACHABLE BRANCH]: \"" __VA_ARGS__ "\""); }(); \
	GAL_PROMETHEUS_UNREACHABLE()

#if __has_cpp_attribute(__cpp_if_consteval)
	#define GAL_PROMETHEUS_IF_CONSTANT_EVALUATED if consteval
	#define GAL_PROMETHEUS_IF_NOT_CONSTANT_EVALUATED if not consteval
#else
#define GAL_PROMETHEUS_IF_CONSTANT_EVALUATED if (std::is_constant_evaluated())
#define GAL_PROMETHEUS_IF_NOT_CONSTANT_EVALUATED if (not std::is_constant_evaluated())
#endif

// fixme
#if __has_cpp_attribute(__cpp_lib_is_implicit_lifetime)
	#define GAL_PROMETHEUS_IS_IMPLICIT_LIFETIME_V(type) std::is_implicit_lifetime_v<type>
#else
#define GAL_PROMETHEUS_IS_IMPLICIT_LIFETIME_V(type) std::is_standard_layout_v<type> and std::is_trivial_v<type>
#endif

#if __has_cpp_attribute(__cpp_lib_start_lifetime_as)
	#define GAL_PROMETHEUS_START_LIFETIME_AS(type, ptr) std::start_lifetime_as<type>(ptr)
	#define GAL_PROMETHEUS_START_LIFETIME_AS_ARRAY(type, ptr) std::start_lifetime_as_array<type>(ptr)
#else
#define GAL_PROMETHEUS_START_LIFETIME_AS(type, ptr) \
				[]<typename GAL_PROMETHEUS_START_LIFETIME_AS_POINTER_TYPE>(GAL_PROMETHEUS_START_LIFETIME_AS_POINTER_TYPE gal_prometheus_start_lifetime_as_p)                                                                           \
					requires(std::is_trivially_copyable_v<type> && GAL_PROMETHEUS_IS_IMPLICIT_LIFETIME_V(type)) \
				{                                                                                                                           \
					if constexpr (std::is_const_v<std::remove_pointer_t<GAL_PROMETHEUS_START_LIFETIME_AS_POINTER_TYPE>>) \
					{ \
						return reinterpret_cast<std::add_pointer_t<std::add_const_t<type>>>(gal_prometheus_start_lifetime_as_p); \
					} \
					else \
					{ \
						auto*		gal_prometheus_start_lifetime_as_dest	= gal_prometheus_start_lifetime_as_p;         \
						const auto* gal_prometheus_start_lifetime_as_source = gal_prometheus_start_lifetime_as_p;                                                                                                                                    \
						GAL_PROMETHEUS_DEBUG_NOT_NULL(gal_prometheus_start_lifetime_as_dest);                                                                                                                       \
						GAL_PROMETHEUS_DEBUG_NOT_NULL(gal_prometheus_start_lifetime_as_source);                                                                                                                     \
						auto* gal_prometheus_start_lifetime_as_moved = static_cast<std::add_pointer_t<type>>(std::memmove(gal_prometheus_start_lifetime_as_dest, gal_prometheus_start_lifetime_as_source, sizeof(type)));                                                         \
						GAL_PROMETHEUS_DEBUG_NOT_NULL(gal_prometheus_start_lifetime_as_moved); \
						return std::launder(gal_prometheus_start_lifetime_as_moved); \
					}                                                          \
				}(ptr)
#define GAL_PROMETHEUS_START_LIFETIME_AS_ARRAY(type, ptr, size)                                                                                                                           \
				[]<typename GAL_PROMETHEUS_START_LIFETIME_AS_POINTER_TYPE>(GAL_PROMETHEUS_START_LIFETIME_AS_POINTER_TYPE gal_prometheus_start_lifetime_as_p)                                                                                                                                                      \
					requires(std::is_trivially_copyable_v<type> && GAL_PROMETHEUS_IS_IMPLICIT_LIFETIME_V(type))                                                                                                                                                      \
				{                                                                                                                                                                                                                                          \
					if constexpr (std::is_const_v<std::remove_pointer_t<GAL_PROMETHEUS_START_LIFETIME_AS_POINTER_TYPE>>) \
					{ \
						return reinterpret_cast<std::add_pointer_t<std::add_const_t<type>>>(gal_prometheus_start_lifetime_as_p);                                                   \
					} \
					else \
					{ \
						auto*		gal_prometheus_start_lifetime_as_dest	= gal_prometheus_start_lifetime_as_p;                                                                                                                                   \
						const auto* gal_prometheus_start_lifetime_as_source = gal_prometheus_start_lifetime_as_p;                                                                                                                                   \
						GAL_PROMETHEUS_DEBUG_NOT_NULL(gal_prometheus_start_lifetime_as_dest);                                                                                                                                                       \
						GAL_PROMETHEUS_DEBUG_NOT_NULL(gal_prometheus_start_lifetime_as_source);                                                                                                                   \
						auto* gal_prometheus_start_lifetime_as_moved = static_cast<std::add_pointer_t<type>>(std::memmove(gal_prometheus_start_lifetime_as_dest, gal_prometheus_start_lifetime_as_source, sizeof(type) * size));                                                                  \
						GAL_PROMETHEUS_DEBUG_NOT_NULL(gal_prometheus_start_lifetime_as_moved); \
						return std::launder(gal_prometheus_start_lifetime_as_moved);                                                                     \
					}                                                                                                                                 \
				}(ptr)
#endif

#define GAL_PROMETHEUS_PRIVATE_STRING_CAT(lhs, rhs) lhs##rhs
#define GAL_PROMETHEUS_STRING_CAT(lhs, rhs) GAL_PROMETHEUS_PRIVATE_STRING_CAT(lhs, rhs)

#define GAL_PROMETHEUS_PRIVATE_ARGS_N_0(_0, ...) _0
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_1(_0, _1, ...) _1
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_2(_0, _1, _2, ...) _2
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_3(_0, _1, _2, _3, ...) _3
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_4(_0, _1, _2, _3, _4, ...) _4
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_5(_0, _1, _2, _3, _4, _5, ...) _5
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_6(_0, _1, _2, _3, _4, _5, _6, ...) _6
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_7(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_8(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_9(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, ...) _11
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, ...) _12
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, ...) _13
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, ...) _14
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ...) _15
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, ...) _16
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, ...) _17
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, ...) _18
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, ...) _19
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, ...) _20
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, ...) _21
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, ...) _22
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, ...) _23
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, ...) _24
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, ...) _25
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, ...) _26
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, ...) _27
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, ...) _28
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, ...) _29
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, ...) _30
#define GAL_PROMETHEUS_PRIVATE_ARGS_N_31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, ...) _31

#define GAL_PROMETHEUS_ARGS_N(n, ...) GAL_PROMETHEUS_STRING_CAT(GAL_PROMETHEUS_PRIVATE_ARGS_N_, n)(__VA_ARGS__)
#define GAL_PROMETHEUS_ARGS_LEN(...) GAL_PROMETHEUS_ARGS_N(31, __VA_ARGS__ __VA_OPT__(, ) 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define GAL_PROMETHEUS_PRIVATE_TO_STRING_1(_0) #_0
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_2(_0, _1) #_0 #_1
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_3(_0, _1, _2) #_0 #_1 #_2
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_4(_0, _1, _2, _3) #_0, #_1, #_2, #_3
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_5(_0, _1, _2, _3, _4) #_0 #_1 #_2 #_3 #_4
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_6(_0, _1, _2, _3, _4, _5) #_0 #_1 #_2 #_3 #_4 #_5
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_7(_0, _1, _2, _3, _4, _5, _6) #_0 #_1 #_2 #_3 #_4 #_5 #_6
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_8(_0, _1, _2, _3, _4, _5, _6, _7) #_0 #_1 #_2 #_3 #_4 #_5 #_6 #_7
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_9(_0, _1, _2, _3, _4, _5, _6, _7, _8) #_0 #_1 #_2 #_3 #_4 #_5 #_6 #_7 #_8
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26 #_27
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26 #_27 #_28
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26 #_27 #_28 #_29
#define GAL_PROMETHEUS_PRIVATE_TO_STRING_31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26 #_27 #_28 #_29 #_30

#define GAL_PROMETHEUS_TO_STRING(...) GAL_PROMETHEUS_STRING_CAT(GAL_PROMETHEUS_PRIVATE_TO_STRING_, GAL_PROMETHEUS_ARGS_LEN(__VA_ARGS__))(__VA_ARGS__)

// =========================
// MODULE: gal.prometheus.infrastructure:exception
// =========================

// fixme
#if defined(NDEBUG)
	#define GAL_PROMETHEUS_DEBUG 0
#else
#define GAL_PROMETHEUS_DEBUG 1
#endif

#define GAL_PROMETHEUS_DEBUG_CALL_DEBUGGER_OR_TERMINATE(message) ::gal::prometheus::infrastructure::try_debug_or_terminate("[" __FILE__ ":" GAL_PROMETHEUS_TO_STRING(__LINE__) "] -> " message)

#define GAL_PROMETHEUS_PRIVATE_DEBUG_DO_CHECK(debug_type, expression, ...) \
	do {                                                                                                                                                \
			GAL_PROMETHEUS_IF_NOT_CONSTANT_EVALUATED                                                                                                           \
			{                                                                                                                                               \
				if (not static_cast<bool>(expression))                                                                                                      \
				{                                                                                                                                           \
					GAL_PROMETHEUS_DEBUG_CALL_DEBUGGER_OR_TERMINATE("[" debug_type "]: \"" __VA_ARGS__ "\" --> {" GAL_PROMETHEUS_TO_STRING(expression) "}"); \
					GAL_PROMETHEUS_DEBUG_TRAP();                                                                                                            \
				}                                                                                                                                           \
			}                                                                                                                                               \
		} while (false)

#if GAL_PROMETHEUS_DEBUG
#define GAL_PROMETHEUS_DEBUG_ASSUME(expression, ...) GAL_PROMETHEUS_PRIVATE_DEBUG_DO_CHECK("ASSUME-CHECK", expression __VA_OPT__(, ) __VA_ARGS__)
#else
	#define GAL_PROMETHEUS_DEBUG_ASSUME(expression, ...) GAL_PROMETHEUS_ASSUME(expression)
#endif

#if GAL_PROMETHEUS_DEBUG
#define GAL_PROMETHEUS_DEBUG_NOT_NULL(pointer, ...) GAL_PROMETHEUS_PRIVATE_DEBUG_DO_CHECK("NOT-NULL-CHECK", pointer != nullptr __VA_OPT__(, ) __VA_ARGS__)
#else
	#define GAL_PROMETHEUS_DEBUG_NOT_NULL(pointer, ...) GAL_PROMETHEUS_ASSUME(pointer != nullptr)
#endif

#if GAL_PROMETHEUS_DEBUG
#define GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED(...) GAL_PROMETHEUS_PRIVATE_DEBUG_DO_CHECK("NOT-IMPLEMENTED", 0 __VA_OPT__(, )__VA_ARGS__);
#else
	#define GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED(...) GAL_PROMETHEUS_UNREACHABLE()
#endif

#if GAL_PROMETHEUS_DEBUG
#define GAL_PROMETHEUS_DEBUG_UNREACHABLE(...) \
		GAL_PROMETHEUS_PRIVATE_DEBUG_DO_CHECK("UNRECHABLE-CHECK", 0 __VA_OPT__(, ) __VA_ARGS__); \
		GAL_PROMETHEUS_UNREACHABLE()
#else
	#define GAL_PROMETHEUS_DEBUG_UNREACHABLE(...) GAL_PROMETHEUS_UNREACHABLE()
#endif

// =========================
// MODULE: gal.prometheus.infrastructure:string
// =========================

#define GAL_PROMETHEUS_PRIVATE_DO_GENERATE_STRING_CHAR_ARRAY(string_type, string, string_length, begin_index) \
	decltype([]<std::size_t... Index>(std::index_sequence<Index...>) constexpr noexcept                       \
			 { return ::gal::prometheus::infrastructure::string_type<                                                       \
					   [](std::size_t index) constexpr noexcept                                               \
					   {                                                                                      \
						   return (string)[(begin_index) + index];                                            \
					   }(Index)...>{}; }(std::make_index_sequence<string_length>{}))

#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(string_type, string) GAL_PROMETHEUS_PRIVATE_DO_GENERATE_STRING_CHAR_ARRAY(string_type, string, sizeof(string) / sizeof((string)[0]), 0)
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_BILATERAL_ARRAY(string_type, inner_string_type, left_string, right_string) ::gal::prometheus::infrastructure::string_type<GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, left_string), GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, right_string)>
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_SYMMETRY_ARRAY(string_type, inner_string_type, string) ::gal::prometheus::infrastructure::string_type<GAL_PROMETHEUS_PRIVATE_DO_GENERATE_STRING_CHAR_ARRAY(inner_string_type, string, sizeof(string) / sizeof((string)[0]) / 2, 0), GAL_PROMETHEUS_PRIVATE_DO_GENERATE_STRING_CHAR_ARRAY(inner_string_type, string, sizeof(string) / sizeof((string)[0]) / 2, sizeof(string) / sizeof((string)[0]) / 2)>

#define GAL_PROMETHEUS_STRING_CHAR_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(char_array, string)
#define GAL_PROMETHEUS_STRING_WCHAR_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(wchar_array, string)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U8CHAR_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(u8char_array, string)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U16CHAR_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(u16char_array, string)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U32CHAR_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(u32char_array, string)

#define GAL_PROMETHEUS_STRING_CHAR_BILATERAL_ARRAY(left_string, right_string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_BILATERAL_ARRAY(char_bilateral_array, char_array, left_string, right_string)
#define GAL_PROMETHEUS_STRING_WCHAR_BILATERAL_ARRAY(left_string, right_string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_BILATERAL_ARRAY(wchar_bilateral_array, wchar_array, left_string, right_string)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U8CHAR_BILATERAL_ARRAY(left_string, right_string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_BILATERAL_ARRAY(u8char_bilateral_array, u8char_array, left_string, right_string)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U16CHAR_BILATERAL_ARRAY(left_string, right_string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_BILATERAL_ARRAY(u16char_bilateral_array, u16char_array, left_string, right_string)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U32CHAR_BILATERAL_ARRAY(left_string, right_string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_BILATERAL_ARRAY(u32char_bilateral_array, u32char_array, left_string, right_string)

#define GAL_PROMETHEUS_STRING_CHAR_SYMMETRY_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_SYMMETRY_ARRAY(char_bilateral_array, char_array, string)
#define GAL_PROMETHEUS_STRING_WCHAR_SYMMETRY_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_SYMMETRY_ARRAY(wchar_bilateral_array, wchar_array, string)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U8CHAR_SYMMETRY_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_SYMMETRY_ARRAY(u8char_bilateral_array, u8char_array, string)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U16CHAR_SYMMETRY_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_SYMMETRY_ARRAY(u16char_bilateral_array, u16char_array, string)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U32CHAR_SYMMETRY_ARRAY(string) GAL_PROMETHEUS_PRIVATE_STRING_CHAR_SYMMETRY_ARRAY(u32char_bilateral_array, u32char_array, string)

#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_1(string_type, inner_string_type, string1) \
	::gal::prometheus::infrastructure::string_type< \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string1) \
	>
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_2(string_type, inner_string_type, string1, string2) \
	::gal::prometheus::infrastructure::string_type< \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string1), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string2) \
	>
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_3(string_type, inner_string_type, string1, string2, string3) \
	::gal::prometheus::infrastructure::string_type< \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string1), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string2), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string3) \
	>
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_4(string_type, inner_string_type, string1, string2, string3, string4) \
	::gal::prometheus::infrastructure::string_type< \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string1), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string2), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string3), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string4) \
	>
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_5(string_type, inner_string_type, string1, string2, string3, string4, string5) \
	::gal::prometheus::infrastructure::string_type< \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string1), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string2), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string3), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string4), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string5) \
	>
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_6(string_type, inner_string_type, string1, string2, string3, string4, string5, string6) \
	::gal::prometheus::infrastructure::string_type< \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string1), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string2), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string3), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string4), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string5), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string6) \
	>
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_7(string_type, inner_string_type, string1, string2, string3, string4, string5, string6, string7) \
	::gal::prometheus::infrastructure::string_type< \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string1), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string2), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string3), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string4), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string5), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string6), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string7) \
	>
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_8(string_type, inner_string_type, string1, string2, string3, string4, string5, string6, string7, string8) \
	::gal::prometheus::infrastructure::string_type< \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string1), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string2), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string3), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string4), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string5), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string6), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string7), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string8) \
	>
#define GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_9(string_type, inner_string_type, string1, string2, string3, string4, string5, string6, string7, string8, string9) \
	::gal::prometheus::infrastructure::string_type< \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string1), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string2), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string3), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string4), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string5), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string6), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string7), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string8), \
		GAL_PROMETHEUS_PRIVATE_STRING_CHAR_ARRAY(inner_string_type, string9) \
	>

#define GAL_PROMETHEUS_STRING_CHAR_MULTIPLE_ARRAY(...) GAL_PROMETHEUS_STRING_CAT(GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_, GAL_PROMETHEUS_ARGS_LEN(__VA_ARGS__))(char_multiple_array, char_array, __VA_ARGS__)
#define GAL_PROMETHEUS_STRING_WCHAR_MULTIPLE_ARRAY(...) GAL_PROMETHEUS_STRING_CAT(GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_, GAL_PROMETHEUS_ARGS_LEN(__VA_ARGS__))(wchar_multiple_array, wchar_array, __VA_ARGS__)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U8CHAR_MULTIPLE_ARRAY(...) GAL_PROMETHEUS_STRING_CAT(GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_, GAL_PROMETHEUS_ARGS_LEN(__VA_ARGS__))(u8char_multiple_array, u8char_array, __VA_ARGS__)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U16CHAR_MULTIPLE_ARRAY(...) GAL_PROMETHEUS_STRING_CAT(GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_, GAL_PROMETHEUS_ARGS_LEN(__VA_ARGS__))(u16char_multiple_array, u16char_array, __VA_ARGS__)
// ReSharper disable once CppInconsistentNaming
#define GAL_PROMETHEUS_STRING_U32CHAR_MULTIPLE_ARRAY(...) GAL_PROMETHEUS_STRING_CAT(GAL_PROMETHEUS_PRIVATE_STRING_CHAR_MULTIPLE_ARRAY_, GAL_PROMETHEUS_ARGS_LEN(__VA_ARGS__))(u32char_multiple_array, u32char_array, __VA_ARGS__)

#define GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW(error_type, expression, message, ...) \
	do \
	{ \
		if (not static_cast<bool>(expression)) \
		{ \
			if constexpr (__VA_OPT__(not) false) { throw error_type{std::format(message __VA_OPT__(, ) __VA_ARGS__)}; } \
			else { throw error_type{message}; } \
		} \
	} while (false)

#define GAL_PROMETHEUS_RUNTIME_THROW(error_type, message, ...) \
	do                                                                                                                   \
	{                                                                                                                    \
		if constexpr (__VA_OPT__(not) false) { throw error_type{std::format(message __VA_OPT__(, ) __VA_ARGS__)}; } \
		else { throw error_type{message}; }                                                                          \
	} while (false)

#define GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW_STRING_PARSE_ERROR(expression, message, ...) GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW(::gal::prometheus::infrastructure::StringParseError, expression, message __VA_OPT__(, ) __VA_ARGS__)
#define GAL_PROMETHEUS_RUNTIME_THROW_STRING_PARSE_ERROR(message, ...) GAL_PROMETHEUS_RUNTIME_THROW(::gal::prometheus::infrastructure::StringParseError, message __VA_OPT__(, ) __VA_ARGS__)
