file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)
target_sources(slake PRIVATE ${HEADERS} ${SRC})
if(SLAKE_BUILD_SHARED)
    target_sources(slaked PRIVATE ${HEADERS} ${SRC})
endif()

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/slake/dbg)

if(WIN32)
    target_link_libraries(slake PUBLIC WSock32 WS2_32)
    target_link_libraries(slaked PUBLIC WSock32 WS2_32)
endif()
