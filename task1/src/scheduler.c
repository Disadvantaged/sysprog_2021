//
// Created by dgolear on 24.03.2021.
//

#include "scheduler.h"
#include "scheduler_internal.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <memory.h>
#include <aio.h>
#include <errno.h>

static struct scheduler_ctx_s scheduler_context;

bool scheduler_add_task(void (*coroutine)(void *), void *ctx) {
  entity_t* new_entity = malloc(sizeof(entity_t));
  if (!new_entity) {
    perror("Couldn't allocate coro_func entity: ");
    return false;
  }
  new_entity->status = Starting;

  void* stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (stack == MAP_FAILED) {
    free(new_entity);
    perror("Couldn't allocate stack: ");
    return false;
  }

  if (getcontext(&new_entity->ctx) == -1) {
    munmap(stack, stack_size);
    free(new_entity);
    perror("Couldn't get context for the coroutine");
    return false;
  }
  new_entity->ctx.uc_stack.ss_sp = stack;
  new_entity->ctx.uc_stack.ss_size = stack_size;
  new_entity->ctx.uc_link = &scheduler_context.scheduler_ctx;
  new_entity->idx = scheduler_context.last_coro_idx++;
  new_entity->running_time = 0;

  makecontext(&new_entity->ctx, (void (*)(void)) coroutine, 1, ctx);

  STAILQ_INSERT_TAIL(&scheduler_context.run_queue, new_entity, entities);

  return true;
}

bool scheduler_initialize(int max_coro_count) {
  STAILQ_INIT(&scheduler_context.run_queue);
  scheduler_context.suspended_coro_count = 0;
  scheduler_context.last_coro_idx = 0;
  scheduler_context.max_coro_count = max_coro_count;

  scheduler_context.suspend_lst = NULL;
  scheduler_context.suspend_lst = reallocarray(scheduler_context.suspend_lst, max_coro_count, sizeof(struct aiocb*));
  if (!scheduler_context.suspend_lst) {
    perror("Couldn't allocate suspend list.");
    return false;
  }
  memset(scheduler_context.suspend_lst, 0, sizeof(struct aiocb*) * max_coro_count);

  return true;
}

void scheduler_destroy() {
  free(scheduler_context.suspend_lst);
}

static void scheduler_submit_task_and_suspend(struct aiocb* ctx) {
  for (int i = 0; i < scheduler_context.max_coro_count; ++i) {
    if (scheduler_context.suspend_lst[i] == NULL) {
      scheduler_context.suspend_lst[i] = ctx;
      break;
    }
  }
  scheduler_coro_suspend();
}

bool scheduler_coro_read_file(const char *file, char **ptr_to_bytes) {
  ssize_t file_size = get_file_size(file);

  if (file_size == -1) {
    return false;
  }

  char* bytes = malloc(file_size + sizeof(entity_t*));
  if (!bytes) {
    perror("Couldn't allocate memory: ");
    return false;
  }
  memset(bytes, 0, file_size);

  int fd = open(file, O_RDONLY);
  if (fd == -1) {
    perror("Couldn't open a file: ");
    return false;
  }

  struct aiocb ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.aio_nbytes = file_size;
  ctx.aio_fildes = fd;
  ctx.aio_buf = bytes + sizeof(entity_t*);
  // Some nasty things, because making proper work with aio and aio_suspend inside the scheduler would take a lot more time.
  *((entity_t **)bytes) = scheduler_context.current_coro;

  aio_read(&ctx);

  scheduler_submit_task_and_suspend(&ctx);

  int status = aio_return(&ctx);
  if (status == -1) {
    perror("Read error: ");
    scheduler_coro_fail();
  }

  // Shift array back.
  memmove(bytes, bytes + sizeof(entity_t*), file_size);
  bytes[file_size - 1] = 0;
  *ptr_to_bytes = bytes;
  return true;
}

void scheduler_coro_enqueue(entity_t* entity) {
  STAILQ_INSERT_TAIL(&scheduler_context.run_queue, entity, entities);
}

static bool scheduler_wait_any() {
  int status = aio_suspend(scheduler_context.suspend_lst, scheduler_context.max_coro_count, NULL);
  if (status == -1) {
    perror("aio_suspend: ");
    return false;
  }

  for (int i = 0; i < scheduler_context.max_coro_count; ++i) {
    if (!scheduler_context.suspend_lst[i]) {
      continue;
    }
    int result = aio_error(scheduler_context.suspend_lst[i]);
    if (result == EINPROGRESS) {
      // Continue waiting for it.
    } else if (result == ECANCELED) {
      return false;
    } else {
      // Either it ended successfully reading from buffer, or an error happened. Let the coroutine decide, what to do with it.
      entity_t** lst = (entity_t **) scheduler_context.suspend_lst[i]->aio_buf;
      entity_t* entity = lst[-1];
      scheduler_coro_enqueue(entity);
      scheduler_context.suspend_lst[i] = NULL;
    }
  }
  return true;
}

static bool get_runnable_entity(entity_t** ptr_to_entity) {
  entity_t* entity = STAILQ_FIRST(&scheduler_context.run_queue);
  if (entity != NULL) {
    STAILQ_REMOVE_HEAD(&scheduler_context.run_queue, entities);
  } else {
    if (scheduler_context.suspended_coro_count) {
      if (!scheduler_wait_any() || !get_runnable_entity(&entity)) {
        return false;
      }
    }
  }
  *ptr_to_entity = entity;
  return true;
}

static bool check_state(entity_t* entity) {
  if (entity->status == Runnable) {
    // The coroutine made a Yield() call. Add it to the end of the run queue.
    scheduler_coro_enqueue(entity);
  } else if (entity->status == Running) {
    // The coroutine has finished executing. Remove it and delete all of its data.

    printf("Coroutine %d finished. Running time: %lfus\n", entity->idx, (double)entity->running_time * 1e6 / CLOCKS_PER_SEC);

    munmap(entity->ctx.uc_stack.ss_sp, entity->ctx.uc_stack.ss_size);
    free(entity);
  } else if (entity->status == Suspended) {
    // The entity was set to Suspended state through
  } else if (entity->status == Failed) {
    munmap(entity->ctx.uc_stack.ss_sp, entity->ctx.uc_stack.ss_size);
    free(entity);
    //There is an error in this coroutine. We should report this back.
    return false;
  }
  return true;
}

static bool switch_to_coro(entity_t* entity) {
  scheduler_context.current_coro = entity;

  entity->status = Running;
  clock_t time = clock();

  if (swapcontext(&scheduler_context.scheduler_ctx, &entity->ctx) == -1) {
    perror("Couldn't switch to coroutine: ");
    return false;
  }
  entity->running_time += clock() - time;
  return true;
}

static void switch_to_scheduler() {
  entity_t* current_entity = scheduler_context.current_coro;
  scheduler_context.current_coro = NULL;
  if (swapcontext(&current_entity->ctx, &scheduler_context.scheduler_ctx) == -1) {
    // We don't know where are we, so abort the program.
    fprintf(stderr, "Couldn't reach the scheduler. Aborting...");
    abort();
  }
}

bool scheduler_run_loop() {
  while (true) {
    entity_t* entity;
    bool ok = get_runnable_entity(&entity);
    // TODO: Add a way to avoid stopping the whole run_loop because one coro failed.
    if (!ok) {
      return false;
    }
    if (!entity) {
      break;
    }

    if (!switch_to_coro(entity)) {
      return false;
    }

    if (!check_state(entity)) {
      fprintf(stderr, "Coroutine failed.\n");
      return false;
    }
  }

  return true;
}

static void check_inside_coroutine() {
  if (!scheduler_context.current_coro) {
    fprintf(stderr, "Yield() called from outside of coroutine");
    abort();
  }
}

void scheduler_coro_yield() {
  check_inside_coroutine();
  scheduler_context.current_coro->status = Runnable;
  switch_to_scheduler();
}

void scheduler_coro_suspend() {
  check_inside_coroutine();
  scheduler_context.current_coro->status = Suspended;
  scheduler_context.suspended_coro_count++;
  switch_to_scheduler();
  scheduler_context.suspended_coro_count--;
}

void scheduler_coro_fail() {
  check_inside_coroutine();
  scheduler_context.current_coro->status = Failed;
  switch_to_scheduler();

  // Something horrible must have happened if we achieved this line.
  fprintf(stderr, "Returned back to failed coroutine. Aborting...");
  abort();
}

