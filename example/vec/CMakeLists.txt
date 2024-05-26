add_executable(vec "main.cc")
target_link_libraries(vec slake)

set_property(TARGET vec PROPERTY CXX_STANDARD 17)
