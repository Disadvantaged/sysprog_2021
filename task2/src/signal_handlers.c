//
// Created by golya on 06.04.2021.
//

#include "shell.h"

#include <signal.h>
#include <stdio.h>

static void shell_handler(int signal) {
//    printf("\n");
//    print_prompt();
}

void set_shell_signal_handlers() {
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, shell_handler);
}

void set_command_signal_handlers() {

}
