include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB_RECURSE SRC *.h *.hh *.c *.cc)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_executable(slkaot ${SRC})

target_link_libraries(slkaot PRIVATE slake)
set_property(TARGET slkaot PROPERTY CXX_STANDARD 17)
