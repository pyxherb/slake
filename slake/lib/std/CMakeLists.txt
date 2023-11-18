file(GLOB SRC *.h *.hh *.c *.cc)
target_sources(slake PUBLIC ${SRC})

add_subdirectory("math")
add_subdirectory("util")
