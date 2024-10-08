file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)
target_sources(slake PRIVATE ${HEADERS} ${SRC})

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/slake/dbg)

if(WIN32)
    target_link_libraries(slake PUBLIC WSock32 WS2_32)
endif()
