#include "platform.h"
#include "debug.h"
#include "stm32_assert.h"
#include "malloc_safe.h"

void *malloc_ck_do(size_t size,  const char *file, uint32_t line)
{
    void *mem = pvPortMalloc(size);
    _malloc_trace(size, mem, file, line);
    if (mem == NULL) {
        _warn_msg(file, line, "MALLOC FAILED");
    }
    return mem;
}

void *calloc_ck_do(size_t nmemb, size_t size, const char *file, uint32_t line)
{
    void *mem = malloc_ck_do(nmemb*size, file, line);
    memset(mem, 0, size*nmemb);
    return mem;
}

char *strdup_ck_do(const char *s, const char* file, uint32_t line)
{
    size_t len = strlen(s) + 1;
    void *new = malloc_ck_do(len, file, line);
    if (new == NULL) return NULL;
    return (char *) memcpy (new, s, len);
}

char *strndup_ck_do(const char *s, uint32_t len, const char* file, uint32_t line)
{
    // TODO verify - this was not tested
    size_t alen = MIN(strlen(s) + 1, len);
    uint8_t *new = malloc_ck_do(alen, file, line);
    if (new == NULL) return NULL;
    memcpy (new, s, alen-1);
    new[alen-1] = '\0';
    return (char *) new;
}
