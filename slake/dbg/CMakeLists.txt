file(GLOB SRC *.h *.hh *.c *.cc)
target_sources(slake PUBLIC ${SRC})

if(WIN32)
    target_link_libraries(slake PUBLIC WSock32 WS2_32)
endif()
