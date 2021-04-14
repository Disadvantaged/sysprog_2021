cmake_minimum_required(VERSION 3.16)
project(task1 C)

set(CMAKE_C_STANDARD 11)

SET(SOURCES
        src/support.c
        src/scheduler.c
        src/sort.c
        src/coroutine.c
        )

add_executable(sort main.c ${SOURCES})

target_link_libraries(sort rt)