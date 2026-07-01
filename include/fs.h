#pragma once

#include <stdint.h>

#define FS_MAX_FILES 16
#define FS_MAX_OPEN_FILES 8
#define FS_NAME_MAX 32

int fs_init(void);
int fs_create(const char *name);
int fs_open(const char *name);
int fs_close(int fd);
int fs_read(int fd, void *buffer, uint32_t size);
int fs_write(int fd, const void *buffer, uint32_t size);
int fs_delete(const char *name);

int cluster_alloc(void);
void fs_debug_dump(void);
