#include "src/support.h"
#include "src/scheduler.h"
#include "src/coroutine.h"
#include "src/sort.h"

#include <stdio.h>
#include <time.h>
#include <memory.h>

int main(int argc, char** argv) {
  clock_t start_time = clock();
  if (argc < 2) {
    fprintf(stderr, "Usage: ./sort in_file1 [in_file2 ...]");
    return -1;
  }
  bool ok = true;
  for (int i = 1; i < argc; ++i) {
    if (!check_if_exists(argv[i])) {
      ok = false;
    }
  }
  if (!ok) {
    return -1;
  }
  int in_file_count = argc - 1;
  const char* out_filename = "result.txt";

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
  for (int i = 1; i < argc; ++i) {
    input_buffers[i - 2].size = 0;
    input_buffers[i - 2].buf = NULL;

    struct s_arg {
      const char* filename;
      buffer_t* buffer;
    }* var = malloc(sizeof(*var));
    var->buffer = &input_buffers[i - 1];
    var->filename = argv[i];
    ok = scheduler_add_task(coro_sort_file, var);
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
  ok = store_buffer_to_file(out_buffer, out_filename);

  if (!ok) {
    return -1;
  }
  clock_t end_time = clock();
  printf("Overall time: %lfus\n", ((double)end_time - start_time) * 1e6 / CLOCKS_PER_SEC);
  return 0;
}
