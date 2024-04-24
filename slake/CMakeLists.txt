file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)
add_library(slake ${HEADERS} ${SRC})

set_property(TARGET slake PROPERTY CXX_STANDARD 17)

add_subdirectory("util")
add_subdirectory("rt")
add_subdirectory("jit")
add_subdirectory("lib")
add_subdirectory("valdef")

set(SLAKE_STACK_MAX 1048576 CACHE STRING "Maximum stack size for Slake runtime")
set(SLAKE_WITH_STDLIB TRUE CACHE BOOL "With Slake standard library")
set(SLAKE_ENABLE_DEBUGGER TRUE CACHE BOOL "Enable runtime debugger")
set(SLAKE_WITH_STRICT_MODE TRUE CACHE BOOL "Enable strict mode")

configure_file(
    config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/config.h
)

if(${SLAKE_ENABLE_DEBUGGER})
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dbg")
endif()

install(TARGETS slake DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/slake)
