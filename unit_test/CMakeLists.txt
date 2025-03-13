project(
		prometheus-unit-test
		LANGUAGES CXX
)

add_executable(
		${PROJECT_NAME}

		${PROJECT_SOURCE_DIR}/src/main.cpp
		
		# =================================
		# META
		# =================================
		
		${PROJECT_SOURCE_DIR}/src/meta/enumeration.cpp
		${PROJECT_SOURCE_DIR}/src/meta/member.cpp
		${PROJECT_SOURCE_DIR}/src/meta/dimension.cpp

		# =================================
		# FUNCTIONAL
		# =================================
		
		${PROJECT_SOURCE_DIR}/src/functional/aligned_union.cpp
		${PROJECT_SOURCE_DIR}/src/functional/function_ref.cpp
		
		# =================================
		# CHARS
		# =================================
		
		${PROJECT_SOURCE_DIR}/src/chars/scalar.cpp
		${PROJECT_SOURCE_DIR}/src/chars/icelake.cpp

		# =================================
		# CONCURRENCY
		# =================================
		
		${PROJECT_SOURCE_DIR}/src/concurrency/queue.cpp
		
		# =================================
		# COROUTINE
		# =================================
		
		${PROJECT_SOURCE_DIR}/src/coroutine/task.cpp
		${PROJECT_SOURCE_DIR}/src/coroutine/generator.cpp
		
		# =========================
		# STRING
		# =========================

		${PROJECT_SOURCE_DIR}/src/string/string_pool.cpp
		
		## =================================
		## WILDCARD
		## =================================
		#
		#${PROJECT_SOURCE_DIR}/src/wildcard/wildcard.cpp
)

target_compile_features(
		${PROJECT_NAME}
		PUBLIC
		cxx_std_23
)

target_link_libraries(
		${PROJECT_NAME}
		PRIVATE
		prometheus
)

add_test(
		NAME ${PROJECT_NAME}
		COMMAND ${PROJECT_NAME}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# assets path
set(ASSETS_PATH_PIC ${CMAKE_SOURCE_DIR}/assets/pic.jpg)
# DRAW
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/draw)
