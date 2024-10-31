include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/module_workaround.cmake)

# =======================
# MODULE SOURCE
# =======================

if (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})
	set(
			${PROJECT_NAME_PREFIX}MODULE_SOURCE_CHARS_ICELAKE

			${PROJECT_SOURCE_DIR}/src/chars/icelake.ixx
			${PROJECT_SOURCE_DIR}/src/chars/icelake_ascii.ixx
			${PROJECT_SOURCE_DIR}/src/chars/icelake_utf8.ixx
			${PROJECT_SOURCE_DIR}/src/chars/icelake_utf16.ixx
			${PROJECT_SOURCE_DIR}/src/chars/icelake_utf32.ixx
	)
else ()
	set(
			${PROJECT_NAME_PREFIX}MODULE_SOURCE_CHARS_ICELAKE
	)
endif (${${PROJECT_NAME_PREFIX}CPU_FEATURES_ICELAKE_SUPPORTED})

set(
		${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC

		# =========================
		# META
		# =========================

		${PROJECT_SOURCE_DIR}/src/meta/string.ixx
		${PROJECT_SOURCE_DIR}/src/meta/name.ixx
		${PROJECT_SOURCE_DIR}/src/meta/enumeration.ixx
		${PROJECT_SOURCE_DIR}/src/meta/member.ixx
		${PROJECT_SOURCE_DIR}/src/meta/to_string.ixx

		${PROJECT_SOURCE_DIR}/src/meta/meta.ixx

		# =========================
		# PLATFORM
		# =========================

		${PROJECT_SOURCE_DIR}/src/platform/exception.ixx
		${PROJECT_SOURCE_DIR}/src/platform/debug.ixx
		${PROJECT_SOURCE_DIR}/src/platform/command_line.ixx
		${PROJECT_SOURCE_DIR}/src/platform/instruction_set.ixx

		${PROJECT_SOURCE_DIR}/src/platform/platform.ixx

		# =========================
		# FUNCTIONAL
		# =========================

		${PROJECT_SOURCE_DIR}/src/functional/type_list.ixx
		${PROJECT_SOURCE_DIR}/src/functional/value_list.ixx
		${PROJECT_SOURCE_DIR}/src/functional/functor.ixx
		${PROJECT_SOURCE_DIR}/src/functional/aligned_union.ixx
		${PROJECT_SOURCE_DIR}/src/functional/function_ref.ixx
		${PROJECT_SOURCE_DIR}/src/functional/math.ixx
		${PROJECT_SOURCE_DIR}/src/functional/flag.ixx
		${PROJECT_SOURCE_DIR}/src/functional/function_signature.ixx
		${PROJECT_SOURCE_DIR}/src/functional/hash.ixx

		${PROJECT_SOURCE_DIR}/src/functional/functional.ixx

		# =========================
		# MEMORY
		# =========================

		${PROJECT_SOURCE_DIR}/src/memory/read_write.ixx

		${PROJECT_SOURCE_DIR}/src/memory/memory.ixx

		# =========================
		# NUMERIC
		# =========================

		${PROJECT_SOURCE_DIR}/src/numeric/random_engine.ixx
		${PROJECT_SOURCE_DIR}/src/numeric/random.ixx

		${PROJECT_SOURCE_DIR}/src/numeric/numeric.ixx

		# =========================
		# PRIMITIVE
		# =========================

		${PROJECT_SOURCE_DIR}/src/primitive/multidimensional.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/point.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/extent.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/rect.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/circle.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/ellipse.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/color.ixx
		${PROJECT_SOURCE_DIR}/src/primitive/vertex.ixx

		${PROJECT_SOURCE_DIR}/src/primitive/primitive.ixx

		# =========================
		# CONCURRENCY
		# =========================

		${PROJECT_SOURCE_DIR}/src/concurrency/thread.ixx
		${PROJECT_SOURCE_DIR}/src/concurrency/unfair_mutex.ixx
		${PROJECT_SOURCE_DIR}/src/concurrency/queue.ixx

		${PROJECT_SOURCE_DIR}/src/concurrency/concurrency.ixx

		# =========================
		# COROUTINE
		# =========================

		${PROJECT_SOURCE_DIR}/src/coroutine/task.ixx
		${PROJECT_SOURCE_DIR}/src/coroutine/generator.ixx

		${PROJECT_SOURCE_DIR}/src/coroutine/coroutine.ixx

		# =========================
		# STRING
		# =========================

		${PROJECT_SOURCE_DIR}/src/string/charconv.ixx
		${PROJECT_SOURCE_DIR}/src/string/string_pool.ixx

		${PROJECT_SOURCE_DIR}/src/string/string.ixx

		# =========================
		# IO
		# =========================

		${PROJECT_SOURCE_DIR}/src/io/device.ixx

		${PROJECT_SOURCE_DIR}/src/io/io.ixx

		# =========================
		# CHARS
		# =========================

		${PROJECT_SOURCE_DIR}/src/chars/encoding.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar_ascii.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar_utf8.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar_utf16.ixx
		${PROJECT_SOURCE_DIR}/src/chars/scalar_utf32.ixx
		${${PROJECT_NAME_PREFIX}MODULE_SOURCE_CHARS_ICELAKE}

		${PROJECT_SOURCE_DIR}/src/chars/chars.ixx

		# =========================
		# COMMAND_LINE_PARSER
		# =========================

		${PROJECT_SOURCE_DIR}/src/command_line_parser/command_line_parser.ixx

		# =========================
		# UNIT_TEST
		# =========================

		${PROJECT_SOURCE_DIR}/src/unit_test/unit_test.ixx

		# =========================
		# WILDCARD
		# =========================

		${PROJECT_SOURCE_DIR}/src/wildcard/wildcard.ixx

		# =========================
		# STATE_MACHINE
		# =========================

		${PROJECT_SOURCE_DIR}/src/state_machine/state_machine.ixx

		# =========================
		# DRAW
		# =========================

		${PROJECT_SOURCE_DIR}/src/draw/def.ixx
		${PROJECT_SOURCE_DIR}/src/draw/font.ixx
		${PROJECT_SOURCE_DIR}/src/draw/draw_list.ixx
		${PROJECT_SOURCE_DIR}/src/draw/theme.ixx
		${PROJECT_SOURCE_DIR}/src/draw/window.ixx
		${PROJECT_SOURCE_DIR}/src/draw/context.ixx

		${PROJECT_SOURCE_DIR}/src/draw/draw.ixx

		# =========================
		# ROOT
		# =========================

		${PROJECT_SOURCE_DIR}/src/prometheus.ixx
)

set(
		${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE

		# =========================
		# PLATFORM
		# =========================

		${PROJECT_SOURCE_DIR}/src/platform/exception.impl.ixx
		${PROJECT_SOURCE_DIR}/src/platform/debug.impl.ixx
		${PROJECT_SOURCE_DIR}/src/platform/command_line.impl.ixx
		${PROJECT_SOURCE_DIR}/src/platform/instruction_set.impl.ixx

		# =========================
		# CONCURRENCY
		# =========================

		${PROJECT_SOURCE_DIR}/src/concurrency/thread.impl.ixx
		${PROJECT_SOURCE_DIR}/src/concurrency/unfair_mutex.impl.ixx

		# =========================
		# DRAW
		# =========================

		${PROJECT_SOURCE_DIR}/src/draw/def.impl.ixx
		${PROJECT_SOURCE_DIR}/src/draw/font.impl.ixx
		${PROJECT_SOURCE_DIR}/src/draw/draw_list.impl.ixx
		${PROJECT_SOURCE_DIR}/src/draw/theme.impl.ixx
		${PROJECT_SOURCE_DIR}/src/draw/window.impl.ixx
		${PROJECT_SOURCE_DIR}/src/draw/context.impl.ixx
)

set(${PROJECT_NAME_PREFIX}MODULE_FRAGMENT_MACRO_NAME "${PROJECT_NAME_PREFIX}MODULE_FRAGMENT_DEFINED")
# ---------------------------------
# #if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED
# ...
# #endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED <---- WARNING!!!
# ---------------------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	if (CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
	else ()
		target_compile_options(
				${PROJECT_NAME}
				PUBLIC
				"-Wno-endif-labels"
		)
	endif (CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(
			${PROJECT_NAME}
			PUBLIC
			"-Wno-endif-labels"
	)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	target_compile_options(
			${PROJECT_NAME}
			PUBLIC
			"-Wno-endif-labels"
	)
else ()
endif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

if (${PROJECT_NAME_PREFIX}MODULE)
	target_compile_definitions(
			${PROJECT_NAME}
			PUBLIC
			${PROJECT_NAME_PREFIX}USE_MODULE=1
	)

	# MSVC =>
	# xxx.impl.ixx
	# #if GAL_PROMETHEUS_USE_MODULE
	# module; <== warning C5201: a module declaration can appear only at the start of a translation unit unless a global module fragment is used
	# #include <...>
	#
	# export import xxx.impl;
	#
	# import ...;
	#
	#else
	# #include <...>
	#
	# #endif

	function(MODIFY_MODULE_FILE file sources)
		file(RELATIVE_PATH relative_path ${PROJECT_SOURCE_DIR}/src ${file})
		set(target_path ${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}/${relative_path})

		string(REPLACE "/" "_" cache_var_name ${relative_path})
		string(TOUPPER ${cache_var_name} cache_var_name)
		set(cache_var_name "${PROJECT_NAME_PREFIX}MODULE_FILE_CACHED_${cache_var_name}")
		file(MD5 ${file} cache_var_md5)

		if (NOT DEFINED ${cache_var_name} OR NOT ${${cache_var_name}} STREQUAL ${cache_var_md5})
			message(STATUS "[PROMETHEUS] [MODULE FILE FRAGMENT] ${file} => ${target_path}")

			get_filename_component(dir ${target_path} DIRECTORY)
			file(MAKE_DIRECTORY ${dir})

			file(READ ${file} file_content LIMIT 2048)
			set(module_fragment_begin "#if not ${${PROJECT_NAME_PREFIX}MODULE_FRAGMENT_MACRO_NAME}")
			set(module_fragment_end "#endif not ${${PROJECT_NAME_PREFIX}MODULE_FRAGMENT_MACRO_NAME}")
			string(FIND "${file_content}" "${module_fragment_begin}" fragment_begin)
			string(FIND "${file_content}" "${module_fragment_end}" fragment_end)
			if (fragment_begin EQUAL -1 OR fragment_end EQUAL -1)
				message(
						FATAL_ERROR
						"File [${file}] does not contain MODULE fragment. \n\t\"${module_fragment_begin}\" => ${fragment_begin}\n\t\"${module_fragment_end}\" => ${fragment_end}"
				)
			endif (fragment_begin EQUAL -1 OR fragment_end EQUAL -1)

			string(LENGTH ${module_fragment_begin} module_fragment_begin_length)
			math(EXPR real_fragment_begin "${fragment_begin} + ${module_fragment_begin_length}")
			math(EXPR fragment_length "${fragment_end} - ${real_fragment_begin}")
			string(SUBSTRING "${file_content}" ${real_fragment_begin} ${fragment_length} module_fragment)

			file(WRITE ${target_path} "module;")
			file(APPEND ${target_path} "${module_fragment}")
			file(APPEND ${target_path} "#define ${${PROJECT_NAME_PREFIX}MODULE_FRAGMENT_MACRO_NAME} 1\n")

			# warning C5244: '#include <xxx.ixx>' in the purview of module 'gal.prometheus' appears erroneous.
			# Consider moving that directive before the module declaration, or replace the textual inclusion with 'import <xxx.ixx>;'.
			file(APPEND ${target_path} "${PROJECT_NAME_PREFIX}COMPILER_DISABLE_WARNING_PUSH\n")
			file(APPEND ${target_path} "#if defined(${PROJECT_NAME_PREFIX}COMPILER_MSVC)\n")
			file(APPEND ${target_path} "${PROJECT_NAME_PREFIX}COMPILER_DISABLE_WARNING(5244)\n")
			file(APPEND ${target_path} "#endif defined(${PROJECT_NAME_PREFIX}COMPILER_MSVC)\n")
			file(APPEND ${target_path} "#include <${relative_path}>\n")
			file(APPEND ${target_path} "${PROJECT_NAME_PREFIX}COMPILER_DISABLE_WARNING_POP\n")

			set(${cache_var_name} ${cache_var_md5} CACHE STRING "MD5 of ${file}'s module fragment." FORCE)
		endif (NOT DEFINED ${cache_var_name} OR NOT ${${cache_var_name}} STREQUAL ${cache_var_md5})

		list(APPEND ${sources} ${target_path})
		set(${sources} ${${sources}} PARENT_SCOPE)
	endfunction(MODIFY_MODULE_FILE file sources)

	# PUBLIC
	set(${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC_ORIGINAL ${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC})
	set(${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC "")

	foreach (file ${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC_ORIGINAL})
		MODIFY_MODULE_FILE(${file} ${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC)
	endforeach (file ${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC_ORIGINAL})

	# PRIVATE
	set(${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE_ORIGINAL ${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE})
	set(${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE "")

	foreach (file ${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE_ORIGINAL})
		MODIFY_MODULE_FILE(${file} ${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE)
	endforeach (file ${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE_ORIGINAL})
else ()
	set(${PROJECT_NAME_PREFIX}USE_MODULE 0)
	target_compile_definitions(
			${PROJECT_NAME}
			PUBLIC
			${PROJECT_NAME_PREFIX}USE_MODULE=0
			${${PROJECT_NAME_PREFIX}MODULE_FRAGMENT_MACRO_NAME}=1
	)
endif (${PROJECT_NAME_PREFIX}MODULE)

if (${PROJECT_NAME_PREFIX}MODULE)
	target_sources(
			${PROJECT_NAME}
			PUBLIC
			FILE_SET public_module_files
			TYPE CXX_MODULES
			#BASE_DIRS "${PROJECT_SOURCE_DIR}"
			BASE_DIRS "${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}"
			FILES

			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC}
			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE}
	)
else ()
	set_source_files_properties(
			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC}
			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE}
			PROPERTIES
			LANGUAGE CXX
	)
	set_source_files_properties(
			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC}
			PROPERTIES
			HEADER_FILE_ONLY ON
	)

	target_sources(
			${PROJECT_NAME}
			PUBLIC
			FILE_SET public_header_files
			TYPE HEADERS
			BASE_DIRS "${PROJECT_SOURCE_DIR}"
			FILES

			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC}
	)

	target_sources(
			${PROJECT_NAME}
			#PUBLIC
			#${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC}
			PRIVATE
			${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PRIVATE}
	)

	#function(MODIFY_MODULE_FILE file sources)
	#	file(RELATIVE_PATH relative_path ${PROJECT_SOURCE_DIR}/src ${file})
	#	# remove suffix
	#	string(REGEX REPLACE "\\.[^.]*$" "" relative_path_no_suffix ${relative_path})

	#	get_filename_component(dir ${relative_path_no_suffix} DIRECTORY)
	#	get_filename_component(filename ${relative_path_no_suffix} NAME)

	#	#string(LENGTH "${dir}" dir_length)
	#	#if (dir_length EQUAL 0 OR dir STREQUAL filename)
	#		string(REPLACE "/" "_" cache_var_name "${relative_path_no_suffix}.hpp")
	#		string(TOUPPER ${cache_var_name} cache_var_name)
	#		set(cache_var_name "${PROJECT_NAME_PREFIX}HEADER_FILE_CACHED_${cache_var_name}")
	#		file(MD5 ${file} cache_var_md5)

	#		if (NOT DEFINED ${cache_var_name} OR NOT ${${cache_var_name}} STREQUAL ${cache_var_md5})
	#			set(target_path "${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}/${relative_path_no_suffix}.hpp")
	#			get_filename_component(target_dir ${target_path} DIRECTORY)
	#			file(MAKE_DIRECTORY ${target_dir})

	#			message(STATUS "[PROMETHEUS] [MODULE FILE FRAGMENT] ${file} => ${target_path}")

	#			file(WRITE ${target_path} "#pragma once\n")
	#			file(APPEND ${target_path} "#include <${relative_path}>\n")

	#			set(${cache_var_name} ${cache_var_md5} CACHE STRING "MD5 of ${file}'s module fragment." FORCE)
	#		endif (NOT DEFINED ${cache_var_name} OR NOT ${${cache_var_name}} STREQUAL ${cache_var_md5})
	#	#endif (dir_length EQUAL 0 OR dir STREQUAL filename)

	#	list(APPEND ${sources} ${target_path})
	#	set(${sources} ${${sources}} PARENT_SCOPE)
	#endfunction(MODIFY_MODULE_FILE file sources)
	#
	## make INTELLISENSE happy :(
	#set(${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC_INTELLISENSE "")

	#foreach (file ${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC})
	#	MODIFY_MODULE_FILE(${file} ${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC_INTELLISENSE)
	#endforeach (file ${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC})

	#target_sources(
	#		${PROJECT_NAME}
	#		PUBLIC
	#		FILE_SET public_intellisense_header_files
	#		TYPE HEADERS
	#		BASE_DIRS "${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}"
	#		FILES

	#		${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC_INTELLISENSE}
	#)

	#target_sources(
	#		${PROJECT_NAME}
	#		PUBLIC
	#		${${PROJECT_NAME_PREFIX}MODULE_SOURCE_PUBLIC_INTELLISENSE}
	#)
endif (${PROJECT_NAME_PREFIX}MODULE)

target_sources(
		${PROJECT_NAME}
		PUBLIC
		FILE_SET macro_header_files
		TYPE HEADERS
		BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
		FILES

		${PROJECT_SOURCE_DIR}/src/prometheus/macro.hpp
)
