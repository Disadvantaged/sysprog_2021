cmake_minimum_required(VERSION 3.16)
project(sysprog C)

set(CMAKE_C_STANDARD 11)

add_executable(Cshell # Hope, you noticed the pun:D
        src/shell.c
        src/parser.c
        src/signal_handlers.c
        src/support.c
        src/main.c)

target_include_directories(Cshell PUBLIC include)
