find_package(ANTLR REQUIRED)

include_directories(${ANTLR_INCLUDE_DIRS}/antlr4-runtime)

antlr_target(SlakeLexer SlakeLexer.g4 LEXER)
antlr_target(SlakeParser SlakeParser.g4 PARSER
            DEPENDS_ANTLR SlakeLexer
            COMPILE_FLAGS -visitor -no-listener -lib ${ANTLR_SlakeLexer_OUTPUT_DIR})

include_directories(${ANTLR_SlakeLexer_OUTPUT_DIR})
include_directories(${ANTLR_SlakeParser_OUTPUT_DIR})

file(GLOB SRC *.h *.hh *.c *.cc)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_executable(slkc
    ${SRC}
    ${ANTLR_SlakeLexer_CXX_OUTPUTS}
    ${ANTLR_SlakeParser_CXX_OUTPUTS})

target_link_libraries(slkc PRIVATE ${ANTLR_LINK_LIBRARIES} slake)
set_property(TARGET slkc PROPERTY CXX_STANDARD 17)

add_subdirectory("compiler")
add_subdirectory("decompiler")
add_subdirectory("lsp")

#add_subdirectory("decompiler")
