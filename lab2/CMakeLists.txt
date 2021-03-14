cmake_minimum_required(VERSION 3.17)
project(lab2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
add_executable(Client WIN32 Client.cpp)
target_link_libraries(Client PUBLIC C:/Windows/system32/Ws2_32.dll C:/Windows/system32/Mswsock.dll C:/Windows/system32/advapi32.dll C:/Windows/system32/gdiplus.dll)

add_executable(Server Server.cpp)
target_link_libraries(Server PUBLIC C:/Windows/system32/Ws2_32.dll C:/Windows/system32/Mswsock.dll C:/Windows/system32/advapi32.dll C:/Windows/system32/gdiplus.dll)

target_link_options(Client PUBLIC -municode)
target_link_options(Server PUBLIC -municode)