project(
		prometheus-unit-test
		LANGUAGES CXX
)

add_executable(
		${PROJECT_NAME}

		${PROJECT_SOURCE_DIR}/src/main.cpp

		# =================================
		# UTILITY
		# =================================

		${PROJECT_SOURCE_DIR}/src/utility/cast.cpp
		${PROJECT_SOURCE_DIR}/src/utility/enum_meta.cpp
		${PROJECT_SOURCE_DIR}/src/utility/aligned_union.cpp
		${PROJECT_SOURCE_DIR}/src/utility/function_ref.cpp

		# =================================
		# STRING
		# =================================

		${PROJECT_SOURCE_DIR}/src/string/wildcard_match.cpp
		${PROJECT_SOURCE_DIR}/src/string/meta_string.cpp

		# =================================
		# CONTAINER
		# =================================

		${PROJECT_SOURCE_DIR}/src/container/plane.cpp

		# =================================
		# COROUTINE
		# =================================

		${PROJECT_SOURCE_DIR}/src/coroutine/task.cpp
		${PROJECT_SOURCE_DIR}/src/coroutine/generator.cpp

		# =================================
		# I18N
		# =================================

		${PROJECT_SOURCE_DIR}/src/i18n/iso_639.cpp
		${PROJECT_SOURCE_DIR}/src/i18n/iso_3166.cpp
		${PROJECT_SOURCE_DIR}/src/i18n/iso_15924.cpp
		${PROJECT_SOURCE_DIR}/src/i18n/ietf_language_tag.cpp

		# =================================
		# CHARS
		# =================================

		${PROJECT_SOURCE_DIR}/src/chars/ascii.cpp
		${PROJECT_SOURCE_DIR}/src/chars/cp_1252.cpp
		${PROJECT_SOURCE_DIR}/src/chars/utf_8.cpp
		${PROJECT_SOURCE_DIR}/src/chars/utf_16.cpp
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
