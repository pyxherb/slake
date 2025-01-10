find_package(re2c REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SRC *.h *.hh *.c *.cc)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_executable(slkc ${SRC})

target_link_libraries(slkc PRIVATE slake)
set_property(TARGET slkc PROPERTY CXX_STANDARD 17)

set(SLKC_WITH_LANGUAGE_SERVER TRUE CACHE BOOL "Build SLKC with language server support")

configure_file("config.h.in" ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory("compiler")
add_subdirectory("decompiler")

if(SLKC_WITH_LANGUAGE_SERVER)
    add_subdirectory("server")
endif()
