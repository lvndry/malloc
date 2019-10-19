#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "types.h"
#include "malloc.h"
#include "utils.h"

void *alloc(size_t size)
{

    if (size == 0)
    {
        return NULL;
    }

    size_t aligned_size = align(size);
    static struct mem_block *first = NULL;
    struct mem_block *lfb = NULL;
    struct mem_block *last = NULL;
    struct mem_block *block = NULL;

    if (first == NULL)
    {
        size_t map_size = getmappedsize(aligned_size);
        block = getPage(last, map_size);
        if (block == NULL)
        {
            return NULL;
        }
        create_block(block, aligned_size);
        block->next = NULL;
        block->prev = NULL;
        if (map_size >= aligned_size + (2 * META_SIZE) + 80)
        {
            lfb = split_block(block, map_size);
        }

        lfb = lfb;
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
                return NULL;
            }
            create_block(block, aligned_size);
            block->next = NULL;
            block->prev = last;
            if (map_size >= aligned_size + (2 * META_SIZE))
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
        my_free(ptr);
        return ptr;
    }

    struct mem_block *addr = get_meta(ptr);
    if (addr->size >= size) {}

    size_t aligned_size = align(size);

    void *tmp = alloc(size);
    memcpy(tmp, ptr, aligned_size);
    free(ptr);

    return tmp;
}

void *my_calloc(size_t nmemb, size_t size)
{
    size_t res = 0;
    if (__builtin_mul_overflow(nmemb, size, &res))
    {
        return NULL;
    }

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

        if (to_free->next != NULL && to_free->next->is_available)
        {
            if (get_first_of_page(to_free) == get_first_of_page(to_free->next))
                merge(to_free, to_free->next);
        }

        if (to_free->prev != NULL && to_free->prev->is_available)
        {
            if (get_first_of_page(to_free) == get_first_of_page(to_free->prev))
                merge(to_free->prev, to_free);
        }
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

