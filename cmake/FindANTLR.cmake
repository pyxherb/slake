#
# This file was originally adopted from ANTLR C++ runtime.
#
# Copyright (c) 2012-2022 The ANTLR Project. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither name of copyright holders nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED.	IN NO EVENT SHALL THE REGENTS OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# SPDX-License-Identifier: BSD-3-Clause
#
find_package(Java QUIET COMPONENTS Runtime)

if(NOT ANTLR_EXECUTABLE)
	find_program(ANTLR_EXECUTABLE
					NAMES antlr4
					HINTS /usr/bin/)
endif()

if(NOT ANTLR_EXECUTABLE)
	find_file(ANTLR_JAR
			NAMES antlr.jar antlr4.jar antlr-4.jar antlr-4.13.0-complete.jar
			HINTS /usr/share/java/
			REQUIRED)
	set(ANTLR_EXECUTABLE ${Java_JAVA_EXECUTABLE} -jar ${ANTLR_JAR} CACHE STRING "Command line for ANTLR executable" FORCE)
endif()

if(ANTLR_EXECUTABLE)
	execute_process(
			COMMAND ${ANTLR_EXECUTABLE}
			OUTPUT_VARIABLE ANTLR_COMMAND_OUTPUT
			ERROR_VARIABLE ANTLR_COMMAND_ERROR
			RESULT_VARIABLE ANTLR_COMMAND_RESULT
			OUTPUT_STRIP_TRAILING_WHITESPACE)

	if(ANTLR_COMMAND_RESULT EQUAL 0)
		string(REGEX MATCH "Version [0-9]+(\\.[0-9]+)*" ANTLR_VERSION ${ANTLR_COMMAND_OUTPUT})
		string(REPLACE "Version " "" ANTLR_VERSION ${ANTLR_VERSION})
	else()
		message(SEND_ERROR
				"Command '${ANTLR_EXECUTABLE}' "
				"failed with the output '${ANTLR_COMMAND_ERROR}'")
	endif()

	macro(ANTLR_TARGET Name InputFile)
		set(ANTLR_OPTIONS LEXER PARSER LISTENER VISITOR)
		set(ANTLR_ONE_VALUE_ARGS PACKAGE OUTPUT_DIRECTORY DEPENDS_ANTLR)
		set(ANTLR_MULTI_VALUE_ARGS COMPILE_FLAGS DEPENDS)
		cmake_parse_arguments(ANTLR_TARGET
							"${ANTLR_OPTIONS}"
							"${ANTLR_ONE_VALUE_ARGS}"
							"${ANTLR_MULTI_VALUE_ARGS}"
							${ARGN})

		set(ANTLR_${Name}_INPUT ${InputFile})

		get_filename_component(ANTLR_INPUT ${InputFile} NAME_WE)

		if(ANTLR_TARGET_OUTPUT_DIRECTORY)
			set(ANTLR_${Name}_OUTPUT_DIR ${ANTLR_TARGET_OUTPUT_DIRECTORY})
		else()
			set(ANTLR_${Name}_OUTPUT_DIR
				${CMAKE_CURRENT_BINARY_DIR}/antlr4cpp_generated_src/${ANTLR_INPUT})
		endif()

		unset(ANTLR_${Name}_CXX_OUTPUTS)

		if((ANTLR_TARGET_LEXER AND NOT ANTLR_TARGET_PARSER) OR
			 (ANTLR_TARGET_PARSER AND NOT ANTLR_TARGET_LEXER))
			list(APPEND ANTLR_${Name}_CXX_OUTPUTS
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}.h
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}.cpp)
			set(ANTLR_${Name}_OUTPUTS
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}.interp
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}.tokens)
		else()
			list(APPEND ANTLR_${Name}_CXX_OUTPUTS
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Lexer.h
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Lexer.cpp
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Parser.h
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Parser.cpp)
			list(APPEND ANTLR_${Name}_OUTPUTS
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Lexer.interp
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Lexer.tokens)
		endif()

		if(ANTLR_TARGET_LISTENER)
			list(APPEND ANTLR_${Name}_CXX_OUTPUTS
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}BaseListener.h
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}BaseListener.cpp
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Listener.h
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Listener.cpp)
			list(APPEND ANTLR_TARGET_COMPILE_FLAGS -listener)
		endif()

		if(ANTLR_TARGET_VISITOR)
			list(APPEND ANTLR_${Name}_CXX_OUTPUTS
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}BaseVisitor.h
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}BaseVisitor.cpp
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Visitor.h
				${ANTLR_${Name}_OUTPUT_DIR}/${ANTLR_INPUT}Visitor.cpp)
			list(APPEND ANTLR_TARGET_COMPILE_FLAGS -visitor)
		endif()

		if(ANTLR_TARGET_PACKAGE)
			list(APPEND ANTLR_TARGET_COMPILE_FLAGS -package ${ANTLR_TARGET_PACKAGE})
		endif()

		list(APPEND ANTLR_${Name}_OUTPUTS ${ANTLR_${Name}_CXX_OUTPUTS})

		if(ANTLR_TARGET_DEPENDS_ANTLR)
			if(ANTLR_${ANTLR_TARGET_DEPENDS_ANTLR}_INPUT)
				list(APPEND ANTLR_TARGET_DEPENDS
					${ANTLR_${ANTLR_TARGET_DEPENDS_ANTLR}_INPUT})
				list(APPEND ANTLR_TARGET_DEPENDS
					${ANTLR_${ANTLR_TARGET_DEPENDS_ANTLR}_OUTPUTS})
			else()
				message(SEND_ERROR "ANTLR target '${ANTLR_TARGET_DEPENDS_ANTLR}' not found")
			endif()
		endif()

		add_custom_command(
				OUTPUT ${ANTLR_${Name}_OUTPUTS}
				COMMAND ${ANTLR_EXECUTABLE}
				${InputFile}
				-o ${ANTLR_${Name}_OUTPUT_DIR}
				-no-listener
				-Dlanguage=Cpp
				${ANTLR_TARGET_COMPILE_FLAGS}
				DEPENDS ${InputFile}
				${ANTLR_TARGET_DEPENDS}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				COMMENT "Building ${Name} with ANTLR ${ANTLR_VERSION}")
	endmacro(ANTLR_TARGET)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
		ANTLR
		REQUIRED_VARS ANTLR_EXECUTABLE Java_JAVA_EXECUTABLE
		VERSION_VAR ANTLR_VERSION)

set(ANTLR4_RUNTIME_INCLUDE_DIRS_FOUND FALSE)
set(ANTLR4_RUNTIME_LIBPATH_FOUND FALSE)

foreach(i ${CMAKE_SYSTEM_PREFIX_PATH})
	if(ANTLR_INCLUDE_DIRS AND ANTLR_LIBPATH)
		break()
	endif()

	if (NOT ANTLR_INCLUDE_DIRS)
		message(CHECK_START "Finding ANTLR include directory: ${i}/LIBANTLR4/include")
		find_path(
			ANTLR_INCLUDE_DIRS
			NAMES antlr4-runtime/antlr4-runtime.h
			HINTS ${i}/LIBANTLR4/include)
		
		if (ANTLR_INCLUDE_DIRS)
			message(CHECK_PASS "Found ANTLR include directory: ${ANTLR_INCLUDE_DIRS}")
		endif()
	endif()
	
	if (NOT ANTLR_LIBPATH)
		message(CHECK_START "Finding ANTLR runtime library: ${i}/LIBANTLR4/lib")
		find_library(
			ANTLR_LIBPATH
			NAMES antlr4-runtime-static antlr4-runtime
			HINTS ${i}/LIBANTLR4/lib)

		if (ANTLR_LIBPATH)
			message(CHECK_PASS "Found ANTLR runtime library: ${ANTLR_LIBPATH}")
		endif()
	endif()
endforeach()

if(NOT ANTLR_INCLUDE_DIRS)
	message(FATAL_ERROR "Antlr4 include directory was not found")
endif()

if(NOT ANTLR_LIBPATH)
	message(FATAL_ERROR "Antlr4 runtime library was not found")
endif()

add_compile_definitions(ANTLR4CPP_STATIC)
set(ANTLR_LINK_LIBRARIES ${ANTLR_LIBPATH})
