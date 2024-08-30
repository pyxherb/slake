file(GLOB HEADERS *.h *.hh)
file(GLOB SRC *.c *.cc)
target_sources(slake PUBLIC ${HEADERS} ${SRC})

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/slake/jit)
