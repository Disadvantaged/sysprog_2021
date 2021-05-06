#include "userfs.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

enum {
	BLOCK_SIZE = 512,
	MAX_FILE_SIZE = 1024 * 1024 * 1024,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
	/** Block memory. */
	char memory[BLOCK_SIZE];
	/** How many bytes are occupied. */
	size_t occupied;
	/** Next block in the file. */
	struct block *next;
	/** Previous block in the file. */
	struct block *prev;
};

static struct block* new_block() {
    struct block* b = calloc(1, sizeof(struct block));
    if (!b) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    return b;
}

struct file {
	/** Double-linked list of file blocks. */
	struct block *block_list;
    /** Pointer to last block. */
	struct block *last_block;
	/** How many file descriptors are opened on the file. */
	int refs;
	/** File name. */
	const char *name;
	/** Files are stored in a double-linked list. */
	struct file *next;
	struct file *prev;

	size_t size;
	bool hanging;
};

/** List of all files. */
static struct file *file_list = NULL;

static struct file* find_file(const char* filename) {
    struct file* node = file_list;
    while (node) {
        if (!strcmp(node->name, filename)) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

static struct file* new_file(const char* filename) {
    if (find_file(filename)) {
        ufs_error_code = UFS_ERR_EXISTS;
        return NULL;
    }
    struct file* f = calloc(1, sizeof(struct file));
    if (!f) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    f->block_list = f->last_block = calloc(1, sizeof(struct block));
    if (!f->block_list) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    f->name = strdup(filename);
    if (!f->name) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    f->next = file_list;
    if (file_list) {
        file_list->prev = f;
    }
    file_list = f;
    return f;
}

static void remove_file_from_list(struct file* f) {
    if (!f) {
        return;
    }
    if (f == file_list) {
        file_list = file_list->next;
    }
    if (f->next) {
        f->next->prev = NULL;
        f->next = NULL;
    }
    if (f->prev) {
        f->prev->next = NULL;
        f->prev = NULL;
    }
}

static void free_file(struct file* f) {
    remove_file_from_list(f);
    if (f->refs) {
        fprintf(stderr, "Cannot remove file. Some descriptors are pointing to it.");
        abort();
    }
    while (f->block_list) {
        struct block* b = f->block_list;
        f->block_list = b->next;
        free(b);
    }
    free((void*)f->name);
    free(f);
}

static struct file* remove_file_with_filename(const char* filename) {
    struct file* f = find_file(filename);
    remove_file_from_list(f);
    return f;
}

struct permissions {
    union {
        unsigned char flags;
        struct {
            unsigned char create : 1;
            unsigned char read : 1;
            unsigned char write : 1;
        };
    };
};

struct filedesc {
	struct file *file;

	struct permissions perms;

	int fd;
	struct block* current;
	size_t offset;
};
/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
static int file_descriptor_count = 0;

static int file_descriptor_capacity = 0;

static void resize_filedesc() {
    file_descriptors = reallocarray(file_descriptors, file_descriptor_capacity, sizeof(struct filedesc*));
    if (!file_descriptors) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
}

static struct filedesc* new_fd() {
    if (!file_descriptor_capacity) {
        file_descriptor_capacity = 5;
        resize_filedesc();
    }
    int i;
    for (i = 0; i < file_descriptor_count; ++i) {
        if (!file_descriptors[i]) {
            break;
        }
    }
    file_descriptors[i] = calloc(1, sizeof(struct filedesc));
    if (!file_descriptors[i]) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    file_descriptors[i]->fd = i;
    if (i == file_descriptor_count) {
        file_descriptor_count++;
        if (file_descriptor_count == file_descriptor_capacity) {
            file_descriptor_capacity *= 2;
            resize_filedesc();
        }
    }
    return file_descriptors[i];
}

enum ufs_error_code
ufs_errno()
{
	return ufs_error_code;
}

int
ufs_open(const char *filename, int flags)
{
    ufs_error_code = UFS_ERR_NO_ERR;
    if (!filename) {
        ufs_error_code = UFS_ERR_WRONG_INPUT;
        return -1;
    }
    struct permissions perms = {.flags = flags};
    if (!perms.read && !perms.write) {
        perms.read = perms.write = 1;
    }

    struct file* f = find_file(filename);
    if (!f && !perms.create) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    if (!f) {
        f = new_file(filename);
        if (!f) {
            return -1;
        }
    }
    f->refs++;
    struct filedesc* fd = new_fd();
    fd->file = f;
    fd->perms = perms;
    fd->current = fd->file->block_list;
    return fd->fd;
}

ssize_t
ufs_write(int fd, const char *buf, size_t size)
{
    if (fd < 0 || fd >= file_descriptor_count || !file_descriptors[fd]) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    if (!size || !buf) {
        return 0;
    }
    struct filedesc* filedesc = file_descriptors[fd];
    if (!filedesc->perms.write) {
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }
    if (filedesc->offset > filedesc->file->size) {
        filedesc->offset = filedesc->file->size;
        filedesc->current = filedesc->file->last_block;
    }
    if (filedesc->offset + size > MAX_FILE_SIZE) {
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }
    size_t buf_offset = 0;
    ssize_t written = 0;
    while (size > 0) {
        // Calculate bytes count to write to current block.
        size_t block_pos = filedesc->offset % BLOCK_SIZE;
        size_t remains = BLOCK_SIZE - block_pos;
        size_t to_write = size < remains ? size : remains;
        // Write to it.
        memcpy(filedesc->current->memory + block_pos, buf + buf_offset, to_write);
        // Update offsets.
        filedesc->offset += to_write;
        buf_offset += to_write;
        size -= to_write;
        written += to_write;
        // If we write past the end of block, extend it.
        if (block_pos + to_write > filedesc->current->occupied) {
            filedesc->file->size += (block_pos + to_write) - filedesc->current->occupied;
            filedesc->current->occupied = block_pos + to_write;
        }
        // If the block has ended, go to the next block.
        if (filedesc->current->occupied == BLOCK_SIZE) {
            struct block* b = filedesc->current->next;
            if (!b) {
                filedesc->current->next = b = filedesc->file->last_block = new_block();
                filedesc->current->next->prev = filedesc->current;
                filedesc->file->last_block = b;
            }
            filedesc->current = b;
        }
    }
    return written;
}

ssize_t
ufs_read(int fd, char *buf, size_t size)
{
    if (fd < 0 || fd >= file_descriptor_count || !file_descriptors[fd]) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    if (!size || !buf) {
        return 0;
    }
    struct filedesc* filedesc = file_descriptors[fd];
    if (!filedesc->perms.read) {
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }
    ssize_t read = 0;
    size_t buf_offset = 0;
    struct block* current = filedesc->current;
    while (size) {
        size_t block_offset = filedesc->offset % BLOCK_SIZE;
        size_t remains = current->occupied - block_offset;
        size_t to_read = size > remains ? remains : size;
        memcpy(buf + buf_offset, current->memory + block_offset, to_read);

        filedesc->offset += to_read;
        size -= to_read;
        buf_offset += to_read;
        read += to_read;

        // If size not empty, then we have read all the block. Check the next one.
        if (block_offset + to_read == current->occupied) {
            if (current->next) {
                filedesc->current = current = current->next;
            } else {
                break;
            }
        }
    }
    return read;
}

int
ufs_close(int fd)
{
    if (fd < 0 || fd >= file_descriptor_count || !file_descriptors[fd]) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    struct filedesc* filedesc = file_descriptors[fd];
    filedesc->file->refs--;
    if (filedesc->file->hanging && !filedesc->file->refs) {
        free_file(filedesc->file);
    }
    free(filedesc);
    file_descriptors[fd] = NULL;
    if (fd == file_descriptor_count) {
        file_descriptor_count--;
    }
    return 0;
}

int
ufs_delete(const char *filename)
{
    struct file* f = remove_file_with_filename(filename);
    if (!f) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    if (f->refs) {
        f->hanging = true;
    } else {
        free_file(f);
    }
    return 0;
}


int
ufs_resize(int fd, size_t new_size) {
    if (fd < 0 || fd >= file_descriptor_count || !file_descriptors[fd]) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    struct filedesc* filedesc = file_descriptors[fd];
    struct file* f = filedesc->file;

    if (new_size > f->size) {
        if (new_size > MAX_FILE_SIZE) {
            ufs_error_code = UFS_ERR_NO_MEM;
            return -1;
        }
        struct block* b = f->last_block;
        size_t bytes_to_add = new_size - f->size;
        while (bytes_to_add) {
            size_t remains = BLOCK_SIZE - b->occupied;
            size_t to_add = (bytes_to_add > remains ? remains : bytes_to_add);
            b->occupied += to_add;
            bytes_to_add -= to_add;
            if (b->occupied == BLOCK_SIZE) {
                b->next = new_block();
                b->next->prev = b;
            }
            b = b->next;
        }
        f->last_block = b;
    } else if (new_size < f->size) {
        struct block* b = f->last_block;
        size_t bytes_to_remove = f->size - new_size;
        while (bytes_to_remove) {
            size_t to_remove = b->occupied > bytes_to_remove ? bytes_to_remove : b->occupied;
            b->occupied -= to_remove;
            bytes_to_remove -= to_remove;
            if (!b->occupied) {
                if (b == f->block_list) {
                    break;
                }
                b = b->prev;
                free(b->next);
                b->next = NULL;
            }
        }
        f->last_block = b;
    }
    f->size = new_size;
    return 0;
}