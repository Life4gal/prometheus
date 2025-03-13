project(
		prometheus-draw-dx12
		LANGUAGES CXX
)

add_executable(
		${PROJECT_NAME}

		${PROJECT_SOURCE_DIR}/../common/glfw_callback_handler.cpp
		${PROJECT_SOURCE_DIR}/backend.cpp
		${PROJECT_SOURCE_DIR}/main.cpp
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

# STB
include(${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/stb/stb.cmake)
cmake_language(
		CALL
		${PROJECT_NAME_PREFIX}ATTACH_EXTERNAL_STB
)

# GLFW
include(${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/glfw/glfw.cmake)
cmake_language(
		CALL
		${PROJECT_NAME_PREFIX}ATTACH_EXTERNAL_GLFW
)

target_compile_definitions(
		${PROJECT_NAME}
		PUBLIC

		ASSETS_PATH_PIC="${ASSETS_PATH_PIC}"
)