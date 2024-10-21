file(GLOB_RECURSE SRC *.h *.hh *.c *.cc)

add_re2c_target(slakeLexer ast/lexer.in.cpp ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)

add_dependencies(slkc slakeLexer)
target_sources(slkc PRIVATE ${SRC} ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
