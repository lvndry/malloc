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
static void create_block(struct mem_block *block, struct mem_block *last, size_t size);
static struct mem_block *find_block(struct mem_block *start, struct mem_block **last, size_t size);
static size_t getmappedsize(size_t size);
static struct mem_block *getPage(struct mem_block *last, size_t map_size);
static int is_adress_valid(void *ptr);
void move_data(struct mem_block *block, struct mem_block *dest, size_t size);
static void split_block(struct mem_block *block, size_t size);
static struct mem_block *get_meta(void *ptr);

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
    char *tmp = ptr;
    ptr = tmp - META_SIZE;

    return ptr;
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
    void *addr = block;
    char *tmp = addr;
    next->data = tmp + META_SIZE;
    next->next = block->next;
    block->next = next;
}

static void create_block(struct mem_block *block, struct mem_block *last, size_t size)
{
    block->size = size;
    block->is_available = 0;
    void *addr = block;
    char *tmp = addr;
    block->data = tmp + META_SIZE;
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

    return mapped_page;
}

static size_t getmappedsize(size_t size)
{
    size_t len = PAGE_SIZE;
    size_t n = (size / len) + 1;

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
        create_block(block, last, aligned_size);
        if (map_size >= aligned_size + META_SIZE + 50)
        {
            split_block(block, map_size);
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
            if ((block->size - aligned_size) >= META_SIZE + 100)
            {
                create_block(block, last, aligned_size);
                split_block(block, remaining);
            }
            else
            {
                size_t map_size = getmappedsize(aligned_size);
                block = getPage(last, map_size);
                create_block(block, last, aligned_size);

                if (block == NULL)
                {
                    return NULL;
                }
                if (map_size >= aligned_size + META_SIZE + 100)
                {
                    split_block(block, map_size);
                }
            }
        }
        else
        {
            size_t map_size = getmappedsize(aligned_size);
            block = getPage(last, map_size);
            if (block == NULL)
            {
                return NULL;
            }
            create_block(block, last, aligned_size);
            if (map_size >= aligned_size + META_SIZE + 100)
            {
                split_block(block, map_size);
            }
        }
    }

    return (void*)block->data;
}

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

    struct mem_block *addr = get_meta(ptr);

    if (addr->size >= size)
    {
        return ptr;
    }

    size_t aligned_size = align(size);

    if (addr->next != NULL && addr->next->is_available)
    {
        if (addr->size + addr->next->size + META_SIZE >= size)
        {
            addr->size = addr->size + addr->next->size + META_SIZE;
            addr->next = addr->next->next;
        }
        if (addr->size >= aligned_size + META_SIZE + 100)
        {
            struct mem_block *next = (struct mem_block*)(addr->data + aligned_size);
            next->size = addr->size - aligned_size - (2 * META_SIZE);
            next->is_available = 1;
            void *vb = addr;
            char *tmp = vb;
            next->data = tmp + META_SIZE;
            next->next = addr->next;
            addr->next = next;
        }
    }
    else
    {
        void *dest = malloc(aligned_size);
        move_data(addr, dest, aligned_size);
    }

    return ptr;
}

void *my_calloc(size_t nmemb, size_t size)
{
    void *call = alloc(nmemb * size);
    if (call == NULL)
    {
        return NULL;
    }
    call = memset(call, 0, size);
    return call;
}


void my_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    if (is_adress_valid(ptr))
    {
        struct mem_block *to_free = get_meta(ptr);
        to_free->is_available = 1;
    }

    return;
}

__attribute__((visibility("default")))
void *calloc(size_t nmemb, size_t size)
{
    return my_calloc(nmemb, size);
}

__attribute__((visibility("default")))
void *malloc(size_t size)
{
    return alloc(size);
}

__attribute__((visibility("default")))
void *realloc(void *ptr, size_t size)
{
    return my_realloc(ptr, size);
}

__attribute__((visibility("default")))
void free(void *ptr)
{
    my_free(ptr);
}

/* int main(void)
{
    for (int i = 0; i < 3000000; i++)
    {
        // printf("malloc number %d\n", i);
        char *tmp = alloc(i);
        if (tmp == NULL)
        {
            continue;
        }
        tmp[0] = 'c';
        my_free(tmp);
        // printf("Stopped after %d iterations\n", i);
    }
    return 0;
}
*/
