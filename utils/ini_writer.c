//
// Created by MightyPork on 2017/12/01.
//

#include "platform.h"
#include "ini_writer.h"

#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#define IWBUFFER_LEN 128

// sprintf from varargs, allocating buffer on stack. Uses 'format' argument
#define IW_VPRINTF() do { \
    char iwbuffer[IWBUFFER_LEN]; \
    va_list args; \
    va_start(args, format); \
    uint32_t len = (int)fixup_vsnprintf(&iwbuffer[0], IWBUFFER_LEN, format, args); \
    iw_buff(iw, (uint8_t *) iwbuffer, len); \
    va_end(args); \
    } while(0)

void iw_buff(IniWriter *iw, const uint8_t *buf, uint32_t len)
{
    if (iw->count == 0) return;

    uint32_t skip = 0;
    if (iw->skip >= len) {
        // Skip entire string
        iw->skip -= len;
        return;
    } else {
        // skip part
        skip = iw->skip;
        iw->skip = 0;
        uint32_t count = MIN(iw->count, len-skip);
        memcpy(iw->ptr, buf+skip, count);
        iw->ptr += count;
        iw->count -= count;
    }
}

void iw_sprintf(IniWriter *iw, const char *format, ...)
{
    if (iw->count == 0) return;

    IW_VPRINTF();
}

// High level stuff
void iw_section(IniWriter *iw, const char *format, ...)
{
    if (iw->count == 0) return;

    iw_string(iw, "\r\n["); // newline before section as a separator
    IW_VPRINTF();
    iw_string(iw, "]\r\n");
}

void iw_comment(IniWriter *iw, const char *format, ...)
{
    if (iw->count == 0) return;

    iw_string(iw, "# ");
    IW_VPRINTF();
    iw_newline(iw);
}

void iw_entry(IniWriter *iw, const char *key, const char *format, ...)
{
    if (iw->count == 0) return;

    iw_string(iw, key);
    iw_string(iw, "=");
    IW_VPRINTF();
    iw_newline(iw);	// one newline after entry
}
