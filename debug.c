//
// Created by MightyPork on 2017/11/04.
//

#include "platform.h"
#include <reent.h>

#if USE_DEBUG_UART

// debug printf
void _DO_PRINTF(const char *format, ...)
{
    va_list args;
    int len;
    char dbg_buf[DBG_BUF_LEN];

    va_start(args, format);
    /*convert into string at buff[0] of length iw*/
    len = (int)fixup_vsnprintf(&dbg_buf[0], DBG_BUF_LEN, format, args);

    if (len >= DBG_BUF_LEN) {
        dbg_buf[DBG_BUF_LEN-1] = 0;
        len = DBG_BUF_LEN-1;
    }

    debug_write(dbg_buf, (uint16_t) len);
    va_end(args);
}

/**
 * Puts with fixed size (print to debug uart)
 * @param string - buffer to print
 * @param len - number of bytes to print
 */
void PUTSN(const char *string, uint16_t len)
{
    if (len == 0) len = (uint16_t) strlen(string);
    debug_write(string, len);
}

/**
 * Print a string to debug uart
 * @param string - string to print, zero-terminated
 * @return number of characters printed
 */
void PUTS(const char *string)
{
    size_t len = strlen(string);
    debug_write(string, (uint16_t) len);
}


#endif
