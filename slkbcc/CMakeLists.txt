find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

bison_target(
    bcparse
    syntax/bcparse.yy ${CMAKE_CURRENT_BINARY_DIR}/bcparse.cc
    COMPILE_FLAGS)
flex_target(
    bclex
    syntax/bclex.ll ${CMAKE_CURRENT_BINARY_DIR}/bclex.cc
    COMPILE_FLAGS
    "--header-file=${CMAKE_CURRENT_BINARY_DIR}/bclex.h --nounistd")

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

link_libraries(slake)

file(GLOB SRC *.h *.hh *.c *.cc)
add_executable(slkbcc ${SRC} ${BISON_bcparse_OUTPUT_SOURCE} ${FLEX_bclex_OUTPUTS})

add_subdirectory("ast")
add_subdirectory("compile")

set_property(TARGET slkbcc PROPERTY CXX_STANDARD 17)
