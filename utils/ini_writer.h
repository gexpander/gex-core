//
// Created by MightyPork on 2017/12/01.
//
// Utility for generating a INI file with support for extracting individual sectors
// and measuring total length without buffering. This is used to build the INI files
// for the VFS and config.
//

#ifndef INIWRITER_H
#define INIWRITER_H

#include "platform.h"

/**
 * INI writer handle
 */
typedef struct iniwriter_ {
    char *ptr;
    uint32_t skip;
    uint32_t count;
    uint32_t tag; // general purpose field (used to identify for which purpose is the file being read)
} IniWriter;

/**
 * IniWriter helper buffer, available within a IW-scope only.
 *
 * This buffer is used internally by printf-like iw functions.
 * It can be used to prepare buffer for iw_buff or iw_string,
 * but must NOT be used for %s substitutions in iw_* functions.
 */
extern char *iwbuffer;

/** Allocate the helper buffer */
void iw_begin(void);

/** Release the helper buffer */
void iw_end(void);

/**
 * Initialize a IniWriter struct (macro)
 *
 * @param buffer - buffer backing the writer, result will be written here
 * @param skip - number of written bytes to skip
 * @param count - number of bytes to write, truncate rest
 * @return structure initializer
 */
#define iw_init(xbuffer, xskip, xcount) (IniWriter){.ptr=(xbuffer), .skip=(xskip), .count=(xcount)}

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

void iw_cmt_newline(IniWriter *iw);
#define iw_newline(iw) iw_string(iw, "\r\n")


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
 * @param text - format, like printf
 */
void iw_comment(IniWriter *iw, const char *text);

/**
 * Try to write a INI comment # blah\r\n
 * @param iw - iniwriter handle
 * @param format - format, like printf
 * @param ... - replacements
 */
void iw_commentf(IniWriter *iw, const char *format, ...)
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

void iw_entry_s(IniWriter *iw, const char *key, const char *value);
void iw_entry_d(IniWriter *iw, const char *key, int32_t value);

/**
 * Measure total ini writer length using a dummy write
 *
 * @param handler - function that normally writes to the writer
 * @return byte count
 */
uint32_t iw_measure_total(void (*handler)(IniWriter *), uint32_t tag);

#endif //INIWRITER_H
