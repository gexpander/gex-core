//
// Created by MightyPork on 2017/11/04.
//

#include "platform.h"
#include <reent.h>

#if USE_DEBUG_UART

// debug printf
int _DO_PRINTF(const char *format, ...)
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
 * Puts a newline
 *
 */
void PUTNL(void)
{
    _write_r(NULL, 2, "\r\n", 2);
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

#endif
