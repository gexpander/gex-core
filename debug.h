//
// Created by MightyPork on 2017/11/04.
//

#ifndef GEX_DEBUG_H
#define GEX_DEBUG_H

#include <inttypes.h>
#include <stddef.h>
#include <stdarg.h>

#if USE_DEBUG_UART

int _DO_PRINTF(const char *format, ...) __attribute__((format(printf,1,2))) ;
void PUTSN(const char *string, size_t len);
int PUTS(const char *string);
void PUTNL(void);
int PUTCHAR(int ch);

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
#define PUTCHAR(ch) do {} while (0)
#endif

#endif //GEX_DEBUG_H
