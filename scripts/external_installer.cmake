# CPM
set(LOCAL_PATH "${${PROJECT_NAME_PREFIX}ROOT_PATH_CMAKE_SCRIPT}/CPM.cmake")
function(${PROJECT_NAME_PREFIX}EXTERNAL_INSTALLER_LOAD_CPM)
	set(GITHUB_URL "https://github.com/cpm-cmake/CPM.cmake")

	if (NOT DEFINED ${PROJECT_NAME_PREFIX}CPM_LATEST_VERSION)
		find_program(GIT_EXE NAMES git REQUIRED)
		execute_process(
				COMMAND
				${GIT_EXE} ls-remote --tags --sort=-v:refname ${GITHUB_URL}.git
				OUTPUT_VARIABLE TAG_LIST
				ERROR_VARIABLE ERROR_MESSAGE
				RESULT_VARIABLE RESULT
		)
		if (RESULT EQUAL 0)
			string(REGEX MATCH "v[0-9]+\\.[0-9]+\\.[0-9]+" LATEST_VERSION ${TAG_LIST})
			set(
					${PROJECT_NAME_PREFIX}CPM_LATEST_VERSION
					${LATEST_VERSION}
					CACHE
					STRING
					"CPM Latest version."
			)
		else ()
			message(WARNING "[PROMETHEUS] [CPM] Cannot get the latest version tag: ${ERROR_MESSAGE}")
		endif (RESULT EQUAL 0)
	else ()
		set(LATEST_VERSION ${${PROJECT_NAME_PREFIX}CPM_LATEST_VERSION})
	endif (NOT DEFINED ${PROJECT_NAME_PREFIX}CPM_LATEST_VERSION)

	# https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.2/CPM.cmake
	set(GITHUB_DOWNLOAD_URL "${GITHUB_URL}/releases/download/${LATEST_VERSION}/CPM.cmake")

	if (NOT EXISTS ${LOCAL_PATH})
		message(STATUS "[PROMETHEUS] [CPM] File `CPM.cmake` does not exist, try to download...")
		if (NOT DEFINED LATEST_VERSION)
			message(FATAL_ERROR "[PROMETHEUS] [CPM] Can't get the latest version tag of CPM, please check your internet connection...")
		else ()
			message(STATUS "[PROMETHEUS] [CPM] Downloading ${GITHUB_DOWNLOAD_URL}...")
			file(DOWNLOAD ${GITHUB_DOWNLOAD_URL} ${LOCAL_PATH} STATUS DOWNLOAD_STATUS)

			if (NOT DOWNLOAD_STATUS EQUAL 0)
				message(FATAL_ERROR "[PROMETHEUS] [CPM] Can't download the latest version tag of CPM, please check your internet connection...")
			else ()
				message(STATUS "[PROMETHEUS] [CPM] The file has been downloaded to `${LOCAL_PATH}`")
			endif (NOT DOWNLOAD_STATUS EQUAL 0)
		endif (NOT DEFINED LATEST_VERSION)
	else ()
		set(CPM_USE_LOCAL_PACKAGES ON)
		include(${LOCAL_PATH})
		if (DEFINED LATEST_VERSION)
			message(STATUS "[PROMETHEUS] [CPM] File `CPM.cmake` does exist, compare the version...")
			if (NOT "v${CURRENT_CPM_VERSION}" STREQUAL "${LATEST_VERSION}")
				message(STATUS "[PROMETHEUS] [CPM] Version mismatch(v${CURRENT_CPM_VERSION} => ${LATEST_VERSION})")
				message(STATUS "[PROMETHEUS] [CPM] Downloading ${GITHUB_DOWNLOAD_URL}...")
				file(DOWNLOAD ${GITHUB_DOWNLOAD_URL} ${LOCAL_PATH}.tmp STATUS DOWNLOAD_STATUS)

				if (NOT DOWNLOAD_STATUS EQUAL 0)
					message(WARNING "[PROMETHEUS] [CPM] Can't download the latest version tag of CPM, use local version...")
				else ()
					file(RENAME ${LOCAL_PATH}.tmp ${LOCAL_PATH})

					message(STATUS "[PROMETHEUS] [CPM] The file has been downloaded to `${LOCAL_PATH}`")
					# force regenerate
					message(FATAL_ERROR "[PROMETHEUS] [CPM] Updated CPM version, please regenerate cmake cache...")
				endif (NOT DOWNLOAD_STATUS EQUAL 0)
			else ()
				message(STATUS "[PROMETHEUS] [CPM] The CPM current version is the latest version(v${CURRENT_CPM_VERSION}), no need to update...")
			endif (NOT "v${CURRENT_CPM_VERSION}" STREQUAL "${LATEST_VERSION}")
		endif (DEFINED LATEST_VERSION)
	endif (NOT EXISTS ${LOCAL_PATH})
endfunction(${PROJECT_NAME_PREFIX}EXTERNAL_INSTALLER_LOAD_CPM)

cmake_language(
		CALL
		${PROJECT_NAME_PREFIX}EXTERNAL_INSTALLER_LOAD_CPM
)
macro(${PROJECT_NAME_PREFIX}EXTERNAL_INSTALL_CPM)
	CPMAddPackage(${ARGN})
endmacro(${PROJECT_NAME_PREFIX}EXTERNAL_INSTALL_CPM)

# NUGET
function(${PROJECT_NAME_PREFIX}EXTERNAL_INSTALL_NUGET package_name config_file_path)
	find_program(NUGET_EXE NAMES nuget REQUIRED)
	if (NUGET_EXE_FOUND)
		configure_file(${config_file} ${CMAKE_BINARY_DIR}/packages.${package_name}.config)
		execute_process(
				COMMAND
				${NUGET_EXE} restore packages.${package_name}.config -SolutionDirectory ${CMAKE_BINARY_DIR}
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		)
	else ()
		message(FATAL_ERROR "[PROMETHEUS] nuget not found!")
	endif (NUGET_EXE_FOUND)
endfunction(${PROJECT_NAME_PREFIX}EXTERNAL_INSTALL_NUGET package_name config_file_path)
