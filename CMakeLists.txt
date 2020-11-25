cmake_minimum_required(VERSION 3.17)
project(SAFmini)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_subdirectory(csv-parser)

message(" csv: ${csv-parser} : csv")

add_executable(server main.cpp)
target_link_libraries(server /usr/local/lib/libpugixml.a csv)

add_executable(client client.cpp)