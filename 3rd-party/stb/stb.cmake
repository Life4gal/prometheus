function(link_3rd_party_library_stb project_name)
	target_include_directories(
		${project_name}
		PRIVATE
		${${PROJECT_NAME_PREFIX}3RD_PARTY_LIBRARY_PATH}/stb/include
	)

	# header-only
endfunction(link_3rd_party_library_stb project_name)
