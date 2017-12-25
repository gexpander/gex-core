//
// Created by MightyPork on 2017/11/04.
//

#include "platform.h"
#include <reent.h>

#if USE_DEBUG_UART

// debug printf
int PRINTF(const char *format, ...)
{
    va_list args;
    int len;
    char dbg_buf[DBG_BUF_LEN];

    va_start(args, format);
    /*convert into string at buff[0] of length iw*/
    len = (int)fixup_vsnprintf(&dbg_buf[0], DBG_BUF_LEN, format, args);
    _write_r(NULL, 2, dbg_buf, (size_t) len);
    va_end(args);
    return len;
}

/**
 * Puts with fixed size (print to debug uart)
 * @param string - buffer to print
 * @param len - number of bytes to print
 */
void PUTSN(const char *string, size_t len)
{
    if (len == 0) len = strlen(string);
    _write_r(NULL, 2, string, (size_t) len);
}

/**
 * Print a string to debug uart
 * @param string - string to print, zero-terminated
 * @return number of characters printed
 */
int PUTS(const char *string)
{
    size_t len = strlen(string);
    _write_r(NULL, 2, string, len);
    return (int) len;
}

/**
 * Print one character to debug uart
 * @param ch - character ASCII code
 * @return the character code
 */
int PUTCHAR(int ch)
{
    _write_r(NULL, 2, &ch, 1);
    return ch; // or EOF
}

/**
 * Print string to debug uart, add newline if missing (printf-like)
 * @param format
 * @param ...
 */
void dbg(const char *format, ...)
{
    va_list args;
    int len;
    char dbg_buf[DBG_BUF_LEN];

    va_start(args, format);
    len = (int)VSNPRINTF(&dbg_buf[0], DBG_BUF_LEN, format, args);
    _write_r(NULL, 2, dbg_buf, (size_t) len);

    // add newline if not present
    if (dbg_buf[len-1] != '\n')
        _write_r(NULL, 2, "\r\n",  2);

    va_end(args);
}

#endif
