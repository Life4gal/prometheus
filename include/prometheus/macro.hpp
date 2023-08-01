#pragma once

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#define GAL_PROMETHEUS_UNREACHABLE() __assume(0)
#define GAL_PROMETHEUS_DEBUG_TRAP() __debugbreak()
#define GAL_PROMETHEUS_IMPORTED_SYMBOL __declspec(dllimport)
#define GAL_PROMETHEUS_EXPORTED_SYMBOL __declspec(dllexport)
#define GAL_PROMETHEUS_LOCAL_SYMBOL

#define GAL_PROMETHEUS_DISABLE_WARNING_PUSH __pragma(warning(push))
#define GAL_PROMETHEUS_DISABLE_WARNING_POP __pragma(warning(pop))
#define GAL_PROMETHEUS_DISABLE_WARNING(warningNumber) __pragma(warning(disable \
																: warningNumber))
#elif defined(GAL_PROMETHEUS_PLATFORM_LINUX)
	#define GAL_PROMETHEUS_UNREACHABLE() __builtin_unreachable()
	#define GAL_PROMETHEUS_DEBUG_TRAP() __builtin_trap()
	#define GAL_PROMETHEUS_IMPORTED_SYMBOL __attribute__((visibility("default")))
	#define GAL_PROMETHEUS_EXPORTED_SYMBOL __attribute__((visibility("default")))
	#define GAL_PROMETHEUS_LOCAL_SYMBOL __attribute__((visibility("hidden")))

	#define GAL_PROMETHEUS_DISABLE_WARNING_PUSH _Pragma("GCC diagnostic push")
	#define GAL_PROMETHEUS_DISABLE_WARNING_POP _Pragma("GCC diagnostic pop")

	#define GAL_PROMETHEUS_PRIVATE_DO_PRAGMA(X) _Pragma(#X)
	#define GAL_PROMETHEUS_DISABLE_WARNING(warningName) GAL_PROMETHEUS_PRIVATE_DO_PRAGMA(GCC diagnostic ignored #warningName)
#elif defined(GAL_PROMETHEUS_PLATFORM_MACOS)
	// todo: check it!
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

#define GAL_PROMETHEUS_STATIC_UNREACHABLE(...) \
[]<bool AlwaysFalse>() { static_assert(AlwaysFalse, "[UNREACHABLE BRANCH]: \"" __VA_ARGS__ "\""); }(); \
GAL_PROMETHEUS_UNREACHABLE()

#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_STRING_CAT(lhs, rhs) lhs##rhs
#define GAL_PROMETHEUS_STRING_CAT(lhs, rhs) GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_STRING_CAT(lhs, rhs)

#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_0(_0, ...) _0
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_1(_0, _1, ...) _1
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_2(_0, _1, _2, ...) _2
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_3(_0, _1, _2, _3, ...) _3
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_4(_0, _1, _2, _3, _4, ...) _4
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_5(_0, _1, _2, _3, _4, _5, ...) _5
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_6(_0, _1, _2, _3, _4, _5, _6, ...) _6
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_7(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_8(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_9(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, ...) _11
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, ...) _12
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, ...) _13
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, ...) _14
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ...) _15
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, ...) _16
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, ...) _17
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, ...) _18
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, ...) _19
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, ...) _20
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, ...) _21
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, ...) _22
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, ...) _23
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, ...) _24
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, ...) _25
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, ...) _26
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, ...) _27
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, ...) _28
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, ...) _29
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, ...) _30
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, ...) _31

#define GAL_PROMETHEUS_ARGS_N(n, ...) GAL_PROMETHEUS_STRING_CAT(GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_ARGS_N_, n)(__VA_ARGS__)
#define GAL_PROMETHEUS_ARGS_LEN(...) GAL_PROMETHEUS_ARGS_N(31, __VA_ARGS__ __VA_OPT__(, ) 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_1(_0) #_0
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_2(_0, _1) #_0 #_1
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_3(_0, _1, _2) #_0 #_1 #_2
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_4(_0, _1, _2, _3) #_0, #_1, #_2, #_3
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_5(_0, _1, _2, _3, _4) #_0 #_1 #_2 #_3 #_4
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_6(_0, _1, _2, _3, _4, _5) #_0 #_1 #_2 #_3 #_4 #_5
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_7(_0, _1, _2, _3, _4, _5, _6) #_0 #_1 #_2 #_3 #_4 #_5 #_6
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_8(_0, _1, _2, _3, _4, _5, _6, _7) #_0 #_1 #_2 #_3 #_4 #_5 #_6 #_7
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_9(_0, _1, _2, _3, _4, _5, _6, _7, _8) #_0 #_1 #_2 #_3 #_4 #_5 #_6 #_7 #_8
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26 #_27
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26 #_27 #_28
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26 #_27 #_28 #_29
#define GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30) #_0 #_1 #_2 #_3 #_4 #_5 #_6, #_7 #_8 #_9 #_10 #_11 #_12 #_13 #_14 #_15 #_16 #_17 #_18 #_19 #_20 #_21 #_22 #_23 #_24 #_25 #_26 #_27 #_28 #_29 #_30

#define GAL_PROMETHEUS_TO_STRING(...) GAL_PROMETHEUS_STRING_CAT(GAL_PROMETHEUS_PRIVATE_DO_NOT_USE_TO_STRING_, GAL_PROMETHEUS_ARGS_LEN(__VA_ARGS__))(__VA_ARGS__)
