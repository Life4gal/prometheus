function(check_cpu_features feature_name)
	string(TOUPPER "${feature_name}" upper_feature_name)	

	try_run(
		RUN_RESULT COMPILE_RESULT
		SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake_utils/check_cpu_features_${feature_name}.cpp
		COMPILE_DEFINITIONS 
		"${${PROJECT_NAME_PREFIX}PLATFORM}"
		CXX_STANDARD 23
	)
	
	if (RUN_RESULT EQUAL 1)
		message(STATUS "[check_cpu_features] '${upper_feature_name}' instruction sets are supported.")
		set(${PROJECT_NAME_PREFIX}CPU_FEATURES_${upper_feature_name}_SUPPORTED 1 PARENT_SCOPE)
	else()
		message(STATUS "[check_cpu_features] '${upper_feature_name}' instruction sets are not supported.")
		set(${PROJECT_NAME_PREFIX}CPU_FEATURES_${upper_feature_name}_SUPPORTED 0 PARENT_SCOPE)
	endif (RUN_RESULT EQUAL 1)
endfunction(check_cpu_features feature_name)

check_cpu_features(icelake)
