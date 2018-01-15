//
// Created by MightyPork on 2017/11/04.
//

#ifndef GEX_DEBUG_H
#define GEX_DEBUG_H

#include <inttypes.h>
#include <stddef.h>
#include <stdarg.h>
#include "macro.h"

#if USE_DEBUG_UART

extern void debug_write(const char *buf, uint16_t len);

void _DO_PRINTF(const char *format, ...) __attribute__((format(printf,1,2))) ;
void PUTSN(const char *string, uint16_t len);
void PUTS(const char *string);

static inline void PUTNL(void)
{
    debug_write("\r\n", 2);
}

/**
 * Print one character to debug uart
 * @param ch - character ASCII code
 * @return the character code
 */
static inline void PUTCHAR(char ch)
{
    debug_write(&ch, 1);
}

#define PRINTF(format, ...) do { \
    if (VA_ARG_COUNT(__VA_ARGS__) == 0) { \
      PUTS(format); \
    } else { \
      _DO_PRINTF(format, ##__VA_ARGS__); \
    } \
  } while (0)

#define dbg(format, ...) do { \
    if (VA_ARG_COUNT(__VA_ARGS__) == 0) { \
      PUTS(format); \
    } else { \
      PRINTF(format, ##__VA_ARGS__); \
    } \
    PUTNL(); \
  } while (0)


#else
#define dbg(format, ...) do {} while (0)
#define PRINTF(format, ...) do {} while (0)
#define PUTSN(string, len) do {} while (0)
#define PUTS(string) do {} while (0)
#define PUTNL() do {} while (0)
#define PUTCHAR(ch) do {} while (0)
#endif

#endif //GEX_DEBUG_H
