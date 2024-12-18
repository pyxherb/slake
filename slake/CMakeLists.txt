file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)

set(SLAKE_BUILD_SHARED TRUE CACHE BOOL "Build shared library")

add_library(slake STATIC)
target_compile_definitions(slake PRIVATE SLAKE_IS_BUILDING=1)
set_property(TARGET slake PROPERTY CXX_STANDARD 17)
target_include_directories(slake PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
set_target_properties(slake PROPERTIES
    VERSION ${PROJECT_VERSION}
)

add_library(slaked SHARED)
target_compile_definitions(slaked PRIVATE SLAKE_IS_BUILDING=1)
target_compile_definitions(slaked PUBLIC SLAKE_BUILD_SHARED=1)
set_property(TARGET slaked PROPERTY CXX_STANDARD 17)
set_property(TARGET slaked PROPERTY PUBLIC_HEADER "")
target_include_directories(slaked PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/> $<INSTALL_INTERFACE:include/>)
set_target_properties(slaked PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    ARCHIVE FALSE
)

target_sources(slake PRIVATE ${HEADERS} ${SRC})

if(SLAKE_BUILD_SHARED)
    target_sources(slaked PRIVATE ${HEADERS} ${SRC})
endif()

add_subdirectory("util")
add_subdirectory("obj")
add_subdirectory("rt")
add_subdirectory("jit")
add_subdirectory("opti")

set(SLAKE_STACK_MAX 1048576 CACHE STRING "Maximum stack size for Slake runtime")
set(SLAKE_WITH_STDLIB TRUE CACHE BOOL "With Slake standard library")
set(SLAKE_ENABLE_DEBUGGER TRUE CACHE BOOL "Enable runtime debugger")

configure_file(
    config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/config.h
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/generated/config.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/slake/generated)

if(${SLAKE_ENABLE_DEBUGGER})
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dbg")
endif()

if(${SLAKE_WITH_STDLIB})
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/flib")
endif()

install(TARGETS slake slaked
    EXPORT slakeTargets
    LIBRARY DESTINATION lib/
    ARCHIVE DESTINATION lib/
    RUNTIME DESTINATION bin
)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/slake)
