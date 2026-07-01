#pragma once

#include <stdint.h>

#define BLOCK_SIZE 512
#define BLOCK_COUNT 16

int block_init(void);
int block_read(uint32_t block, void *buffer);
int block_write(uint32_t block, const void *buffer);
