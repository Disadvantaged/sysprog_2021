//
// Created by dgolear on 07.04.2021.
//

#include "support.h"

#include "commands.h"

buffer_t new_buffer(size_t size_element) {
    buffer_t buf = {.count=0,.size=5, .element_size=size_element,.buf=NULL};

    buf.buf = realloc(NULL, buf.size * buf.element_size);
    if (!buf.buf) {
        perror("Couldn't allocate new buffer");
        exit(1);
    }

    return buf;
}

void buffer_push_back(buffer_t* buf, const void* element) {
    if (buf->count == buf->size) {
        buf->size *= 2;
        buf->buf = realloc(buf->buf, buf->size * buf->element_size);
        if (!buf->buf) {
            perror("Couldn't reallocate memory.");
            exit(1);
        }
    }

    memcpy((char*)buf->buf + buf->count * buf->element_size, element, buf->element_size);
    buf->count++;
}

void free_command(command_t *command) {
    // free(commands[i].name); name will be cleared as args[0] is pointing to it also.
    free(command->redirect_output);
    int i = 0;
    while (command->args[i]) {
        free(command->args[i]);
        i++;
    }
    free(command->args);
}

void free_commands(buffer_t cmds) {
    command_t* commands = cmds.buf;

    for (size_t i = 0; i < cmds.count; ++i) {
        free_command(&commands[i]);
    }
    free(commands);
}

void free_pipeline(pipeline_t *pipeline) {
    free_commands(pipeline->commands);
}

void free_pipelines(buffer_t pipelines) {
    pipeline_t* ps = pipelines.buf;

    for (size_t i = 0; i < pipelines.count; ++i) {
        free_pipeline(&ps[i]);
    }
    free(ps);
}
