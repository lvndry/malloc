#ifndef UTILS_H
#define UTILS_H

#include "types.h"

size_t align(size_t size);
void create_block(struct mem_block *block, size_t size);
struct mem_block *find_block(
        struct mem_block *start,
        struct mem_block **last, size_t size
);
void merge(struct mem_block *dest, struct mem_block *src);
void *get_first_of_page(void *ptr);

size_t getmappedsize(size_t size);
struct mem_block *getPage(struct mem_block* last, size_t map_size);
int is_adress_valid(void *ptr);
void move_data(struct mem_block *block, struct mem_block *dest, size_t size);
struct mem_block* split_block(struct mem_block *block, size_t size);
struct mem_block *get_meta(void *ptr);

#endif