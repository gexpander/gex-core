//
// Created by MightyPork on 2017/11/04.
//

#ifndef GEX_DEBUG_H
#define GEX_DEBUG_H

#include <inttypes.h>
#include <stddef.h>
#include <stdarg.h>

#if USE_DEBUG_UART

void dbg(const char *format, ...) __attribute__((format(printf,1,2))) ;

int PRINTF(const char *format, ...) __attribute__((format(printf,1,2))) ;
void PUTSN(const char *string, size_t len);
int PUTS(const char *string);
int PUTCHAR(int ch);

#else
#define dbg(format, ...) do {} while (0)
#define PRINTF(format, ...) do {} while (0)
#define PUTSN(string, len) do {} while (0)
#define PUTS(string) do {} while (0)
#define PUTCHAR(ch) do {} while (0)
#endif

#endif //GEX_DEBUG_H
