//
// Created by dgolear on 25.03.2021.
//

#ifndef TASK1_SCHEDULER_INTERNAL_H
#define TASK1_SCHEDULER_INTERNAL_H

#include "scheduler.h"

#include <sys/queue.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <ucontext.h>

enum COROUTINE_STATUS {
  Starting,
  Runnable,
  Running,
  Suspended,
  Failed,
};

#define stack_size (1024 * 32)

typedef struct entity_s {
  enum COROUTINE_STATUS status;

  struct ucontext_t ctx;

  int idx;
  clock_t running_time;

  STAILQ_ENTRY(entity_s) entities;
} entity_t;


struct scheduler_ctx_s {
  STAILQ_HEAD(wait_queue_t, entity_s) run_queue;
  struct ucontext_t scheduler_ctx;

  int suspended_coro_count;

  entity_t* current_coro;

  size_t last_coro_idx;
  size_t max_coro_count;

  struct aiocb** suspend_lst;
};

// Coroutine API. Should be called only inside coroutines.
// ------------------------------------
// Suspends coroutine execution and puts it into running queue. Eventually, it would be resumed by scheduler.
void scheduler_coro_yield();
// Suspends coroutine execution and puts it into sleeping coroutines queue. An event should also be set in order for coroutine to be awoken by scheduler.
void scheduler_coro_suspend();
// Non-returning call. Sets Coroutine status to Failed and switches to scheduler.
void scheduler_coro_fail();
// Call to read entire file, with blocking the coroutine
bool scheduler_coro_read_file(const char* file, char** ptr_to_bytes);
// ------------------------------------

#endif //TASK1_SCHEDULER_INTERNAL_H
