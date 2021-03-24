//
// Created by dgolear on 24.03.2021.
//

#ifndef TASK1_SUPPORT_H
#define TASK1_SUPPORT_H

#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
  size_t size;

  int* buf;
} buffer_t;

// Checks if file exists and open for reading.
bool check_if_exists(const char* file);

// Stores the int buffer to the file, truncating the file beforehand.
// The buffer remains untouched.
// Returns false in case of any error.
bool store_buffer_to_file(buffer_t buffer, const char* filename);

// Blocking read from file.
// Parses the file until EOF.
// Returns false in case of any error.
bool read_buffer_sync(const char* file, buffer_t* buffer);

#endif //TASK1_SUPPORT_H
