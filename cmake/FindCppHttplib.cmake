#
# Copyright (c) 2024 Slake Project. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
foreach(i ${CMAKE_SYSTEM_PREFIX_PATH})
	if(CppHttplib_INCLUDE_DIRS AND CppHttplib_LINK_LIBRARIES)
		break()
	endif()

	if (NOT CppHttplib_INCLUDE_DIRS)
		message(CHECK_START "Finding CppHttplib include directory: ${i}/LIBCppHttplib4/include")
		find_path(
			CppHttplib_INCLUDE_DIRS
			NAMES httplib.h
			HINTS ${i}/LIBCppHttplib4/include)

		if (CppHttplib_INCLUDE_DIRS)
			message(CHECK_PASS "Found CppHttplib include directory: ${CppHttplib_INCLUDE_DIRS}")
		endif()
	endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	CppHttplib
	REQUIRED_VARS CppHttplib_INCLUDE_DIRS)

