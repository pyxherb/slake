cmake_minimum_required(VERSION 3.13)
project(swampeak VERSION 0.1.0)

set(CXX_STANDARD 11)
include_directories(${PROJECT_SOURCE_DIR})

enable_testing()

add_subdirectory("swampeak")
add_subdirectory("spkc")
