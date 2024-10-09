file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)

set(SLAKE_BUILD_SHARED TRUE CACHE BOOL "Build shared library")

add_library(slake STATIC)
target_compile_definitions(slake PRIVATE SLAKE_IS_BUILDING=1)
set_property(TARGET slake PROPERTY CXX_STANDARD 17)

add_library(slaked SHARED)
target_compile_definitions(slaked PRIVATE SLAKE_IS_BUILDING=1)
target_compile_definitions(slaked PUBLIC SLAKE_BUILD_SHARED=1)
set_property(TARGET slaked PROPERTY CXX_STANDARD 17)

target_sources(slake PRIVATE ${HEADERS} ${SRC})
if(SLAKE_BUILD_SHARED)
    target_sources(slaked PRIVATE ${HEADERS} ${SRC})
endif()

add_subdirectory("util")
add_subdirectory("valdef")
add_subdirectory("rt")
add_subdirectory("jit")
add_subdirectory("opti")

set(SLAKE_STACK_MAX 1048576 CACHE STRING "Maximum stack size for Slake runtime")
set(SLAKE_WITH_STDLIB TRUE CACHE BOOL "With Slake standard library")
set(SLAKE_ENABLE_DEBUGGER TRUE CACHE BOOL "Enable runtime debugger")

configure_file(
    config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/config.h
)

if(${SLAKE_ENABLE_DEBUGGER})
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dbg")
endif()

#if(${SLAKE_WITH_STDLIB})
#    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lib")
#endif()

install(TARGETS slake DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/slake)
