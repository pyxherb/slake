set(CXX_STANDARD 17)

include_directories(${FLEX_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

bison_target(slkparse syntax/slkparse.yy ${CMAKE_CURRENT_BINARY_DIR}/slkparse.cc COMPILE_FLAGS)
flex_target(slklex syntax/slklex.ll ${CMAKE_CURRENT_BINARY_DIR}/slklex.cc COMPILE_FLAGS)

file(GLOB SRC *.h *.hh *.c *.cc)
add_executable(slkc ${SRC} ${BISON_slkparse_OUTPUT_SOURCE} ${FLEX_slklex_OUTPUTS})

add_subdirectory("compiler")

set(VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
set(VS_DEBUGGER_COMMAND_ARGUMENTS "tests/example.slk")

add_test(NAME slkc COMMAND slkc "tests/example.slk")
set_tests_properties(slkc PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
