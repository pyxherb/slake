find_package(httplib REQUIRED)
find_package(jsoncpp REQUIRED)

file(GLOB_RECURSE SRC *.h *.hh *.c *.cc)
target_sources(slkc PRIVATE ${SRC})
target_link_libraries(slkc PUBLIC httplib::httplib jsoncpp_static)
