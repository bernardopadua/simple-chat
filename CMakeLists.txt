cmake_minimum_required(VERSION 3.15)
project(SimpleChatnSever CXX)

set(CMAKE_CXX_STANDARD 17)

# Include
include_directories(include)

# LibDEPS
set(LIB_DEPS ws2_32)

add_subdirectory(simple-chat)
add_subdirectory(simple-chat-server)