//
// Created by dgolear on 06.04.2021.
//

#include "shell.h"

#include <string.h>

int main(int argc, char** argv) {
    if (argc > 2 && strcmp(argv[1], "--prompt") == 0) {
        set_prompt(argv[2]);
    } else if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
        printf("Usage: ./Cshell [--prompt prompt_str]\n");
        return 0;
    }

    run_loop();
}
