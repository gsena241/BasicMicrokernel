#include "fs.h"
#include "block.h"
#include "uart.h"


#define FS_SIGNATURE 0x54414653u   /* 'SFAT' */
#define FS_CLUSTER_COUNT 12
#define FS_CLUSTER_SIZE BLOCK_SIZE
#define FS_FAT_FREE 0x00000000u
#define FS_FAT_EOF  0xffffffffu
#define FS_SUPER_BLOCK 0
#define FS_FAT_BLOCK 1
#define FS_DIR_BLOCK_START 2
#define FS_DIR_BLOCKS 2
#define FS_DATA_BLOCK_START (FS_DIR_BLOCK_START + FS_DIR_BLOCKS)

typedef struct {
    uint32_t signature;
    uint32_t total_blocks;
    uint32_t cluster_size;
    uint32_t cluster_count;
    uint32_t fat_block;
    uint32_t dir_block;
    uint32_t data_block;
} superblock_t;

typedef struct {
    char name[FS_NAME_MAX];
    uint32_t size;
    uint32_t first_cluster;
    uint8_t used;
    uint8_t reserved[7];
} dir_entry_t;

typedef struct {
    uint8_t used;
    uint32_t dir_index;
} open_file_t;

static superblock_t superblock;
static uint32_t fat[FS_CLUSTER_COUNT];
static dir_entry_t root_dir[FS_MAX_FILES];
static open_file_t open_table[FS_MAX_OPEN_FILES];

static void fs_zero(void *ptr, uint32_t size)
{
    uint8_t *p = (uint8_t *)ptr;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

static void fs_copy(void *dst, const void *src, uint32_t size)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (uint32_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

static uint32_t fs_strlen_local(const char *s)
{
    uint32_t len = 0;

    if (s == 0) {
        return 0;
    }

    while (s[len] != '\0') {
        len++;
    }

    return len;
}

static int fs_str_equal(const char *a, const char *b)
{
    uint32_t i = 0;

    if (a == 0 || b == 0) {
        return 0;
    }

    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }

    return a[i] == b[i];
}

static void fs_str_copy_name(char *dst, const char *src)
{
    uint32_t i = 0;

    for (; i < FS_NAME_MAX - 1 && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }

    dst[i] = '\0';
}

static uint32_t cluster_to_block(uint32_t cluster)
{
    return FS_DATA_BLOCK_START + cluster;
}

static int sync_superblock(void)
{
    uint8_t block[BLOCK_SIZE];

    fs_zero(block, BLOCK_SIZE);
    fs_copy(block, &superblock, sizeof(superblock));

    return block_write(FS_SUPER_BLOCK, block);
}

static int sync_fat(void)
{
    uint8_t block[BLOCK_SIZE];

    fs_zero(block, BLOCK_SIZE);
    fs_copy(block, fat, sizeof(fat));

    return block_write(FS_FAT_BLOCK, block);
}

static int sync_dir(void)
{
    const uint8_t *dir_bytes = (const uint8_t *)root_dir;
    uint32_t total = sizeof(root_dir);

    for (uint32_t b = 0; b < FS_DIR_BLOCKS; b++) {
        uint8_t block[BLOCK_SIZE];
        uint32_t offset = b * BLOCK_SIZE;
        uint32_t remaining = 0;

        fs_zero(block, BLOCK_SIZE);

        if (offset < total) {
            remaining = total - offset;
            if (remaining > BLOCK_SIZE) {
                remaining = BLOCK_SIZE;
            }
            fs_copy(block, dir_bytes + offset, remaining);
        }

        if (block_write(FS_DIR_BLOCK_START + b, block) != 0) {
            return -1;
        }
    }

    return 0;
}

static int load_metadata(void)
{
    uint8_t block[BLOCK_SIZE];
    uint8_t *dir_bytes = (uint8_t *)root_dir;
    uint32_t total = sizeof(root_dir);

    if (block_read(FS_SUPER_BLOCK, block) != 0) {
        return -1;
    }
    fs_copy(&superblock, block, sizeof(superblock));

    if (superblock.signature != FS_SIGNATURE) {
        return -1;
    }

    if (block_read(FS_FAT_BLOCK, block) != 0) {
        return -1;
    }
    fs_copy(fat, block, sizeof(fat));

    fs_zero(root_dir, sizeof(root_dir));
    for (uint32_t b = 0; b < FS_DIR_BLOCKS; b++) {
        uint32_t offset = b * BLOCK_SIZE;
        uint32_t remaining = 0;

        if (block_read(FS_DIR_BLOCK_START + b, block) != 0) {
            return -1;
        }

        if (offset < total) {
            remaining = total - offset;
            if (remaining > BLOCK_SIZE) {
                remaining = BLOCK_SIZE;
            }
            fs_copy(dir_bytes + offset, block, remaining);
        }
    }

    return 0;
}

static void clear_open_table(void)
{
    for (uint32_t i = 0; i < FS_MAX_OPEN_FILES; i++) {
        open_table[i].used = 0;
        open_table[i].dir_index = 0;
    }
}

static int fs_format(void)
{
    superblock.signature = FS_SIGNATURE;
    superblock.total_blocks = BLOCK_COUNT;
    superblock.cluster_size = FS_CLUSTER_SIZE;
    superblock.cluster_count = FS_CLUSTER_COUNT;
    superblock.fat_block = FS_FAT_BLOCK;
    superblock.dir_block = FS_DIR_BLOCK_START;
    superblock.data_block = FS_DATA_BLOCK_START;

    fs_zero(fat, sizeof(fat));
    fs_zero(root_dir, sizeof(root_dir));
    clear_open_table();

    if (sync_superblock() != 0) {
        return -1;
    }
    if (sync_fat() != 0) {
        return -1;
    }
    if (sync_dir() != 0) {
        return -1;
    }

    return 0;
}

static int find_dir_entry(const char *name)
{
    for (uint32_t i = 0; i < FS_MAX_FILES; i++) {
        if (root_dir[i].used && fs_str_equal(root_dir[i].name, name)) {
            return (int)i;
        }
    }

    return -1;
}

static int find_free_dir_entry(void)
{
    for (uint32_t i = 0; i < FS_MAX_FILES; i++) {
        if (!root_dir[i].used) {
            return (int)i;
        }
    }

    return -1;
}

static void clear_cluster(uint32_t cluster)
{
    uint8_t block[BLOCK_SIZE];

    fs_zero(block, BLOCK_SIZE);
    block_write(cluster_to_block(cluster), block);
}

int cluster_alloc(void)
{
    for (uint32_t i = 0; i < FS_CLUSTER_COUNT; i++) {
        if (fat[i] == FS_FAT_FREE) {
            fat[i] = FS_FAT_EOF;
            clear_cluster(i);
            sync_fat();
            return (int)i;
        }
    }

    return -1;
}

static void cluster_free_chain(uint32_t first_cluster)
{
    uint32_t current = first_cluster;

    while (current < FS_CLUSTER_COUNT && current != FS_FAT_EOF) {
        uint32_t next = fat[current];

        fat[current] = FS_FAT_FREE;
        clear_cluster(current);

        if (next == FS_FAT_EOF) {
            break;
        }

        current = next;
    }

    sync_fat();
}

static int allocate_chain(uint32_t size)
{
    uint32_t clusters_needed = (size + FS_CLUSTER_SIZE - 1) / FS_CLUSTER_SIZE;
    int first = -1;
    int previous = -1;

    if (clusters_needed == 0) {
        clusters_needed = 1;
    }

    for (uint32_t i = 0; i < clusters_needed; i++) {
        int current = cluster_alloc();

        if (current < 0) {
            if (first >= 0) {
                cluster_free_chain((uint32_t)first);
            }
            return -1;
        }

        if (first < 0) {
            first = current;
        }

        if (previous >= 0) {
            fat[previous] = (uint32_t)current;
        }

        previous = current;
    }

    if (previous >= 0) {
        fat[previous] = FS_FAT_EOF;
    }

    sync_fat();
    return first;
}

int fs_init(void)
{
    if (block_init() != 0) {
        return -1;
    }

    clear_open_table();

    if (load_metadata() == 0) {
        return 0;
    }

    return fs_format();
}

int fs_create(const char *name)
{
    int entry;
    int first_cluster;

    if (name == 0 || fs_strlen_local(name) == 0 || fs_strlen_local(name) >= FS_NAME_MAX) {
        return -1;
    }

    if (find_dir_entry(name) >= 0) {
        return -1;
    }

    entry = find_free_dir_entry();
    if (entry < 0) {
        return -1;
    }

    first_cluster = cluster_alloc();
    if (first_cluster < 0) {
        return -1;
    }

    fs_zero(&root_dir[entry], sizeof(dir_entry_t));
    fs_str_copy_name(root_dir[entry].name, name);
    root_dir[entry].size = 0;
    root_dir[entry].first_cluster = (uint32_t)first_cluster;
    root_dir[entry].used = 1;

    return sync_dir();
}

int fs_open(const char *name)
{
    int entry = find_dir_entry(name);

    if (entry < 0) {
        return -1;
    }

    for (uint32_t i = 0; i < FS_MAX_OPEN_FILES; i++) {
        if (!open_table[i].used) {
            open_table[i].used = 1;
            open_table[i].dir_index = (uint32_t)entry;
            return (int)i;
        }
    }

    return -1;
}

int fs_close(int fd)
{
    if (fd < 0 || fd >= FS_MAX_OPEN_FILES || !open_table[fd].used) {
        return -1;
    }

    open_table[fd].used = 0;
    open_table[fd].dir_index = 0;

    return 0;
}

int fs_write(int fd, const void *buffer, uint32_t size)
{
    dir_entry_t *entry;
    uint32_t old_first;
    uint32_t current;
    uint32_t written = 0;
    int new_first;

    if (fd < 0 || fd >= FS_MAX_OPEN_FILES || !open_table[fd].used || buffer == 0) {
        return -1;
    }

    entry = &root_dir[open_table[fd].dir_index];
    new_first = allocate_chain(size);
    if (new_first < 0) {
        return -1;
    }

    current = (uint32_t)new_first;
    while (current < FS_CLUSTER_COUNT) {
        uint8_t block[BLOCK_SIZE];
        uint32_t chunk = size - written;

        if (chunk > FS_CLUSTER_SIZE) {
            chunk = FS_CLUSTER_SIZE;
        }

        fs_zero(block, BLOCK_SIZE);
        if (chunk > 0) {
            fs_copy(block, ((const uint8_t *)buffer) + written, chunk);
        }

        if (block_write(cluster_to_block(current), block) != 0) {
            cluster_free_chain((uint32_t)new_first);
            return -1;
        }

        written += chunk;

        if (fat[current] == FS_FAT_EOF) {
            break;
        }

        current = fat[current];
    }

    old_first = entry->first_cluster;
    entry->first_cluster = (uint32_t)new_first;
    entry->size = size;

    cluster_free_chain(old_first);
    sync_fat();
    sync_dir();

    return (int)written;
}

int fs_read(int fd, void *buffer, uint32_t size)
{
    dir_entry_t *entry;
    uint32_t current;
    uint32_t read_total = 0;
    uint32_t to_read;

    if (fd < 0 || fd >= FS_MAX_OPEN_FILES || !open_table[fd].used || buffer == 0) {
        return -1;
    }

    entry = &root_dir[open_table[fd].dir_index];
    to_read = size;
    if (to_read > entry->size) {
        to_read = entry->size;
    }

    current = entry->first_cluster;
    while (current < FS_CLUSTER_COUNT && read_total < to_read) {
        uint8_t block[BLOCK_SIZE];
        uint32_t chunk = to_read - read_total;

        if (chunk > FS_CLUSTER_SIZE) {
            chunk = FS_CLUSTER_SIZE;
        }

        if (block_read(cluster_to_block(current), block) != 0) {
            return -1;
        }

        fs_copy(((uint8_t *)buffer) + read_total, block, chunk);
        read_total += chunk;

        if (fat[current] == FS_FAT_EOF) {
            break;
        }

        current = fat[current];
    }

    return (int)read_total;
}

int fs_delete(const char *name)
{
    int entry = find_dir_entry(name);

    if (entry < 0) {
        return -1;
    }

    for (uint32_t i = 0; i < FS_MAX_OPEN_FILES; i++) {
        if (open_table[i].used && open_table[i].dir_index == (uint32_t)entry) {
            open_table[i].used = 0;
        }
    }

    cluster_free_chain(root_dir[entry].first_cluster);
    fs_zero(&root_dir[entry], sizeof(dir_entry_t));

    return sync_dir();
}

void fs_debug_dump(void)
{
    uart_print("\n--- SimpleFAT: diretorio raiz ---\n");

    for (uint32_t i = 0; i < FS_MAX_FILES; i++) {
        if (root_dir[i].used) {
            uart_print("arquivo: ");
            uart_print(root_dir[i].name);
            uart_print(" | tamanho: ");
            uart_print_uint(root_dir[i].size);
            uart_print(" bytes | primeiro cluster: ");
            uart_print_uint(root_dir[i].first_cluster);
            uart_print("\n");
        }
    }
}
