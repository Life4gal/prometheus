project(
		prometheus-unit-test
		LANGUAGES CXX
)

add_executable(
		${PROJECT_NAME}

		${PROJECT_SOURCE_DIR}/src/main.cpp

		# =================================
		# CHARS
		# =================================

		${PROJECT_SOURCE_DIR}/src/chars/ascii.cpp
		${PROJECT_SOURCE_DIR}/src/chars/utf8.cpp
		${PROJECT_SOURCE_DIR}/src/chars/utf16.cpp
		${PROJECT_SOURCE_DIR}/src/chars/utf32.cpp

		# =================================
		# META
		# =================================

		${PROJECT_SOURCE_DIR}/src/meta/enumeration.cpp

		# =================================
		# WILDCARD
		# =================================

		${PROJECT_SOURCE_DIR}/src/wildcard/wildcard.cpp
)

target_compile_features(
		${PROJECT_NAME}
		PUBLIC
		cxx_std_23
)

target_link_libraries(
		${PROJECT_NAME}
		PUBLIC
		prometheus
)

add_test(
		NAME ${PROJECT_NAME}
		COMMAND ${PROJECT_NAME}
)

# assets path
set(ASSETS_PATH_PIC ${CMAKE_SOURCE_DIR}/assets/pic.jpg)

#find_package(Vulkan)
#
#if (Vulkan_FOUND)
#	add_executable(
#			${PROJECT_NAME}-gui-vulkan
#
#			${PROJECT_SOURCE_DIR}/src/gui/main_vulkan.cpp
#	)
#
#	# glfw: clone from github
#	CPMAddPackage(
#			NAME glfw
#			GIT_TAG 3.4
#			GITHUB_REPOSITORY "glfw/glfw"
#			OPTIONS "BUILD_SHARED_LIBS OFF" "GLFW_BUILD_EXAMPLES OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_DOCS OFF" "GLFW_INSTALL OFF"
#	)
#	cmake_language(
#			CALL
#			${PROJECT_NAME_PREFIX}cpm_install
#			${PROJECT_NAME}-gui-vulkan
#			glfw 
#			PUBLIC 
#			glfw
#	)
#
#	target_link_libraries(${PROJECT_NAME}-gui-vulkan PRIVATE ${Vulkan_LIBRARY})
#	target_include_directories(${PROJECT_NAME}-gui-vulkan PRIVATE ${Vulkan_INCLUDE_DIRS})
#
#	target_compile_features(
#		${PROJECT_NAME}-gui-vulkan
#		PUBLIC
#		cxx_std_23
#	)
#
#	target_link_libraries(
#			${PROJECT_NAME}-gui-vulkan
#			PUBLIC
#			prometheus
#	)
#endif (Vulkan_FOUND)

add_executable(
		${PROJECT_NAME}-gui-dx12

		${PROJECT_SOURCE_DIR}/src/gui/glfw_callback_handler.cpp
		${PROJECT_SOURCE_DIR}/src/gui/backend_dx12.cpp
		${PROJECT_SOURCE_DIR}/src/gui/main_dx12.cpp
)

target_compile_features(
		${PROJECT_NAME}-gui-dx12
		PUBLIC
		cxx_std_23
)

target_link_libraries(
		${PROJECT_NAME}-gui-dx12
		PUBLIC
		prometheus
)
link_3rd_library_stb(${PROJECT_NAME}-gui-dx12)
link_3rd_library_glfw(${PROJECT_NAME}-gui-dx12)

target_compile_definitions(
		${PROJECT_NAME}-gui-dx12
		PUBLIC

		ASSETS_PATH_PIC="${ASSETS_PATH_PIC}"
)

add_executable(
		${PROJECT_NAME}-gui-dx11

		${PROJECT_SOURCE_DIR}/src/gui/glfw_callback_handler.cpp
		${PROJECT_SOURCE_DIR}/src/gui/backend_dx11.cpp
		${PROJECT_SOURCE_DIR}/src/gui/main_dx11.cpp
)

target_compile_features(
		${PROJECT_NAME}-gui-dx11
		PUBLIC
		cxx_std_23
)

target_link_libraries(
		${PROJECT_NAME}-gui-dx11
		PUBLIC
		prometheus
)
link_3rd_library_stb(${PROJECT_NAME}-gui-dx11)
link_3rd_library_glfw(${PROJECT_NAME}-gui-dx11)

target_compile_definitions(
		${PROJECT_NAME}-gui-dx11
		PUBLIC

		ASSETS_PATH_PIC="${ASSETS_PATH_PIC}"
)
