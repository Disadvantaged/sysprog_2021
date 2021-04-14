//
// Created by dgolear on 07.04.2021.
//

#ifndef SYSPROG_SUPPORT_H
#define SYSPROG_SUPPORT_H

#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

typedef struct pipeline_s pipeline_t;
typedef struct command_s command_t;

typedef struct {
    void* buf;
    size_t count;
    size_t size;
    size_t element_size;
} buffer_t;

buffer_t new_buffer(size_t size_element);
void buffer_push_back(buffer_t* buf, const void* element);

void free_pipeline(pipeline_t* pipeline);
void free_command(command_t* command);
void free_pipelines(buffer_t pipelines);
void free_commands(buffer_t cmds);

#endif //SYSPROG_SUPPORT_H
