#ifndef MALLOC_SAFE_H
#define MALLOC_SAFE_H

/**
 * Malloc that prints error and restarts the system on failure.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void *malloc_safe_do(size_t size, const char* file, uint32_t line) __attribute__((malloc));
void *calloc_safe_do(size_t nmemb, size_t size, const char* file, uint32_t line) __attribute__((malloc));
void *malloc_ck_do(size_t size, bool *suc, const char* file, uint32_t line) __attribute__((malloc));
void *calloc_ck_do(size_t nmemb, size_t size, bool *suc, const char* file, uint32_t line) __attribute__((malloc));

#define malloc_s(size)        malloc_safe_do(size,        __BASE_FILE__, __LINE__)
#define calloc_s(nmemb, size) calloc_safe_do(nmemb, size, __BASE_FILE__, __LINE__)
#define malloc_ck(size, suc)        malloc_ck_do(size, suc,   __BASE_FILE__, __LINE__)
#define calloc_ck(nmemb, size, suc) calloc_ck_do(nmemb, size, suc, __BASE_FILE__, __LINE__)

#endif // MALLOC_SAFE_H
