find_package(re2c REQUIRED)

file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)

target_sources(slkc PRIVATE ${HEADERS} ${SRC})

add_subdirectory("parser")
