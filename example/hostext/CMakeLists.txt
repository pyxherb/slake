add_executable(hostext "main.cc")
add_dependencies(hostext slake)
target_link_libraries(hostext slaked)

set_property(TARGET hostext PROPERTY CXX_STANDARD 17)
