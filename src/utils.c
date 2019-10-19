#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "malloc.h"

static pthread_mutex_t foo_mutex = PTHREAD_MUTEX_INITIALIZER;

void move_data(struct mem_block *block, struct mem_block *dest, size_t size)
{
    struct mem_block *src = block->next;
    memmove(src, dest, size);
    block->size += size;
    dest->size -= size;
}

size_t align(size_t size)
{
    if (size % 16 != 0)
    {
        size = ((size >> 4) << 4) + 16;
    }

    return size;
}

struct mem_block *get_meta(void *ptr)
{
    char *tmp = ptr;
    ptr = tmp - META_SIZE;

    return ptr;
}

int is_adress_valid(void *ptr)
{
    struct mem_block *meta = get_meta(ptr);
    void *data = meta->data;
    return data == ptr;
}


struct mem_block *find_block(
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

struct mem_block* split_block(struct mem_block *block, size_t free_size)
{
    pthread_mutex_lock(&foo_mutex);
    struct mem_block *next = (struct mem_block*)(block->data + block->size);
    next->size = align(free_size - block->size - (2 * META_SIZE));
    next->is_available = 1;
    void *addr = next;
    char *tmp = addr;
    next->data = tmp + META_SIZE;
    next->next = block->next;
    if (block->next != NULL)
    {
        block->next->prev = next;
    }
    next->prev = block;
    block->next = next;
    pthread_mutex_unlock(&foo_mutex);

    return next;
}

void create_block(struct mem_block *block, size_t size)
{
    pthread_mutex_lock(&foo_mutex);
    block->size = size;
    block->is_available = 0;
    void *addr = block;
    char *tmp = addr;
    block->data = tmp + META_SIZE;
    pthread_mutex_unlock(&foo_mutex);
}

struct mem_block *getPage(struct mem_block *last, size_t map_size)
{
    pthread_mutex_lock(&foo_mutex);
    void *map_res = mmap(
            NULL,
            map_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0);

    if (map_res == MAP_FAILED)
    {
        pthread_mutex_unlock(&foo_mutex);
        return NULL;
    }

    struct mem_block *mapped_page = map_res;
    mapped_page->prev = last;

    if (last != NULL)
    {
        last->next = map_res;
    }

    pthread_mutex_unlock(&foo_mutex);
    return mapped_page;
}

void merge(struct mem_block *dest, struct mem_block *src)
{

    pthread_mutex_lock(&foo_mutex);
    dest->size += (src->size + META_SIZE);
    void *dest_addr = src->next;
    dest->next = dest_addr;
    pthread_mutex_unlock(&foo_mutex);
}

size_t getmappedsize(size_t size)
{
    size_t len = PAGE_SIZE;
    size_t n = ((size + META_SIZE) / len) + 1;

    return (n * PAGE_SIZE);
}

void *get_first_of_page(void *ptr)
{
    uintptr_t add = (uintptr_t)ptr;
    size_t tmp = add & (PAGE_SIZE - 1);
    char *p = ptr;

    return p - tmp;
}