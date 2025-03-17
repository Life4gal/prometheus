if (${PROJECT_NAME_PREFIX}PLATFORM_WINDOWS)
	add_subdirectory(dx11)
	add_subdirectory(dx12)
endif (${PROJECT_NAME_PREFIX}PLATFORM_WINDOWS)

#find_package(Vulkan)
#
#if (Vulkan_FOUND)
#	add_subdirectory(vulkan)
#endif (Vulkan_FOUND)
