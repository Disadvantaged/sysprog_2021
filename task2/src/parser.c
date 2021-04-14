//
// Created by golya on 06.04.2021.
//

#include "parser.h"
#include "support.h"

char *get_line() {
    buffer_t line = new_buffer(sizeof(char));

    char looking_for_endline = 0;
    while (true) {
        char c = getchar();

        if (c == '"' || c == '\'' || c == '`') {
            if (!looking_for_endline) {
                looking_for_endline = c;
            } else if (looking_for_endline == c) {
                looking_for_endline = 0;
            }
        } else if (c == '\n') {
            if (!looking_for_endline) {
                break;
            }
        } else if (c == '\\') {
            char next = getchar();
            if (next == EOF) {
                free(line.buf);
                return NULL;
            } else if (next == '\n') {
                continue;
            } else {
                buffer_push_back(&line, &c);
                buffer_push_back(&line, &next);
                continue;
            }
        } else if (c == EOF) {
            free(line.buf);
            return NULL;
        }

        buffer_push_back(&line, &c);
    }
    char guard = '\0';
    buffer_push_back(&line, &guard);
    return line.buf;
}

static char *get_token(const char *line, int* shift) {
    int i = 0;

    buffer_t token = new_buffer(1);
    while (line[i]) {
        if (line[i] == '#') {
            while (line[i]) {
                i++;
            }
            break;
        } else if (line[i] == '\\') {
            buffer_push_back(&token, &line[i + 1]);
            i += 2;
        } else if ('\'' == line[i] || line[i] == '"') {
            char cur = line[i];
            ++i;
            bool guard = false;
            while (line[i] && (line[i] != cur || (guard))) {
                if (line[i] == '\\') {
                    if (guard) {
                        buffer_push_back(&token, &line[i]);
                        guard = false;
                    } else {
                        guard = true;
                    }
                } else {
                    buffer_push_back(&token, &line[i]);
                    guard = false;
                }
                ++i;
            }
            if (line[i]) {
                ++i;
            }
            *shift = i;
            break;
        } else if (isspace(line[i])) {
            break;
        } else if (line[i] == '|' || line[i] == '&' || line[i] == '>') {
            if (token.count) {
                break;
            }
            char c = line[i];
            buffer_push_back(&token, &c);
            if (line[i + 1] && line[i] == line[i + 1]) {
                buffer_push_back(&token, &c);
                i += 2;
                break;
            }
            i++;
            break;
        } else {
            char c = line[i];
            buffer_push_back(&token, &c);
            i++;
        }
    }
    char guard = '\0';
    *shift = i;
    if (!token.count) {
        free(token.buf);
        return NULL;
    }
    buffer_push_back(&token, &guard);
    return token.buf;
}

static buffer_t tokenize(const char *line) {
    buffer_t tokens = new_buffer(sizeof(char *));

    int i = 0;
    while (line[i]) {
        if (!isspace(line[i])) {
            int shift;
            char *token = get_token(&line[i], &shift);
            if (!token) {
                break;
            }

            i += shift;

            buffer_push_back(&tokens, &token);

        } else {
            i++;
        }
    }
    return tokens;
}

static int get_next_command(buffer_t tokens_buf, size_t *i, command_t *cmd) {
    if (*i == tokens_buf.count) {
        return 0;
    }

    size_t current_token = *i;
    char **tokens = tokens_buf.buf;

    // End of pipeline.
    if (!strcmp(tokens[current_token], "&")) {
        return 0;
    }

    if (!strcmp(tokens[current_token], "&&")) {
        if (current_token == 0) {
            fprintf(stderr, "'&&' cannot come as first argument.\n");
            return -1;
        }
        cmd->after_and = true;
        current_token++;
    } else if (!strcmp(tokens[current_token], "||")) {
        if (current_token == 0) {
            fprintf(stderr, "'||' cannot come as first argument.\n");
            return -1;
        }
        cmd->after_or = true;
        current_token++;
    }
    if (current_token < tokens_buf.count
        && (!strcmp(tokens[current_token], ">")
            || !strcmp(tokens[current_token], ">>")
            || !strcmp(tokens[current_token], "|")
            || !strcmp(tokens[current_token], "||")
            || !strcmp(tokens[current_token], "&&")
            || !strcmp(tokens[current_token], "&"))) {
        fprintf(stderr, "Invalid syntax: '%s' not expected", tokens[current_token]);
        return -1;
    } else if (current_token == tokens_buf.count) {
        fprintf(stderr, "Invalid syntax: '%s' not expected\n", (cmd->after_and ? "&&" : "||"));
        return -1;
    }
    cmd->name = strdup(tokens[current_token]);
    if (cmd->name == NULL) {
        perror("couldn't copy cmd name: ");
        exit(1);
    }

    buffer_t args = new_buffer(sizeof(char*));
    buffer_push_back(&args, &cmd->name);

    current_token++;
    while (current_token < tokens_buf.count) {
        if (!strcmp(tokens[current_token], "&&") || !strcmp(tokens[current_token], "||")) {
            break;
        } else if (!strcmp(tokens[current_token], "|")) {
            cmd->pipe_to_next = true;
            current_token++;
            break;
        } else if (!strcmp(tokens[current_token], ">") || !strcmp(tokens[current_token], ">>")) {
            if (tokens[current_token][1] == '>') { // '>>'
                cmd->do_append = true;
            }
            
            current_token++;
            if (current_token == tokens_buf.count) {
                fprintf(stderr, "Expected file to redirect output to.\n");
                free_command(cmd);
                return -1;
            }

            cmd->redirect_output = strdup(tokens[current_token]);
            if (!cmd->redirect_output) {
                perror("allocation error");
                exit(1);
            }

            current_token++;
            break;
        } else if (!strcmp(tokens[current_token], "&")) {
            break;
        } else {
            char* arg = strdup(tokens[current_token]);
            if (!arg) {
                perror("allocation error");
                exit(1);
            }
            buffer_push_back(&args, &arg);
        }
        current_token++;
    }

    char* guard = NULL;
    buffer_push_back(&args, &guard);
    cmd->args = args.buf;

    *i = current_token;
    return 1;
}

static int get_next_pipeline(buffer_t tokens, size_t *i, pipeline_t *pipeline) {
    if (*i == tokens.count) {
        return 0;
    }

    pipeline->commands = new_buffer(sizeof(command_t));
    command_t command;
    memset(&command, 0, sizeof(command_t));
    int ret = 0;
    while ((ret = get_next_command(tokens, i, &command)) > 0) {
        buffer_push_back(&pipeline->commands, &command);
        memset(&command, 0, sizeof(command));
    }
    if (ret == -1) {
        free_commands(pipeline->commands);
        return -1;
    }

    if (*i < tokens.count && !strcmp(((char **) tokens.buf)[*i], "&")) {
        pipeline->is_async = true;
        (*i)++;
    }
    return 1;
}

void free_tokens(buffer_t tokens_buf) {
    char **tokens = tokens_buf.buf;
    for (size_t i = 0; i < tokens_buf.count; ++i) {
        free(tokens[i]);
    }
    free(tokens_buf.buf);
}

buffer_t parse_line(char *line) {
    buffer_t tokens = tokenize(line);

    fflush(stdout);
    buffer_t pipelines = new_buffer(sizeof(pipeline_t));
    int ret;
    size_t current_token = 0;
    pipeline_t pipeline;
    memset(&pipeline, 0, sizeof(pipeline_t));
    while ((ret = get_next_pipeline(tokens, &current_token, &pipeline)) > 0) {
        buffer_push_back(&pipelines, &pipeline);
        memset(&pipeline, 0, sizeof(pipeline_t));
    }
    if (ret == -1) {
        free_pipelines(pipelines);
        pipelines.buf = NULL;
    }

    free_tokens(tokens);
    return pipelines;
}
