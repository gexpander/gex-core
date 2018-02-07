//
// Safe malloc with error file:line logging, using the FreeRTOS-provided malloc facility
// The custom malloc implementation is safer than the poorly documented hacks provided by
// newlib, written primarily for the desktop rather than embedded.
//

#ifndef MALLOC_SAFE_H
#define MALLOC_SAFE_H

/**
 * Malloc that prints error and restarts the system on failure.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void *malloc_ck_do(size_t size, const char* file, uint32_t line) __attribute__((malloc,warn_unused_result));
void *calloc_ck_do(size_t nmemb, size_t size, const char* file, uint32_t line) __attribute__((malloc,warn_unused_result));
char *strdup_ck_do(const char *s, const char* file, uint32_t line) __attribute__((malloc,warn_unused_result));
char *strndup_ck_do(const char *s, uint32_t len, const char* file, uint32_t line) __attribute__((malloc,warn_unused_result));

#if DEBUG_MALLOC

    #define _malloc_trace(len, obj, file, line) do { PRINTF("~ malloc(%d) -> 0x%p at ", len, obj); PUTS(file); PUTCHAR(':'); PRINTF("%d\r\n", (int)line); } while (0)
    #define _free_trace(obj, file, line)   do { PRINTF("~ free(0x%p) at ", obj); PUTS(file); PUTCHAR(':'); PRINTF("%d\r\n", (int)line); } while (0)
    #define malloc_ck(size) malloc_ck_do(size, __BASE_FILE__, __LINE__)
    #define calloc_ck(nmemb, size) calloc_ck_do(nmemb, size, __BASE_FILE__, __LINE__)
    #define strdup_ck(s) strdup_ck_do(s, __BASE_FILE__, __LINE__)
    #define strndup_ck(s, len) strndup_ck_do(s, (uint32_t)(len), __BASE_FILE__, __LINE__)
#else
    #define _malloc_trace(len, obj, file, line)
    #define _free_trace(obj, file, line)
    #define malloc_ck(size) malloc_ck_do(size, "", 0)
    #define calloc_ck(nmemb, size) calloc_ck_do(nmemb, size, "", 0)
    #define strdup_ck(s) strdup_ck_do(s, "", 0)
    #define strndup_ck(s, len) strndup_ck_do(s, (uint32_t)(len), "", 0)
#endif

/**
 * Free an allocated object, and assign it to NULL.
 */
#define free_ck(obj) do { \
    _free_trace(obj, __BASE_FILE__, __LINE__); \
    if ((obj) != NULL) vPortFree((void *)(obj)); \
    obj = NULL; \
} while (0)

#endif // MALLOC_SAFE_H
