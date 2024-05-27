add_executable(rbtree "main.cc")
target_link_libraries(rbtree slake)

set_property(TARGET rbtree PROPERTY CXX_STANDARD 17)
