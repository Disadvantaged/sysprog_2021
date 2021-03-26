#include "src/support.h"
#include "src/scheduler.h"
#include "src/coroutine.h"
#include "src/sort.h"

#include <stdio.h>
#include <time.h>

int main(int argc, char** argv) {
  clock_t start_time = clock();
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

  ok = scheduler_initialize(in_file_count);
  if (!ok) {
    return -1;
  }

  buffer_t* input_buffers = reallocarray(NULL, in_file_count, sizeof(buffer_t));
  if (!input_buffers) {
    perror("couldn't allocate buffers.");
    return -1;
  }
  buffer_t out_buffer = {.size =  0, .buf =  NULL};
  for (int i = 2; i < argc; ++i) {
    input_buffers[i - 2].size = 0;
    input_buffers[i - 2].buf = NULL;

    struct {
      const char* filename;
      buffer_t* buffer;
    } var = {.filename = argv[i], .buffer = &input_buffers[i - 2]};
    ok = scheduler_add_task(coro_sort_file, &var);
    if (!ok) {
      return -1;
    }
  }

  ok = scheduler_run_loop();
  if (!ok) {
    return -1;
  }
  scheduler_destroy();

  ok = merge_sorted_buffers(input_buffers, in_file_count, &out_buffer);
  if (!ok) {
    return -1;
  }
  ok = store_buffer_to_file(out_buffer, argv[1]);

  if (!ok) {
    return -1;
  }
  clock_t end_time = clock();
  printf("Overall time: %lfms\n", ((double)end_time - start_time) * 1e3 / CLOCKS_PER_SEC);
  return 0;
}
