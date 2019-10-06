#define _GNU_SOURCE
#include <sys/mman.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

#include "malloc.h"

#define META_SIZE sizeof(struct mem_block)
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

size_t align(size_t n) {
    return (n + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
}

void *find_block(struct mem_block *start, size_t size)
{
    struct mem_block *ptr = start;

    while (ptr && !(ptr->is_available && ptr->size >= size))
    {
        *start = *ptr;
        ptr = ptr->next;
    }

    return ptr;
}

void split_block(struct mem_block *block, size_t size)
{
    struct mem_block *next = (struct mem_block*)(block->data + size);
    next->size = PAGE_SIZE - block->size - META_SIZE;
    next->is_available = 1;
    next->next = block->next;
    next->data = block->data + block->size + META_SIZE;
    block->next = next;
}

void create_block(struct mem_block *last, struct mem_block *block, size_t size)
{
    block->size = size;
    block->is_available = 0;
    block->data = (char*)(block + META_SIZE);
    block->next = NULL;

    if (last != NULL)
    {
        last->next = block;
    }
}

struct mem_block *getPage(struct mem_block *last, size_t map_size)
{
    struct mem_block *mapped_page = mmap(last, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapped_page == MAP_FAILED)
    {
        return NULL;
    }
    
    return mapped_page;
}

size_t getmappedsize(size_t size)
{
    size_t n = 1;
    size_t len = PAGE_SIZE;

    while (size >= len)
    {
        size -= len;
        n++;
    }

    return n * PAGE_SIZE;
}

__attribute__((visibility("default")))
void *malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    size_t aligned_size = align(size);

    static void *first = NULL;
    struct mem_block *last = NULL;
    struct mem_block *block = NULL;

    if (first == NULL)
    {
        size_t map_size = getmappedsize(size);
        block = getPage(NULL, map_size);
        if (block == NULL)
        {
            return NULL;
        }

        create_block(last, block, size);
        first = block;
    }
    else
    {
        printf("Address of first %p\n", first);
        return NULL;
    }

    return (void*)block->data;
}

// __attribute__((visibility("default")))
// void free(void *ptr)
// {
// }

// __attribute__((visibility("default")))
// void *realloc(void *ptr,
//              size_t size)
// {
//     return NULL;
// }

// __attribute__((visibility("default")))
// void *calloc(size_t nmemb,
//              size_t size)
// {
//     return NULL;
// }

int main(void)
{
    printf("Test of malloc..\n");

    char* str = malloc(10);
    if (str == NULL)
    {
        printf("Failed to allocate memory..\n");
        return 1;
    }

    printf("Address returned: %p\n", str);

    // char* more = malloc(100);
    // if (more == NULL)
    // {
    //     printf("Failed to allocate memory..\n");
    //     return 1;
    // }

    // printf("Address returned: %p\n", more);
    // return 0;
}
