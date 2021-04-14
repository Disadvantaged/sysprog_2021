//
// Created by dgolear on 07.04.2021.
//

#ifndef SYSPROG_COMMANDS_H
#define SYSPROG_COMMANDS_H

#include "support.h"

#include <stdbool.h>
#include <unistd.h>

typedef struct command_s {
    char* name;

    char** args;

    bool do_append;
    char* redirect_output;

    bool pipe_to_next;

    bool after_or;
    bool after_and;
} command_t;

typedef struct pipeline_s {
    buffer_t commands;

    bool is_async;
} pipeline_t;

#endif //SYSPROG_COMMANDS_H
