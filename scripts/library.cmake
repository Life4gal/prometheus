# ===================================================================================================
# LIBRARY & LIBRARY ALIAS

add_library(${PROJECT_NAME} STATIC)
add_library(gal::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# ===================================================================================================
# COMPILE FLAGS

if (${PROJECT_NAME_PREFIX}COMPILER_MSVC)
	set(${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/D_CRT_SECURE_NO_WARNINGS")
	if (${PROJECT_NAME_PREFIX}PEDANTIC)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/W4")
	else ()
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/W3")
	endif (${PROJECT_NAME_PREFIX}PEDANTIC)
	if (${PROJECT_NAME_PREFIX}WERROR)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/WX")
	endif ()
elseif (${PROJECT_NAME_PREFIX}COMPILER_CLANG_CL)
	set(${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/D_CRT_SECURE_NO_WARNINGS")
	if (${PROJECT_NAME_PREFIX}PEDANTIC)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/W4")
	else ()
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/W3")
	endif (${PROJECT_NAME_PREFIX}PEDANTIC)
	if (${PROJECT_NAME_PREFIX}WERROR)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "/WX")
	endif ()

	if (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
		# chars/icelake_xxx
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-march=native")
	endif (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
elseif (${PROJECT_NAME_PREFIX}COMPILER_CLANG)
	set(${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wall")
	if (${PROJECT_NAME_PREFIX}PEDANTIC)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wextra" "-Wpedantic")
	endif (${PROJECT_NAME_PREFIX}PEDANTIC)
	if (${PROJECT_NAME_PREFIX}WERROR)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Werror")
	endif ()

	if (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
		# chars/icelake_xxx
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-march=native")
	endif (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
elseif (${PROJECT_NAME_PREFIX}COMPILER_GNU)
	set(${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wall")
	if (${PROJECT_NAME_PREFIX}PEDANTIC)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wextra" "-Wpedantic")
	endif (${PROJECT_NAME_PREFIX}PEDANTIC)
	if (${PROJECT_NAME_PREFIX}WERROR)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Werror")
	endif ()

	if (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
		# chars/icelake_xxx
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-march=native")
	endif (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
elseif (${PROJECT_NAME_PREFIX}COMPILER_CLANG_APPLE)
	set(${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wall")
	if (${PROJECT_NAME_PREFIX}PEDANTIC)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Wextra" "-Wpedantic")
	endif (${PROJECT_NAME_PREFIX}PEDANTIC)
	if (${PROJECT_NAME_PREFIX}WERROR)
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-Werror")
	endif ()

	if (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
		# chars/icelake_xxx
		list(APPEND ${PROJECT_NAME_PREFIX}COMPILE_FLAGS "-march=native")
	endif (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
endif (${PROJECT_NAME_PREFIX}COMPILER_MSVC)

target_compile_options(
		${PROJECT_NAME}
		PUBLIC

		${${PROJECT_NAME_PREFIX}COMPILE_FLAGS}
		# __VA_OPT__
		$<$<CXX_COMPILER_ID:MSVC>:-Zc:preprocessor>
)

# ===================================================================================================
# COMPILE DEFINITIONS & FEATURES

target_compile_definitions(
		${PROJECT_NAME}
		PUBLIC

		${PROJECT_NAME_PREFIX}MAJOR_VERSION=${${PROJECT_NAME_PREFIX}MAJOR_VERSION}
		${PROJECT_NAME_PREFIX}MINOR_VERSION=${${PROJECT_NAME_PREFIX}MINOR_VERSION}
		${PROJECT_NAME_PREFIX}PATCH_VERSION=${${PROJECT_NAME_PREFIX}PATCH_VERSION}
		${PROJECT_NAME_PREFIX}VERSION="${${PROJECT_NAME_PREFIX}VERSION}"

		${PROJECT_NAME_PREFIX}VERSION_NAMESPACE_NAME=${${PROJECT_NAME_PREFIX}VERSION_NAMESPACE_NAME}

		${PROJECT_NAME_PREFIX}COMPILER_NAME="${${PROJECT_NAME_PREFIX}COMPILER_NAME}"
		${PROJECT_NAME_PREFIX}COMPILER_VERSION="${${PROJECT_NAME_PREFIX}COMPILER_VERSION}"
		${PROJECT_NAME_PREFIX}COMPILER_FULL_NAME="${${PROJECT_NAME_PREFIX}COMPILER_FULL_NAME}"

		${${PROJECT_NAME_PREFIX}PLATFORM_NAME}

		${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED=${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED}

		# msvc
		$<$<CXX_COMPILER_ID:MSVC>:${PROJECT_NAME_PREFIX}COMPILER_MSVC>
		# g++
		$<$<CXX_COMPILER_ID:GNU>:${PROJECT_NAME_PREFIX}COMPILER_GNU>
		# clang-cl
		$<$<AND:$<CXX_COMPILER_ID:Clang>,$<STREQUAL:"${CMAKE_CXX_SIMULATE_ID}","MSVC">>:${PROJECT_NAME_PREFIX}COMPILER_CLANG_CL>
		# clang
		$<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<STREQUAL:"${CMAKE_CXX_SIMULATE_ID}","MSVC">>>:${PROJECT_NAME_PREFIX}COMPILER_CLANG>
		# apple clang
		$<$<CXX_COMPILER_ID:AppleClang>:${PROJECT_NAME_PREFIX}COMPILER_APPLE_CLANG>
)

target_compile_features(
		${PROJECT_NAME}
		PRIVATE
		cxx_std_23
)

set(${PROJECT_NAME_PREFIX}DEBUG_POSTFIX d CACHE STRING "Debug library postfix.")
set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${${PROJECT_NAME_PREFIX}VERSION}
		SOVERSION ${${PROJECT_NAME_PREFIX}MAJOR_VERSION}
		PUBLIC_HEADER "${${PROJECT_NAME_PREFIX}HEADER}"
		DEBUG_POSTFIX "${${PROJECT_NAME_PREFIX}DEBUG_POSTFIX}"
)

# ===================================================================================================
# LIBRARY SOURCE

set(
		${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PUBLIC

		# =========================
		# META
		# =========================

		${PROJECT_SOURCE_DIR}/src/meta/name.hpp
		${PROJECT_SOURCE_DIR}/src/meta/string.hpp
		${PROJECT_SOURCE_DIR}/src/meta/enumeration.hpp
		${PROJECT_SOURCE_DIR}/src/meta/member.hpp
		${PROJECT_SOURCE_DIR}/src/meta/to_string.hpp

		${PROJECT_SOURCE_DIR}/src/meta/meta.hpp

		# =========================
		# PLATFORM
		# =========================

		${PROJECT_SOURCE_DIR}/src/platform/exception.hpp
		${PROJECT_SOURCE_DIR}/src/platform/os.hpp
		${PROJECT_SOURCE_DIR}/src/platform/cpu.hpp
		${PROJECT_SOURCE_DIR}/src/platform/environment.hpp

		${PROJECT_SOURCE_DIR}/src/platform/platform.hpp

		# =========================
		# FUNCTIONAL
		# =========================

		${PROJECT_SOURCE_DIR}/src/functional/type_list.hpp
		${PROJECT_SOURCE_DIR}/src/functional/value_list.hpp
		${PROJECT_SOURCE_DIR}/src/functional/functor.hpp
		${PROJECT_SOURCE_DIR}/src/functional/aligned_union.hpp
		${PROJECT_SOURCE_DIR}/src/functional/function_ref.hpp

		${PROJECT_SOURCE_DIR}/src/functional/functional.hpp

		# =========================
		# MATH
		# =========================

		${PROJECT_SOURCE_DIR}/src/math/cmath.hpp

		${PROJECT_SOURCE_DIR}/src/math/math.hpp
)

set(
		${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PRIVATE

		# =========================
		# PLATFORM
		# =========================

		${PROJECT_SOURCE_DIR}/src/platform/exception.cpp
		${PROJECT_SOURCE_DIR}/src/platform/os.cpp
		${PROJECT_SOURCE_DIR}/src/platform/cpu.cpp
		${PROJECT_SOURCE_DIR}/src/platform/environment.cpp
)

if (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
	set(
			${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PUBLIC_CHARS_ICELAKE

			${PROJECT_SOURCE_DIR}/src/chars/icelake.ixx
			${PROJECT_SOURCE_DIR}/src/chars/icelake_ascii.ixx
			${PROJECT_SOURCE_DIR}/src/chars/icelake_utf8.ixx
			${PROJECT_SOURCE_DIR}/src/chars/icelake_utf16.ixx
			${PROJECT_SOURCE_DIR}/src/chars/icelake_utf32.ixx
	)
	set(${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PUBLIC ${${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PUBLIC} ${${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PUBLIC_CHARS_ICELAKE})
endif (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})

set_source_files_properties(
		${${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PUBLIC}
		${${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PRIVATE}
		PROPERTIES
		LANGUAGE CXX
)

target_sources(
		${PROJECT_NAME}
		PUBLIC
		FILE_SET public_header_files
		TYPE HEADERS
		BASE_DIRS "${PROJECT_SOURCE_DIR}"
		FILES

		${${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PUBLIC}
)

target_sources(
		${PROJECT_NAME}
		PRIVATE
		${${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PRIVATE}
)

target_sources(
		${PROJECT_NAME}
		PUBLIC
		FILE_SET macro_header_files
		TYPE HEADERS
		BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
		FILES

		${PROJECT_SOURCE_DIR}/src/prometheus/macro.hpp
)

# ===================================================================================================
# EXTERNAL LIBRARY

# STB
include(${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/stb/stb.cmake)
cmake_language(
		CALL
		${PROJECT_NAME_PREFIX}ATTACH_EXTERNAL_STB
)

# FREETYPE
include(${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/freetype/freetype.cmake)
cmake_language(
		CALL
		${PROJECT_NAME_PREFIX}ATTACH_EXTERNAL_FREETYPE
)

if (${PROJECT_NAME_PREFIX}COMPILER_CLANG OR ${PROJECT_NAME_PREFIX}COMPILER_GNU)
	target_link_libraries(
			${PROJECT_NAME}
			PRIVATE

			# <stacktrace>
			#stdc++_libbacktrace
			# <print>
			stdc++exp
	)
endif (${PROJECT_NAME_PREFIX}COMPILER_CLANG OR ${PROJECT_NAME_PREFIX}COMPILER_GNU)

# ===================================================================================================
# LIBRARY BINARY

# lib${PROJECT_NAME} -> Release
# lib${PROJECT}d -> Debug
set(${PROJECT_NAME_PREFIX}DEBUG_POSTFIX d CACHE STRING "Debug library postfix.")
set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${${PROJECT_NAME_PREFIX}VERSION}
		SOVERSION ${${PROJECT_NAME_PREFIX}MAJOR_VERSION}
		PUBLIC_HEADER "${${PROJECT_NAME_PREFIX}LIBRARY_SOURCE_PUBLIC}"
		DEBUG_POSTFIX "${${PROJECT_NAME_PREFIX}DEBUG_POSTFIX}"
)

# ===================================================================================================
# LIBRARY INSTALL

#if (${PROJECT_NAME_PREFIX}INSTALL)
#	# PackageProject.cmake will be used to make our target installable
#	CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.12.0")
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
#			# (optional) option to install only header files with matching pattern
#			INCLUDE_HEADER_PATTERN "*.hpp"
#			# (optional) create a header containing the version info
#			# Note: that the path to headers should be lowercase
#			VERSION_HEADER "${VERSION_HEADER_LOCATION}"
#			# (optional) define the project's version compatibility, defaults to `AnyNewerVersion`
#			# supported values: `AnyNewerVersion|SameMajorVersion|SameMinorVersion|ExactVersion`
#			COMPATIBILITY AnyNewerVersion
#			# semicolon separated list of the project's dependencies
#			DEPENDENCIES ${${PROJECT_NAME_PREFIX}EXTERNAL_DEPENDENCIES}
#			# (optional) option to disable the versioning of install destinations
#			DISABLE_VERSION_SUFFIX YES
#			# (optional) option to ignore target architecture for package resolution
#			# defaults to YES for header only (i.e. INTERFACE) libraries
#			ARCH_INDEPENDENT YES
#	)
#endif (${PROJECT_NAME_PREFIX}INSTALL)
