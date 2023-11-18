file(GLOB SRC *.h *.hh *.c *.cc)
target_sources(slake PUBLIC ${SRC})

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    add_subdirectory("x86_64")
endif()
