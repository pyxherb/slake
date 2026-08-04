add_executable(bctest "main.cc")
target_link_libraries(bctest slake)

set_property(TARGET bctest PROPERTY CXX_STANDARD 17)
