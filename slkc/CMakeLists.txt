find_package(ANTLR REQUIRED)

include_directories(${ANTLR_INCLUDE_DIRS}/antlr4-runtime)

antlr_target(SlakeLexer SlakeLexer.g4 LEXER)
antlr_target(SlakeParser SlakeParser.g4 PARSER
            DEPENDS_ANTLR SlakeLexer
            COMPILE_FLAGS -visitor -no-listener -lib ${ANTLR_SlakeLexer_OUTPUT_DIR})

include_directories(${ANTLR_SlakeLexer_OUTPUT_DIR})
include_directories(${ANTLR_SlakeParser_OUTPUT_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SRC *.h *.hh *.c *.cc)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_executable(slkc
    ${SRC}
    ${ANTLR_SlakeLexer_CXX_OUTPUTS}
    ${ANTLR_SlakeParser_CXX_OUTPUTS})

set(SLKC_WITH_LSP_ENABLED TRUE CACHE BOOL "Enable built-in LSP server of SLKC")
configure_file("config.h.in" ${CMAKE_CURRENT_BINARY_DIR}/config.h)

target_link_libraries(slkc PRIVATE ${ANTLR_LINK_LIBRARIES} slake)
set_property(TARGET slkc PROPERTY CXX_STANDARD 17)

add_subdirectory("compiler")
add_subdirectory("decompiler")

if(SLKC_WITH_LSP_ENABLED)
    add_subdirectory("lsp")
endif()

#add_subdirectory("decompiler")
