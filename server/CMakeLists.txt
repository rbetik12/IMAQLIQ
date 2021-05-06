cmake_minimum_required(VERSION 3.17)

add_executable(server Server.cpp Server.h)
target_link_libraries(server pthread)