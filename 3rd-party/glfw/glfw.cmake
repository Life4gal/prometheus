function(link_3rd_party_library_glfw project_name)
	CPMAddPackage(
			NAME glfw
			GIT_TAG 3.4
			GITHUB_REPOSITORY "glfw/glfw"
			OPTIONS "BUILD_SHARED_LIBS OFF" "GLFW_BUILD_EXAMPLES OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_DOCS OFF" "GLFW_INSTALL OFF"
	)

	cmake_language(
			CALL
			${PROJECT_NAME_PREFIX}cpm_install
			${project_name}
			glfw 
			PRIVATE
	)
endfunction(link_3rd_party_library_glfw project_name)
