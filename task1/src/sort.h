//
// Created by dgolear on 24.03.2021.
//

#ifndef TASK1_SORT_H
#define TASK1_SORT_H

#include "support.h"
#include "coroutine.h"

// Expects buf to be allocated. Sorts it in ascending order.
void sort_buffer(buffer_t buf);

// Checks if out_buf is allocated. If yes, and if there is enough memory to hold all input_buffers, then fits data in it.
// Otherwise, frees out_buf memory and allocates new chunk of memory.
// Returns false in case of any error.
bool merge_sorted_buffers(buffer_t* in_buffers, int in_buf_count, buffer_t* out_buf);

#endif //TASK1_SORT_H
