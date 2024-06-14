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

#		${PROJECT_SOURCE_DIR}/src/chars/ascii.cpp
#		${PROJECT_SOURCE_DIR}/src/chars/cp_1252.cpp
#		${PROJECT_SOURCE_DIR}/src/chars/utf_8.cpp
#		${PROJECT_SOURCE_DIR}/src/chars/utf_16.cpp
#		${PROJECT_SOURCE_DIR}/src/chars/icelake_ascii.cpp
#		${PROJECT_SOURCE_DIR}/src/chars/icelake_utf16.cpp

		# =================================
		# STRING
		# =================================

		${PROJECT_SOURCE_DIR}/src/string/wildcard_match.cpp
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

# ===================================
option(${PROJECT_NAME_PREFIX}UNIT_TEST_GUI_BACKEND_VULKAN "ON => VULKAN / OFF => DX12" ON)

if (${PROJECT_NAME_PREFIX}UNIT_TEST_GUI_BACKEND_VULKAN)
	add_executable(
			${PROJECT_NAME}-gui

			${PROJECT_SOURCE_DIR}/src/gui/main_vulkan.cpp
	)

	# todo
	# glfw: clone from github
	CPMAddPackage(
			NAME glfw
			GIT_TAG 3.4
			GITHUB_REPOSITORY "glfw/glfw"
			OPTIONS "BUILD_SHARED_LIBS OFF" "GLFW_BUILD_EXAMPLES OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_DOCS OFF" "GLFW_INSTALL OFF"
	)
	cmake_language(
			CALL
			${PROJECT_NAME_PREFIX}cpm_install
			${PROJECT_NAME}-gui 
			glfw 
			PUBLIC 
			glfw
	)
	# vulkan: local
	find_package(Vulkan REQUIRED)
	target_link_libraries(${PROJECT_NAME}-gui PRIVATE ${Vulkan_LIBRARY})
	target_include_directories(${PROJECT_NAME}-gui PRIVATE ${Vulkan_INCLUDE_DIRS})
else()
	add_executable(
			${PROJECT_NAME}-gui

			${PROJECT_SOURCE_DIR}/src/gui/main_dx12.cpp
	)
endif (${PROJECT_NAME_PREFIX}UNIT_TEST_GUI_BACKEND_VULKAN)

target_compile_features(
		${PROJECT_NAME}-gui
		PUBLIC
		cxx_std_23
)

target_link_libraries(
		${PROJECT_NAME}-gui
		PUBLIC
		prometheus
)

add_test(
		NAME ${PROJECT_NAME}-gui
		COMMAND ${PROJECT_NAME}-gui
)
