file(GLOB_RECURSE SRC *.h *.hh *.c *.cc)
add_library(slake ${SRC})

set_property(TARGET slake PROPERTY CXX_STANDARD 17)
