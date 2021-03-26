//
// Created by dgolear on 24.03.2021.
//

#include "support.h"

#include <memory.h>
#include <sys/stat.h>

bool check_if_exists(const char *file) {
  return !access(file, R_OK);
}

bool store_buffer_to_file(const buffer_t buffer, const char *filename) {
  FILE* f = fopen(filename, "w");
  if (f == NULL) {
    perror("Couldn't create file: ");
    return false;
  }

  for (int i = 0; i < buffer.size; ++i) {
    fprintf(f, "%d%s", buffer.buf[i], (i == buffer.size ? "" : " "));
    if (ferror(f)) {
      fprintf(stderr, "Couldn't write to a file.");
      fclose(f);
      return false;
    }
  }
  fclose(f);
  return true;
}

static bool expand(buffer_t* buffer, int* capacity) {
  *capacity *= 2;
  buffer->buf = reallocarray(buffer->buf, *capacity, sizeof(int));
  if (!buffer->buf) {
    perror("Couldn't allocate memory: ");
    return false;
  }
  return true;
}

bool bytes_to_buffer(char* bytes, buffer_t* buffer) {
  char* ptr = bytes;
  int capacity = 64;
  if (!expand(buffer, &capacity)) {
    return false;
  }
  buffer->size = 0;
  while (true) {
    char* ptr2;
    int val = strtol(ptr, &ptr2, 10);
    if (ptr == ptr2) {
      break;
    }
    ptr = ptr2;
    buffer->buf[buffer->size++] = val;
    if (buffer->size == capacity && !expand(buffer, &capacity)) {
      return false;
    }
  }
  return true;
}


bool read_buffer_sync(const char *file, buffer_t *buffer) {
  int fd = open(file, O_RDONLY);
  if (fd == -1) {
    perror("Couldn't open a file: ");
    return false;
  }
  int capacity = 128;
  int size = 0;
  char* bytes = malloc(capacity);
  if (!bytes) {
    close(fd);
    perror("Couldn't allocate memory: ");
    return false;
  }
  memset(bytes, 0, capacity);
  while (true) {
    int bytes_read = read(fd, bytes + size, capacity - size);
    if (bytes_read < 0) {
      perror("Couldn't read from file: ");
      free(bytes);
      close(fd);
      return false;
    } else if (bytes_read < capacity - size) {
      size += bytes_read;
      break;
    }
    size += bytes_read;
    if (capacity == size) {
      capacity *= 2;
      bytes = realloc(bytes, capacity);
      memset(bytes + size, 0, size);
      if (!bytes) {
        close(fd);
        perror("Couldn't allocate memory: ");
        return false;
      }
    }
  }
  close(fd);
  bool res = bytes_to_buffer(bytes, buffer);
  free(bytes);
  return res;
}

ssize_t get_file_size(const char *file) {
  struct stat st;
  int res = stat(file, &st);

  if (res == -1) {
    perror("Couldn't stat the file: ");
    return -1;
  }

  return st.st_size;
}
