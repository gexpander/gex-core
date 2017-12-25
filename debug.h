//
// Created by MightyPork on 2017/11/04.
//

#ifndef GEX_DEBUG_H
#define GEX_DEBUG_H

#include <inttypes.h>
#include <stddef.h>
#include <stdarg.h>

#if USE_DEBUG_UART

int PRINTF(const char *format, ...) __attribute__((format(printf,1,2))) ;
void PUTSN(const char *string, size_t len);
int PUTS(const char *string);
void PUTNL(void);
int PUTCHAR(int ch);

#define dbg(format, ...) do { PRINTF(format, ##__VA_ARGS__); PUTNL(); } while (0)

#else
#define dbg(format, ...) do {} while (0)
#define PRINTF(format, ...) do {} while (0)
#define PUTSN(string, len) do {} while (0)
#define PUTS(string) do {} while (0)
#define PUTCHAR(ch) do {} while (0)
#endif

#endif //GEX_DEBUG_H
