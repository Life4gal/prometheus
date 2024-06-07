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

add_executable(
		${PROJECT_NAME}-gui

		${PROJECT_SOURCE_DIR}/src/gui_main.cpp
)

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
