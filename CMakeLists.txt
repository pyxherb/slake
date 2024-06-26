cmake_minimum_required(VERSION 3.13)
project(slake VERSION 0.1.0)

include_directories(${PROJECT_SOURCE_DIR})

#
# Check for AddressSanitizer if user has chosen to enable it for debugging.
#
if(CMAKE_BUILD_TYPE)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(ENABLE_ADDRESS_SANITIZER TRUE CACHE BOOL "With AddressSanitizer")

        if(${ENABLE_ADDRESS_SANITIZER})
            message(CHECK_START "Finding AddressSanitizer")

            if(CMAKE_COMPILER_IS_GNUCXX OR (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))
                # GCC/Clang
                message(CHECK_PASS "AddressSanitizer mode: GCC/Clang")
                set(CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address")
            elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
                message(CHECK_FAIL "You should enable AddressSanitizer manually in CMakeSettings.json")
            else()
                # Incompatible compiler
                message(CHECK_FAIL "No suitable AddressSanitizer mode for the compiler: \"${CMAKE_CXX_COMPILER_ID}\"")
            endif()
        endif()
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    enable_language(ASM_NASM)
endif()

set(SLAKE_BUILD_SLKC TRUE CACHE BOOL "Build SLKC")
set(SLAKE_BUILD_SLKBCC TRUE CACHE BOOL "Build SLKBCC")

add_subdirectory("slake")

if(SLAKE_BUILD_SLKC)
    add_subdirectory("slkc")
endif()

add_subdirectory("example")

install(FILES "cmake/slakeConfig.cmake" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/slake)
