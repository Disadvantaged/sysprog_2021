#include "src/support.h"
#include "src/scheduler.h"
#include "src/coroutine.h"
#include "src/sort.h"

#include <stdio.h>

static void free_bufs(buffer_t* bufs, int bufs_count, buffer_t buf) {
  for (int i = 0; i < bufs_count; ++i) {
    free(bufs[i].buf);
  }
  free(bufs);
  free(buf.buf);
}

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: ./sort out_file1 in_file1 [in_file2 ...]");
    return -1;
  }
  bool ok = true;
  for (int i = 2; i < argc; ++i) {
    if (!check_if_exists(argv[i])) {
      ok = false;
    }
  }
  if (!ok) {
    return -1;
  }
  int in_file_count = argc - 2;

//  ok = scheduler_initialize(in_file_count);
//  if (!ok) {
//    return -1;
//  }

  buffer_t* input_buffers = reallocarray(NULL, in_file_count, sizeof(buffer_t));
  if (!input_buffers) {
    perror("couldn't allocate buffers.");
    return -1;
  }
  buffer_t out_buffer = {.size =  0, .buf =  NULL};
  for (int i = 2; i < argc; ++i) {
    input_buffers[i - 2].size = 0;
    input_buffers[i - 2].buf = NULL;

    ok = read_buffer_sync(argv[i], &input_buffers[i - 2]);
    for (int j = 0; j < input_buffers[i - 2].size; ++j) {
      printf("%d ", input_buffers[i - 2].buf[j]);
    }
    printf("\n");
    if (!ok) {
      free_bufs(input_buffers, in_file_count, out_buffer);
      return -1;
    }
    sort_buffer(input_buffers[i - 2]);
//    scheduler_add_task(CORO_SORT_FILE, argv[i]);
  }


//  ok = scheduler_run_loop();
//  if (!ok) {
//    return -1;
//  }

  merge_sorted_buffers(input_buffers, in_file_count, &out_buffer);
  ok = store_buffer_to_file(out_buffer, argv[1]);

  free_bufs(input_buffers, in_file_count, out_buffer);
  if (!ok) {
    return -1;
  }
  return 0;
}
