# INSTALL TARGETS
#if (${PROJECT_NAME_PREFIX}INSTALL)
#	# PackageProject.cmake will be used to make our target installable
#	CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.10.0")
#
#	# the location where the project's version header will be placed should match the project's regular
#	# header paths
#	string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
#
#	packageProject(
#			# the name of the target to export
#			NAME ${PROJECT_NAME}
#			# the version of the target to export
#			VERSION ${PROJECT_VERSION}
#			# (optional) install your library with a namespace (Note: do NOT add extra '::')
#			NAMESPACE ${PROJECT_NAME}
#			# a temporary directory to create the config files
#			BINARY_DIR ${PROJECT_BINARY_DIR}
#			# location of the target's public headers
#			# see target_include_directories -> BUILD_INTERFACE
#			INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src
#			# should match the target's INSTALL_INTERFACE include directory
#			# see target_include_directories -> INSTALL_INTERFACE
#			INCLUDE_DESTINATION ${${PROJECT_NAME_PREFIX}INSTALL_HEADERS}/${PROJECT_NAME}-${PROJECT_VERSION}
#			# (optional) create a header containing the version info
#			# Note: that the path to headers should be lowercase
#			VERSION_HEADER "${VERSION_HEADER_LOCATION}"
#			# (optional) define the project's version compatibility, defaults to `AnyNewerVersion`
#			# supported values: `AnyNewerVersion|SameMajorVersion|SameMinorVersion|ExactVersion`
#			COMPATIBILITY AnyNewerVersion
#			# semicolon separated list of the project's dependencies
#			# see `LINK 3rd-PARTY LIBRARIES`
#			DEPENDENCIES ${${PROJECT_NAME_PREFIX}3RD_PARTY_DEPENDENCIES}
#			# (optional) option to disable the versioning of install destinations
#			DISABLE_VERSION_SUFFIX YES
#			# (optional) option to ignore target architecture for package resolution
#			# defaults to YES for header only (i.e. INTERFACE) libraries
#			ARCH_INDEPENDENT YES
#	)
#endif (${PROJECT_NAME_PREFIX}INSTALL)