file(GLOB SRC *.h *.hh *.c *.cc)
target_sources(slake PRIVATE ${SRC})
if(SLAKE_BUILD_SHARED)
    target_sources(slaked PRIVATE ${SRC})
endif()

if((${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64") OR
    (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64"))
    add_subdirectory("x86-64")
endif()
