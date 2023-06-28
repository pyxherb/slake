file(GLOB SRC *.h *.hh *.c *.cc)
add_library(slake ${SRC} "lib/core.h")

set_property(TARGET slake PROPERTY CXX_STANDARD 17)

add_subdirectory("util")
add_subdirectory("rt")
add_subdirectory("jit")
add_subdirectory("lib")
add_subdirectory("dbg")
add_subdirectory("valdef")

set(SLAKE_STACK_MAX 1048576 CACHE STRING "Maximum stack size for Slake runtime")
set(SLAKE_ENABLE_DEBUGGER TRUE CACHE BOOL "Enable runtime debugger")

configure_file(
    config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/config.h
)
