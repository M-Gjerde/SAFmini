cmake_minimum_required(VERSION 3.17)
project(SAFmini)

set(CMAKE_CXX_STANDARD 17)

add_executable(SAFServer main.cpp)
target_link_libraries(SAFServer /usr/local/lib/libpugixml.a)

add_executable(SAFClient client.cpp)