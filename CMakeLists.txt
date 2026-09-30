cmake_minimum_required(VERSION 4.0)

project(custom-shell)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 17)

add_executable(custom-shell ${SOURCE_FILES})

target_link_libraries(custom-shell PRIVATE readline)
