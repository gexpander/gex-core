//
// Created by MightyPork on 2017/11/09.
//

#ifndef GEX_SNPRINTF_H
#define GEX_SNPRINTF_H

#include <stdarg.h>
#include <limits.h>
#include "macro.h"

size_t fixup_vsnprintf(char *str, size_t count, const char *fmt, va_list args);
size_t fixup_snprintf(char *str, size_t count,const char *fmt,...);
size_t fixup_vasprintf(char **ptr, const char *format, va_list ap);
size_t fixup_asprintf(char **ptr, const char *format, ...);
size_t fixup_sprintf(char *ptr, const char *format, ...);

// Trap for using newlib functions
//#define vsnprintf fuck1
//#define snprintf fuck2
//#define vasprintf fuck3
//#define asprintf fuck4
//#define sprintf fuck5

#define VSNPRINTF(...) fixup_vsnprintf(__VA_ARGS__)
#define SNPRINTF(...) fixup_snprintf(__VA_ARGS__)
#define VASPRINTF(...) fixup_vasprintf(__VA_ARGS__)
#define ASPRINTF(...) fixup_asprintf(__VA_ARGS__)
#define SPRINTF(...) fixup_sprintf(__VA_ARGS__)

#endif //GEX_SNPRINTF_H
