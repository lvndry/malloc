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

static size_t align(size_t n);
static void *alloc(size_t size);
static void create_block(struct mem_block *last, struct mem_block *block, size_t size);
static struct mem_block *find_block(struct mem_block *start, struct mem_block **last, size_t size);
static size_t getmappedsize(size_t size);
static struct mem_block *getPage(struct mem_block *last, size_t map_size);
// static int is_adress_valid(void *ptr);
void move_data(struct mem_block *block, struct mem_block *dest, size_t size);
static void split_block(struct mem_block *block, size_t size);
static struct mem_block *get_meta(void *ptr);

// TO REMOVE BEFORE PUSH TO PROD
// static void print_block(struct mem_block *block)
// {
//     printf("Block address: %p\n", (void*)block);
//     printf("Block size: %ld\n", block->size);
//     printf("Block availability: %d\n", block->is_available);
//     printf("Block data: %p\n", block->data);
//     printf("Block next: %p\n", (void*)block->next);
//     printf("\n");
// }

void move_data(struct mem_block *block, struct mem_block *dest, size_t size)
{
    struct mem_block *src = block->next;
    memmove(src, dest, size);
    block->size += size;
    dest->size -= size;
}

static size_t align(size_t n)
{
    return (n + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
}

struct mem_block *get_meta(void *ptr)
{
    return ptr - META_SIZE;
}

static int is_adress_valid(void *ptr)
{
    struct mem_block *meta = get_meta(ptr);
    return (void*)meta->data == ptr;
}

static struct mem_block *find_block(struct mem_block *start, struct mem_block **last, size_t size)
{
    struct mem_block *ptr = start;
    while (ptr && !(ptr->is_available && ptr->size >= size + META_SIZE))
    {
        **last = *ptr;
        ptr = ptr->next;
    }

    return ptr;
}

static void split_block(struct mem_block *block, size_t free_size)
{
    struct mem_block *next = (struct mem_block*)(block->data + block->size);
    next->size = free_size - block->size - (2 * META_SIZE);
    next->is_available = 1;
    next->next = block->next;
    block->next = next;
}

static void create_block(struct mem_block *last, struct mem_block *block, size_t size)
{
    block->size = size;
    block->is_available = 0;
    block->next = NULL;

    if (last != NULL)
    {
        last->next = block;
    }
}

static struct mem_block *getPage(struct mem_block *last, size_t map_size)
{
    struct mem_block *mapped_page = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapped_page == MAP_FAILED)
    {
        return NULL;
    }

    if (last != NULL)
    {
        last->next = mapped_page;
        return last;
    }

    return mapped_page;
}

static size_t getmappedsize(size_t size)
{
    size_t len = PAGE_SIZE;
    size_t n = (len / size) + 1;

    return (n * PAGE_SIZE);
}

static void *alloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    size_t aligned_size = align(size);
    static struct mem_block *first = NULL;
    struct mem_block *last = NULL;
    struct mem_block *block = NULL;

    if (first == NULL)
    {
        size_t map_size = getmappedsize(aligned_size);
        block = getPage(NULL, map_size);
        if (block == NULL)
        {
            return NULL;
        }
        create_block(last, block, aligned_size);
        if (map_size >= aligned_size + META_SIZE + 50)
        {
            split_block(block, PAGE_SIZE);
        }
        first = block;
    }
    else
    {
        last = first;
        block = find_block(first, &last, aligned_size);
        if (block != NULL)
        {
            size_t remaining = block->size;
            if ((block->size - aligned_size) >= META_SIZE + 50)
            {
                create_block(last, block, aligned_size);
                split_block(block, remaining);
            }
            else
            {
                size_t map_size = getmappedsize(aligned_size);
                block = getPage(last, map_size);
                if (block == NULL)
                {
                    return NULL;
                }
            }
        }
        else
        {
            size_t map_size = getmappedsize(aligned_size);
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
void *malloc(size_t size)
{
    return alloc(size);
}

/*  __attribute__((visibility("default")))
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

__attribute__((visibility("default")))
void *my_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return malloc(size);
    }

    if (size == 0)
    {
        free(ptr);
        return ptr;
    }

    void *addr = get_meta(ptr);
    struct mem_block *meta = addr;
    size_t aligned_size = align(size);
    struct mem_block *last = meta;
    struct mem_block *next_free = find_block(meta, &last, aligned_size);
    if (next_free != NULL)
    {
        if (meta->next == next_free)
        {
            meta->size += aligned_size;
            next_free += aligned_size;
        }
        else
        {
            move_data(meta, next_free, aligned_size);
        }
    }
    else
    {
        struct mem_block *dest = getPage(NULL, aligned_size);
        move_data(meta, dest, aligned_size);
    }

    return ptr;
}
*/

void my_free(void *ptr)
{
    if (is_adress_valid(ptr))
    {
        struct mem_block *to_free = get_meta(ptr);
        to_free->is_available = 1;
    }
    return;
}

/*  __attribute__((visibility("default")))
void free(void *ptr)
{
}
*/

int main(void)
{
    for (size_t i = 0; i < 6000000; i++)
    {
        char *ptr = alloc(i);
        my_free(ptr);
    }
    /* char *val =*/ alloc(5000000);
    // free(val);
    /* char *other = */alloc(120);
    // free(other);
    return 0;
}
/* int main(void)
{
    for (int i = 0; i < 50000; i++)
    {
        int *n = alloc(100);
        if (n == NULL)
        {
            printf("malloc %d failed...\n", i);
        }
        printf("malloc %d\n", i);
    }

    printf("more: Address returned: %p\n", more);

    str = (char*)my_realloc(str, 5);
    if (str == NULL)
    {
        printf("Failed to reallocate memory..\n");
    }
    printf("str: Address returned: %p\n", str);

    free(str);
    free(more);

    return 0;
}*/
