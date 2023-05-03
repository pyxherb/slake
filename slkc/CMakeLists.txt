find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

include_directories(${FLEX_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

bison_target(
    slkparse
    syntax/slkparse.yy ${CMAKE_CURRENT_BINARY_DIR}/slkparse.cc
    COMPILE_FLAGS)
flex_target(
    slklex
    syntax/slklex.ll ${CMAKE_CURRENT_BINARY_DIR}/slklex.cc
    COMPILE_FLAGS
    "--header-file=${CMAKE_CURRENT_BINARY_DIR}/slklex.h --nounistd")

file(GLOB SRC *.h *.hh *.c *.cc)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_executable(slkc ${SRC} ${BISON_slkparse_OUTPUT_SOURCE} ${FLEX_slklex_OUTPUTS})

add_subdirectory("compiler")
add_subdirectory("lsp")
if(WIN32)
    target_link_libraries(slkc PRIVATE ws2_32)
endif()
add_subdirectory("decompiler")
