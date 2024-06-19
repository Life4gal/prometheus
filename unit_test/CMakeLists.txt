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

find_package(Vulkan REQUIRED)

if (Vulkan_FOUND)
	add_executable(
			${PROJECT_NAME}-gui-vulkan

			${PROJECT_SOURCE_DIR}/src/gui/main_vulkan.cpp
	)

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
			${PROJECT_NAME}-gui-vulkan
			glfw 
			PUBLIC 
			glfw
	)

	target_link_libraries(${PROJECT_NAME}-gui-vulkan PRIVATE ${Vulkan_LIBRARY})
	target_include_directories(${PROJECT_NAME}-gui-vulkan PRIVATE ${Vulkan_INCLUDE_DIRS})

	target_compile_features(
		${PROJECT_NAME}-gui-vulkan
		PUBLIC
		cxx_std_23
	)

	target_link_libraries(
			${PROJECT_NAME}-gui-vulkan
			PUBLIC
			prometheus
	)
endif (Vulkan_FOUND)

add_executable(
		${PROJECT_NAME}-gui-dx12

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

add_executable(
		${PROJECT_NAME}-gui-dx11

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
