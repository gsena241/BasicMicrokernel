#include "block.h"

static uint8_t virtual_disk[BLOCK_COUNT][BLOCK_SIZE];
static int disk_started = 0;

static void block_zero(void *ptr, uint32_t size)
{
    uint8_t *p = (uint8_t *)ptr;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

static void block_copy(void *dst, const void *src, uint32_t size)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (uint32_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

int block_init(void)
{
    if (!disk_started) {
        block_zero(virtual_disk, BLOCK_COUNT * BLOCK_SIZE);
        disk_started = 1;
    }

    return 0;
}

int block_read(uint32_t block, void *buffer)
{
    if (block >= BLOCK_COUNT || buffer == 0) {
        return -1;
    }

    block_copy(buffer, virtual_disk[block], BLOCK_SIZE);
    return 0;
}

int block_write(uint32_t block, const void *buffer)
{
    if (block >= BLOCK_COUNT || buffer == 0) {
        return -1;
    }

    block_copy(virtual_disk[block], buffer, BLOCK_SIZE);
    return 0;
}
