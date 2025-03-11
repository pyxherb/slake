file(GLOB SRC *.h *.hh *.c *.cc)
target_sources(slkaot PRIVATE ${SRC})

add_subdirectory("compiler")
