cmake_minimum_required(VERSION 3.13)
project(slake VERSION 0.1.0)

include_directories(${PROJECT_SOURCE_DIR})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

add_subdirectory("slake")
add_subdirectory("slkbcc")
add_subdirectory("slkc")

add_subdirectory("example")
