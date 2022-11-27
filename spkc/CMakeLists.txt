include_directories(${FLEX_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

bison_target(spkparse syntax/spkparse.yy ${CMAKE_CURRENT_BINARY_DIR}/spkparse.cc COMPILE_FLAGS)
flex_target(spklex syntax/spklex.ll ${CMAKE_CURRENT_BINARY_DIR}/spklex.cc COMPILE_FLAGS)

file(GLOB SRC *.h *.hh *.c *.cc)
add_executable(spkc ${SRC} ${BISON_spkparse_OUTPUT_SOURCE} ${FLEX_spklex_OUTPUTS})

add_subdirectory("compiler")

set(VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
set(VS_DEBUGGER_COMMAND_ARGUMENTS "tests/example.spk")

add_test(NAME spkc COMMAND spkc "tests/example.spk")
set_tests_properties(spkc PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
