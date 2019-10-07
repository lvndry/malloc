#define _GNU_SOURCE
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "malloc.h"

#define META_SIZE sizeof(struct mem_block)
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

size_t align(size_t n) {
    return (n + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
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

static void *find_block(struct mem_block *start, struct mem_block **last, size_t size)
{
    struct mem_block *ptr = start;
    while (ptr != NULL && !(ptr->is_available && ptr->size >= size))
    {
        **last = *ptr;
        ptr = ptr->next;
    }

    return ptr;
}

static void split_block(struct mem_block *block, size_t size)
{
    struct mem_block *next = (void*)(block->data + size);
    next->size = PAGE_SIZE - block->size - size - META_SIZE;
    next->is_available = 1;
    next->next = block->next;
    next->data = block->data + block->size + META_SIZE;
    block->next = next;
}

static void create_block(struct mem_block *last, struct mem_block *block, size_t size)
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

static struct mem_block *getPage(struct mem_block *last, size_t map_size)
{
    struct mem_block *mapped_page = mmap(last, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapped_page == MAP_FAILED)
    {
        return NULL;
    }

    return mapped_page;
}

static size_t getmappedsize(size_t size)
{
    size_t n = 1;
    size_t len = PAGE_SIZE;

    while (size >= len)
    {
        size -= len;
        n++;
    }

    return (n * PAGE_SIZE);
}

 __attribute__((visibility("default")))
void *alloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    int aligned_size = align(size);
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
        split_block(block, aligned_size);
        first = block;
    }
    else
    {
        last = first;
        block = find_block(first, &last, size);
        if (block != NULL)
        {
            if (block->size >= META_SIZE + 4)
            {
                split_block(block, aligned_size);
            }
        }
        else
        {
            block = getPage(last, aligned_size);
            size_t map_size = getmappedsize(size);
            block = getPage(NULL, map_size);
            if (block == NULL)
            {
                return NULL;
            }
        }
    }

    return (void*)block->data;
}

 __attribute__((visibility("default")))
void *calloc(size_t nmemb, size_t size)
{
    void *call = alloc(nmemb * size);
    if (call == NULL)
    {
        return NULL;
    }
    call = memset(call, 0, size);
    return call;
}

int main(void)
{
    printf("Test of malloc..\n");

    char* str = (char*)alloc(10);
    if (str == NULL)
    {
        printf("Failed to allocate memory..\n");
    }
    printf("str: Address returned: %p\n", str);

    char* more = alloc(100);
    if (more == NULL)
    {
        printf("Failed to allocate memory..\n");
    }
    printf("more: Address returned: %p\n", more);

    return 0;
}
