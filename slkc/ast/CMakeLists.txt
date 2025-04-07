find_package(re2c REQUIRED)

add_re2c_target(slkc_lexer lexer.in.cpp)

file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)

target_sources(slkc PRIVATE ${HEADERS} ${SRC})
target_link_libraries(slkc PRIVATE slkc_lexer)

add_subdirectory("parser")
