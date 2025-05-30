cmake_minimum_required(VERSION 3.20)

project(coro-epoll-example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address -fno-omit-frame-pointer -O1")

add_executable(test echo_server.cpp io_context.cpp socket.cpp)