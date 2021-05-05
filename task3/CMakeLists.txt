cmake_minimum_required(VERSION 3.16)
project(sysprog C)

set(CMAKE_C_STANDARD 11)

add_executable(test test.c userfs.c)
