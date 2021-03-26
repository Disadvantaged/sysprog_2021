//
// Created by dgolear on 24.03.2021.
//

#include "coroutine.h"
#include "scheduler_internal.h"

bool read_buffer_async(const char *file, buffer_t *buffer) {
  char* bytes = NULL;
  if (!scheduler_coro_read_file(file, &bytes)) {
    return false;
  }

  bool status = bytes_to_buffer(bytes, buffer);
  free(bytes);
  return status;
}

void coro_sort_file(void *ctx) {
  struct {
    const char* filename;
    buffer_t* buffer;
  }* var = ctx;

  bool ok = read_buffer_async(var->filename, var->buffer);

  if (!ok) {
    scheduler_coro_fail();
    return;
  }

  sort_buffer(*var->buffer);
}
