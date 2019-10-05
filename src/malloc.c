#define _GNU_SOURCE
#include <sys/mman.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

#include "malloc.h"

#define align (((((x) - 1) >> 2) << 2) + 4)
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

static void *find_block(struct mem_block *start, size_t size)
{
    struct mem_block *ptr = start;

    while (ptr && !(ptr->next && ptr->size >= size))
    {
        *start = *ptr;
        ptr = ptr->next;
    }

    return ptr;
}

__attribute__((visibility("default")))
void *malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    
    size_t n = 1;

    while (size >= PAGE_SIZE)
    {
        size -= PAGE_SIZE;
        n++;
    }

    static struct mem_block *base = NULL;

    if (base == NULL)
    {
        base = mmap(NULL, n * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        base->size = size;
        base->is_available = 0;
        base->data = (char*)(base + META_SIZE);
        base->next = (void*)(base->data + size);
        split_block(base);
    }
    else
    {
        find_block(base, size);
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

    char* more = malloc(100);
    if (more == NULL)
    {
        printf("Failed to allocate memory..\n");
        return 1;
    }

    printf("Address returned: %p\n", more);
    return 0;
}
