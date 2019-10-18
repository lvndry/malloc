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

static pthread_mutex_t foo_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static struct mem_block *find_block(
        struct mem_block *start,
        struct mem_block **last,
        size_t size
    )
{
    struct mem_block *ptr = start;
    while (ptr && !(ptr->is_available && ptr->size >= size))
    {
        *last = ptr;
        ptr = ptr->next;
    }

    return ptr;
}

static void split_block(struct mem_block *block, size_t free_size)
{
    struct mem_block *next = (struct mem_block*)(block->data + block->size);

    next->size = align(free_size - block->size - (2 * META_SIZE));
    next->is_available = 1;
    void *addr = next;
    char *tmp = addr;
    next->data = tmp + META_SIZE;
    next->next = block->next;
    block->next = next;
}

static void create_block(struct mem_block *block, size_t size)
{
    block->size = size;
    block->is_available = 0;
    void *addr = block;
    char *tmp = addr;
    block->data = tmp + META_SIZE;
}

static struct mem_block *getPage(struct mem_block *last, size_t map_size)
{
    void *map_res = mmap(
            NULL,
            map_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0);

    if (map_res == MAP_FAILED)
    {
        return NULL;
    }

    struct mem_block *mapped_page = map_res;

    if (last != NULL)
    {
        last->next = map_res;
    }

    return mapped_page;
}

static size_t getmappedsize(size_t size)
{
    size_t len = PAGE_SIZE;
    size_t n = ((size + META_SIZE) / len) + 1;

    return (n * PAGE_SIZE);
}

static void *alloc(size_t size)
{

    // pthread_mutex_lock(&foo_mutex);
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
        block = getPage(last, map_size);
        if (block == NULL)
        {
            pthread_mutex_unlock(&foo_mutex);
            return NULL;
        }
        create_block(block, aligned_size);
        block->next = NULL;
        if (map_size >= aligned_size + (2 * META_SIZE) + 80)
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
            create_block(block, aligned_size);
            if (block->size >= aligned_size + (2 * META_SIZE) + 80)
            {
                split_block(block, remaining);
            }
        }
        else
        {
            size_t map_size = getmappedsize(aligned_size);
            block = getPage(last, map_size);
            if (block == NULL)
            {
                pthread_mutex_unlock(&foo_mutex);
                return NULL;
            }
            create_block(block, aligned_size);
            block->next = NULL;
            if (map_size >= aligned_size + (2 * META_SIZE))
            {
                split_block(block, map_size);
            }
        }
    }

    pthread_mutex_unlock(&foo_mutex);
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
        my_free(ptr);
        return ptr;
    }

    struct mem_block *addr = get_meta(ptr);
    if (addr->size >= size)
    {
        return ptr;
    }

    size_t aligned_size = align(size);

    void *tmp = ptr;
    ptr = alloc(size);
    memcpy(ptr, tmp, aligned_size);
    free(tmp);

    return ptr;
}

void *my_calloc(size_t nmemb, size_t size)
{
    char *call = alloc(nmemb * size);
    if (call == NULL)
    {
        return NULL;
    }

    call = memset(call, 0, align(nmemb * size));

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
    return;
}

