file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)

target_sources(slkc PRIVATE ${HEADERS} ${SRC})

add_subdirectory("compiler")
