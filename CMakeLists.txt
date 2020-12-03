cmake_minimum_required(VERSION 3.10)
project(SAFmini)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread -std=c++0x")


add_executable(server main_win.cpp)
target_link_libraries(server -lws2_32)