//
// Created by dgolear on 24.03.2021.
//

#ifndef TASK1_SCHEDULER_H
#define TASK1_SCHEDULER_H

#include "support.h"

#include <stdbool.h>
#include <stdlib.h>

bool scheduler_add_task(void (*)(void *), void *ctx);

bool scheduler_initialize(int max_coroutine_count);
bool scheduler_run_loop();
void scheduler_destroy();

#endif //TASK1_SCHEDULER_H
