if(WIN32)
	foreach(i ${CMAKE_SYSTEM_PREFIX_PATH})
		message(CHECK_START "Finding re2c: ${i}/re2c/bin")

		find_program(RE2C_EXECUTABLE re2c NAMES re2c HINTS ${i}/re2c/bin)

		if (RE2C_EXECUTABLE)
			message(CHECK_PASS "Found re2c: ${RE2C_EXECUTABLE}")
			break()
		endif()
	endforeach()
else()
	message(CHECK_START "Finding re2c...")

	find_program(RE2C_EXECUTABLE re2c NAMES re2c)

	if (RE2C_EXECUTABLE)
		message(CHECK_PASS "Found re2c: ${RE2C_EXECUTABLE}")
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    re2c
    REQUIRED_VARS RE2C_EXECUTABLE)

if(RE2C_EXECUTABLE)
    macro(add_re2c_target Name Re2cInput)
		string(RANDOM OutputFileName)
        add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/re2c_${OutputFileName}.cc
            COMMAND ${RE2C_EXECUTABLE} -c ${Re2cInput} -o ${CMAKE_CURRENT_BINARY_DIR}/re2c_${OutputFileName}.cc
            DEPENDS ${Re2cInput}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
		add_library(
			${Name}
			INTERFACE
		)
		target_sources(${Name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/re2c_${OutputFileName}.cc)
    endmacro()
endif()
