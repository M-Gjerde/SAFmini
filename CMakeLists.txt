cmake_minimum_required(VERSION 3.17)
project(SAFmini)

set(CMAKE_CXX_STANDARD 17)

add_executable(server main.cpp)
target_link_libraries(server /usr/local/lib/libpugixml.a)

add_executable(client client.cpp)