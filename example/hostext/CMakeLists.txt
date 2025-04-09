add_executable(hostext "main.cc")
target_link_libraries(hostext slake)

set_property(TARGET hostext PROPERTY CXX_STANDARD 17)
