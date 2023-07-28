cmake_minimum_required(VERSION 3.13)
project(slake VERSION 0.1.0)

include_directories(${PROJECT_SOURCE_DIR})

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(ENABLE_ADDRESS_SANITIZER TRUE CACHE BOOL "With AddressSanitizer")

    if(${ENABLE_ADDRESS_SANITIZER})
        message("AddressSanitizer enabled")

        if(CMAKE_COMPILER_IS_GNUCXX OR (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))
            message("AddressSanitizer mode: GCC/Clang")
            set(CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address")
        else()
            message(WARNING "No suitable AddressSanitizer mode for the compiler: ${CMAKE_CXX_COMPILER_ID}")
        endif()
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    enable_language(ASM_NASM)
endif()

add_subdirectory("slake")
add_subdirectory("slkbcc")
add_subdirectory("slkc")

add_subdirectory("example")
