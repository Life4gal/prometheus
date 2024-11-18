function(${PROJECT_NAME_PREFIX}ATTACH_EXTERNAL_FREETYPE)
	target_include_directories(
			${PROJECT_NAME}
			PRIVATE
			${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/freetype/include
	)

	if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		# freetype 2.13.2
		set(freetype_dll_path ${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/freetype/freetype.dll)
		set(freetype_lib_path ${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/freetype/freetype.lib)

		set(zlib1_dll_path ${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/freetype/zlib1.dll)
		set(bz2_dll_path ${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/freetype/bz2.dll)
		set(libpng16_dll_path ${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/freetype/libpng16.dll)
		set(brotlidec_dll_path ${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/freetype/brotlidec.dll)
		set(brotlicommon_path ${${PROJECT_NAME_PREFIX}ROOT_PATH_EXTERNAL_LIBRARY}/freetype/brotlicommon.dll)

		if (NOT TARGET freetype_library)
			add_library(
					freetype_library
					SHARED
					IMPORTED
			)

			set_target_properties(
					freetype_library
					PROPERTIES
					IMPORTED_LOCATION ${freetype_dll_path}
					IMPORTED_IMPLIB ${freetype_lib_path}
					LINKER_LANGUAGE C
			)
		endif (NOT TARGET freetype_library)

		get_target_property(
				target_binary_directory
				${PROJECT_NAME}
				RUNTIME_OUTPUT_DIRECTORY
		)

		add_custom_command(
				TARGET ${PROJECT_NAME}
				POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory ${target_binary_directory}

				COMMAND ${CMAKE_COMMAND} -E echo "[freetype] Copying dll from '${freetype_dll_path}' to '${target_binary_directory}'."
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${freetype_dll_path} ${target_binary_directory}/

				COMMAND ${CMAKE_COMMAND} -E echo "[freetype] Copying dll from '${zlib1_dll_path}' to '${target_binary_directory}'."
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${zlib1_dll_path} ${target_binary_directory}/

				COMMAND ${CMAKE_COMMAND} -E echo "[freetype] Copying dll from '${bz2_dll_path}' to '${target_binary_directory}'."
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${bz2_dll_path} ${target_binary_directory}/

				COMMAND ${CMAKE_COMMAND} -E echo "[freetype] Copying dll from '${libpng16_dll_path}' to '${target_binary_directory}'."
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${libpng16_dll_path} ${target_binary_directory}/

				COMMAND ${CMAKE_COMMAND} -E echo "[freetype] Copying dll from '${brotlidec_dll_path}' to '${target_binary_directory}'."
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${brotlidec_dll_path} ${target_binary_directory}/

				COMMAND ${CMAKE_COMMAND} -E echo "[freetype] Copying dll from '${brotlicommon_path}' to '${target_binary_directory}'."
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${brotlicommon_path} ${target_binary_directory}/
		)

		target_link_libraries(
				${PROJECT_NAME}
				PRIVATE
				freetype_library
		)
	elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		find_package(Freetype REQUIRED)
		target_link_libraries(${PROJECT_NAME} PRIVATE Freetype::Freetype)
	elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		find_package(Freetype REQUIRED)
		target_link_libraries(${PROJECT_NAME} PRIVATE Freetype::Freetype)
	else ()
		message(FATAL_ERROR "Unsupported Platform: ${CMAKE_SYSTEM_NAME}")
	endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endfunction(${PROJECT_NAME_PREFIX}ATTACH_EXTERNAL_FREETYPE)
