//
// Created by golya on 06.04.2021.
//

#include "shell.h"
#include "parser.h"

#include <wait.h>
#include <fcntl.h>

static char default_prompt[] = "$> ";
static char *prompt = default_prompt;

void print_prompt() {
//    printf("%s", prompt);
//    fflush(stdout);
}

void set_prompt(const char *new_prompt) {
    if (strcmp(prompt, default_prompt) != 0) {
        free(prompt);
    }
    prompt = strdup(new_prompt);
}


static void shell_init() {
    set_shell_signal_handlers();
}

static void redirect_output_to_fd(int old, int new) {
    int ret = dup2(old, new);
    if (ret == -1) {
        perror("Couldn't duplicate output descriptor");
        exit(1);
    }
    close(old);
}

static void redirect_output_to_file(const char *output, bool do_append) {
    int fd = open(output, O_WRONLY | O_CREAT | (do_append ? O_APPEND : O_TRUNC), 0655);
    if (fd == -1) {
        perror("Couldn't open file");
        exit(1);
    }
    redirect_output_to_fd(fd, STDOUT_FILENO);
}

static int exec_cd(command_t cmd) {
    char* pwd = cmd.args[1];
    if (pwd == NULL) {
        pwd = getenv("HOME");
        if (pwd == NULL) {
            return 1;
        }
    } else if (cmd.args[2]) {
        return 1;
    }
    return chdir(pwd);
}

static int exec_exit(command_t cmd) {
    if (cmd.pipe_to_next) {
        if (cmd.args[1]) {
            return atoi(cmd.args[1]);
        }
        return 0;
    }
    if (cmd.args[1]) {
        exit(atoi(cmd.args[1]));
    }
    exit(0);
}

static void execute_commands(command_t *commands, size_t count) {
    int last_result = 0;

    int pipe_in = 0;
    int pipe_out = 1;
    for (size_t i = 0; i < count; ++i) {
        if ((commands[i].after_and && last_result != 0)
            || (commands[i].after_or && last_result == 0)) {
            continue;
        }
        if (!strcmp(commands[i].name, "cd")) {
            last_result = exec_cd(commands[i]);
            continue;
        } else if (!strcmp(commands[i].name, "exit")) {
            last_result = exec_exit(commands[i]);
            continue;
        }

        int pipes[2];
        if (commands[i].pipe_to_next) {
            if (pipe(pipes) == -1) {
                perror("pipe failed");
                return;
            }
            pipe_out = pipes[1];
        }

        int pid = fork();
        if (pid == 0) {
            set_command_signal_handlers();

            if (commands[i].redirect_output != NULL) {
                redirect_output_to_file(commands[i].redirect_output, commands[i].do_append);
            }
            if (i > 0 && commands[i - 1].pipe_to_next) {
                redirect_output_to_fd(pipe_in, STDIN_FILENO);
            }
            if (commands[i].pipe_to_next) {
                close(pipes[0]);
                redirect_output_to_fd(pipe_out, STDOUT_FILENO);
            }

            execvp(commands[i].name, commands[i].args);
            perror("exec error");
            exit(1);
        } else if (pid > 0) {
            if (commands[i].pipe_to_next) {
                close(pipe_out);
                pipe_in = pipes[0];
            } else if (i > 0 && commands[i - 1].pipe_to_next) {
                close(pipe_in);
            }

            int stat;
            waitpid(pid, &stat, 0);
            last_result = WEXITSTATUS(stat);
        } else {
            perror("fork error");
            break;
        }
    }
}

static void execute_pipeline(pipeline_t pipeline) {
    if (pipeline.is_async) {
        int pid = fork();
        if (pid == 0) {
            execute_commands(pipeline.commands.buf, pipeline.commands.count);
            exit(0);
        }
    } else {
        execute_commands(pipeline.commands.buf, pipeline.commands.count);
    }
}

void run_loop() {
    shell_init();

    while (1) {
        print_prompt();
        char *line = get_line();
        if (!line) {
            break;
        }
        buffer_t buf_of_pipelines = parse_line(line);
        if (!buf_of_pipelines.buf) {
            free(line);
            continue;
        }

        pipeline_t *pipelines = buf_of_pipelines.buf;
        for (size_t i = 0; i < buf_of_pipelines.count; ++i) {
            execute_pipeline(pipelines[i]);
        }

        free_pipelines(buf_of_pipelines);
        free(line);
    }
}
