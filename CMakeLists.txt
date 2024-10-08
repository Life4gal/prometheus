﻿cmake_minimum_required(VERSION 3.25)

if (CMAKE_VERSION VERSION_LESS "3.26")
	# 3.25
	set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "3c375311-a3c9-4396-a187-3227ef642046")
elseif (CMAKE_VERSION VERSION_LESS "3.27")
	# 3.26
	set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
elseif (CMAKE_VERSION VERSION_LESS "3.28")
	# 3.27
	set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "aa1f7df0-828a-4fcd-9afc-2dc80491aca7")
elseif (NOT CMAKE_VERSION VERSION_GREATER "3.28")
	message(FATAL_ERROR "See `https://github.com/Kitware/CMake/blob/v${CMAKE_VERSION}/Help/dev/experimental.rst`.")
endif (CMAKE_VERSION VERSION_LESS "3.26")
# turn on the dynamic depends for ninja
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP ON)

set(PROJECT_NAME_PREFIX "GAL_PROMETHEUS_")

set(
		${PROJECT_NAME_PREFIX}MAJOR_VERSION
		0
)

set(
		${PROJECT_NAME_PREFIX}MINOR_VERSION
		6
)

set(
		${PROJECT_NAME_PREFIX}PATCH_VERSION
		0
)

set(${PROJECT_NAME_PREFIX}VERSION ${${PROJECT_NAME_PREFIX}MAJOR_VERSION}.${${PROJECT_NAME_PREFIX}MINOR_VERSION}.${${PROJECT_NAME_PREFIX}PATCH_VERSION})

# Determine if our project is built as a subproject (using add_subdirectory)
# or if it is the master project.
set(${PROJECT_NAME_PREFIX}MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(${PROJECT_NAME_PREFIX}MASTER_PROJECT ON)
else ()
endif (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)

# fetch remote cmake utils
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/fetch_utils.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/doc_var.cmake)

# Set the default CMAKE_BUILD_TYPE to RelWithDebInfo.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself.
if (${PROJECT_NAME_PREFIX}MASTER_PROJECT AND NOT CMAKE_BUILD_TYPE)
	cmake_language(
			CALL
			${PROJECT_NAME_PREFIX}doc_var
			CMAKE_BUILD_TYPE
			RelWithDebInfo
			STRING
			"Choose the type of build, options are: None(CMAKE_CXX_FLAGS or "
			"CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
	)
endif (${PROJECT_NAME_PREFIX}MASTER_PROJECT AND NOT CMAKE_BUILD_TYPE)

project(
		prometheus
		VERSION ${${PROJECT_NAME_PREFIX}VERSION}
		DESCRIPTION "prometheus"
		HOMEPAGE_URL "https://github.com/Life4gal/prometheus"
		LANGUAGES CXX
)

message(STATUS "CMAKE VERSION: ${CMAKE_VERSION}. Compiler ID: ${CMAKE_CXX_COMPILER_ID}. Platform: ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.")
message(STATUS "${PROJECT_NAME} VERSION: ${${PROJECT_NAME_PREFIX}VERSION}")
message(STATUS "${PROJECT_NAME} BUILD TYPE: ${CMAKE_BUILD_TYPE}")

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

include(${PROJECT_SOURCE_DIR}/cmake_utils/CPM.cmake)
# Let CPMAddPackage first check if the required package exists locally.
# Compared to CPMFindPackage, CPMAddPackage will call cpm_export_variables after finding the local package.
set(CPM_USE_LOCAL_PACKAGES ON)
include(${PROJECT_SOURCE_DIR}/cmake_utils/cpm_install.cmake)
include(${PROJECT_SOURCE_DIR}/cmake_utils/nuget_install.cmake)
set(${PROJECT_NAME_PREFIX}3RD_PARTY_PATH ${PROJECT_SOURCE_DIR}/3rd-party)

include(${${PROJECT_NAME_PREFIX}3RD_PARTY_PATH}/freetype/freetype.cmake)
include(${${PROJECT_NAME_PREFIX}3RD_PARTY_PATH}/stb/stb.cmake)
include(${${PROJECT_NAME_PREFIX}3RD_PARTY_PATH}/glfw/glfw.cmake)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(${PROJECT_NAME_PREFIX}PLATFORM_WINDOWS ON)
	set(${PROJECT_NAME_PREFIX}PLATFORM ${PROJECT_NAME_PREFIX}PLATFORM_WINDOWS)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(${PROJECT_NAME_PREFIX}PLATFORM_LINUX ON)
	set(${PROJECT_NAME_PREFIX}PLATFORM ${PROJECT_NAME_PREFIX}PLATFORM_LINUX)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(${PROJECT_NAME_PREFIX}PLATFORM_MACOS ON)
	set(${PROJECT_NAME_PREFIX}PLATFORM ${PROJECT_NAME_PREFIX}PLATFORM_MACOS)
else ()
	message(FATAL_ERROR "Unsupported Platform: ${CMAKE_SYSTEM_NAME}")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# check cpu features
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/check_cpu_features.cmake)

cmake_language(
		CALL
		${PROJECT_NAME_PREFIX}doc_var
		${PROJECT_NAME_PREFIX}INSTALL_HEADERS
		${CMAKE_INSTALL_INCLUDEDIR}
		STRING
		"Installation directory for include files, a relative path that "
		"will be joined with ${CMAKE_INSTALL_PREFIX} or an absolute path."
)

option(${PROJECT_NAME_PREFIX}PEDANTIC "Enable extra warnings and expensive tests." ON)
option(${PROJECT_NAME_PREFIX}WERROR "Halt the compilation with an error on compiler warnings." OFF)
option(${PROJECT_NAME_PREFIX}MODULE "Enable module." OFF)
option(${PROJECT_NAME_PREFIX}DOC "Generate the doc target." ${${PROJECT_NAME_PREFIX}MASTER_PROJECT}) # Do we have the documentation? :)
option(${PROJECT_NAME_PREFIX}INSTALL "Generate the install target." ${${PROJECT_NAME_PREFIX}MASTER_PROJECT})
option(${PROJECT_NAME_PREFIX}TEST "Generate the test target." ${${PROJECT_NAME_PREFIX}MASTER_PROJECT})
option(${PROJECT_NAME_PREFIX}SYSTEM_HEADERS "Expose headers with marking them as system.(This allows other libraries that use this library to ignore the warnings generated by this library.)" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)

# Macros about the compiler.
set(${PROJECT_NAME_PREFIX}COMPILER_ID ${CMAKE_CXX_COMPILER_ID})
set(${PROJECT_NAME_PREFIX}COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
set(${PROJECT_NAME_PREFIX}COMPILER_NAME ${${PROJECT_NAME_PREFIX}COMPILER_ID}.${${PROJECT_NAME_PREFIX}COMPILER_VERSION})

if (${PROJECT_NAME_PREFIX}MASTER_PROJECT AND NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
	cmake_language(
			CALL
			${PROJECT_NAME_PREFIX}doc_var
			CMAKE_CXX_VISIBILITY_PRESET
			hidden
			STRING
			"Preset for the export of private symbols"
	)
	set_property(
			CACHE CMAKE_CXX_VISIBILITY_PRESET
			PROPERTY STRINGS
			hidden default
	)
endif (${PROJECT_NAME_PREFIX}MASTER_PROJECT AND NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)

if (${PROJECT_NAME_PREFIX}PLATFORM_WINDOWS)
	#########################
	# MSVC / CLANG-CL
	########################
	set(${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/D_CRT_SECURE_NO_WARNINGS")
	if (${PROJECT_NAME_PREFIX}PEDANTIC)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/W4")
	else ()
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/W3")
	endif (${PROJECT_NAME_PREFIX}PEDANTIC)
	if (${PROJECT_NAME_PREFIX}WERROR)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/WX")
	endif ()

	if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		if (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
			# chars/icelake_xxx
			list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512f")
			list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512bw")
			list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512vbmi")
			list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512vbmi2")
			list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512vpopcntdq")
			list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mbmi2")

			list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512vl")
			list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512dq")
		endif (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
	endif (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
elseif (${PROJECT_NAME_PREFIX}PLATFORM_LINUX)
	#########################
	# GCC / CLANG
	########################
	set(${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wall")
	if (${PROJECT_NAME_PREFIX}PEDANTIC)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wextra" "-Wpedantic")
	endif (${PROJECT_NAME_PREFIX}PEDANTIC)
	if (${PROJECT_NAME_PREFIX}WERROR)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Werror")
	endif ()

	# <stacktrace>
	#list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "--enable-libstdcxx-backtrace")
	list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-lstdc++_libbacktrace")

	if (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
		# chars/icelake_xxx
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512f")
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512bw")
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512vbmi")
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512vbmi2")
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mavx512vpopcntdq")
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-mbmi2")
	endif (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
elseif (${PROJECT_NAME_PREFIX}PLATFORM_MACOS)
	#########################
	# APPLE CLANG
	########################
	set(${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wall")
	if (${PROJECT_NAME_PREFIX}PEDANTIC)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wextra" "-Wpedantic")
	endif (${PROJECT_NAME_PREFIX}PEDANTIC)
	if (${PROJECT_NAME_PREFIX}WERROR)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Werror")
	endif ()
else ()
	message(FATAL_ERROR "Unsupported compilers: ${CMAKE_CXX_COMPILER}")
endif (${PROJECT_NAME_PREFIX}PLATFORM_WINDOWS)

if (${PROJECT_NAME_PREFIX}SYSTEM_HEADERS)
	set(${PROJECT_NAME_PREFIX}HEADER_ATTRIBUTES SYSTEM)
else ()
	set(${PROJECT_NAME_PREFIX}HEADER_ATTRIBUTES)
endif (${PROJECT_NAME_PREFIX}SYSTEM_HEADERS)

# LIBRARY
add_library(
		${PROJECT_NAME}
		${${PROJECT_NAME_PREFIX}MODULE_KEYWORD_INTERFACE}
)

# LIBRARY ALIAS
add_library(
		gal::${PROJECT_NAME}
		ALIAS
		${PROJECT_NAME}
)

if (${PROJECT_NAME_PREFIX}MODULE)
	set(${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH ${CMAKE_BINARY_DIR}/temp_module)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		if (NOT DEFINED ENV{VCToolsInstallDir})
			message(FATAL_ERROR "Unable to find path to VC Tools in environment variables!")
		endif (NOT DEFINED ENV{VCToolsInstallDir})

		if (NOT EXISTS $ENV{VCToolsInstallDir})
			message(FATAL_ERROR "Invalid VC Tools installation path!")
		else ()
			message(STATUS "Found VC Tools in: [$ENV{VCToolsInstallDir}]")
		endif (NOT EXISTS $ENV{VCToolsInstallDir})

		if (NOT EXISTS $ENV{VCToolsInstallDir}modules/std.ixx)
			message(FATAL_ERROR "Cannot find std.ixx! Please check MSVC version(${CMAKE_CXX_COMPILER_VERSION})!")
		else ()
			message(STATUS "Found module std.ixx in: [$ENV{VCToolsInstallDir}modules\\std.ixx]")
		endif (NOT EXISTS $ENV{VCToolsInstallDir}modules/std.ixx)

		configure_file(
				$ENV{VCToolsInstallDir}modules/std.ixx
				${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}/std.ixx
				COPYONLY
		)

		set_source_files_properties(
				${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}/std.ixx
				PROPERTIES
				COMPILE_OPTIONS "/Wv:18"
		)

		target_sources(
				${PROJECT_NAME}
				# fixme: Possible conflicts with other libraries?
				PUBLIC
				FILE_SET std_module
				TYPE CXX_MODULES
				BASE_DIRS "${PROJECT_SOURCE_DIR}"
				FILES

				$<$<CXX_COMPILER_ID:MSVC>:${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}/std.ixx>
		)
	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		# https://gitlab.kitware.com/cmake/cmake/-/blob/master/.gitlab/ci/cxx_modules_rules_clang.cmake
		# Default to C++ extensions being off. Clang's modules support have trouble
		# with extensions right now.
		set(CMAKE_CXX_EXTENSIONS OFF)

		# fixme: https://discourse.cmake.org/t/c-20-modules-update/7330/24
		# fixme:
		# PLEASE submit a bug report to https://github.com/llvm/llvm-project/issues/ and include the crash backtrace.
		# Stack dump:
		# 0.	Program arguments: "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Tools/Llvm/x64/bin/clang-scan-deps.exe" -format=p1689 -- C:\\PROGRA~1\\MIB055~1\\2022\\Preview\\VC\\Tools\\Llvm\\x64\\bin\\clang-cl.exe -DGAL_PROMETHEUS_COMPILER_CLANG_CL -DGAL_PROMETHEUS_COMPILER_ID=\"Clang\" -DGAL_PROMETHEUS_COMPILER_NAME=\"Clang.16.0.5\" -DGAL_PROMETHEUS_COMPILER_VERSION=\"16.0.5\" -DGAL_PROMETHEUS_MAJOR_VERSION=0 -DGAL_PROMETHEUS_MINOR_VERSION=0 -DGAL_PROMETHEUS_PATCH_VERSION=6 -DGAL_PROMETHEUS_PLATFORM_WINDOWS -DGAL_PROMETHEUS_VERSION=\"0.0.6\" -IC:\workspace\life4gal\prometheus\src  -x c++ C:\workspace\life4gal\prometheus\src\infrastructure\type_traits.ixx -c -o CMakeFiles\prometheus.dir\src\infrastructure\type_traits.ixx.obj -MT CMakeFiles\prometheus.dir\src\infrastructure\type_traits.ixx.obj.ddi -MD -MF CMakeFiles\prometheus.dir\src\infrastructure\type_traits.ixx.obj.ddi.d > CMakeFiles\prometheus.dir\src\infrastructure\type_traits.ixx.obj.ddi"
		# Exception Code: 0xC0000005
		# #0 0x00007ff7b4803c75 C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe 0x3c75 C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe 0x3efc9d0
		# #1 0x00007ff7b4803c75 (C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe+0x3c75)
		# #2 0x00007ff7b86fc9d0 (C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe+0x3efc9d0)
		# 0x00007FF7B4803C75, C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe(0x00007FF7B4800000) + 0x3C75 byte(s)
		# 0x00007FF7B86FC9D0, C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe(0x00007FF7B4800000) + 0x3EFC9D0 byte(s)
		# 0x00007FFA728526AD, C:\WINDOWS\System32\KERNEL32.DLL(0x00007FFA72840000) + 0x126AD byte(s), BaseThreadInitThunk() + 0x1D byte(s)
		# 0x00007FFA72C2AA68, C:\WINDOWS\SYSTEM32\ntdll.dll(0x00007FFA72BD0000) + 0x5AA68 byte(s), RtlUserThreadStart() + 0x28 byte(s)
		string(CONCAT CMAKE_EXPERIMENTAL_CXX_SCANDEP_SOURCE
				"\"${CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS}\""
				" -format=p1689"
				" --"
				" \"<CMAKE_CXX_COMPILER>\" <DEFINES> <INCLUDES> "#<FLAGS>"
				" -x c++ <SOURCE> -c -o <OBJECT>"
				" -MT <DYNDEP_FILE>"
				" -MD -MF <DEP_FILE>"
				" > <DYNDEP_FILE>")
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "clang")
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FLAG "@<MODULE_MAP_FILE>")
	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		# https://gitlab.kitware.com/cmake/cmake/-/blob/master/.gitlab/ci/cxx_modules_rules_gcc.cmake
		string(CONCAT CMAKE_EXPERIMENTAL_CXX_SCANDEP_SOURCE
				"<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -E -x c++ <SOURCE>"
				" -MT <DYNDEP_FILE> -MD -MF <DEP_FILE>"
				" -fmodules-ts "#-fdep-file=<DYNDEP_FILE> -fdep-output=<OBJECT> -fdep-format=trtbd"
				" -o <PREPROCESSED_SOURCE>")
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "gcc")
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FLAG "-fmodules-ts -fmodule-mapper=<MODULE_MAP_FILE> -fdep-format=trtbd -x c++")
	else ()
		message(FATAL_ERROR "Unsupported compilers: ${CMAKE_CXX_COMPILER}")
	endif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif (${PROJECT_NAME_PREFIX}MODULE)

# =======================
# MODULE SOURCE
# =======================

if (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
	set(
		${PROJECT_NAME_PREFIX}MODULE_SOURCE_CHARS_ICELAKE

		${PROJECT_SOURCE_DIR}/src/chars/icelake.ixx
		${PROJECT_SOURCE_DIR}/src/chars/icelake_ascii.ixx
		${PROJECT_SOURCE_DIR}/src/chars/icelake_utf8.ixx
		${PROJECT_SOURCE_DIR}/src/chars/icelake_utf16.ixx
		${PROJECT_SOURCE_DIR}/src/chars/icelake_utf32.ixx
	)
else()
	set(
		${PROJECT_NAME_PREFIX}MODULE_SOURCE_CHARS_ICELAKE
	)
endif (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})

set(
		${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC

		# =========================
		# CHARS
		# =========================
		
		${PROJECT_SOURCE_DIR}/src/chars/encoding.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar_ascii.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar_utf8.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar_utf16.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar_utf32.ixx
		${${PROJECT_NAME_PREFIX}MODULE_SOURCE_CHARS_ICELAKE}
		
		${PROJECT_SOURCE_DIR}/src/chars/chars.ixx

		# =========================
		# COMMAND_LINE_PARSER
		# =========================

		${PROJECT_SOURCE_DIR}/src/command_line_parser/command_line_parser.ixx

		# =========================
		# CONCURRENCY
		# =========================

		${PROJECT_SOURCE_DIR}/src/concurrency/thread.ixx
		${PROJECT_SOURCE_DIR}/src/concurrency/unfair_mutex.ixx

		${PROJECT_SOURCE_DIR}/src/concurrency/concurrency.ixx

		# =========================
		# COROUTINE
		# =========================

		${PROJECT_SOURCE_DIR}/src/coroutine/task.ixx
		${PROJECT_SOURCE_DIR}/src/coroutine/generator.ixx

		${PROJECT_SOURCE_DIR}/src/coroutine/coroutine.ixx

		# =========================
		# ERROR
		# =========================

		${PROJECT_SOURCE_DIR}/src/error/exception.ixx
		${PROJECT_SOURCE_DIR}/src/error/debug.ixx
		${PROJECT_SOURCE_DIR}/src/error/platform.ixx
		${PROJECT_SOURCE_DIR}/src/error/command_line.ixx
		${PROJECT_SOURCE_DIR}/src/error/instruction_set.ixx

		${PROJECT_SOURCE_DIR}/src/error/error.ixx

		# =========================
		# FUNCTIONAL
		# =========================

		${PROJECT_SOURCE_DIR}/src/functional/type_list.ixx
		${PROJECT_SOURCE_DIR}/src/functional/value_list.ixx
		${PROJECT_SOURCE_DIR}/src/functional/functor.ixx
		${PROJECT_SOURCE_DIR}/src/functional/aligned_union.ixx
		${PROJECT_SOURCE_DIR}/src/functional/function_ref.ixx
		${PROJECT_SOURCE_DIR}/src/functional/math.ixx
		${PROJECT_SOURCE_DIR}/src/functional/flag.ixx
		${PROJECT_SOURCE_DIR}/src/functional/function_signature.ixx

		${PROJECT_SOURCE_DIR}/src/functional/functional.ixx

		# =========================
		# GUI
		# =========================

		${PROJECT_SOURCE_DIR}/src/gui/draw_list.ixx
		${PROJECT_SOURCE_DIR}/src/gui/font.ixx

		${PROJECT_SOURCE_DIR}/src/gui/gui.ixx

		# =========================
		# MEMORY
		# =========================

		${PROJECT_SOURCE_DIR}/src/memory/read_write.ixx

		${PROJECT_SOURCE_DIR}/src/memory/memory.ixx

		# =========================
		# META
		# =========================

		${PROJECT_SOURCE_DIR}/src/meta/string.ixx
		${PROJECT_SOURCE_DIR}/src/meta/name.ixx
		${PROJECT_SOURCE_DIR}/src/meta/enumeration.ixx
		${PROJECT_SOURCE_DIR}/src/meta/member.ixx
		${PROJECT_SOURCE_DIR}/src/meta/to_string.ixx

		${PROJECT_SOURCE_DIR}/src/meta/meta.ixx

		# =========================
		# NUMERIC
		# =========================

		${PROJECT_SOURCE_DIR}/src/numeric/random_engine.ixx
		${PROJECT_SOURCE_DIR}/src/numeric/random.ixx

		${PROJECT_SOURCE_DIR}/src/numeric/numeric.ixx

		# =========================
		# PRIMITIVE
		# =========================

		${PROJECT_SOURCE_DIR}/src/primitive/multidimensional.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/point.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/extent.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/rect.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/circle.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/ellipse.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/color.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/vertex.ixx

		${PROJECT_SOURCE_DIR}/src/primitive/primitive.ixx

		# =========================
		# STATE_MACHINE
		# =========================

		${PROJECT_SOURCE_DIR}/src/state_machine/state_machine.ixx

		# =========================
		# STRING
		# =========================

		${PROJECT_SOURCE_DIR}/src/string/charconv.ixx
		${PROJECT_SOURCE_DIR}/src/string/string_pool.ixx

		${PROJECT_SOURCE_DIR}/src/string/string.ixx

		# =========================
		# UNIT_TEST
		# =========================

		${PROJECT_SOURCE_DIR}/src/unit_test/unit_test.ixx

		# =========================
		# WILDCARD
		# =========================

		${PROJECT_SOURCE_DIR}/src/wildcard/wildcard.ixx

		# =========================
		# ROOT
		# =========================

		${PROJECT_SOURCE_DIR}/src/prometheus.ixx
)

set(
		${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE

		# =========================
		# CONCURRENCY
		# =========================

		${PROJECT_SOURCE_DIR}/src/concurrency/thread.impl.ixx
		${PROJECT_SOURCE_DIR}/src/concurrency/unfair_mutex.impl.ixx

		# =========================
		# ERROR
		# =========================

		${PROJECT_SOURCE_DIR}/src/error/debug.impl.ixx
		${PROJECT_SOURCE_DIR}/src/error/platform.impl.ixx
		${PROJECT_SOURCE_DIR}/src/error/command_line.impl.ixx
		${PROJECT_SOURCE_DIR}/src/error/instruction_set.impl.ixx

		# =========================
		# GUI
		# =========================

		${PROJECT_SOURCE_DIR}/src/gui/font.impl.ixx
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set_source_files_properties(
			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE}
			PROPERTIES
			LANGUAGE CXX
	)
endif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

if (${PROJECT_NAME_PREFIX}MODULE)
	target_sources(
			${PROJECT_NAME}
			PUBLIC
			FILE_SET public_module_files
			TYPE CXX_MODULES
			BASE_DIRS "${PROJECT_SOURCE_DIR}"
			FILES

			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC}
			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE}
	)
else ()
	target_sources(
			${PROJECT_NAME}
			PUBLIC
			FILE_SET public_header_files
			TYPE HEADERS
			BASE_DIRS "${PROJECT_SOURCE_DIR}"
			FILES

			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC}
	)

	target_sources(
			${PROJECT_NAME}
			PRIVATE

			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE}
	)
endif (${PROJECT_NAME_PREFIX}MODULE)

target_sources(
		${PROJECT_NAME}
		PUBLIC
		FILE_SET macro_header_files
		TYPE HEADERS
		BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
		FILES

		${PROJECT_SOURCE_DIR}/src/prometheus/macro.hpp
)

# SET FLAGS
target_compile_options(
		${PROJECT_NAME}
		PUBLIC

		${${PROJECT_NAME_PREFIX}COMPILE_FLAGS}
		# __VA_OPT__
		$<$<CXX_COMPILER_ID:MSVC>:-Zc:preprocessor>
)

# SET DEFINITIONS
if (${PROJECT_NAME_PREFIX}MODULE)
	set(${PROJECT_NAME_PREFIX}MODULE_MACRO_VARIABLE_USE_MODULE 1)
else ()
	set(${PROJECT_NAME_PREFIX}MODULE_MACRO_VARIABLE_USE_MODULE 0)
endif (${PROJECT_NAME_PREFIX}MODULE)

target_compile_definitions(
		${PROJECT_NAME}
		PUBLIC

		${PROJECT_NAME_PREFIX}MAJOR_VERSION=${${PROJECT_NAME_PREFIX}MAJOR_VERSION}
		${PROJECT_NAME_PREFIX}MINOR_VERSION=${${PROJECT_NAME_PREFIX}MINOR_VERSION}
		${PROJECT_NAME_PREFIX}PATCH_VERSION=${${PROJECT_NAME_PREFIX}PATCH_VERSION}
		${PROJECT_NAME_PREFIX}VERSION="${${PROJECT_NAME_PREFIX}VERSION}"
		${PROJECT_NAME_PREFIX}COMPILER_ID="${${PROJECT_NAME_PREFIX}COMPILER_ID}"
		${PROJECT_NAME_PREFIX}COMPILER_VERSION="${${PROJECT_NAME_PREFIX}COMPILER_VERSION}"
		${PROJECT_NAME_PREFIX}COMPILER_NAME="${${PROJECT_NAME_PREFIX}COMPILER_NAME}"
		${${PROJECT_NAME_PREFIX}PLATFORM}

		${PROJECT_NAME_PREFIX}USE_MODULE=${${PROJECT_NAME_PREFIX}MODULE_MACRO_VARIABLE_USE_MODULE}

		${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED=${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED}

		# Tool macros for platform determination.
		$<$<CXX_COMPILER_ID:MSVC>:${PROJECT_NAME_PREFIX}COMPILER_MSVC>
		$<$<CXX_COMPILER_ID:GNU>:${PROJECT_NAME_PREFIX}COMPILER_GNU>
		# clang-cl
		$<$<AND:$<CXX_COMPILER_ID:Clang>,$<STREQUAL:"${CMAKE_CXX_SIMULATE_ID}","MSVC">>:${PROJECT_NAME_PREFIX}COMPILER_CLANG_CL>
		# clang
		$<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<STREQUAL:"${CMAKE_CXX_SIMULATE_ID}","MSVC">>>:${PROJECT_NAME_PREFIX}COMPILER_CLANG>
		# apple clang
		$<$<CXX_COMPILER_ID:AppleClang>:${PROJECT_NAME_PREFIX}COMPILER_APPLE_CLANG>
)

# SET FEATURES
target_compile_features(
		${PROJECT_NAME}
		PRIVATE
		cxx_std_23
)

link_3rd_library_freetype(${PROJECT_NAME})
link_3rd_library_stb(${PROJECT_NAME})

# SET DEBUG POSTFIX ==> lib${PROJECT_NAME} -> Release lib${PROJECT}d -> Debug
set(${PROJECT_NAME_PREFIX}DEBUG_POSTFIX d CACHE STRING "Debug library postfix.")
set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${${PROJECT_NAME_PREFIX}VERSION}
		SOVERSION ${${PROJECT_NAME_PREFIX}MAJOR_VERSION}
		PUBLIC_HEADER "${${PROJECT_NAME_PREFIX}HEADER}"
		DEBUG_POSTFIX "${${PROJECT_NAME_PREFIX}DEBUG_POSTFIX}")

# INSTALL TARGETS
#if (${PROJECT_NAME_PREFIX}INSTALL)
#	# PackageProject.cmake will be used to make our target installable
#	CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.10.0")
#
#	# the location where the project's version header will be placed should match the project's regular
#	# header paths
#	string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
#
#	packageProject(
#			# the name of the target to export
#			NAME ${PROJECT_NAME}
#			# the version of the target to export
#			VERSION ${PROJECT_VERSION}
#			# (optional) install your library with a namespace (Note: do NOT add extra '::')
#			NAMESPACE ${PROJECT_NAME}
#			# a temporary directory to create the config files
#			BINARY_DIR ${PROJECT_BINARY_DIR}
#			# location of the target's public headers
#			# see target_include_directories -> BUILD_INTERFACE
#			INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src
#			# should match the target's INSTALL_INTERFACE include directory
#			# see target_include_directories -> INSTALL_INTERFACE
#			INCLUDE_DESTINATION ${${PROJECT_NAME_PREFIX}INSTALL_HEADERS}/${PROJECT_NAME}-${PROJECT_VERSION}
#			# (optional) create a header containing the version info
#			# Note: that the path to headers should be lowercase
#			VERSION_HEADER "${VERSION_HEADER_LOCATION}"
#			# (optional) define the project's version compatibility, defaults to `AnyNewerVersion`
#			# supported values: `AnyNewerVersion|SameMajorVersion|SameMinorVersion|ExactVersion`
#			COMPATIBILITY AnyNewerVersion
#			# semicolon separated list of the project's dependencies
#			# see `LINK 3rd-PARTY LIBRARIES`
#			DEPENDENCIES ${${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES}
#			# (optional) option to disable the versioning of install destinations
#			DISABLE_VERSION_SUFFIX YES
#			# (optional) option to ignore target architecture for package resolution
#			# defaults to YES for header only (i.e. INTERFACE) libraries
#			ARCH_INDEPENDENT YES
#	)
#endif (${PROJECT_NAME_PREFIX}INSTALL)

# DOCUMENTS
if (${PROJECT_NAME_PREFIX}DOC)
	# add_subdirectory(doc)
endif (${PROJECT_NAME_PREFIX}DOC)

# TESTS
if (${PROJECT_NAME_PREFIX}TEST)
	enable_testing()
	add_subdirectory(unit_test)
endif (${PROJECT_NAME_PREFIX}TEST)
