project(
		prometheus-unit-test
		LANGUAGES CXX
)

add_executable(
		${PROJECT_NAME}

		${PROJECT_SOURCE_DIR}/src/main.cpp

		# =================================
		# INFRASTRUCTURE
		# =================================

		${PROJECT_SOURCE_DIR}/src/infrastructure/cast.cpp
		${PROJECT_SOURCE_DIR}/src/infrastructure/enum_meta.cpp
		${PROJECT_SOURCE_DIR}/src/infrastructure/aligned_union.cpp
		${PROJECT_SOURCE_DIR}/src/infrastructure/function_ref.cpp
		${PROJECT_SOURCE_DIR}/src/infrastructure/wildcard_match.cpp
		${PROJECT_SOURCE_DIR}/src/infrastructure/plane.cpp

		# =================================
		# COROUTINE
		# =================================

		${PROJECT_SOURCE_DIR}/src/coroutine/task.cpp
		${PROJECT_SOURCE_DIR}/src/coroutine/generator.cpp
)

target_link_libraries(
		${PROJECT_NAME}
		PRIVATE
		gal::prometheus
)

add_test(
		NAME ${PROJECT_NAME}
		COMMAND ${PROJECT_NAME}
)
