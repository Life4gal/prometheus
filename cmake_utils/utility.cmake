include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

# fetch remote cmake utils
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/fetch_utils.cmake)
# documented variables
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/doc_var.cmake)

# CPM ==> DEPENDENCIES (SOURCE CODE)
include(${PROJECT_SOURCE_DIR}/cmake_utils/CPM.cmake)
# Let CPMAddPackage first check if the required package exists locally.
# Compared to CPMFindPackage, CPMAddPackage will call cpm_export_variables after finding the local package.
set(CPM_USE_LOCAL_PACKAGES ON)
include(${PROJECT_SOURCE_DIR}/cmake_utils/cpm_install.cmake)

# NUGET ==> DEPENDENCIES (WINDOWS PREBUILT BINARY)
include(${PROJECT_SOURCE_DIR}/cmake_utils/nuget_install.cmake)


