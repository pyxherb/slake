find_package(Boost REQUIRED)

file(GLOB_RECURSE SRC *.h *.hpp *.hh *.c *.cpp *.cc)
target_include_directories(slkc PRIVATE ${Boost_INCLUDE_DIRS})
target_sources(slkc PRIVATE ${SRC})

if(WIN32)
    target_link_libraries(slkc PRIVATE ws2_32)
endif()
