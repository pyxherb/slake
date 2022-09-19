find_package(blare REQUIRED)
include_directories(AFTER ${blare_INCLUDE_DIR})

file(GLOB_RECURSE VZC_SRCS *.h *.cc)
add_executable(vzc ${VZC_SRCS})
