#include "platform.h"
#include "platform/status_led.h"

/**
 * Abort at file, line with a custom tag (eg. ASSERT FAILED)
 * @param msg - tag message
 * @param filename - file
 * @param line - line
 */
void __attribute__((noreturn)) abort_msg(const char *msg, const char *filename, uint32_t line)
{
    dbg("\r\n\033[31m%s:\033[m %s:%"PRIu32"\r\n", msg, filename, line);
    vPortEnterCritical();
    StatusLed_On(STATUS_FAULT);
    while(1);
}

/**
 * Warn at file, line with a custom tag (eg. ASSERT FAILED)
 * @param msg - tag message
 * @param filename - file
 * @param line - line
 */
void warn_msg(const char *msg, const char *filename, uint32_t line)
{
    dbg("\r\n\033[33m%s:\033[m %s:%"PRIu32"\r\n", msg, filename, line);
}

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   */
void __attribute__((noreturn)) assert_failed_(const char *file, uint32_t line)
{
    abort_msg("ASSERT FAILED", file, line);
}
