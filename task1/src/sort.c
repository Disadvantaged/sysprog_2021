//
// Created by dgolear on 24.03.2021.
//

#include "sort.h"

#include <limits.h>
#include <memory.h>

static void merge(int* left, const int* right, int size_left, int size_right) {
  int buf[size_left + size_right];

  int l = 0, r = 0;
  int res_index = 0;
  while (l < size_left && r < size_right) {
    if (left[l] < right[r]) {
      buf[res_index++] = left[l++];
    } else {
      buf[res_index++] = right[r++];
    }
  }
  while (l < size_left) {
    buf[res_index++] = left[l++];
  }
  while (r < size_right) {
    buf[res_index++] = right[r++];
  }
  memcpy(left, buf, (size_left + size_right) * sizeof(int));
}


static void sort_internal(int* ptr, int start, int end) {
  if (start < end) {
    int mid = start + (end - start) / 2;
    sort_internal(ptr, start, mid);
    sort_internal(ptr, mid + 1, end);
    merge(&ptr[start], &ptr[mid + 1], mid - start + 1, end - mid);
  }
}

void sort_buffer(buffer_t buf) {
  if (!buf.size) {
    return;
  }
  sort_internal(buf.buf, 0, buf.size - 1);
}

bool merge_sorted_buffers(buffer_t *in_buffers, int in_buf_count, buffer_t *out_buf) {
  int all_size = 0;
  for (int i = 0; i < in_buf_count; ++i) {
    all_size += in_buffers[i].size;
  }
  if (out_buf->size < all_size) {
    out_buf->buf = reallocarray(out_buf->buf, all_size, sizeof(int));
    if (out_buf->buf == NULL) {
      perror("Couldn't allocate buffer for merging: ");
      return false;
    }
    out_buf->size = all_size;
  }

  int indices[in_buf_count];
  for (int i = 0; i < in_buf_count; ++i) {
    indices[i] = 0;
  }

  int current_out_index = 0;
  while (current_out_index < all_size) {
    int min_elem = INT_MAX;
    int index = -1;
    for (int i = 0; i < in_buf_count; ++i) {
      if (indices[i] < in_buffers[i].size && in_buffers[i].buf[indices[i]] < min_elem) {
        min_elem = in_buffers[i].buf[indices[i]];
        index = i;
      }
    }
    out_buf->buf[current_out_index++] = min_elem;
    indices[index]++;
  }
  return true;
}
