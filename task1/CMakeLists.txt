cmake_minimum_required(VERSION 3.16)
project(task1 C)

set(CMAKE_C_STANDARD 11)

SET(SOURCES
        ${PROJECT_SOURCE_DIR}/src/support.c
        ${PROJECT_SOURCE_DIR}/src/scheduler.c
        ${PROJECT_SOURCE_DIR}/src/sort.c
        ${PROJECT_SOURCE_DIR}/src/coroutine.c
        )

add_executable(sort ${SOURCES} ${PROJECT_SOURCE_DIR}/main.c)

target_link_libraries(sort rt)

target_include_directories(sort PRIVATE ${PROJECT_SOURCE_DIR}/src)