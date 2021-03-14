cmake_minimum_required(VERSION 3.17)
project(lab1)

set(CMAKE_CXX_STANDARD 14)

add_executable(lab1 main.cpp)
#target_link_options(lab1 PUBLIC -municode)
target_link_libraries(lab1 PUBLIC C:/Windows/System32/netapi32.dll C:/Windows/System32/mpr.dll)