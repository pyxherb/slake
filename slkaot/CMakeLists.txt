add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_executable(slkaot)

target_link_libraries(slkaot PRIVATE slake)
set_property(TARGET slkaot PROPERTY CXX_STANDARD 17)

file(GLOB SRC *.h *.hh *.c *.cc)
target_sources(slkaot PRIVATE ${SRC})

add_subdirectory("bc2cxx")
