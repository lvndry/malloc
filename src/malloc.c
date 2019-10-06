#define _GNU_SOURCE
#include <sys/mman.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

#include "malloc.h"

#define META_SIZE sizeof(struct mem_block)
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

static void split_block(struct mem_block *block)
{
    struct mem_block *next = block->next;
    next->size = PAGE_SIZE - block->size;
    next->is_available = 1;
    next->next = block->next;
    block->next = next;
}

inline size_t align(size_t n) {
  return (n + sizeof(intptr_t) - 1) & ~(sizeof(intptr_t) - 1);
}

static void *find_block(struct mem_block *start, size_t size)
{
    struct mem_block *ptr = start;

    while (ptr && !(ptr->is_available && ptr->size >= size))
    {
        *start = *ptr;
        ptr = ptr->next;
    }

    return ptr;
}

static void create_block(struct mem_block *last, struct mem_block *block, size_t size)
{
        block->size = size;
        block->is_available = 0;
        block->data = (char*)(block + META_SIZE);
        block->next = NULL;
        split_block(block);
        if (last != NULL)
        {
            last->next = block;
        }
}

static struct mem_block *getPage(struct mem_block *last, size_t map_size)
{
        struct mem_block *base = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED)
        {
            return NULL;
        }
        return base;
}

static size_t getmappedsize(size_t size)
{
    size_t n = 1;
    while (size >= PAGE_SIZE)
    {
        size -= PAGE_SIZE;
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
    
    size = align(size);

    static struct mem_block *base = NULL;
    static struct mem_block *last = NULL;
    
    if (base == NULL)
    {
        size_t map_size = getmappedsize(size);
        base = getPage(NULL, map_size);
        if (base == NULL)
        {
            return NULL;
        }
        create_block(last, base, size);
    }
    else
    {
        base = find_block(base, size);
    }

    return (void*)base->data;
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
    printf("Test of malloc\n");
    
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
