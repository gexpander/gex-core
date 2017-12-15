#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "debug.h"
#include "stm32_assert.h"
#include "malloc_safe.h"

#if 1

void *malloc_safe_do(size_t size, const char *file, uint32_t line)
{
    void *mem = malloc(size);
    if (mem == NULL) abort_msg("MALLOC FAILED", file, line);
    return mem;
}

void *calloc_safe_do(size_t nmemb, size_t size, const char *file, uint32_t line)
{
    void *mem = calloc(size, nmemb);
    if (mem == NULL) abort_msg("CALLOC FAILED", file, line);
    return mem;
}


void *malloc_ck_do(size_t size, bool *suc, const char *file, uint32_t line)
{
    void *mem = malloc(size);
    if (mem == NULL) {
        warn_msg("MALLOC FAILED", file, line);
        *suc = false;
        mem = NULL;
    }
    return mem;
}

void *calloc_ck_do(size_t nmemb, size_t size, bool *suc, const char *file, uint32_t line)
{
    void *mem = calloc(size, nmemb);
    if (mem == NULL) {
        warn_msg("CALLOC FAILED", file, line);
        *suc = false;
        mem = NULL;
    }
    return mem;
}

#endif
