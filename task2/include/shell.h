//
// Created by golya on 06.04.2021.
//

#ifndef SYSPROG_SHELL_H
#define SYSPROG_SHELL_H

#include "signal_handlers.h"
#include "commands.h"
#include "support.h"

#include <malloc.h>
#include <string.h>
#include <stdio.h>

void run_loop();

void print_prompt();
void set_prompt(const char* new_prompt);

#endif //SYSPROG_SHELL_H
