#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

struct mem_block {
    size_t size;
    int is_available;
    char data[1];
    struct mem_block *next;
};

void *malloc(size_t);
void free(void*);
void *realloc(void*, size_t);
void *calloc(size_t, size_t);

#endif
