find_package(peff REQUIRED)
find_package(re2c REQUIRED)

if(WIN32)
    find_package(protobuf CONFIG)
    set(utf8_range_DIR "${protobuf_DIR}/../utf8_range")
    #message(FATAL_ERROR "${protobuf_DIR}/../utf8_range")
    find_package(utf8_range CONFIG REQUIRED)
    find_package(protobuf CONFIG REQUIRED)
else()
    find_package(protobuf CONFIG REQUIRED)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SRC *.h *.hh *.c *.cc)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS SLKC_IS_BUILDING=1)
add_executable(slkc ${SRC} "proto/sls.proto")

target_link_libraries(slkc PRIVATE slake peff_advutils_static protobuf::libprotobuf)
set_property(TARGET slkc PROPERTY CXX_STANDARD 17)

set(SLKC_WITH_LANGUAGE_SERVER TRUE CACHE BOOL "Build SLKC with language server support")

configure_file("config.h.in" ${CMAKE_CURRENT_BINARY_DIR}/config.h)

protobuf_generate(
    TARGET slkc
)

add_subdirectory("ast")
add_subdirectory("comp")
add_subdirectory("decomp")

if(SLKC_WITH_LANGUAGE_SERVER)
    add_subdirectory("server")
endif()
