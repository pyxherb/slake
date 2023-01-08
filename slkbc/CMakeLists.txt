set(CXX_STANDARD 17)

include_directories(${FLEX_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# bison_target(slkbparse syntax/slkparse.yy ${CMAKE_CURRENT_BINARY_DIR}/slkparse.cc COMPILE_FLAGS)
# flex_target(slkblex syntax/slklex.ll ${CMAKE_CURRENT_BINARY_DIR}/slklex.cc COMPILE_FLAGS)

file(GLOB SRC *.h *.hh *.c *.cc)
add_executable(slkbc ${SRC} ${BISON_slkparse_OUTPUT_SOURCE} ${FLEX_slklex_OUTPUTS})
