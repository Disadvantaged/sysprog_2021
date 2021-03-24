//
// Created by dgolear on 24.03.2021.
//

#include "coroutine.h"

void coro_sort_file(void *ctx) {
  struct {
    const char* filename;
    buffer_t* buffer;
  }* var = ctx;

  bool ok = read_buffer_sync(var->filename, var->buffer);

  scheduler_coro_yield();

  if (!ok) {
    scheduler_coro_fail();
    return;
  }

  sort_buffer(*var->buffer);
}
