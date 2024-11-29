file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)
target_sources(slake PRIVATE ${HEADERS} ${SRC})
if(SLAKE_BUILD_SHARED)
    target_sources(slaked PRIVATE ${HEADERS} ${SRC})
endif()

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/slake/jit/os/linux)
