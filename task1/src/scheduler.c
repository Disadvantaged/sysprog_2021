//
// Created by dgolear on 24.03.2021.
//

#include "scheduler.h"

#include <sys/queue.h>
#include <sys/epoll.h>
#include "sys/mman.h"
#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

enum STATUS {
  Starting,
  Runnable,
  Running,
  Suspended,
  Failed,
};

#define stack_size 1024 * 1024

typedef struct entity_s {
  enum STATUS status;

  struct ucontext_t ctx;

  STAILQ_ENTRY(entity_s) entities;
} entity_t;


static struct {
  STAILQ_HEAD(wait_queue_t, entity_s) run_queue;
  int epoll_fd;
  struct ucontext_t scheduler_ctx;

  int suspended_coro_count;

  entity_t* current_coro;
} scheduler_context;

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

  makecontext(&new_entity->ctx, (void (*)(void)) coroutine, 1, ctx);

  STAILQ_INSERT_TAIL(&scheduler_context.run_queue, new_entity, entities);

  return true;
}

bool scheduler_initialize(int coroutine_count) {
  STAILQ_INIT(&scheduler_context.run_queue);
  scheduler_context.epoll_fd = epoll_create1(0);
  if (scheduler_context.epoll_fd == -1) {
    perror("epoll error: ");
    return false;
  }
  scheduler_context.suspended_coro_count = 0;

  return true;
}

void scheduler_destroy() {
  close(scheduler_context.epoll_fd);
}

static void scheduler_wait_any() {

}

static entity_t* get_runnable_entity() {
  entity_t* entity = STAILQ_FIRST(&scheduler_context.run_queue);
  if (entity != NULL) {
    STAILQ_REMOVE_HEAD(&scheduler_context.run_queue, entities);
  } else {
    if (scheduler_context.suspended_coro_count) {
      scheduler_wait_any();
      entity = get_runnable_entity();
    }
  }
  scheduler_context.current_coro = entity;
  return entity;
}

static bool check_state(entity_t* entity) {
  if (entity->status == Runnable) {
    // The coroutine made a Yield() call. Add it to the end of the run queue.
    STAILQ_INSERT_TAIL(&scheduler_context.run_queue, entity, entities);
  } else if (entity->status == Running) {
    // The coroutine has finished executing. Remove it and delete all of its data.
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

bool scheduler_run_loop() {

  while (true) {
    entity_t* entity = get_runnable_entity();
    if (!entity) {
      break;
    }
    entity->status = Running;

    if (swapcontext(&scheduler_context.scheduler_ctx, &entity->ctx) == -1) {
      return false;
    }

    if (!check_state(entity)) {
      fprintf(stderr, "Coroutine failed.\n");
      return false;
    }
  }

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


void scheduler_coro_yield() {
  scheduler_context.current_coro->status = Runnable;
  switch_to_scheduler();
}

void scheduler_coro_fail() {
  scheduler_context.current_coro->status = Failed;
  switch_to_scheduler();

  // Something horrible must have happened if we achieved this line.
  fprintf(stderr, "Returned back to failed coroutine. Aborting...");
  abort();
}

