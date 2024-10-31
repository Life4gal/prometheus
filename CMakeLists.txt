cmake_minimum_required(VERSION 3.25)

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
	message(FATAL_ERROR "[PROMETHEUS] See `https://github.com/Kitware/CMake/blob/v${CMAKE_VERSION}/Help/dev/experimental.rst`.")
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
		7
)

set(
		${PROJECT_NAME_PREFIX}PATCH_VERSION
		0
)

set(${PROJECT_NAME_PREFIX}VERSION ${${PROJECT_NAME_PREFIX}MAJOR_VERSION}.${${PROJECT_NAME_PREFIX}MINOR_VERSION}.${${PROJECT_NAME_PREFIX}PATCH_VERSION})
set(${PROJECT_NAME_PREFIX}VERSION_NAMESPACE_NAME v_${${PROJECT_NAME_PREFIX}MAJOR_VERSION}_${${PROJECT_NAME_PREFIX}MINOR_VERSION}_${${PROJECT_NAME_PREFIX}PATCH_VERSION})

set(${PROJECT_NAME_PREFIX}MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(${PROJECT_NAME_PREFIX}MASTER_PROJECT ON)
else ()
endif (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)

project(
		prometheus
		VERSION ${${PROJECT_NAME_PREFIX}VERSION}
		DESCRIPTION "prometheus"
		HOMEPAGE_URL "https://github.com/Life4gal/prometheus"
		LANGUAGES CXX
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/compiler_and_platform.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/utility.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/3rd_party_library.cmake)

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

cmake_language(
		CALL
		${PROJECT_NAME_PREFIX}doc_var
		${PROJECT_NAME_PREFIX}INSTALL_HEADERS
		${CMAKE_INSTALL_INCLUDEDIR}
		STRING
		"Installation directory for include files, a relative path that "
		"will be joined with ${CMAKE_INSTALL_PREFIX} or an absolute path."
)

if (${PROJECT_NAME_PREFIX}MASTER_PROJECT)
	if (NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
		cmake_language(
				CALL
				${PROJECT_NAME_PREFIX}doc_var
				CMAKE_CXX_VISIBILITY_PRESET
				hidden
				STRING
				"Preset for the export of private symbols"
		)
		set(CMAKE_CXX_VISIBILITY_PRESET hidden)
		set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
	endif (NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
endif (${PROJECT_NAME_PREFIX}MASTER_PROJECT)

option(${PROJECT_NAME_PREFIX}PEDANTIC "Enable extra warnings and expensive tests." ON)
option(${PROJECT_NAME_PREFIX}WERROR "Halt the compilation with an error on compiler warnings." OFF)
option(${PROJECT_NAME_PREFIX}MODULE "Enable module." OFF)
option(${PROJECT_NAME_PREFIX}DOC "Generate the doc target." ${${PROJECT_NAME_PREFIX}MASTER_PROJECT}) # Do we have the documentation? :)
option(${PROJECT_NAME_PREFIX}INSTALL "Generate the install target." ${${PROJECT_NAME_PREFIX}MASTER_PROJECT})
option(${PROJECT_NAME_PREFIX}TEST "Generate the test target." ${${PROJECT_NAME_PREFIX}MASTER_PROJECT})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)

# LIBRARY
add_library(
		${PROJECT_NAME}
		STATIC
)

# LIBRARY ALIAS
add_library(
		gal::${PROJECT_NAME}
		ALIAS
		${PROJECT_NAME}
)

# SOURCE
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/compile_flag_and_definition.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/module_source.cmake)

# 3RD PARTY
link_3rd_party_library_freetype(${PROJECT_NAME})
link_3rd_party_library_stb(${PROJECT_NAME})

# DOCUMENTS
if (${PROJECT_NAME_PREFIX}DOC)
	# add_subdirectory(doc)
endif (${PROJECT_NAME_PREFIX}DOC)

# TESTS
if (${PROJECT_NAME_PREFIX}TEST)
	enable_testing()
	add_subdirectory(unit_test)
endif (${PROJECT_NAME_PREFIX}TEST)
