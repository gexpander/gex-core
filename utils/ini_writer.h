//
// Created by MightyPork on 2017/12/01.
//

#ifndef INIWRITER_H
#define INIWRITER_H

#include "platform.h"

typedef struct iniwriter_ {
    char *ptr;
    uint32_t skip;
    uint32_t count;
} IniWriter;

/**
 * Initialize a IniWriter struct (macro)
 *
 * @param buffer - buffer backing the writer, result will be written here
 * @param skip - number of written bytes to skip
 * @param count - number of bytes to write, truncate rest
 * @return structure initializer
 */
#define iw_init(buffer, skip, count) (IniWriter){buffer, skip, count}

/**
 * Try to write a buffer to the file
 *
 * @param iw - iniwriter handle
 * @param buf - buffer to write
 * @param len - buffer len
 */
void iw_buff(IniWriter *iw, const uint8_t *buf, uint32_t len);

/**
 * Try to write a string to the file
 *
 * @param iw - iniwriter handle
 * @param str - string to write
 */
static inline void iw_string(IniWriter *iw, const char *str)
{
    if (iw->count != 0) {
        iw_buff(iw, (uint8_t *) str, (uint32_t) strlen(str));
    }
}

#define iw_newline(iw) iw_string(iw, "\r\n")
#define iw_cmt_newline(iw) do { \
    if (SystemSettings.ini_comments) iw_string(iw, "\r\n"); \
} while (0)

/**
 * Try to snprintf to the file
 *
 * @param iw - iniwriter handle
 * @param format - format, like printf
 * @param ... - replacements
 */
void iw_sprintf(IniWriter *iw, const char *format, ...)
    __attribute__((format(printf,2,3)));

// High level stuff

/**
 * Try to write a INI section header [foobar]\r\n
 * @param iw - iniwriter handle
 * @param format - format, like printf
 * @param ... - replacements
 */
void iw_section(IniWriter *iw, const char *format, ...)
    __attribute__((format(printf,2,3)));

/**
 * Try to write a INI comment # blah\r\n
 * @param iw - iniwriter handle
 * @param format - format, like printf
 * @param ... - replacements
 */
void iw_comment(IniWriter *iw, const char *format, ...)
    __attribute__((format(printf,2,3)));

/**
 * Same as iw_comment(), but ignores the systemsettings option to disable comments
 */
void iw_hdr_comment(IniWriter *iw, const char *format, ...)
__attribute__((format(printf,2,3)));

/**
 * Try to write a key-value entry
 *
 * @param iw - iniwriter handle
 * @param key - key
 * @param format - value format (like printf)
 * @param ... - replacements
 */
void iw_entry(IniWriter *iw, const char *key, const char *format, ...)
    __attribute__((format(printf,3,4)));

#endif //INIWRITER_H
