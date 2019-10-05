#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>

void *malloc(size_t sz)
{
        void *(*orig_malloc)(size_t) = dlsym(RTLD_NEXT, "malloc");
        fprintf(stderr, "[!] entering malloc(%zu)\n", sz);

        void *res = orig_malloc(sz);

        fprintf(stderr, "[!] exiting malloc(%zu) = %p\n", sz, res);
        return res;
}

void *calloc(size_t nmemb, size_t size)
{
        void *(*orig_calloc)(size_t,size_t) = dlsym(RTLD_NEXT, "calloc");
        fprintf(stderr, "[!] entering calloc(%zu)\n", size);

        void *res = orig_calloc(nmemb, size);

        fprintf(stderr, "[!] exiting calloc(%zu) = %p\n", size, res);
        return res;
}

void *realloc(void *ptr, size_t sz)
{
        void *(*orig_realloc)(void*, size_t) = dlsym(RTLD_NEXT, "realloc");
        fprintf(stderr, "[!] entering realloc(%p, %zu)\n", ptr, sz);

        void *res = orig_realloc(ptr, sz);

        fprintf(stderr, "[!] exiting realloc(%p, %zu) = %p\n", ptr, sz, res);
        return res;
}

void free(void *ptr)
{
        void (*orig_free)(void*) = dlsym(RTLD_NEXT, "free");
        fprintf(stderr, "[!] entering free(%p)\n", ptr);

        orig_free(ptr);

        fprintf(stderr, "[!] exiting free(%p)\n", ptr);
}
