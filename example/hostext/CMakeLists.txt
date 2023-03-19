add_executable(hostext "main.cc")
add_dependencies(hostext slake)
target_link_libraries(hostext slake)
