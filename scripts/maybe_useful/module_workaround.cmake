set(${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH ${CMAKE_BINARY_DIR}/temp_module CACHE STRING "Temp module path" FORCE)
if (${PROJECT_NAME_PREFIX}MODULE)
	if (CMAKE_VERSION VERSION_LESS "3.30")
		if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			if (NOT DEFINED ENV{VCToolsInstallDir})
				message(FATAL_ERROR "[PROMETHEUS] Unable to find path to VC Tools in environment variables!")
			endif (NOT DEFINED ENV{VCToolsInstallDir})

			if (NOT EXISTS $ENV{VCToolsInstallDir})
				message(FATAL_ERROR "[PROMETHEUS] Invalid VC Tools installation path!")
			else ()
				message(STATUS "[PROMETHEUS] Found VC Tools in: [$ENV{VCToolsInstallDir}]")
			endif (NOT EXISTS $ENV{VCToolsInstallDir})

			if (NOT EXISTS $ENV{VCToolsInstallDir}modules/std.ixx)
				message(FATAL_ERROR "[PROMETHEUS] Cannot find std.ixx! Please check MSVC version(${CMAKE_CXX_COMPILER_VERSION})!")
			else ()
				message(STATUS "[PROMETHEUS] Found module std.ixx in: [$ENV{VCToolsInstallDir}modules\\std.ixx]")
			endif (NOT EXISTS $ENV{VCToolsInstallDir}modules/std.ixx)

			configure_file(
					$ENV{VCToolsInstallDir}modules/std.ixx
					${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}/std.ixx
					COPYONLY
			)

			set_source_files_properties(
					${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}/std.ixx
					PROPERTIES
					COMPILE_OPTIONS "/Wv:18"
			)

			target_sources(
					${PROJECT_NAME}
					# fixme: Possible conflicts with other libraries?
					PUBLIC
					FILE_SET std_module
					TYPE CXX_MODULES
					BASE_DIRS "${PROJECT_SOURCE_DIR}"
					FILES

					$<$<CXX_COMPILER_ID:MSVC>:${${PROJECT_NAME_PREFIX}TEMP_CXX_MODULE_PATH}/std.ixx>
			)
		elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
			# https://gitlab.kitware.com/cmake/cmake/-/blob/master/.gitlab/ci/cxx_modules_rules_clang.cmake
			# Default to C++ extensions being off. Clang's modules support have trouble
			# with extensions right now.
			set(CMAKE_CXX_EXTENSIONS OFF)

			# fixme: https://discourse.cmake.org/t/c-20-modules-update/7330/24
			# fixme:
			# PLEASE submit a bug report to https://github.com/llvm/llvm-project/issues/ and include the crash backtrace.
			# Stack dump:
			# 0.	Program arguments: "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Tools/Llvm/x64/bin/clang-scan-deps.exe" -format=p1689 -- C:\\PROGRA~1\\MIB055~1\\2022\\Preview\\VC\\Tools\\Llvm\\x64\\bin\\clang-cl.exe -DGAL_PROMETHEUS_COMPILER_CLANG_CL -DGAL_PROMETHEUS_COMPILER_ID=\"Clang\" -DGAL_PROMETHEUS_COMPILER_NAME=\"Clang.16.0.5\" -DGAL_PROMETHEUS_COMPILER_VERSION=\"16.0.5\" -DGAL_PROMETHEUS_MAJOR_VERSION=0 -DGAL_PROMETHEUS_MINOR_VERSION=0 -DGAL_PROMETHEUS_PATCH_VERSION=6 -DGAL_PROMETHEUS_PLATFORM_WINDOWS -DGAL_PROMETHEUS_VERSION=\"0.0.6\" -IC:\workspace\life4gal\prometheus\src  -x c++ C:\workspace\life4gal\prometheus\src\infrastructure\type_traits.ixx -c -o CMakeFiles\prometheus.dir\src\infrastructure\type_traits.ixx.obj -MT CMakeFiles\prometheus.dir\src\infrastructure\type_traits.ixx.obj.ddi -MD -MF CMakeFiles\prometheus.dir\src\infrastructure\type_traits.ixx.obj.ddi.d > CMakeFiles\prometheus.dir\src\infrastructure\type_traits.ixx.obj.ddi"
			# Exception Code: 0xC0000005
			# #0 0x00007ff7b4803c75 C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe 0x3c75 C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe 0x3efc9d0
			# #1 0x00007ff7b4803c75 (C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe+0x3c75)
			# #2 0x00007ff7b86fc9d0 (C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe+0x3efc9d0)
			# 0x00007FF7B4803C75, C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe(0x00007FF7B4800000) + 0x3C75 byte(s)
			# 0x00007FF7B86FC9D0, C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\Llvm\x64\bin\clang-scan-deps.exe(0x00007FF7B4800000) + 0x3EFC9D0 byte(s)
			# 0x00007FFA728526AD, C:\WINDOWS\System32\KERNEL32.DLL(0x00007FFA72840000) + 0x126AD byte(s), BaseThreadInitThunk() + 0x1D byte(s)
			# 0x00007FFA72C2AA68, C:\WINDOWS\SYSTEM32\ntdll.dll(0x00007FFA72BD0000) + 0x5AA68 byte(s), RtlUserThreadStart() + 0x28 byte(s)
			string(CONCAT CMAKE_EXPERIMENTAL_CXX_SCANDEP_SOURCE
					"\"${CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS}\""
					" -format=p1689"
					" --"
					" \"<CMAKE_CXX_COMPILER>\" <DEFINES> <INCLUDES> "#<FLAGS>"
					" -x c++ <SOURCE> -c -o <OBJECT>"
					" -MT <DYNDEP_FILE>"
					" -MD -MF <DEP_FILE>"
					" > <DYNDEP_FILE>")
			set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "clang")
			set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FLAG "@<MODULE_MAP_FILE>")
		elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			# https://gitlab.kitware.com/cmake/cmake/-/blob/master/.gitlab/ci/cxx_modules_rules_gcc.cmake
			string(CONCAT CMAKE_EXPERIMENTAL_CXX_SCANDEP_SOURCE
					"<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -E -x c++ <SOURCE>"
					" -MT <DYNDEP_FILE> -MD -MF <DEP_FILE>"
					" -fmodules-ts "#-fdep-file=<DYNDEP_FILE> -fdep-output=<OBJECT> -fdep-format=trtbd"
					" -o <PREPROCESSED_SOURCE>")
			set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "gcc")
			set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FLAG "-fmodules-ts -fmodule-mapper=<MODULE_MAP_FILE> -fdep-format=trtbd -x c++")
		else ()
			message(FATAL_ERROR "[PROMETHEUS] Unsupported compilers: ${CMAKE_CXX_COMPILER}")
		endif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	else()
		# todo
		#set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
		#set(CMAKE_CXX_MODULE_STD 1)
	endif (CMAKE_VERSION VERSION_LESS "3.30")
endif (${PROJECT_NAME_PREFIX}MODULE)