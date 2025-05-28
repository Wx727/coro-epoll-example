cmake_minimum_required(VERSION 3.20)

project(coro-epoll-example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test main.cpp)