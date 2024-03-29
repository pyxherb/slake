foreach(i ${CMAKE_SYSTEM_PREFIX_PATH})
	if(RE2C_EXECUTABLE)
		break()
	endif()

	message(CHECK_START "Finding re2c: ${i}/re2c/bin")
	find_program(RE2C_EXECUTABLE re2c NAMES re2c HINTS ${i}/re2c/bin)
	if (RE2C_EXECUTABLE)
		message(CHECK_PASS "Found re2c: ${RE2C_EXECUTABLE}")
	endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    re2c
    REQUIRED_VARS RE2C_EXECUTABLE)

if(RE2C_EXECUTABLE)
    macro(add_re2c_target Name Re2cInput Re2cOutput)
        add_custom_command(
            OUTPUT ${Re2cOutput}
            DEPENDS ${Re2cInput}
            COMMAND ${RE2C_EXECUTABLE} --no-debug-info -c ${Re2cInput} -o ${Re2cOutput}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endmacro()
endif()
