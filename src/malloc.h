#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

struct mem_block {
    size_t size;
    int is_available;
    char *data;
    struct mem_block *prev;
    struct mem_block *next;
};

static size_t align(size_t n);
static void *alloc(size_t size);
static void create_block(struct mem_block *block, size_t size);
static struct mem_block *find_block(
        struct mem_block *start,
        struct mem_block **last, size_t size
);

static size_t getmappedsize(size_t size);
static struct mem_block *getPage(struct mem_block* last, size_t map_size);
static int is_adress_valid(void *ptr);
void move_data(struct mem_block *block, struct mem_block *dest, size_t size);
static void split_block(struct mem_block *block, size_t size);
static struct mem_block *get_meta(void *ptr);

void *alloc(size_t);
void *my_realloc(void*, size_t);
void *my_calloc(size_t, size_t);
void my_free(void*);

void *malloc(size_t);
void free(void*);
void *realloc(void*, size_t);
void *calloc(size_t, size_t);

#endif
