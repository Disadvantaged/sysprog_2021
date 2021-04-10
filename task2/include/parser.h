//
// Created by golya on 06.04.2021.
//

#ifndef SYSPROG_PARSER_H
#define SYSPROG_PARSER_H

#include "shell.h"

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

char* get_line();

// Parses line and splits it into commands.
// Commands, then, are grouped into, so called, pipelines.
// A pipeline entity is used to represent a flow of commands.
// If the user doesn't use '&' operation, then only one pipeline will be returned.
// Otherwise, command list is splitted into pipelines and each pipeline is executed separately.
// This is done because multiple '&' operations can be used.
// Each '&' operation makes all ops before it be done in the background mode.
//
// Returns: array of separate pipelines, or NULL on error.
buffer_t parse_line(char* line);

#endif //SYSPROG_PARSER_H
