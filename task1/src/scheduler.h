//
// Created by dgolear on 24.03.2021.
//

#ifndef TASK1_SCHEDULER_H
#define TASK1_SCHEDULER_H

#include <stdbool.h>
#include <stdlib.h>

// Coroutine API. Should be called only inside coroutines.
// ------------------------------------
// Suspends coroutine execution. Eventually, it would be resumed by scheduler.
void scheduler_coro_yield();
// Non-returning call. Sets Coroutine status to Failed and switches to scheduler.
void scheduler_coro_fail();
// ------------------------------------


bool scheduler_add_task(void (*)(void *), void *ctx);

bool scheduler_initialize();
bool scheduler_run_loop();
void scheduler_destroy();

#endif //TASK1_SCHEDULER_H
