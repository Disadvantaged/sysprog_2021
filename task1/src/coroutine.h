//
// Created by dgolear on 24.03.2021.
//

#ifndef TASK1_COROUTINE_H
#define TASK1_COROUTINE_H

#include "support.h"
#include "sort.h"
#include "scheduler.h"

// Asynchronous read from file.
// Needs to be run inside the coroutine.
// Blocks the coroutine and switches execution to the scheduler.
bool read_buffer_async(const char* file, buffer_t* buffer);

void coro_sort_file(void* ctx);

#endif //TASK1_COROUTINE_H
