#ifndef TYPES_H
#define TYPES_H

#include <unistd.h>
#include <stddef.h>

struct mem_block {
    size_t size;
    int is_available;
    char *data;
    struct mem_block *prev;
    struct mem_block *next;
};

#define META_SIZE sizeof(struct mem_block)
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

#endif
