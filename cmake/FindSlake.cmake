foreach(i ${CMAKE_SYSTEM_PREFIX_PATH})
	message(CHECK_START "Finding include directory of Slake: ${i}/slake/include")

	find_path(SLAKE_INCLUDE_DIRS NAMES "slake/runtime.h" HINTS "${i}/slake/include")

	if (SLAKE_INCLUDE_DIRS)
		message(CHECK_PASS "Found include directory of Slake: ${SLAKE_INCLUDE_DIRS}")
		break()
	endif()
endforeach()

foreach(i ${CMAKE_SYSTEM_PREFIX_PATH})
	message(CHECK_START "Finding library of Slake: ${i}/slake/include")

	find_library(SLAKE_LINK_LIBRARIES NAMES slake HINTS "${i}/slake/lib")

	if (SLAKE_LINK_LIBRARIES)
		message(CHECK_PASS "Found library of Slake: ${SLAKE_LINK_LIBRARIES}")
		break()
	endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    re2c
    REQUIRED_VARS SLAKE_INCLUDE_DIRS SLAKE_LINK_LIBRARIES)
