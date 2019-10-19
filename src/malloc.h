#ifndef MALLOC_H
#define MALLOC_H

#include "types.h"

#include <stddef.h>

void *alloc(size_t);
void my_free(void*);
void *my_realloc(void*, size_t);
void *my_calloc(size_t, size_t);

void *malloc(size_t);
void free(void*);
void *realloc(void*, size_t);
void *calloc(size_t, size_t);

#endif
