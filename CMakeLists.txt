cmake_minimum_required(VERSION 3.21)
project(HW4 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(HW4 main.c)
